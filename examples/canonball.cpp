/**
 * Author: Svenson Han Göthel und Sven Göthel
 * Funktion Name: Panzer.cpp
 */
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cmath>

#include <cstdlib>
#include <memory>
#include <numeric>
#include <numbers>
#include <pixel/pixel4f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"
#include <physics.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <thread>

#include <random>
#include <iostream>
#include <vector>

using namespace pixel;
using namespace pixel::f2;

//float rho = physiks::rho_default;

constexpr static const float basket_height = 1.0f; // [m]
constexpr static const float basket_width = 1.5 * basket_height; // [m]
constexpr static const float basket_frame_thickness = basket_height / 7.0f; // [m] // basket_height / 7.0

constexpr static const float space_height = 10 * basket_height; // [m]

constexpr static const float cannon_height = 1 * basket_height; // [m]
constexpr static const float cannon_width = 2 * cannon_height; // [m]

constexpr static const int player_balls_min = 0;

constexpr static const int text_height = 24;

constexpr static const float barrel_start_height = cannon_height/3.0f;
constexpr static const float barrel_end_height = barrel_start_height * (2.0f / 3);
constexpr static const float barrel_width = cannon_width*0.75f;
constexpr static const float barrel_start_width = barrel_width * (1.0f / 3);
constexpr static const float barrel_end_width = barrel_width * (2.0f / 3);

constexpr static const float bullet_radius = barrel_end_height * 0.45f;

constexpr static const float frame_gap = (float)text_height / 100 + 0.02f * basket_height;
constexpr static const float frame_thickness = 3.0f * bullet_radius; // 3.0f * bullet_radius
constexpr static const float frame_offset = frame_thickness + frame_gap;

constexpr static const float sf_thickness = frame_thickness;
constexpr static const float sf_height = basket_height;

bool debug_gfx = false;

template<class T, class C>
bool erase_one(C& list, const T& p) {
    auto it = std::find(list.begin(), list.end(), p);
    if( it != list.end() ) {
        it = list.erase(it);
        return true;
    }
    return false;
}

static aabbox_t korb_box;

class canon_t {
    private:
        constexpr static const float velocity_max = 24; // 14;
        constexpr static const float radius = cannon_height*0.8f;
        constexpr static const float velocity_min = 1.0f; // [m/s]
        
        #if 0
        rect_t aabbox = {point_t(cart_coord.min_x() + frame_gap + frame_thickness, 
                                 cart_coord.min_y() + frame_gap + frame_thickness + cannon_height), 
                         cannon_width, cannon_height};
        #endif
        point_t m_center_half_circle;
        
        float m_velocity; // [m/s]
        int m_ball;
        rect_t m_barrel_start; // Abschussrohr
        rect_t m_barrel_end;
        point_t rot_point;
        circle_seg_ref_t m_platform;
        int m_cp = 0;
        int m_score = 0;
    public:
        std::vector<physiks::ball_ref_t> m_pengs_flying;
        std::vector<physiks::ball_ref_t> m_pengs_all;
        canon_t()
        : m_center_half_circle(
            cart_coord.min_x() + frame_gap + frame_thickness + cannon_width *0.5f,
            cart_coord.min_y() + frame_gap + frame_thickness),
        m_velocity(velocity_min),
        m_ball(player_balls_min),   
        m_barrel_start(point_t(0, radius*0.5f), barrel_start_width, barrel_start_height),
        m_barrel_end(point_t(m_barrel_start.m_tr.x, m_barrel_start.m_tr.y - barrel_start_height * ((1.0f / 3) / 2)), 
                     barrel_end_width, barrel_end_height) {
            m_barrel_start.move(m_center_half_circle);
            m_barrel_end.move(m_center_half_circle);
            rot_point = point_t(m_barrel_start.m_tl.x, m_barrel_start.m_tl.y - barrel_start_height * 0.5f);
            m_platform = std::make_shared<circle_seg_t>(m_center_half_circle, radius, 0.0f, 
                                                        std::numbers::pi_v<float>);
            agobjects().push_back(m_platform);
        }
        
        void resize() {
            vec_t reverse = -1*m_center_half_circle;
            m_barrel_start.move(reverse);
            m_barrel_end.move(reverse);
            m_center_half_circle.set( 
                cart_coord.min_x() + frame_gap + frame_thickness + cannon_width *0.5f,
                cart_coord.min_y() + frame_gap + frame_thickness);
            m_barrel_start.move(m_center_half_circle);
            m_barrel_end.move(m_center_half_circle);
            m_platform->set_center(m_center_half_circle);            
            rot_point = point_t(m_barrel_start.m_tl.x, m_barrel_start.m_tl.y - barrel_start_height * 0.5f);
        }
        
