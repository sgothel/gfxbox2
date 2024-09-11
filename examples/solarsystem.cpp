/**
 * Author: Svenson Han Göthel und Sven Göthel
 * Funktion Name: solarsystem.cpp
 */
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cassert>

#include <cstdlib>
#include <limits>
#include <memory>
#include <numeric>
#include <numbers>
#include <pixel/pixel4f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel3f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"
#include "pixel/unit.hpp"
#include <physics.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>

#include <random>
#include <vector>

using namespace pixel;
using namespace pixel::literals;

bool draw_all_orbits = false;

extern "C" {
    EMSCRIPTEN_KEEPALIVE void set_draw_all_orbits(bool v) noexcept { draw_all_orbits = v; }
}

static const uint8_t rgba_white[/*4*/] = { 255, 255, 255, 255 };
// static const uint8_t rgba_gray[/*4*/] = { 170, 170, 170, 255 };
static const uint8_t rgba_yellow[/*4*/] = { 255, 255, 0, 255 };
// static const uint8_t rgba_red[/*4*/] = { 255, 0, 0, 255 };
// static const uint8_t rgba_green[/*4*/] = { 0, 255, 0, 255 };
// static const uint8_t rgba_blue[/*4*/] = { 0, 0, 255, 255 };

static bool show_cbody_velo = false;

enum class cbodyid_t : size_t {
    none = std::numeric_limits<size_t>::max(),
    sun = 0,
    mercury,
    venus,
    earth,
    mars,
    jupiter,
    saturn,
    uranus,
    neptun,
    pluto
};
constexpr size_t number(const cbodyid_t rhs) noexcept {
    return static_cast<size_t>(rhs);
}
constexpr bool operator==(const cbodyid_t lhs, const cbodyid_t rhs) noexcept {
    return number(lhs) == number(rhs);
}
/** Three way std::strong_ordering comparison operator */
std::strong_ordering operator<=>(const cbodyid_t lhs, const cbodyid_t rhs) noexcept {
    return number(lhs) == number(rhs) ? std::strong_ordering::equal :
           ( number(lhs) < number(rhs) ? std::strong_ordering::less : std::strong_ordering::greater );
}
constexpr cbodyid_t& operator++(cbodyid_t& rhs) noexcept {
    rhs = static_cast<cbodyid_t>( number(rhs) + 1 );
    return rhs;
}
constexpr cbodyid_t& operator--(cbodyid_t& rhs) noexcept {
    rhs = static_cast<cbodyid_t>( number(rhs) - 1 );
    return rhs;
}

static float default_scale[] { 
  1.0f, 5*5, 5*5, 5*5, 5*5,
  5, 5, 5, 5, 10*5
};

float default2_scale[] { 
  1.0f, 4*5*5, 4*5*5, 4*5*5, 4*5*5,
  5*5, 5*5, 5*5, 5*5, 100*5
};

float normal_scale[] { 
  1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
  1.0f, 1.0f, 1.0f, 1.0f, 1.0f
};


class CBodyConst {
  public:
    si_length_t radius;
    si_length_t d_sun;
    si_accel_t g_s;
    si_velo_t v;
    f4::vec_t color;
    std::string name;
};

/**
                  |  Radius [km]  | Abstand zur Sonne [km] |     Gravitation [m/s²]     | Geschwindigkeit [km/h]  |
Sun               |      695 700  |                      0 |    132 615 586 260 000     |                        0|
Mercury           |         2440  |             58 000 000 |             22 028 320     |                  172 800|
Venus             |        6 051,8|            108 000 000 |            324 857 392,339 |                  126 072|
Earth             |        6 371  |            149 600 000 |            398 062 609,287 |                  108 000|
Mars              |        3 389,5|            227 900 000 |             42 393 340,8225|                   86 652|
Jupiter           |       69 911  |            778 000 000 |        121 162 312 962     |                   47 160|
Saturn            |       69 911  |          1 486 000 000 |         51 026 000 295,2   |                   34 884|
Uranus            |       25 362  |          2 900 000 000 |          5 705 459 360,28  |                   24 516|
Neptune           |       24 622  |          4 500 000 000 |          6 759 608 156,6   |                   20 000|
Pluto             |        1 151  |          7 300 000 000 |                768 384,58  |                   17 064|

Abstand vom Ringsystem des Saturns zur Saturnoberflaeche = ca. 11000000 km
Dicke vom Ringsystem des Saturns = ca. 1700 km
*/

