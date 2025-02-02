/**
 * Author: Svenson Han Göthel und Sven Göthel
 * Funktion Name: solarsystem.cpp
 */
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cassert>

#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <numbers>
#include <numbers>
#include <pixel/pixel2i.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel3f.hpp>
#include <pixel/pixel4f.hpp>
// #include <pixel/pixel3d.hpp>
#include "pixel/pixel.hpp"
#include <jau/float_si_types.hpp>
#include <jau/fraction_type.hpp>
#include <jau/utils.hpp>
#include <physics.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>

#include <vector>

using namespace jau;
using namespace jau::si_f32_literals;
using namespace pixel;

static int gravity_formula = 2;
static bool with_oobj = false;

static const uint8_t rgba_white[/*4*/] = { 255, 255, 255, 255 };
static const uint8_t rgba_black[/*4*/] = { 0, 0, 0, 255 };
// static const uint8_t rgba_gray[/*4*/] = { 170, 170, 170, 255 };
static const uint8_t rgba_yellow[/*4*/] = { 255, 255, 0, 255 };
static const uint8_t rgba_red[/*4*/] = { 255, 0, 0, 255 };
// static const uint8_t rgba_green[/*4*/] = { 0, 255, 0, 255 };
// static const uint8_t rgba_blue[/*4*/] = { 0, 0, 255, 255 };

static const float text_lum0 = 0.75f;
static const float text_lum1 = 0.85f;
static const int text_height = 24;

static bool draw_all_orbits = false;
static bool ref_cbody_stop = false;
static bool show_cbody_velo = false;
static bool color_inverse = false;

static f4::vec_t vec4_text_color0(text_lum0, text_lum0, text_lum0, 1.0f);
static f4::vec_t vec4_text_color1(text_lum1, text_lum1, text_lum1, 1.0f);
static uint8_t rgba_orbit[/*4*/] = { 255, 255, 255, 255 };
static uint8_t rgba_dbg_velo[/*4*/] = { 255, 255, 0, 255 };

static void set_colorinv(bool v) noexcept {
    float lum0, lum1;
     if(v) {
        color_inverse = true;
        lum0 = 1.0f - text_lum0;
        lum1 = 1.0f - text_lum1;
        ::memcpy(rgba_orbit, rgba_black, sizeof(rgba_black));
        ::memcpy(rgba_dbg_velo, rgba_red, sizeof(rgba_red));
     } else {
        color_inverse = false;
        lum0 = text_lum0;
        lum1 = text_lum1;
        ::memcpy(rgba_orbit, rgba_white, sizeof(rgba_white));
        ::memcpy(rgba_dbg_velo, rgba_yellow, sizeof(rgba_yellow));
     }
    vec4_text_color0 = f4::vec_t(lum0, lum0, lum0, 1.0f);
    vec4_text_color1 = f4::vec_t(lum1, lum1, lum1, 1.0f);
}

extern "C" {
    EMSCRIPTEN_KEEPALIVE void set_draw_all_orbits(bool v) noexcept { draw_all_orbits = v; }
    EMSCRIPTEN_KEEPALIVE void set_data_stop(bool v) noexcept { ref_cbody_stop = v; }
    EMSCRIPTEN_KEEPALIVE void set_showvelo(bool v) noexcept { show_cbody_velo = v; }
}

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
    pluto,
    oobj
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
  5, 5, 5, 5, 10*5, 7
};

float default2_scale[] {
  1.0f, 4*5*5, 4*5*5, 4*5*5, 4*5*5,
  5*5, 5*5, 5*5, 5*5, 5*5
};

float normal_scale[] {
  1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
  1.0f, 1.0f, 1.0f, 1.0f, 1.0f
};


class CBodyConst {
  public:
    std::string name;
    si_length_f32 radius; // [m]
    si_length_f32 d_sun; // [m]
    si_gravityparam_f64 GM; // [m^3/s^2]
    si_accel_f32 g_surface; // [m/s^2]
    si_velo_f32 v; // [m/s]
    f4::vec_t color;
    si_mass_f64 mass; // [kg]
};

static constexpr si_time_f32 light_second = 299792458.0f;
static constexpr si_time_f32 light_minute = 60 * light_second;
static cbodyid_t max_planet_id = cbodyid_t::mars;

// Gravitational constant
static constexpr double M_G = 6.6743015e-11; // [N⋅m2⋅kg−2]