        int score() const noexcept { return m_score; }
        
        void removePengs() {
            // erase_all_one<geom_ref_t>(pengs, gobjects());
            geom_list_t& list = gobjects();
            for(geom_ref_t p : m_pengs_flying ) {
                erase_one(list, p);
            }
            m_pengs_flying.clear();           
            m_pengs_all.clear();           
        }
        
        void reset(bool doRemovePengs=true){
            m_barrel_start.rotate(-m_barrel_start.dir_angle, rot_point); 
            m_barrel_end.rotate(-m_barrel_end.dir_angle, rot_point); 
            m_velocity = velocity_min;
            m_ball = player_balls_min;
            if( doRemovePengs ) {
                removePengs();
                m_score = 0;
            }
        }
        
        float adeg() { return rad_to_adeg(m_barrel_start.dir_angle); }
        
        void rotate_adeg(const float adeg){
            const float rad = adeg_to_rad(adeg);
            m_barrel_start.rotate(rad, rot_point);
            m_barrel_end.rotate(rad, rot_point);
        }
        
        long pow(const int a, const int b){
            long result = 1;
            for(int i = 0; i < b; ++i){
                result *= a;
            }
            return result;
        }
        
        bool tick(const float dt){
            for (auto it = m_pengs_all.begin(); it != m_pengs_all.end();) {
                physiks::ball_ref_t& p = *it;
                if( !p->tick(dt) ) {
                    std::shared_ptr<int> s1, s2;
                    erase_one(gobjects(), p);
                    erase_one(m_pengs_flying, p);
                    it = m_pengs_all.erase(it);
                } else {
                    ++it;
                }
            }
            point_t tr = korb_box.tr;
            point_t bl = korb_box.bl;
            korb_box.tr.add(-basket_frame_thickness, 0);
            korb_box.bl.add( basket_frame_thickness, basket_frame_thickness);
            for (auto it = m_pengs_flying.begin(); it != m_pengs_flying.end();) {
                physiks::ball_ref_t& p = *it;
                if(p->box().intersects( korb_box )){
                    m_score += (m_cp + 1);//static_cast<int>( pow(2.0f, m_cp) );
                    it = m_pengs_flying.erase(it);
                    // FIXME
                    // reset(false);
                    // rotate_adeg(rot_step * dt * next_rnd() * 3.0f);
                } else {
                    ++it;
                    for(geom_ref_t g : agobjects()){
                        if(g->intersects(p->box()) && g != p){
                            ++m_cp;
                        }
                    }
                }
            }
            korb_box.tr = tr;
            korb_box.bl = bl;
            return true;
        }

        bool change_velocity(const float o) {
            m_velocity *= o;
            if(m_velocity > velocity_max){
                m_velocity = velocity_max;
                return false;
            }
            return true;
        }
        
        void set_velocity(const float o) { 
            m_velocity = std::min(o, velocity_max);
            log_printf(0, "XX set velo %f\n", m_velocity);
        }
        
        float velocity(){ return m_velocity; }
        
        int ball(){ return m_ball; }
        
        int peng_idn = 0;
        
        void peng(){
            log_printf(0, "XX peng velo %f\n", m_velocity);
            vec_t v = vec_t::from_length_angle(barrel_end_height*0.5f, m_barrel_end.dir_angle + (float)M_PI_2);
            const point_t peng_start_center = m_barrel_end.m_br + v;
            physiks::ball_ref_t p = physiks::ball_t::create("peng_"+std::to_string(peng_idn++), 
                peng_start_center, bullet_radius, m_velocity, m_barrel_end.dir_angle, 
                physiks::earth_accel, 0, debug_gfx, false);
            m_pengs_flying.push_back(p);
            m_pengs_all.push_back(p);
            geom_list_t& list = gobjects();
            list.push_back(p);
            ++m_ball;
            m_velocity = velocity_min;
        }

        void draw() {
            m_platform->draw();
            m_barrel_start.draw(false);
            const float s_width = (m_velocity / velocity_max) * barrel_end_width;
            vec_t v = vec_t::from_length_angle(barrel_end_width - s_width, m_barrel_end.dir_angle);
            m_barrel_end.m_tr = m_barrel_end.m_tl + v;
            m_barrel_end.m_br = m_barrel_end.m_bl + v;
            m_barrel_end.draw(false);
            // printf("barrel_end: ")
        }
};