static constexpr float light_second = 299792458.0f;
static constexpr float light_minute = 60 * light_second;

CBodyConst CBodyConstants[] {
  { 695700_km, 0_km, 274_m_s2, 0,
   {1, 1, 0, 1}, "Sun"},
  { 2440_km, 58000000_km, 3700_mm_s2, 172800_km_h,
   {0.5f, 0.5f, 0.5f, 0.5f}, "Mercury"},
  { 6051800_m, 108000000_km, 887_cm_s2, 126072_km_h,
   {1, 0.5f, 0, 1}, "Venus"},
  { 6371_km, 149600000_km, 9807_mm_s2, 108000_km_h,
   {0, 0, 1, 1}, "Earth"},
  { 3389500_m, 227900000_km, 369_cm_s2, 86652_km_h,
   {1, 0, 0, 1}, "Mars"},
  { 69911_km, 778000000_km, 2479_cm_s2, 47160_km_h,
   {0.7f, 0.5f, 0.5f, 0.5f}, "Jupiter"},
  { 69911_km, 1486000000_km, 1044_cm_s2, 34884_km_h,
   {0.7f, 0.7f, 0.5f, 0.5f}, "Saturn"},
  { 25362_km, 2900000000_km, 887_cm_s2, 24516_km_h,
   {0, 0, 1, 1}, "Uranus"},
  { 24622_km, 45000000000_km, 1115_cm_s2, 20000_km_h,
   {0, 0, 1, 1}, "Neptun"},
  { 1151_km, 7300000000_km, 58_cm_s2, 17064_km_h,
   {0.7f, 0.5f, 0.7f, 0.5f}, "Pluto"}
};

static cbodyid_t max_planet_id = cbodyid_t::mars;

static float space_height; // [m]

static float _global_scale = 20;
static void global_scale(float s) noexcept { _global_scale *= s; }
static float global_scale() noexcept { return _global_scale; }

class CBody;
typedef std::shared_ptr<CBody> CBodyRef;
std::vector<CBodyRef> cbodies; 

std::string to_magnitude_timestr(si_time_t v) {
    if( v >= 1_year ) {
        return pixel::to_string("%0.2f year", v/1_year);
    } else if( v >= 1_month ) {
        return pixel::to_string("%0.2f month", v/1_month);
    } else if( v >= 1_week ) {
        return pixel::to_string("%0.2f week", v/1_week);
    } else if( v >= 1_day ) {
        return pixel::to_string("%0.2f day", v/1_day);
    } else if( v >= 1_h ) {
        return pixel::to_string("%0.2f h", v/1_h);
    } else if( v >= 1_min ) {
        return pixel::to_string("%0.2f min", v/1_min);
    } else {
        return pixel::to_string("%0.2f s", v);
    }    
}

#include "solarsystem_cbodies.hpp"

ssize_t findSolarData(int64_t time_min, int64_t time_max) {
    for(size_t i=0; i < solarDataSet.setCount; ++i ) {
        const SolarData& sd = solarDataSet.set[i];
        if( time_min <= sd.time_u && sd.time_u <= time_max ) {
            return static_cast<ssize_t>(i);
        }
    }
    return -1;
}

class CBody {
  private:
    cbodyid_t _id;
    f2::vec_t _velo;
    float _g_c;
    f4::vec_t _color;
    float _scale;
    float _d_sun; // [m]
    float _radius; // [m]
    f2::disk_t _body;
    std::vector<f2::point_t> _orbit_points;
    std::string _id_s;
    int64_t _world_sec; // [s]
    // Millisecond component with its absolute value in range [0..1'000[ or [0..1'000).
    uint64_t _world_msec = 0;
  public:
    CBody()
    : _id(cbodyid_t::none), _radius(), _body() {}