// Source: [Horizon JPL](https://ssd.jpl.nasa.gov/horizons/manual.html)
CBodyConst CBodyConstants[] {
  {"Sun",    695700_km,           0_km,      132712000000_km3_s2,   274199_mm_s2, 0,
   {1, 1, 0, 1},            1988400e+24},
  {"Mercury",  2440_km,    58000000_km,     22031869000000_m3_s2,     3701_mm_s2, 172800_km_h,
   {0.5f, 0.5f, 0.5f, 0.5f},    330e+24},
  {"Venus",    6052_km,   108000000_km,    324858592000000_m3_s2,     8870_mm_s2, 126072_km_h,
   {1, 0.5f, 0, 1},            4.87e+24},
  {"Earth",    6378_km,   149600000_km,    398600435436000_m3_s2,     9820_mm_s2, 108000_km_h,
   {0, 0, 1, 1},               5.97e+24},
  {"Mars",     3396_km,   227900000_km,     42828375214000_m3_s2,     3710_mm_s2,  86652_km_h,
   {1, 0, 0, 1},                642e+24},
  {"Jupiter", 71492_km,   778000000_km, 126686531900000000_m3_s2,    24790_mm_s2,  47160_km_h,
   {0.7f, 0.5f, 0.5f, 0.5f},   1898e+24},
  {"Saturn",  60268_km,  1486000000_km,  37931206234000000_m3_s2,    10440_mm_s2,  34884_km_h,
   {0.7f, 0.7f, 0.5f, 0.5f},    568e+24},
  {"Uranus",  25559_km,  2900000000_km,   5793951256000000_m3_s2,     8870_mm_s2,  24516_km_h,
   {0, 0, 1, 1},               86.8e+24},
  {"Neptun",  24766_km, 45000000000_km,   6835099970000000_m3_s2,    11150_mm_s2,  20000_km_h,
   {0, 0, 1, 1},                102e+24},
  {"Pluto",    1188_km,  7300000000_km,       869326000000_m3_s2,      611_mm_s2,  17064_km_h,
   {0.7f, 0.5f, 0.7f, 0.5f}, 0.0130e+24},
  {"oobj",     62110_km, 730000000_km,        1327120000_km3_s2,  344022_mm_s2, -52200_km_h,
  {0.5, 0.5, 0.5, 1}, 1988400e+22}
};

static si_length_f32 space_height; // [m]

static float _global_scale = 20;
static void global_scale(float s) noexcept { _global_scale = std::max(1.0f, _global_scale * s); }
static float global_scale() noexcept { return _global_scale; }

class CBody;
typedef std::shared_ptr<CBody> CBodyRef;
std::vector<CBodyRef> cbodies;

std::string to_magnitude_timestr(si_time_f32 v) {
    if( v >= 1_year ) {
        return to_string("%0.2f year", v/1_year);
    } else if( v >= 1_month ) {
        return to_string("%0.2f month", v/1_month);
    } else if( v >= 1_week ) {
        return to_string("%0.2f week", v/1_week);
    } else if( v >= 1_day ) {
        return to_string("%0.2f day", v/1_day);
    } else if( v >= 1_h ) {
        return to_string("%0.2f h", v/1_h);
    } else if( v >= 1_min ) {
        return to_string("%0.2f min", v/1_min);
    } else {
        return to_string("%0.2f s", v);
    }
}

#include "solarsystem_cbodies.hpp"

ssize_t findSolarData(const fraction_timespec& time_min, const fraction_timespec& time_max) {
    for(size_t i=0; i < solarDataSet.setCount; ++i ) {
        const SolarData& sd = solarDataSet.set[i];
        const fraction_timespec sd_t(sd.time_u, 0);
        if( time_min <= sd_t && sd_t <= time_max ) {
            return static_cast<ssize_t>(i);
        }
    }
    return -1;
}

class CBody {
  private:
    cbodyid_t _id;
    f4::vec_t _color;
    float _scale;
    si_time_f32 _d_sun; // [m]
    si_time_f32 _radius; // [m]
    si_mass_f64 _mass; // [kg]
    f3::vec_t _velo; // [m/s]
    si_gravityparam_f64 GM; // [m^3/s^2]
    f3::point_t _center;
    std::string _id_s;
    fraction_timespec _world_time;
    fraction_timespec _orbit_world_time_last;
    int64_t _time_scale_last = 1_day;
    std::vector<f2::point_t> _orbit_points;