static std::random_device rng;
static const float rng_range = (float)std::random_device::max() - (float)std::random_device::min() + 1.0f;

float next_rnd() noexcept {
    if constexpr (false) {
        const float r0 = (float)rng();
        const float r = r0 / rng_range;
        std::cout << "rnd: r0 " << r0 << " / " << rng_range << " = " << r << std::endl;
        return r;
    } else {
        return (float)rng() / rng_range;
    }
}

static const float text_lum = 0.0f;
static const f4::vec_t vec4_text_color(text_lum, text_lum, text_lum, 1.0f);
static std::vector<geom_ref_t> korb;
static canon_t player = canon_t();
static rect_ref_t sf;
static bool make_sf = false;
static bool UpOrDown = true;

void mainloop() {
    static pixel::texture_ref hud_text;
    static uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]
    static pixel::input_event_t event;
    float rot_step_default = 20.0f; // [ang-degrees / s]
    
    const uint64_t t1 = getElapsedMillisecond(); // [ms]
    const float dt = (float)( t1 - t_last ) / 1000.0f; // [s]
    t_last = t1;
    while(pixel::handle_one_event(event)){
        if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_CLOSE_REQ ) ) {
            printf("Exit Application\n");
            #if defined(__EMSCRIPTEN__)
                emscripten_cancel_main_loop();
            #else
                exit(0);
            #endif
        } else if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_RESIZED ) ) {
            pixel::cart_coord.set_height(-space_height/2.0f, space_height/2.0f);
            player.resize();
        }
        if( !event.paused() ) {
            if( event.released_and_clr(input_event_type_t::RESET) ) {
                player.reset();
            }
            if( event.has_any_p1() ) {
                float rot_step = rot_step_default;
                if(event.pressed(pixel::input_event_type_t::P2_ACTION2 ) ) {
                    rot_step *= 2;
                }
                if( event.pressed_and_clr(input_event_type_t::P1_UP) ) {
                    player.rotate_adeg(rot_step*dt);
                } else if( event.pressed_and_clr(input_event_type_t::P1_DOWN ) ) {
                    player.rotate_adeg(-rot_step*dt);
                } else if( event.released_and_clr(input_event_type_t::P1_ACTION1) ) {
                    player.peng();
                }
            }
        }
    }
    // const bool animating = !event.paused();
    const point_t tl_text(cart_coord.min_x(), cart_coord.max_y());
    // resized = event.has_and_clr( input_event_type_t::WINDOW_RESIZED );
                             
    float fps = get_gpu_fps();
    hud_text = pixel::make_text(tl_text, 0, vec4_text_color, text_height,
                    "fps %f, td %d [ms], energie %f, adeg %3.3f, points %d, ball %d", 
                    fps, t1, player.velocity(), player.adeg(), player.score(), player.ball());
                    

    // white background
    clear_pixel_fb( 255, 255, 255, 255);
    set_pixel_color(0, 0, 255, 255);
    if( !event.paused() ) {
        if( event.pressed(input_event_type_t::P1_ACTION1) ) {
            player.change_velocity(1.05);
        }
        player.tick(dt);
        UpOrDown = ((sf->m_tl.y < (cart_coord.max_y() - frame_offset)) && UpOrDown) || 
        !(sf->m_bl.y > frame_offset);
        if( make_sf ) {
            if( sf->m_bl.y > frame_offset && !UpOrDown ) {
                sf->m_tr.add(0, -1);
            } else if ( UpOrDown ) {
                sf->m_tr.add(0,  1);
            }
        }
    }
    
    player.draw();
    set_pixel_color(0, 0, 0, 255);
    geom_list_t &list = gobjects();
    for(const geom_ref_t& g : list) {
        g->draw();
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
    float adeg=0, velocity=1;
    bool auto_shoot = false;
    pixel::forced_fps = 30;
    {
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-width", argv[i]) && i+1<argc) {
                window_width = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-height", argv[i]) && i+1<argc) {
                window_height = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-debug_gfx", argv[i]) ) {
                debug_gfx = true;
            } else if( 0 == strcmp("-fps", argv[i]) && i+1<argc) {
                pixel::forced_fps = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-adeg", argv[i]) && i+1<argc) {
                adeg = static_cast<float>( std::atof(argv[i+1]) );
                ++i;
            } else if( 0 == strcmp("-velo", argv[i]) && i+1<argc) {
                velocity = static_cast<float>( std::atof(argv[i+1]) );
                ++i;
         /* } else if( 0 == strcmp("-rho", argv[i]) && i+1<argc) {
                rho = static_cast<float>( std::atof(argv[i+1]) );
                ++i;
         */
            } else if( 0 == strcmp("-shoot", argv[i]) ) {
                auto_shoot = true;
            } else if( 0 == strcmp("-sf", argv[i]) ) {
                make_sf = true;
            }
        }
    }
    {
        pixel::log_printf(0, "Usage %s -1p -width <int> -height <int> "
                                      " -debug_gfx -show_velo\n", argv[0]);
        pixel::log_printf(0, "- win size %d x %d\n", window_width, window_height);
        pixel::log_printf(0, "- forced_fps %d\n", pixel::forced_fps);
        pixel::log_printf(0, "- debug_gfx %d\n", debug_gfx);        
    }
    {
        const float origin_norm[] = { 0.5f, 0.5f };
        init_gfx_subsystem("gfxbox example01", window_width, window_height, origin_norm);
    }
    pixel::cart_coord.set_height(-space_height/2.0f, space_height/2.0f);
    player.resize();

    std::vector<texture_ref> texts;

    log_printf(0, "XX %s\n", cart_coord.toString().c_str());
    log_printf(0, "XX adeg %f, velocity %f, auto_shoot %d\n", adeg, velocity, auto_shoot);
    printf("Pre-Loop\n");
    // const float s_height = (player.m_asr.p_b - player.m_asr.p_d).length();

    geom_list_t &list = gobjects();
    {
        rect_ref_t l = std::make_shared<rect_t>(
            point_t(cart_coord.max_x() - frame_offset - basket_width,
                    cart_coord.min_y() + frame_offset + basket_height),
            basket_frame_thickness, basket_height-basket_frame_thickness);
        rect_ref_t b = std::make_shared<rect_t>(
            point_t(cart_coord.max_x() - frame_offset - basket_width,
                    cart_coord.min_y() + frame_offset + basket_frame_thickness),
            basket_width, basket_frame_thickness);
        rect_ref_t r = std::make_shared<rect_t>(
            point_t(cart_coord.max_x() - frame_offset - basket_frame_thickness,
                    cart_coord.min_y() + frame_offset + basket_height),
            basket_frame_thickness, basket_height-basket_frame_thickness);
        sf = std::make_shared<rect_t>(point_t(l->m_tl.x - sf_thickness, 0), 
        sf_thickness, sf_height);
        if(make_sf){
            list.push_back(sf);
        }                                            
        list.push_back(l);                                            
        list.push_back(b);
        list.push_back(r);
        korb.push_back(l);                                            
        korb.push_back(b);
        korb.push_back(r);
        korb_box.resize(l->box());
        korb_box.resize(b->box());
        korb_box.resize(r->box());
    }
                                               
    rect_ref_t r1 = std::make_shared<rect_t>(point_t(cart_coord.min_x() + frame_gap,
                                                     cart_coord.min_y() + frame_offset),
                                             cart_coord.width() - 2 * frame_gap, frame_thickness);
    rect_ref_t r2 = std::make_shared<rect_t>(point_t(cart_coord.min_x() + frame_gap,
                                                     cart_coord.max_y() - frame_gap),
                                             cart_coord.width() - 2 * frame_gap, frame_thickness);
    rect_ref_t r3 = std::make_shared<rect_t>(point_t(cart_coord.min_x() + frame_gap,
                                                     cart_coord.max_y() - frame_offset),
                                             frame_thickness, cart_coord.height() - 2 * frame_offset);
    rect_ref_t r4 = std::make_shared<rect_t>(point_t(cart_coord.max_x() - frame_offset,
                                                     cart_coord.max_y() - frame_offset),
                                             frame_thickness, cart_coord.height() - 2 * frame_offset);
    list.push_back(r1);
    list.push_back(r2);
    list.push_back(r3);
    list.push_back(r4);

    player.rotate_adeg(adeg);
    player.set_velocity(velocity);
    /*
    for(physiks::ball_ref_t &b : player.m_pengs_all){
        b->set_roh(rho);
    }
    for(physiks::ball_ref_t &b : player.m_pengs_flying){
        b->set_roh(rho);
    }
    */
    if( auto_shoot ) {
        player.peng();
    }

    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
}