    CBody(const CBody&) = default;
    CBody(CBody&&) = default;
    CBody& operator=(const CBody&) = default;
    CBody& operator=(CBody&&) = default;
    CBody(cbodyid_t id_, size_t dataset_idx=0)
    : _id(id_) {
        f2::point_t center;
        float g_0;
        {
            CBodyConst cbc = CBodyConstants[number(_id)];
            size_t id_idx = number(_id);
            if( 0 < id_idx && dataset_idx < solarDataSet.setCount ) { // not sun
                const SolarData& solar_data = solarDataSet.set[dataset_idx];
                _world_sec = solar_data.time_u; // [s]
                const CBodyData& planet = solar_data.planets[id_idx - 1];
                center.x = static_cast<float>(planet.position[0]*1000.0);
                center.y = static_cast<float>(planet.position[1]*1000.0);
                _velo.x = static_cast<float>(planet.velocity[0]*1000.0);
                _velo.y = static_cast<float>(planet.velocity[1]*1000.0);
            } else {
                _world_sec = solarDataSet.set[0].time_u; // [s]
                center = {cbc.d_sun, 0};
                float angle = (f2::point_t(0, 0) - center).angle() - (float)M_PI_2;
                _velo = f2::vec_t::from_length_angle(cbc.v, angle);
            }
            _d_sun = center.length();
            _radius = cbc.radius;
            g_0 = cbc.g_s;   
            _color = cbc.color;
            _id_s = cbc.name;
        }
        
        _g_c = g_0 * _radius * _radius;
        _scale = default_scale[number(_id)];
        _body = f2::disk_t(center, _radius);
    }

    cbodyid_t id() const noexcept { return _id; }
    const std::string& ids() const noexcept { return _id_s; }
    float radius() const noexcept { return _radius; }
    
    /// Returns world time in seconds since unix epoch
    int64_t world_time() const noexcept { return _world_sec; }
    
    float sun_dist() const noexcept { return _d_sun; }
    float orbit_extend() const noexcept { return _d_sun + _radius; }
    
    f2::point_t position() const noexcept { return _body.center; }
    
    pixel::f2::vec_t gravity(const pixel::f2::point_t& p) {
        pixel::f2::vec_t v_d = _body.center - p;
        const float d = v_d.length();
        if( pixel::is_zero(d) ) {
            return pixel::f2::vec_t();
        } else {
            // v.normalize() -> v / d
            // return v.normalize() * ( g0 / ( d * d ) );
            return ( v_d /= d ) *= ( _g_c / ( d * d ) ); // same as above but reusing vector 'v'
        }
    }
    
    void sub_tick(const float dt){
        if(_id == cbodyid_t::sun){
            return;
        }
        // std::vector<cbody_ref_t> cbodies;
        for(const auto& cb : cbodies) {
             if( this != cb.get() ) {
                const f2::vec_t g = cb->gravity(_body.center);
                _velo += g * dt;
            }
        }
        _body.move(_velo * dt);
    }
    
    static constexpr si_time_t max_time_step = 1_day;
    
    int64_t _orbit_world_sec_last = 0;
    si_time_t _time_scale_last = 1_day;
    
    void tick(const float dt, const si_time_t time_scale) {
        float dt_world = dt * time_scale; // world [s]
        _time_scale_last = time_scale;        
        
        // Adjust world time
        {            
            const uint64_t dtw_msec1 = (uint64_t)(dt_world*1000.0f); // 1000*[s] + [ms]
            const uint64_t dtw_sec = dtw_msec1 / 1000; // [s]
            const uint64_t dtw_msec2 = dtw_msec1 - (dtw_sec * 1000); // [ms] only -> dtw_msec1 % 1000
            
            _world_sec += (int64_t)(dtw_sec);
            _world_msec += dtw_msec2;
            if( _world_msec > 1000 ) {
                const uint64_t s = _world_msec / 1000;
                _world_msec = _world_msec - (s * 1000); // -> world_msec % 1000;
                _world_sec += (int64_t)s;
            }
        }        

        const bool orbit_tick = (float)(_world_sec - _orbit_world_sec_last) >= 1_week;
        while( dt_world > std::numeric_limits<float>::epsilon() ) {
            sub_tick( std::min(dt_world, max_time_step) );
            dt_world -= max_time_step;
        }
        if( orbit_tick ) {
            _orbit_points.push_back(_body.center);
            _orbit_world_sec_last = _world_sec;
        }
        _d_sun = _body.center.length();
    }
    