  public:
    CBody()
    : _id(cbodyid_t::none), _radius() {}

    CBody(const CBody&) = default;
    CBody(CBody&&) = default;
    CBody& operator=(const CBody&) = default;
    CBody& operator=(CBody&&) = default;
    CBody(cbodyid_t id_, size_t dataset_idx=0)
    : _id(id_) {
        f3::point_t center;
        {
            CBodyConst cbc = CBodyConstants[number(_id)];
            size_t id_idx = number(_id);
            if( 0 < id_idx &&
                dataset_idx < solarDataSet.setCount
                && id_idx <= number(cbodyid_t::pluto))
            { // not sun and not oobj
                const SolarData& solar_data = solarDataSet.set[dataset_idx];
                _world_time.tv_sec = solar_data.time_u; // [s]
                const CBodyData& planet = solar_data.planets[id_idx - 1];
                center.x = static_cast<float>(planet.position[0]*1000.0); // "*1000" = km -> m
                center.y = static_cast<float>(planet.position[1]*1000.0);
                center.z = static_cast<float>(planet.position[2]*1000.0);
                _velo.x = static_cast<float>(planet.velocity[0]*1000.0); // "*1000" = km/s -> m/s
                _velo.y = static_cast<float>(planet.velocity[1]*1000.0);
                _velo.z = static_cast<float>(planet.velocity[2]*1000.0);
            } else {
                _world_time.tv_sec = solarDataSet.set[0].time_u; // [s]
                center = {cbc.d_sun, 0, 0};
                f2::point_t center2(center.x, center.y);
                float angle;
                if(_id == cbodyid_t::oobj){
                    angle = 25_deg;
                } else {
                    angle = (f2::point_t(0, 0) - center2).angle() - (float)M_PI_2;
                }
                f2::vec_t velo2 = f2::vec_t::from_length_angle(cbc.v, angle);
                _velo = {velo2.x, velo2.y, 0};
                // _velo = {cbc.v, 0, 0};
            }
            GM = cbc.GM;
            _d_sun = center.length();
            _radius = cbc.radius;
            _color = cbc.color;
            _id_s = cbc.name;
            _orbit_world_time_last = _world_time;
            _center = center;
            _mass = cbc.mass;
        }
        _scale = default_scale[number(_id)];
    }

    cbodyid_t id() const noexcept { return _id; }
    const std::string& ids() const noexcept { return _id_s; }
    float radius() const noexcept { return _radius; }

    /// Returns world time in seconds since unix epoch
    fraction_timespec world_time() const noexcept { return _world_time; }

    float sun_dist() const noexcept { return _d_sun; }
    float space_height() const noexcept {
        // return ( _d_sun + _radius ) * 1.075f;
        // prefer using average sun distance for same screen layout at any given time
        return ( CBodyConstants[number(_id)].d_sun + CBodyConstants[number(_id)].radius ) * 1.075f;
    }

    const f3::point_t& position() const noexcept { return _center; }
    const f3::point_t& velo() const noexcept { return _velo; }
    
    // Returns gravity [m/s^2] acceleration body `b` towards body `a`
    // @param a attracting body
    // @param b attracted body towards body 'a'
    static pixel::f3::vec_t gravity1(const CBody& a, const CBody& b) {
        pixel::f3::vec_t v_d = a._center - b._center;
        const double d = v_d.length();
        pixel::f3::vec_t v_g; // Gravitationsbeschleunigung (Ergebnis)
        if( !is_zero(d) ) {
            // normal-vector: v_d / d (einheitsvektor, richtung)
            // const double F = M_G * (a._mass * b._mass) / ( d * d );
            // v_g = ( v_d / (float)d ) * (float)( F / b._mass );
            v_g = ( v_d / (float)d ) * (float)(M_G * a._mass / ( d * d ));
        }
        return v_g;
    }

