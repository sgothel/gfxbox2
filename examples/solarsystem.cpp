/**
 * Author: Svenson Han Göthel und Sven Göthel
 * Funktion Name: solarsystem.cpp
 */
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cmath>

#include <cstdlib>
#include <limits>
#include <memory>
#include <numeric>
#include <numbers>
#include <pixel/pixel4f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"
#include "pixel/unit.hpp"
#include <physics.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <thread>

#include <random>
#include <iostream>
#include <vector>

using namespace pixel;
using namespace pixel::f2;
bool draw_all_orbits = false;

extern "C" {
    EMSCRIPTEN_KEEPALIVE void set_draw_all_orbits(bool v) noexcept { draw_all_orbits = v; }
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
        std::vector<point_t> orbit_points = {};
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

class CBody {
  private:
    cbodyid_t id;
    vec_t velo;
    float g_c;
    f4::vec_t color;
    float scale;
  public:
    disk_t body;
    float d_sun; // [m]
    float radius; // [m]
    std::string id_s;
    std::vector<point_t> orbit_points;
    CBody()
    : id(cbodyid_t::none), body(), radius() {}

    CBody(const CBody&) = default;
    CBody(CBody&&) = default;
    CBody& operator=(const CBody&) = default;
    CBody& operator=(CBody&&) = default;
    CBody(cbodyid_t id_)
    : id(id_) {
        point_t center;
        float g_0;
        float angle, velo_s;
        {
            CBodyConst cbc = CBodyConstants[number(id)];
            d_sun = cbc.d_sun;
            velo_s = cbc.v;
            radius = cbc.radius;
            g_0 = cbc.g_s;   
            color = cbc.color;
            id_s = cbc.name;
            orbit_points = cbc.orbit_points;
        }
        center = {d_sun, 0};
        angle = (point_t(0, 0) - center).angle() - (float)M_PI_2;
        velo = vec_t::from_length_angle(velo_s, angle);
        
        g_c = g_0 * radius * radius;
        scale = default_scale[number(id)];
        body = disk_t(center, radius);
    }

    pixel::f2::vec_t gravity(const pixel::f2::point_t& p) {
        pixel::f2::vec_t v_d = body.center - p;
        const float d = v_d.length();
        if( pixel::is_zero(d) ) {
            return pixel::f2::vec_t();
        } else {
            // v.normalize() -> v / d
            // return v.normalize() * ( g0 / ( d * d ) );
            return ( v_d /= d ) *= ( g_c / ( d * d ) ); // same as above but reusing vector 'v'
        }
    }
    
    void sub_tick(const float dt){
        if(id == cbodyid_t::sun){
            return;
        }
        // std::vector<cbody_ref_t> cbodies;
        for(const auto& cb : cbodies) {
             if( this != cb.get() ) {
                const vec_t g = cb->gravity(body.center);
                velo += g * dt;
            }
        }
        body.move(velo * dt);
    }
    
    static constexpr si_time_t max_time_step = 1_day;
    
    void tick(const float dt, const si_time_t time_scale) {
        float dts = dt * time_scale;        
        while( dts > std::numeric_limits<float>::epsilon() ) {
            sub_tick( std::min(dts, max_time_step) );
            dts -= max_time_step;
        }
        orbit_points.push_back(body.center);
        d_sun = body.center.length();
    }
    
    void draw() {
        set_pixel_color4f(color.x, color.y, color.z, color.w);
        const float r = body.radius;
        body.radius *= scale * global_scale();
        body.draw(true);
        body.radius = r;
    }
    
    std::string toString() noexcept {
        return id_s+
            "[d_sun "+std::to_string(d_sun/1000.0f)+
            " km, velo "+std::to_string(velo.length()/1000.0f)+" km/s]";
    }
};

static cbodyid_t info_id = cbodyid_t::earth;
std::vector<point_t> pl;
std::vector<point_t> ipl;
f4::vec_t normal_orbit_color = {1, 1, 1, 1};

void mainloop() {
    // scale_all_numbers(0.000001f);
    static pixel::texture_ref hud_text;
    static uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]
    static pixel::input_event_t event;
    static si_time_t tick_ts = 1_month;
    static const f4::vec_t vec4_text_color = {1, 1, 1, 1};
    static const int text_height = 30;    
    static bool animating = true;
    
    const point_t tl_text(cart_coord.min_x(), cart_coord.max_y());
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
            space_height = cbodies[number(max_planet_id)]->d_sun +
                           cbodies[number(max_planet_id)]->radius;
            pixel::cart_coord.set_height(-space_height, space_height);
            tick_ts = 1_month;
            pl.clear();
            ipl.clear();
            cbodies.clear();
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
                } else if( event.released_and_clr(input_event_type_t::P1_UP) && tick_ts < 1_year) {
                    tick_ts *= 2;
                } else if( event.released_and_clr(input_event_type_t::P1_DOWN) && tick_ts > 1_h) {
                    tick_ts /= 2;
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
                    space_height = cbodies[number(max_planet_id)]->d_sun + 
                                   cbodies[number(max_planet_id)]->radius;
                    pixel::cart_coord.set_height(-space_height, space_height);
                }
            } 
            if( event.has_any_p2() ) {
                if (event.released_and_clr(input_event_type_t::P2_ACTION1)) {
                    draw_all_orbits = !draw_all_orbits;
                }
            }
            if( event.released_and_clr(input_event_type_t::ANY_KEY) && 
                event.last_key_code == ' ') {
                if(info_id < max_planet_id){
                    ++info_id;
                } else {
                    info_id = cbodyid_t::mercury;
                }
                if(!draw_all_orbits){
                    for(size_t i = 1; i <= number(cbodyid_t::pluto); ++i){
                        cbodies[i]->orbit_points.clear();
                    }
                }
            }
        }
    }
    const uint64_t t1 = animating ? pixel::getElapsedMillisecond() : t_last; // [ms]    
    const float dt = (float)( t1 - t_last ) / 1000.0f; // [s]
    t_last = t1;
    float fps = get_gpu_fps();
    hud_text = pixel::make_text(tl_text, 0, vec4_text_color, text_height,
                    "fps %f, td %d [ms], gscale %.2f, %s, 1s -> %s", 
                    fps, t1, global_scale(), 
                    cbodies[number(info_id)]->toString().c_str(), 
                    to_magnitude_timestr(tick_ts).c_str());
    if(animating) {
        for(CBodyRef &cb : cbodies){
            cb->tick(dt, tick_ts);
        }
    }
    // white background
    clear_pixel_fb( 0, 0, 0, 0);
    
    set_pixel_color(255, 255, 255, 255);
        
    if(info_id != cbodyid_t::sun && !draw_all_orbits){
        set_pixel_color4f(normal_orbit_color.x, normal_orbit_color.y, normal_orbit_color.z, 
                          normal_orbit_color.w);
        for(point_t &p : cbodies[number(info_id)]->orbit_points){
            p.draw();
        }
    } else if(draw_all_orbits){
        for(size_t i = 1; i <= number(max_planet_id); ++i){
            CBodyRef cb = cbodies[i];
            if(cb->orbit_points != cbodies[number(info_id)]->orbit_points){
                set_pixel_color4f(normal_orbit_color.x, normal_orbit_color.y, normal_orbit_color.z, 
                                  normal_orbit_color.w);
            } else {
                set_pixel_color(255, 0, 0, 255);
            }
            for(point_t &p : cb->orbit_points) {
                p.draw();
            }
        } 
    }
    for(size_t i = 0; i <= number(max_planet_id); ++i){
        cbodies[i]->draw();
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
            } else if( 0 == strcmp("-max_planet_id", argv[i]) && i+1<argc) {
                max_planet_id = static_cast<cbodyid_t>(atoi(argv[i+1]));
                ++i;
            } else if( 0 == strcmp("-info_id", argv[i]) && i+1<argc) {
                info_id = static_cast<cbodyid_t>(atoi(argv[i+1]));
                ++i;
            } else if( 0 == strcmp("-draw_all_orbits", argv[i])) {
                draw_all_orbits = true;
            }
        }
    }
    {
        pixel::log_printf(0, "- win size %d x %d\n", window_width, window_height);
        pixel::log_printf(0, "- forced_fps %d\n", pixel::forced_fps);
    }
    {
        const float origin_norm[] = { 0.5f, 0.5f };
        init_gfx_subsystem("solarsystem", window_width, window_height, origin_norm);
    }
    // space_height = n.sfplts + n.pluto_radius; // [km]
    // space_height = n.sfnets; //  + n.neptun_radius; // [km]
    for(size_t i = 0; i <= number(cbodyid_t::pluto); ++i){
        CBodyRef cb = std::make_shared<CBody>( static_cast<cbodyid_t>( i ) );
        cbodies.push_back(cb);
        printf("%s\n", cb->toString().c_str());
    }

    space_height = cbodies[number(max_planet_id)]->d_sun + 
                   cbodies[number(max_planet_id)]->radius;
    pixel::cart_coord.set_height(-space_height, space_height);

    std::vector<texture_ref> texts;

    log_printf(0, "XX %s\n", cart_coord.toString().c_str());
    printf("Pre-Loop\n");
    if( info_id > max_planet_id ) {
        info_id = max_planet_id;
    }
    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
}
