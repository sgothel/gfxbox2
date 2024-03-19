/**
 * Author: Svenson Han Göthel
 * Funktion Name: Panzerjagd.cpp
 */
#include <pixel/pixel4f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <thread>

#include <random>
#include <cstdio>
#include <cmath>
#include <iostream>

class peng_t {
public:
    constexpr static const float diag = 1.0f * 25.0f / 2.0f ;
    pixel::f2::vec_t velo; // [m/s]
    pixel::f2::rect_t peng;

    peng_t(const pixel::f2::point_t& p0, const float v, const float angle)
    : velo( pixel::f2::point_t::from_length_angle(v, angle) ),
      peng(p0 + pixel::f2::point_t(-diag/2, +diag/2), diag, diag, angle)
    { }
    peng_t(const pixel::f2::point_t& p0, const pixel::f2::vec_t& v)
    : velo( v ),
      peng(p0 + pixel::f2::point_t(-diag/2, +diag/2), diag, diag, v.angle())
    { }

    bool tick(const float dt) noexcept {
        velo.add(-0.0001f, -0.0001f);
        peng.move( velo * dt );
        return true;
    }

    void draw() const noexcept {
        peng.draw(false);
    }

    void changeSpeed(float a){
        velo *= a;
    }

    bool on_screen(){
        return peng.on_screen();
    }
    bool intersection(const peng_t& o) const {
        return peng.intersects(o.peng);
    }
};

class Panzer : public pixel::f2::linestrip_t {
public:
    constexpr static const float velo_0 = 10.0f; // 10 pixel pro s
    constexpr static const float velo_max = 200.0f; // 10 pixel pro s
    constexpr static const float peng_velo_0 = 100.0f; // 10 pixel pro s
    constexpr static const int peng_inventory_max = 50; // 45 pengs

    constexpr static const float length = 100.0f;
    constexpr static const float width = length*0.6f;
    constexpr static const float barrel_l = length * 0.8f;
    constexpr static const float barrel_w = barrel_l * 0.25f;
    constexpr static pixel::f2::point_t body_tl(const pixel::f2::point_t& sp) noexcept {
        return sp + pixel::f2::vec_t(-width/2, length/2);
    }
    constexpr static pixel::f2::point_t barrel_tl(const pixel::f2::point_t& sp) noexcept {
        return sp + pixel::f2::vec_t(0, barrel_w / 2);
    }

    const pixel::f2::point_t sp;
    float velo;
    // pixel::f2::disk_t body;
    pixel::f2::rect_t body;
    pixel::f2::rect_t barrel;
    std::vector<peng_t> pengs;
    int peng_inventory;

    Panzer(pixel::f2::point_t sp_)
    : sp(sp_), velo(velo_0),
      body(body_tl(sp), width, length, 0),
      barrel( barrel_tl( sp ), barrel_l, barrel_w, 0 ),
      peng_inventory(peng_inventory_max)
    {
        rotate_barrel(M_PI_2);
    }

    void reset(const bool e = true) noexcept {
        peng_inventory = peng_inventory_max;
        velo = velo_0;
        body = pixel::f2::rect_t(body_tl(sp), width, length, 0);
        barrel = pixel::f2::rect_t( barrel_tl( sp ), barrel_l, barrel_w, 0);
        if(e){
            pengs.clear();
        }
        rotate_barrel(M_PI_2);
    }

    const pixel::f2::point_t& center() const noexcept {
        return body.p_center;
    }

    void rotate(float angrad) noexcept override {
        // std::cout << "rot.pre " << toString() << std::endl ;
        body.rotate(angrad, center());
        rotate_barrel(angrad);
        //std::cout << "rot.post " << toString() << std::endl ;
    }
    void rotate_barrel(float angrad) noexcept {
        // std::cout << "rot.pre " << toString() << std::endl ;
        barrel.rotate(angrad, center());
        //std::cout << "rot.post " << toString() << std::endl ;
    }

    void peng() noexcept {
        if(peng_inventory > 0){
            pixel::f2::point_t p0 = center() +
                                    pixel::f2::point_t::from_length_angle(barrel_l + 0.001f, barrel.dir_angle);
            pengs.push_back( peng_t(p0, velo + peng_velo_0, barrel.dir_angle) );
            --peng_inventory;
        }
    }
    size_t peng_count() const noexcept { return pengs.size(); }