    // Returns gravity [m/s^2] acceleration body `b` towards body `a`
    // @param a attracting body
    // @param b attracted body towards body 'a'
    static pixel::f3::vec_t gravity2(const CBody& a, const CBody& b) {
        pixel::f3::vec_t v_d = a._center - b._center;
        const double d = v_d.length();
        pixel::f3::vec_t v_g; // Gravitationsbeschleunigung (Ergebnis)
        if( !is_zero(d) ) {
            // normal-vector: v_d / d (einheitsvektor, richtung)
            // const double F = a.GM * o._mass / ( d * d );
            // v_g = ( v_d / d ) * ( F / o._mass);
            v_g = ( v_d / (float)d ) * (float)( a.GM / ( d * d ) );
        }
        return v_g;
    }

    void sub_tick(const fraction_timespec& dt, const fraction_timespec& wts) {
        if( _id == cbodyid_t::sun && !with_oobj) {
            return;
        }
        const float dt_f = (float)dt.to_us()/1000000.0f;
        for( const CBodyRef& cb : cbodies ) {
            if( this != cb.get() ) {
                f3::vec_t g;
                switch( gravity_formula ) {
                    case 1:  g = gravity1(*cb, *this); break;
                    default: g = gravity2(*cb, *this); break;
                }
                _velo += g * dt_f;
            }
        }
        _center += _velo * dt_f;

        const fraction_timespec orbit_th(color_inverse ? 0 : 1_day);
        if( wts - _orbit_world_time_last > orbit_th ) {
            _orbit_points.emplace_back(_center.x, _center.y);
            _orbit_world_time_last = wts;
        }
    }

    void tick(const fraction_timespec& dt, const int64_t time_scale) {
        constexpr fraction_timespec zero;
        constexpr fraction_timespec max_time_step(1_h);
        const fraction_timespec dt_world = dt * time_scale; // world [s]
        _time_scale_last = time_scale;

        fraction_timespec wts = _world_time;
        for(fraction_timespec i=dt_world; i > zero; i-=max_time_step ) {
            const fraction_timespec step = jau::min(i, max_time_step);
            sub_tick(step, wts);
            wts += step;
        }
        _d_sun = _center.length();

        _world_time += dt_world;
    }

    void draw(bool filled, bool orbit) {
        if( orbit ) {
            set_pixel_color(rgba_orbit);
            for(f2::point_t &p : _orbit_points){
                p.draw();
            }
        }
        f2::point_t c = {_center.x, _center.y};
        set_pixel_color(_color);
        f2::disk_t body = f2::disk_t(c, _radius * _scale * global_scale());
        body.draw(filled);
        if( show_cbody_velo ) {
            set_pixel_color(rgba_dbg_velo);
            f2::vec_t v = {_velo.x, _velo.y};
            f2::lineseg_t::draw(c, c +v*(float)_time_scale_last);
        }
    }
    void clear_orbit() {
        _orbit_points.clear();
    }

    std::string toString() const noexcept {
        fraction_timespec t(_world_time);
        t.tv_nsec = 0;
        return to_string("%s[%s, d_sun %.2f lm, velo %.2f km/s]",
            _id_s.c_str(),
            t.to_iso8601_string(true, _time_scale_last > int64_t(1_day)).c_str(),
            _d_sun/light_minute,
            _velo.length()/1000.0f);
    }
};

static cbodyid_t info_id = cbodyid_t::earth;
std::vector<f2::point_t> pl;
std::vector<f2::point_t> ipl;
bool tick_ts_down = false;
static std::string record_bmpseq_basename;
static FILE* stats_file = nullptr;