    void draw(bool filled, bool orbit) {        
        if( orbit ) {
            set_pixel_color(rgba_white);
            for(f2::point_t &p : _orbit_points){
                p.draw();
            }            
        }
        set_pixel_color(_color);
        const float r = _body.radius;
        _body.radius *= _scale * global_scale();
        _body.draw(filled);
        _body.radius = r;
        if( show_cbody_velo ) {
            set_pixel_color(rgba_yellow);
            f2::lineseg_t::draw(_body.center, _body.center+_velo*_time_scale_last);
        }
    }
    void clear_orbit() {
        _orbit_points.clear();
    }
    
    std::string toString() noexcept {
        return pixel::to_string("%s[%s, d_sun %.2f lm, velo %.2f km/s]", 
            _id_s.c_str(), 
            pixel::to_iso8601_string(_world_sec, _time_scale_last <= 1_day).c_str(),
            _d_sun/light_minute,
            _velo.length()/1000.0f );
    }
};

static cbodyid_t info_id = cbodyid_t::earth;
std::vector<f2::point_t> pl;
std::vector<f2::point_t> ipl;
bool tick_ts_down = false;
bool data_stop = false;
void mainloop() {
    // scale_all_numbers(0.000001f);
    static pixel::texture_ref hud_text;
    static uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]
    static pixel::input_event_t event;
    static si_time_t tick_ts = 1_month;
    static const float text_lum = 0.75f;
    static const f4::vec_t vec4_text_color(text_lum, text_lum, text_lum, 1.0f);
    static const int text_height = 24;    
    static bool animating = true;
    static CBodyRef selPlanetNextPos = nullptr;
    static ssize_t selPlanetNextPosDataSetIdx = -1;
    static cbodyid_t selPlanetNextPosCBodyID = info_id;
    static int64_t data_t0 = 0;
    const f2::point_t tl_text(cart_coord.min_x(), cart_coord.max_y());
    // resized = event.has_and_clr( input_event_type_t::WINDOW_RESIZED );

    while (pixel::handle_one_event(event)) {
        if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_CLOSE_REQ ) ) {
            printf("Exit Application\n");
            #if defined(__EMSCRIPTEN__)
                emscripten_cancel_main_loop();
            #else
                exit(0);
            #endif
        } else if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_RESIZED ) ) {
            pixel::cart_coord.set_height(-space_height, space_height);
        }
        if( event.paused() ) {
            animating = false;
        } else {
            if( !animating ) {
                t_last = pixel::getElapsedMillisecond(); // [ms]
            }
            animating = true;
        }
        // constexpr const float rot_step = 10.0f; // [ang-degrees / s]
        if( event.released_and_clr(input_event_type_t::RESET) ) {
            _global_scale = 20;
            info_id = cbodyid_t::earth;
            max_planet_id = cbodyid_t::mars;
            space_height = cbodies[number(max_planet_id)]->orbit_extend();
            pixel::cart_coord.set_height(-space_height, space_height);
            tick_ts = 1_month;
            pl.clear();
            ipl.clear();
            cbodies.clear();
            selPlanetNextPos = nullptr;
            selPlanetNextPosDataSetIdx = -1;
            selPlanetNextPosCBodyID = info_id;
            for(size_t i = 0; i <= number(cbodyid_t::pluto); ++i){
                CBodyRef cb = std::make_shared<CBody>( static_cast<cbodyid_t>( i ) );
                cbodies.push_back(cb);
                printf("%s\n", cb->toString().c_str());
            }
        }
        if( animating ) {
            if( event.has_any_p1() ) {
                bool reset_space_height = false;
                if( event.pressed(input_event_type_t::P1_ACTION1) ) {
                    if (event.released_and_clr(input_event_type_t::P1_UP)) {
                        global_scale(1.5f);
                    } else if (event.released_and_clr(input_event_type_t::P1_DOWN)) {
                        global_scale(0.5f);
                    }
                } else if( event.released_and_clr(input_event_type_t::P1_UP) ) {
                    if(tick_ts * 2 <= 1_year){
                        tick_ts *= 2;
                    } else {
                        tick_ts = 1_year;
                    }
                } else if( event.released_and_clr(input_event_type_t::P1_DOWN) ) {
                    if(tick_ts / 2 >= 1_s){
                        tick_ts /= 2;
                    } else {
                        tick_ts = 1_s;
                    }
                } else if (event.released_and_clr(input_event_type_t::P1_RIGHT)) {
                    if( max_planet_id < cbodyid_t::pluto ) {                    
                        ++max_planet_id;
                        reset_space_height = true;
                    }
                } else if (event.released_and_clr(input_event_type_t::P1_LEFT)) {
                    if( max_planet_id > cbodyid_t::mercury ) {
                        --max_planet_id;
                        reset_space_height = true;
                        if(info_id > max_planet_id){
                            info_id = max_planet_id;
                            pl.clear();
                        }
                    }
                }
                if( reset_space_height ) {
                    space_height = cbodies[number(max_planet_id)]->orbit_extend();
                    pixel::cart_coord.set_height(-space_height, space_height);
                }
            } 
            if( event.has_any_p2() ) {
                if (event.released_and_clr(input_event_type_t::P2_ACTION1)) {
                    draw_all_orbits = !draw_all_orbits;
                }
            }
            if( event.has_any_p3() ) {
                if (event.released_and_clr(input_event_type_t::P3_ACTION1)) {
                    show_cbody_velo = !show_cbody_velo;
                }
            }
            if( event.released_and_clr(input_event_type_t::ANY_KEY) ) {
                if( event.last_key_code == ' ') {
                    if(info_id < max_planet_id){
                        ++info_id;
                    } else {
                        info_id = cbodyid_t::mercury;
                    }
                    for(CBodyRef &cb : cbodies){
                        cb->clear_orbit();
                    }                
                }
            }
            // log_printf(0, "XXX event: %s\n", event.to_string().c_str());
        }
    }
    const uint64_t t1 = animating ? pixel::getElapsedMillisecond() : t_last; // [ms]    
    const float dt = (float)(t1 - t_last) / 1000.0f; // [s]
    t_last = t1;
    CBody& sel_cbody = *cbodies[number(info_id)];
    if(animating) {
        const int64_t world_t0_sec = sel_cbody.world_time();
        if( 0 != data_t0 ) {
            float tick_ts_pre = tick_ts;
            int mode = 0;
            if(world_t0_sec >= data_t0 - (int64_t)5){
                tick_ts = 1.0f;
                mode = 1;
            } else if(world_t0_sec >= data_t0 - (int64_t)1_min){
                tick_ts = std::max(10.0f, tick_ts * 0.24f);
                mode = 2;
            } else if(world_t0_sec >= data_t0 - (int64_t)1_h){
                tick_ts = 30_min; // 2s 
                mode = 3;
            } else if(world_t0_sec >= data_t0 - (int64_t)1_day){
                tick_ts = 24_h; // 1s
                mode = 4;
            } else if(world_t0_sec >= data_t0 - (int64_t)1_week){
                tick_ts = 7_day; // ~1s
                mode = 5;
            }
            if( tick_ts_pre != tick_ts ) {
                pixel::log_printf(0, "PAUSE -> RT: %s -> %s, %d, tick %s -> %s\n",
                    pixel::to_iso8601_string(world_t0_sec, true).c_str(),
                    pixel::to_iso8601_string(data_t0, true).c_str(),
                    mode, 
                    pixel::to_iso8601_string((int64_t)tick_ts_pre, true).c_str(), 
                    pixel::to_iso8601_string((int64_t)tick_ts, true).c_str() );                
            }
        }
        for(CBodyRef &cb : cbodies){
            cb->tick(dt, tick_ts);
        }
        if( 0 != data_t0 && world_t0_sec >= data_t0 ) {
            animating = false;
            event.set_paused(true);
            data_t0 = 0;
            int64_t t_diff_s = sel_cbody.world_time() - selPlanetNextPos->world_time();
            f2::point_t p_has = sel_cbody.position();
            f2::point_t p_exp = selPlanetNextPos->position();
            f2::point_t v_err = ( p_has - p_exp ); // [m]
            float l_err = v_err.length(); // [m]
            const float diam = sel_cbody.radius() * 2.0f;
            const float circum = sel_cbody.sun_dist() * 2.0f * (float)M_PI;
            pixel::log_printf(0, "Data Approx:\n  - this %s\n  - next %s\n"
                                 "  - dt %.2f [h], %s\n"
                                 "  - dist %.2f km, %.2f ls\n"
                                 ", %.2f diam, %.2f%% orbit (%.2f km)\n", 
                sel_cbody.toString().c_str(), 
                selPlanetNextPos->toString().c_str(),
                (float)t_diff_s/1_h, pixel::to_iso8601_string(t_diff_s, true).c_str(),
                l_err/1000.0f, l_err/light_second, 
                l_err/diam, 
                (l_err/circum)*100.0f, circum/1000.0f);                     
        }
    }
    const int64_t world_t0_sec = sel_cbody.world_time();
    hud_text = pixel::make_text(tl_text, 0, vec4_text_color, text_height,
                    "%s -> %s, time[x %s, td %ds], gscale %.2f, fps %.1f%s", 
                    sel_cbody.toString().c_str(),
                    cbodies[number(max_planet_id)]->ids().c_str(),
                    to_magnitude_timestr(tick_ts).c_str(), t1/1000, 
                    global_scale(), get_gpu_fps(),
                    animating?"":", paused");

    {                                        
        int64_t next_time_min = world_t0_sec-(int64_t)1_month;
        int64_t next_time_max = world_t0_sec+(int64_t)1_year;
        ssize_t ds_idx = findSolarData(next_time_min, next_time_max);
        if( 0 > ds_idx ) {
            if( nullptr != selPlanetNextPos ) {            
                pixel::log_printf(0, "NEXT: NULL, [%s - %s]\n", 
                    to_iso8601_string(next_time_min, true).c_str(),
                    to_iso8601_string(next_time_max, true).c_str());
                selPlanetNextPos = nullptr;
                data_t0 = 0;
            }
        } else if( nullptr == selPlanetNextPos || 
                   selPlanetNextPosDataSetIdx != ds_idx || 
                   selPlanetNextPosCBodyID != info_id 
                 ) 
        {
            selPlanetNextPosDataSetIdx = ds_idx;
            selPlanetNextPosCBodyID = info_id;
            selPlanetNextPos = std::make_shared<CBody>( info_id, ds_idx );            
            if( data_stop && (world_t0_sec < selPlanetNextPos->world_time()) ) {
                data_t0 = selPlanetNextPos->world_time();
            }
            pixel::log_printf(0, "NEXT: ds_idx %d, %s\n", 
                (int)ds_idx, selPlanetNextPos->toString().c_str());                     
        }
    }
    
    // white background
    clear_pixel_fb( 0, 0, 0, 0);
    
    set_pixel_color(255, 255, 255, 255);
        
    if( nullptr != selPlanetNextPos ) {
        selPlanetNextPos->draw(false, false);
    }
    for(size_t i = 0; i <= number(max_planet_id); ++i){
        cbodies[i]->draw(true, draw_all_orbits || info_id == cbodies[i]->id());
    }
    pixel::swap_pixel_fb(false);
    if( nullptr != hud_text ) {
        const int dx = ( pixel::fb_width - pixel::round_to_int((float)hud_text->width*hud_text->dest_sx) ) / 2;
        hud_text->draw(dx, 0);
    }
    pixel::swap_gpu_buffer();
}