    /**
     *
     * @param dt in seconds
     */
    bool tick(const float dt) noexcept override {
        if( velo > 0.01f ) {
            velo -= 0.01f;
        }
        pixel::f2::point_t p1_move = pixel::f2::point_t::from_length_angle(velo * dt, body.dir_angle + M_PI_2);
        // std::cout << "tick " << p1_move.toString() << " + " << head.toString();
        body.move(p1_move);
        barrel.move(p1_move);
        // std::cout << " = " << head.toString() << std::endl;

        if( !body.on_screen()) {
            pixel::f2::point_t new_center = center();
            if( new_center.x < pixel::cart_coord.min_x() ) {
                new_center.x = pixel::cart_coord.max_x() - width;
            }
            if( new_center.x > pixel::cart_coord.max_x() ) {
                new_center.x = pixel::cart_coord.min_x() + width;
            }
            if( new_center.y < pixel::cart_coord.min_y() ) {
                new_center.y = pixel::cart_coord.max_y() - width;
            }
            if( new_center.y > pixel::cart_coord.max_y() ) {
                new_center.y = pixel::cart_coord.min_y() + width;
            }
            pixel::f2::vec_t m = new_center - center();
            body.move(m);
            barrel.move(m);
        }

        for(auto it = pengs.begin(); it != pengs.end(); ) {
            peng_t& p = *it;
            if(p.on_screen() && p.velo.length_sq() > 0){
                p.tick(dt);
                ++it;
            } else {
                it = pengs.erase(it);
            }
        }
        return true;
    }

    void draw() const noexcept override {
        barrel.draw(false);
        body.draw(false);
        // std::cout << "b " << body.toString() << ", r " << rect.toString() << std::endl;
        // std::vector<peng_t>::iterator it ;-)
        for(auto it = pengs.begin(); it != pengs.end(); ++it) {
            (*it).draw();
        }
    }

    bool on_screen(){ return body.on_screen(); }

    void changeSpeed(float a){
        velo *= a;
        if( a > 1 && velo < velo_0 ) {
            velo = velo_0;
        }
        if( velo > velo_max ) {
            velo = velo_max;
        } else if( velo < -velo_max ) {
            velo = -velo_max;
        }
    }

    bool hit(Panzer o){
        for(auto it = pengs.begin(); it != pengs.end(); ) {
            peng_t& p = *it;
            if( o.body.intersects(p.peng)){
                it = pengs.erase(it);
                return true;
            } else {
                ++it;
            }
        }
        return false;
    }

    std::string toString() const noexcept {
        return "Panzer[c "+center().toString()+", a "+std::to_string(barrel.dir_angle)+
                ", v "+std::to_string(velo)+
                ", tail "+std::to_string(p_list.size())+"]";
    }
};