void mainloop() {
    // scale_all_numbers(0.000001f);
    static pixel::texture_ref hud_text;
    static fraction_timespec t_start, t_last;
    static int64_t frame_count_total = 0;
    static int64_t snap_count = 0;
    static pixel::input_event_t event;
    static int64_t tick_ts = 1_month;
    static bool animating = true;
    static CBodyRef selPlanetNextPos = nullptr;
    static ssize_t selPlanetNextPosDataSetIdx = -1;
    static cbodyid_t selPlanetNextPosCBodyID = info_id;
    static fraction_timespec ref_cbody_t0;
    const f2::point_t tl_text(cart_coord.min_x(), cart_coord.max_y());
    bool do_snapshot = false;
    // resized = event.has_and_clr( input_event_type_t::WINDOW_RESIZED );

    const fraction_timespec t1 = getMonotonicTime();
    if( 0 == frame_count_total ) {
        t_start = t1;
        t_last = t_start;
    }

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
            animating = true;
        }
        // constexpr const float rot_step = 10.0f; // [ang-degrees / s]
        if( event.released_and_clr(input_event_type_t::RESET) ) {
            _global_scale = 20;
            info_id = cbodyid_t::earth;
            max_planet_id = cbodyid_t::mars;
            space_height = cbodies[number(max_planet_id)]->space_height();
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
            if(with_oobj) {
                CBodyRef cb = std::make_shared<CBody>( cbodyid_t::oobj );
                cbodies.push_back(cb);
                printf("%s\n", cb->toString().c_str());
            }
        } else if( event.released_and_clr(pixel::input_event_type_t::F12) ) {
            do_snapshot = true;
        }

        if( animating ) {
            if( event.has_any_p1() ) {
                bool reset_space_height = false;
                if( event.pressed(input_event_type_t::P1_ACTION1) ) {
                    if (event.released_and_clr(input_event_type_t::P1_UP)) {
                        global_scale(2.0f);
                    } else if (event.released_and_clr(input_event_type_t::P1_DOWN)) {
                        global_scale(0.5f);
                    }
                } else if( event.released_and_clr(input_event_type_t::P1_UP) ) {
                    tick_ts = std::min<int64_t>(1_year, tick_ts * 2);
                } else if( event.released_and_clr(input_event_type_t::P1_DOWN) ) {
                    tick_ts = std::max<int64_t>(1_s, tick_ts / 2);
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
                    space_height = cbodies[number(max_planet_id)]->space_height();
                    pixel::cart_coord.set_height(-space_height, space_height);
                }
            }
            if( event.has_any_p2() ) {
                if (event.released_and_clr(input_event_type_t::P2_ACTION1)) {
                    draw_all_orbits = !draw_all_orbits;
                }
            }
            if( event.released_and_clr(input_event_type_t::P3_ACTION1) ) {
                show_cbody_velo = !show_cbody_velo;
            } else if( event.released_and_clr(input_event_type_t::F1) ) {
                gravity_formula = 1;
            } else if( event.released_and_clr(input_event_type_t::F2) ) {
                gravity_formula = 2;
            }
            if( event.released_and_clr(input_event_type_t::ANY_KEY) ) {
                if( event.last_key_code == ' ') {
                    if(info_id < max_planet_id){
                        ++info_id;
                    } else {
                        info_id = cbodyid_t::sun;
                    }
                    for(CBodyRef &cb : cbodies){
                        cb->clear_orbit();
                    }
                }
            }
            // log_printf(0, "XXX event: %s\n", event.to_string().c_str());
        }
    }
    // Better accuracy using the average dt over 5s
    // const jau::fraction_timespec dt = animating ? t1 - t_last : jau::fraction_timespec();
    const jau::fraction_timespec dt = animating ? pixel::gpu_avg_framedur() : jau::fraction_timespec();
    CBody& sel_cbody = *cbodies[number(info_id)];
    if(animating) {
        const fraction_timespec world_t0 = sel_cbody.world_time();
        if( !ref_cbody_t0.isZero() ) {
            int64_t tick_ts_pre = tick_ts;
            int mode = 0;
            if(world_t0.tv_sec >= ref_cbody_t0.tv_sec - (int64_t)5){
                tick_ts = 1;
                mode = 1;
            } else if(world_t0.tv_sec >= ref_cbody_t0.tv_sec - (int64_t)1_min){
                tick_ts = std::max<int64_t>(10, tick_ts / 4);
                mode = 2;
            } else if(world_t0.tv_sec >= ref_cbody_t0.tv_sec - (int64_t)1_h){
                tick_ts = 30_min; // 2s
                mode = 3;
            } else if(world_t0.tv_sec >= ref_cbody_t0.tv_sec - (int64_t)1_day){
                tick_ts = 24_h; // 1s
                mode = 4;
            } else if(world_t0.tv_sec >= ref_cbody_t0.tv_sec - (int64_t)1_week){
                tick_ts = 7_day; // ~1s
                mode = 5;
            }
            if( tick_ts_pre != tick_ts ) {
                const fraction_timespec a(tick_ts_pre, 0), b(tick_ts, 0);
                log_printf(0, "PAUSE -> RT: %s -> %s, %d, tick %s -> %s\n",
                    world_t0.to_iso8601_string(true).c_str(),
                    ref_cbody_t0.to_iso8601_string(true).c_str(),
                    mode,
                    ref_cbody_t0.to_iso8601_string(true).c_str(),
                    a.to_iso8601_string(true).c_str(),
                    b.to_iso8601_string(true).c_str() );
            }
        }
        for(CBodyRef &cb : cbodies){
            cb->tick(dt, tick_ts);
        }
        if( !ref_cbody_t0.isZero() && world_t0 >= ref_cbody_t0 ) {
            animating = false;
            event.set_paused(true);
            ref_cbody_t0.clear();
            const fraction_timespec t_diff_s = sel_cbody.world_time() - selPlanetNextPos->world_time();

            const f3::point_t p_has = sel_cbody.position();
            const f3::point_t p_exp = selPlanetNextPos->position();
            const f3::point_t v_perr = ( p_has - p_exp ); // [m]
            const double      l_perr = v_perr.length(); // [m]
            const double       diam = sel_cbody.radius() * 2.0;
            const double     circum = sel_cbody.sun_dist() * 2.0 * std::numbers::pi_v<double>;
            const f3::point_t v_has = sel_cbody.velo();
            const f3::point_t v_exp = selPlanetNextPos->velo();
            const f3::point_t v_verr = ( v_has - v_exp ); // [m]
            const double      l_verr = v_verr.length(); // [m]

            const f2::point_t p_has2 { (float)p_has.x, (float)p_has.y };
            const f2::point_t p_exp2 { (float)p_exp.x, (float)p_exp.y };
            const f2::point_t v_perr2 = ( p_has2 - p_exp2 ); // [m]
            const float       l_perr2 = v_perr2.length(); // [m]

            printf("Data Approx:\n  - simulated %s\n  - ref-data  %s\n"
                                 "  - dt %.2f [h], %s\n"
                                 "  - 3d pos exp %s [km]\n"
                                 "  - 3d pos has %s [km]\n"
                                 "  - 3d pos err %s [km]\n"
                                 "  - 2d pos exp %s [km]\n"
                                 "  - 2d pos has %s [km]\n"
                                 "  - 2d pos err %s [km]\n"
                                 "  - vel exp %s [km]\n"
                                 "  - vel has %s [km]\n"
                                 "  - vel err %s [km]\n"
                                 "  - diam %.2f km\n"
                                 "  - 3d dist %.2f km, %.2f ls\n"
                                 "  - 3d dist %.2f diam, %.2f%% orbit (%.0f km), %.2f%% sun-dist (%.0f km)\n"
                                 "  - 2d dist %.2f km, %.2f ls\n"
                                 "  - 2d dist %.2f diam, %.2f%% orbit (%.0f km), %.2f%% sun-dist (%.0f km)\n"
                                 "  - 3d dvel %.2f km/s\n",
                sel_cbody.toString().c_str(),
                selPlanetNextPos->toString().c_str(),
                (float)t_diff_s.tv_sec/1_h, t_diff_s.to_iso8601_string(true).c_str(),
                (p_exp/1000.0).toString().c_str(), (p_has/1000.0).toString().c_str(), (v_perr/1000.0).toString().c_str(),
                (p_exp2/1000.0).toString().c_str(), (p_has2/1000.0).toString().c_str(), (v_perr2/1000.0).toString().c_str(),
                (v_exp/1000.0).toString().c_str(), (v_has/1000.0).toString().c_str(), (v_verr/1000.0).toString().c_str(),
                diam/1000.0,
                l_perr/1000.0, l_perr/light_second,
                l_perr/diam,
                (l_perr/circum)*100.0, circum/1000.0, (l_perr/sel_cbody.sun_dist())*100.0, sel_cbody.sun_dist()/1000.0,
                l_perr2/1000.0f, l_perr2/light_second,
                l_perr2/diam,
                (l_perr2/circum)*100.0f, circum/1000.0f, (l_perr2/sel_cbody.sun_dist())*100.0, sel_cbody.sun_dist()/1000.0,
                l_verr/1000.0);
            printf("\n");
        }
    }
    const fraction_timespec world_t0_sec = sel_cbody.world_time();
    hud_text = pixel::make_text(tl_text, 0, animating ? vec4_text_color0 : vec4_text_color1, text_height,
                    "%s -> %s, time[x %s, td %" PRIi64 "s], gscale %0.2f, formula %d, fps %0.1f",
                    sel_cbody.toString().c_str(),
                    cbodies[number(max_planet_id)]->ids().c_str(),
                    to_magnitude_timestr((float)tick_ts).c_str(),
                    (t1-t_start).tv_sec,
                    global_scale(), gravity_formula, gpu_avg_fps());
    {
        fraction_timespec next_time_min = world_t0_sec-fraction_timespec(1_month);
        fraction_timespec next_time_max = world_t0_sec+fraction_timespec(1_year);
        ssize_t ds_idx = findSolarData(next_time_min, next_time_max);
        if( 0 > ds_idx ) {
            if( nullptr != selPlanetNextPos ) {
                log_printf(0, "NEXT: NULL, [%s - %s]\n",
                    next_time_min.to_iso8601_string(true).c_str(),
                    next_time_max.to_iso8601_string(true).c_str());
                selPlanetNextPos = nullptr;
                ref_cbody_t0.clear();
            }
        } else if( nullptr == selPlanetNextPos ||
                   selPlanetNextPosDataSetIdx != ds_idx ||
                   selPlanetNextPosCBodyID != info_id
                 )
        {
            selPlanetNextPosDataSetIdx = ds_idx;
            selPlanetNextPosCBodyID = info_id;
            selPlanetNextPos = std::make_shared<CBody>( info_id, ds_idx );
            if( ref_cbody_stop && (world_t0_sec < selPlanetNextPos->world_time()) ) {
                ref_cbody_t0 = selPlanetNextPos->world_time();
            }
            log_printf(0, "NEXT: ds_idx %d, %s\n",
                (int)ds_idx, selPlanetNextPos->toString().c_str());
        }
    }

    // black background
    if(color_inverse) {
        clear_pixel_fb( 255, 255, 255, 255);
        set_pixel_color(rgba_black);
    } else {
        clear_pixel_fb( 0, 0, 0, 0);
        set_pixel_color(rgba_white);
    }

    if( nullptr != selPlanetNextPos ) {
        selPlanetNextPos->draw(false, false);
    }
    for(CBodyRef &cb : cbodies) {
        const cbodyid_t id = cb->id();
        if( number(id) <= number(max_planet_id) || id == cbodyid_t::oobj ) {
            cb->draw(true, draw_all_orbits || info_id == id);
        }
    }
    pixel::swap_pixel_fb(false);
    if( nullptr != hud_text ) {
        const int dx = ( pixel::fb_width - round_to_int((float)hud_text->width*hud_text->dest_sx) ) / 2;
        hud_text->draw_fbcoord(dx, 0);
    }
    pixel::swap_gpu_buffer();
    if( record_bmpseq_basename.size() > 0 ) {
        std::string snap_fname(128, '\0');
        const int written = std::snprintf(&snap_fname[0], snap_fname.size(), "%s-%7.7" PRIi64 ".bmp", record_bmpseq_basename.c_str(), frame_count_total);
        snap_fname.resize(written);
        pixel::save_snapshot(snap_fname);
    }
    if( do_snapshot ) {
        std::string snap_fname(128, '\0');
        const int written = std::snprintf(&snap_fname[0], snap_fname.size(), "solarsystem-%7.7" PRIi64 ".bmp", snap_count++);
        snap_fname.resize(written);
        pixel::save_snapshot(snap_fname);
        printf("Snapshot written to %s\n", snap_fname.c_str());
    }
    if ( stats_file && frame_count_total > 0 ) {
        const jau::fraction_timespec dt2 = t1 - t_last;
        const double fps_2 = 1.0 / ( double(dt2.to_us()) / 1'000'000.0 );
        const fraction_timespec t0 = jau::getMonotonicTime();
        const fraction_timespec dt_total = t0 - t_start;
        const double dur_total = double(( dt_total / frame_count_total ).to_us()) / 1'000.0;
        const double fps_total = double(frame_count_total) / ( double(dt_total.to_us()) / 1'000'000.0 );
        const double dur_one = double(dt.to_us()) / 1000.0;
        const double dur_one2 = double(dt2.to_us()) / 1000.0;
        const double dur_avg = double(pixel::gpu_avg_framedur().to_us()) / 1000.0;
        const float fps_avg = pixel::gpu_avg_fps();
        if( 1 == frame_count_total ) {
            fprintf(stats_file, "Frame;TimeT;dt1;dt2;dtT;dtA;FPS-2;FPS-T;FPS-A\n");
        }
        fprintf(stats_file, "%7.7" PRIi64 ";%9.9" PRIu64 ";%.4f;%.4f;%.4f;%.4f;%.4f;%.4f;%.4f\n",
            frame_count_total, dt_total.to_us(), dur_one, dur_one2, dur_total, dur_avg, fps_2, fps_total, fps_avg);
    }
    ++frame_count_total;
    t_last = t1;
}