int main(int argc, char *argv[])
{
    int window_width = 1920, window_height = 1000;
    pixel::forced_fps = 30;
    #if defined(__EMSCRIPTEN__)
        window_width = 1024, window_height = 576; // 16:9
    #endif
    {
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-width", argv[i]) && i+1<argc) {
                window_width = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-height", argv[i]) && i+1<argc) {
                window_height = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-fps", argv[i]) && i+1<argc) {
                pixel::forced_fps = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-show_velo", argv[i]) ) {
                show_cbody_velo = true;
            } else if( 0 == strcmp("-max_planet_id", argv[i]) && i+1<argc) {
                max_planet_id = static_cast<cbodyid_t>(atoi(argv[i+1]));
                ++i;
            } else if( 0 == strcmp("-info_id", argv[i]) && i+1<argc) {
                info_id = static_cast<cbodyid_t>(atoi(argv[i+1]));
                ++i;
            } else if( 0 == strcmp("-draw_all_orbits", argv[i])) {
                draw_all_orbits = true;
            } else if( 0 == strcmp("-data_stop", argv[i])) {
                data_stop = true;
            }
        }
    }
    {
        pixel::log_printf(0, "- win size %d x %d\n", window_width, window_height);
        pixel::log_printf(0, "- forced_fps %d\n", pixel::forced_fps);
        pixel::log_printf(0, "- data_stop %d\n", data_stop);
    }
    {
        const float origin_norm[] = { 0.5f, 0.5f };
        if( !pixel::init_gfx_subsystem("solarsystem", window_width, window_height, origin_norm) ) {
            return 1;
        }
    }
    // space_height = n.sfplts + n.pluto_radius; // [km]
    // space_height = n.sfnets; //  + n.neptun_radius; // [km]
    for(size_t i = 0; i <= number(cbodyid_t::pluto); ++i){
        CBodyRef cb = std::make_shared<CBody>( static_cast<cbodyid_t>( i ) );
        cbodies.push_back(cb);
        printf("%s\n", cb->toString().c_str());
    }

    space_height = cbodies[number(max_planet_id)]->orbit_extend();
    pixel::cart_coord.set_height(-space_height, space_height);

    std::vector<texture_ref> texts;

    log_printf(0, "XX %s\n", cart_coord.toString().c_str());
    printf("Pre-Loop\n");
    if( info_id > max_planet_id ) {
        info_id = max_planet_id;
    }
    {
        int64_t t0 = pixel::to_unix_seconds(1968, 1, 1);
        assert(0 > t0);
        std::string s0 = pixel::to_iso8601_string(t0, true);
        log_printf(0, "XX0: %s\n", s0.c_str());
    }
    {
        int64_t t0 = pixel::to_unix_seconds(1970, 1, 1);
        assert(0 == t0);
        std::string s0 = pixel::to_iso8601_string(t0, true);
        log_printf(0, "XX1: %s\n", s0.c_str());
    }
    {
        int64_t t0 = pixel::to_unix_seconds(1970, 1, 2);
        assert((int64_t)24*3600 == t0);
        std::string s0 = pixel::to_iso8601_string(t0, true);
        log_printf(0, "XX2: %s\n", s0.c_str());
    }
    {
        int64_t t0 = pixel::to_unix_seconds(2024, 1, 1);        
        std::string s0 = pixel::to_iso8601_string(t0, true);
        log_printf(0, "XX3: %s\n", s0.c_str());
    }
    {
        int64_t t0 = pixel::to_unix_seconds("2024-01-01");
        int64_t t1 = pixel::to_unix_seconds("2024-01-01 00:00:01");
        int64_t t10 = pixel::to_unix_seconds(2024, 1, 1);        
        std::string s0 = pixel::to_iso8601_string(t0, true);
        std::string s1 = pixel::to_iso8601_string(t1, true);
        std::string s10 = pixel::to_iso8601_string(t10, true);
        log_printf(0, "XX4_00: %s, %ld\n", s0.c_str(), t0);
        log_printf(0, "XX4_01: %s, %ld\n", s1.c_str(), t1);
        log_printf(0, "XX4_10: %s, %ld\n", s10.c_str(), t10);
        assert(t1 - 1 == t0);
        assert(t0 == t10);
    }
    printf("stop_time = second\n");
    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
}