int main(int argc, char *argv[])
{
    unsigned int win_width = 1920, win_height = 1000;
    {
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-width", argv[i]) && i+1<argc) {
                win_width = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-height", argv[i]) && i+1<argc) {
                win_height = atoi(argv[i+1]);
                ++i;
            }
        }
    }
    {
        const float origin_norm[] = { 0.5f, 0.5f };
        pixel::init_gfx_subsystem("gfxbox example01", win_width, win_height, origin_norm);
    }

    std::vector<pixel::texture_ref> texts;
    pixel::f2::point_t origin(0, 0);
    pixel::f4::vec_t text_color(0, 0, 0, 1);

    pixel::log_printf(0, "XX %s\n", pixel::cart_coord.toString().c_str());
    {
        float w = pixel::cart_coord.width();
        float h = pixel::cart_coord.height();
        float r01 = h/w;
        float a = w / h;
        printf("-w %f [x]\n-h %f [y]\n-r1 %f [y/x]\n-r2 %f [x/y]", w, h, r01, a);
    }
    printf("Pre-Loop\n");
    const float ax1 = pixel::cart_coord.min_x() + 3 * pixel::cart_coord.width() / 4;
    const float ax2 = pixel::cart_coord.min_x() + pixel::cart_coord.width() / 4;
    const float ay1 = pixel::cart_coord.min_y() + 100;
    Panzer p1(pixel::f2::point_t(ax1, ay1));
    Panzer p2(pixel::f2::point_t(ax2, ay1));
    uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]
    int a1 = 0;
    int a2 = 0;
    pixel::input_event_t event;

    while( !event.pressed_and_clr( pixel::input_event_type_t::WINDOW_CLOSE_REQ ) ) {
        const pixel::f2::point_t tl_text(pixel::cart_coord.min_x(), pixel::cart_coord.max_y());
        if( pixel::handle_events(event) ) {
            // std::cout << "Event " << pixel::to_string(event) << std::endl;
        }
        // resized = event.has_and_clr( pixel::input_event_type_t::WINDOW_RESIZED );

        if( true ) {
            float fps = pixel::get_gpu_fps();
            texts.push_back( pixel::make_text(tl_text, 0,
                    "fps "+std::to_string(fps)+", "+(event.paused()?"paused":"animating"), text_color));
            texts.push_back( pixel::make_text(tl_text, 1,
                    "Pengs Velocity [pixel pro sec] = Velocity + 100 | Pengs: Tron "
                    +std::to_string(p1.peng_inventory)+
                    ", MCP "+std::to_string(p2.peng_inventory), text_color));
            texts.push_back( pixel::make_text(tl_text, 2,
                    "Velocity [pixel pro sec]: Tron "+std::to_string(p1.velo)+", MCP "+std::to_string(p2.velo)+
                    " | Score: Tron "+std::to_string(a1)+", MCP "+std::to_string(a2), text_color));
        }

        // white background
        pixel::clear_pixel_fb( 255, 255, 255, 255);
        const uint64_t t1 = pixel::getElapsedMillisecond(); // [ms]
        const float dt = (float)( t1 - t_last ) / 1000.0f; // [s]
        t_last = t1;
        if( !event.paused() ) {
            if( event.released_and_clr(pixel::input_event_type_t::RESET) ) {
                p1.reset();
                p2.reset();
                a1 = 0;
                a2 = 0;
            }
            if( event.has_any_p1() ) {
                if( event.pressed(pixel::input_event_type_t::P1_UP) ) {
                    p1.changeSpeed(1.05f);
                } else if( event.pressed(pixel::input_event_type_t::P1_DOWN) ) {
                    p1.changeSpeed(0.95f);
                } else if( event.pressed(pixel::input_event_type_t::P1_LEFT) ) {
                    if( event.pressed(pixel::input_event_type_t::P1_ACTION1) ) {
                        p1.rotate_barrel(M_PI / 200);
                    } else {
                        p1.rotate(M_PI / 100);
                    }
                } else if( event.pressed(pixel::input_event_type_t::P1_RIGHT) ) {
                    if( event.pressed(pixel::input_event_type_t::P1_ACTION1) ) {
                        p1.rotate_barrel(-(M_PI / 200));
                    } else {
                        p1.rotate(-(M_PI / 100));
                    }
                } else if( event.released_and_clr(pixel::input_event_type_t::P1_ACTION2) ) {
                    p1.peng();
                }
            }
            if( event.has_any_p2() ) {
                if( event.pressed(pixel::input_event_type_t::P2_UP) ) {
                    p2.changeSpeed(1.05f);
                } else if( event.pressed(pixel::input_event_type_t::P2_DOWN) ) {
                    p2.changeSpeed(0.95f);
                } else if( event.pressed(pixel::input_event_type_t::P2_LEFT) ) {
                    if( event.pressed(pixel::input_event_type_t::P2_ACTION1) ) {
                        p2.rotate_barrel((M_PI / 200));
                    } else {
                        p2.rotate((M_PI / 100));
                    }
                } else if( event.pressed(pixel::input_event_type_t::P2_RIGHT) ) {
                    if( event.pressed(pixel::input_event_type_t::P2_ACTION1) ) {
                        p2.rotate_barrel(-(M_PI / 200));
                    } else {
                        p2.rotate(-(M_PI / 100));
                    }
                } else if( event.released_and_clr(pixel::input_event_type_t::P2_ACTION2) ) {
                    p2.peng();
                }
            }

            p1.tick(dt);
            p2.tick(dt);

            if (p1.hit(p2)){
                p2.reset(false);
                a1 += p1.velo;
            }
            if (p1.hit(p1)){
                p1.reset(false);
                a2 += 100;
            }
            if (p2.hit(p2)){
                p2.reset(false);
                a1 += 100;
            }
            if (p2.hit(p1)){
                p1.reset(false);
                a2 += p2.velo;
            }
        }
        if(p1.body.intersects(p2.body)){
            p1.reset();
            p2.reset();
        }
        pixel::set_pixel_color(0, 0, 255, 255);
        p1.draw();

        pixel::set_pixel_color(255, 0, 0, 255);
        p2.draw();

        pixel::swap_pixel_fb(false);
        for(pixel::texture_ref tex : texts) {
            tex->draw(0, 0);
        }
        texts.clear();
        pixel::swap_gpu_buffer(30);
    }
    printf("Exit\n");
    exit(0);
}