int main(int argc, char *argv[])
{
    int window_width = 1920, window_height = 1000;
    #if defined(__EMSCRIPTEN__)
        window_width = 1024, window_height = 576; // 16:9
    #endif
    bool write_stats = false;
    {
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-width", argv[i]) && i+1<argc) {
                window_width = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-height", argv[i]) && i+1<argc) {
                window_height = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-fps", argv[i]) && i+1<argc) {
                pixel::set_gpu_forced_fps(atoi(argv[i+1]));
                ++i;
            } else if( 0 == strcmp("-color_inv", argv[i]) ) {
                set_colorinv(true);
            } else if( 0 == strcmp("-record", argv[i]) && i+1<argc) {
                record_bmpseq_basename = argv[i+1];
                ++i;
            } else if( 0 == strcmp("-show_velo", argv[i]) ) {
                show_cbody_velo = true;
            } else if( 0 == strcmp("-stats", argv[i]) ) {
                write_stats = true;
            } else if( 0 == strcmp("-max_planet_id", argv[i]) && i+1<argc) {
                max_planet_id = static_cast<cbodyid_t>(atoi(argv[i+1]));
                ++i;
            } else if( 0 == strcmp("-info_id", argv[i]) && i+1<argc) {
                info_id = static_cast<cbodyid_t>(atoi(argv[i+1]));
                ++i;
            } else if( 0 == strcmp("-draw_all_orbits", argv[i])) {
                draw_all_orbits = true;
            } else if( 0 == strcmp("-data_stop", argv[i])) {
                ref_cbody_stop = true;
            } else if( 0 == strcmp("-woobj", argv[i])) {
                with_oobj = true;
            } else if( 0 == strcmp("-formula", argv[i]) && i+1<argc) {
                gravity_formula = std::max(1, std::min(2, atoi(argv[i+1]))); // [1..2]
                ++i;
            } else {
                log_printf(0, "ERROR: Unknown argument %s\n", argv[i]);
                return 1;
            }
        }
    }
    {
        log_printf(0, "- win size %d x %d\n", window_width, window_height);
        log_printf(0, "- forced_fps %d\n", pixel::gpu_forced_fps());
        log_printf(0, "- data_stop %d\n", ref_cbody_stop);
        log_printf(0, "- gravity formula %d\n", gravity_formula);
        log_printf(0, "- show_velo %d\n", show_cbody_velo);
        log_printf(0, "- record %s\n", record_bmpseq_basename.size()==0 ? "disabled" : record_bmpseq_basename.c_str());
    }
    {
        const float origin_norm[] = { 0.5f, 0.5f };
        if( !pixel::init_gfx_subsystem(argv[0], "solarsystem", window_width, window_height, origin_norm) ) {
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
    if(with_oobj) {
        CBodyRef cb = std::make_shared<CBody>( cbodyid_t::oobj );
        cbodies.push_back(cb);
        printf("%s\n", cb->toString().c_str());
    }
    space_height = cbodies[number(max_planet_id)]->space_height();
    pixel::cart_coord.set_height(-space_height, space_height);

    std::vector<texture_ref> texts;

    log_printf(0, "XX %s\n", cart_coord.toString().c_str());
    printf("Pre-Loop\n");
    if( info_id > max_planet_id ) {
        info_id = max_planet_id;
    }
    printf("stop_time = second\n");
    #if defined(__EMSCRIPTEN__)
        (void)write_stats;
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        static const char* stats_filename = "solarsystem.cvs";
        if( write_stats ) {
            stats_file = ::fopen(stats_filename, "w");
            if( !stats_file ) {
                const int err_no = errno;
                fprintf(stdout, "Couldn't open stats-file %s, err: %d %s\n", stats_filename, err_no, ::strerror(err_no));
            } else {
                fprintf(stdout, "Writing stats to %s\n", stats_filename);
            }
        }
        while( true ) { mainloop(); }
        if( stats_file ) {
            ::fclose(stats_file);
        }
    #endif
}
