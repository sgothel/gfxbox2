/**
 * Author: Svenson Han Göthel
 * Funktion Name: Tron.cpp
 */
#include <pixel/pixel4f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"
#include <random>
#include <cstdio>
#include <cmath>
#include <iostream>

class Tron : public pixel::f2::linestrip_t {
public:
    const pixel::f2::point_t sp;
    float angle;
    float velo;
    pixel::f2::point_t head;
    pixel::f2::point_t last;
    pixel::f2::point_t last_last;
    pixel::f2::disk_t body;

    Tron(pixel::f2::point_t sp_)
    : sp(sp_), body(head, 5)
    {
        reset();
    }

    void rotate(float angrad) noexcept override {
        // std::cout << "rot.pre " << toString() << std::endl ;
        angle += angrad;
        if(false){
            if( pixel::compare(angle, 2*M_PI) >= 0 ) { // p1_angle >= 2*M_PI
                angle = 0;
            } else if( pixel::compare(angle, 0) < 0 ) { // p1_angle < 0
                angle = 1.5f * M_PI;
            }
        }
        p_list.push_back(head);
        last_last = last;
        last = head;
        //std::cout << "rot.post " << toString() << std::endl ;
    }

    /**
     *
     * @param dt in seconds
     */
    bool tick(const float dt) noexcept override {
        pixel::f2::point_t p1_move = pixel::f2::point_t::from_length_angle(velo * dt, angle);
        // std::cout << "tick " << p1_move.toString() << " + " << head.toString();
        head += p1_move;
        // std::cout << " = " << head.toString() << std::endl;
        body.center = head;

        return true;
    }

    void reset() noexcept {
        last = sp;
        head = sp;
        last_last = sp;
        body.center = head;
        p_list.clear();
        p_list.push_back(last_last);
        angle = M_PI_2;
        velo = 2.0f / 0.016f; // 2 pixel pro 16 ms
    }

    void draw() const noexcept override {
        pixel::f2::lineseg_t::draw(last, head);
        pixel::f2::lineseg_t::draw(last_last, last);
        pixel::f2::linestrip_t::draw();
        body.draw(true);
    }

    void changeSpeed(float a){
        velo *= a;
    }

    std::string toString() const noexcept override {
        return "Tron[h "+head.toString()+", a "+std::to_string(angle)+", v "+std::to_string(velo)+
                ", tail "+std::to_string(p_list.size())+"]";
    }

    bool intersects(const Tron& o) const {
        const pixel::f2::lineseg_t h(head, last);
        const pixel::f2::lineseg_t o_h(o.head, o.last);
        return my_intersects(h) ||
                o.intersects_lineonly(h) ||
                o_h.intersects(h);
    }
private:
    bool my_intersects(const pixel::f2::lineseg_t & o) const noexcept {
        if( p_list.size() < 3 ) {
            return false;
        }
        pixel::f2::point_t p0 = p_list[0];
        for(size_t i=1; i<p_list.size()-1; ++i) {
            const pixel::f2::point_t& p1 = p_list[i];
            const pixel::f2::lineseg_t l(p0, p1);
            if( l.intersects(o) ) {
                return true;
            }
            p0 = p1;
        }
        return false;
    }

};

void mainloop() {
    static const pixel::f4::vec_t text_color(0.5f, 0.5f, 0.5f, 1.0f);
    static const float ax1 = pixel::cart_coord.min_x() + 3 * pixel::cart_coord.width() / 4;
    static const float ax2 = pixel::cart_coord.min_x() + pixel::cart_coord.width() / 4;
    static const float ay1 = pixel::cart_coord.min_y() + 100;
    static const int text_height = 24;

    static Tron p1(pixel::f2::point_t(ax1, ay1));
    static Tron p2(pixel::f2::point_t(ax2, ay1));
    static uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]
    static int a1 = 0;
    static int a2 = 0;
    static pixel::input_event_t event;

    pixel::handle_events(event);
    if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_CLOSE_REQ ) ) {
        printf("Exit Application\n");
        #if defined(__EMSCRIPTEN__)
            emscripten_cancel_main_loop();
        #else
            exit(0);
        #endif
    } else if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_RESIZED ) ) {
        // nop for this demo, resize already performed
    }
    const bool animating = !event.paused();

    const pixel::f2::point_t tl_text(pixel::cart_coord.min_x(), pixel::cart_coord.max_y());
    pixel::texture_ref hud_text = pixel::make_text(tl_text, 0, text_color, text_height,
            "Tron %4d (%.2f m/s), MCP %4d (%.2f m/s), fps %5.2f, %s",
            a1, p1.velo, a2, p2.velo, pixel::get_gpu_fps(), animating?"animating":"paused");

    // white background
    pixel::clear_pixel_fb( 255, 255, 255, 255);
    const uint64_t t1 = pixel::getElapsedMillisecond(); // [ms]
    const float dt = (float)( t1 - t_last ) / 1000.0f; // [s]
    t_last = t1;
    if( event.released_and_clr(pixel::input_event_type_t::RESET) ){
        p1.reset();
        p2.reset();
        a1 = 0;
        a2 = 0;
    }
    if( event.has_any_p1() ) {
        if( event.pressed_and_clr(pixel::input_event_type_t::P1_UP) && p1.velo < 3000.0f) {
            p1.changeSpeed(1.10f);
        } else if( event.pressed_and_clr(pixel::input_event_type_t::P1_DOWN) && p1.velo > 1.0f) {
            p1.changeSpeed(0.90f);
        } else if( event.pressed_and_clr(pixel::input_event_type_t::P1_LEFT) ) {
            p1.rotate(M_PI_2);
        } else if( event.pressed_and_clr(pixel::input_event_type_t::P1_RIGHT) ) {
            p1.rotate(-M_PI_2);
        }
    }
    if( event.has_any_p2() ) {
        if( event.pressed_and_clr(pixel::input_event_type_t::P2_UP) && p2.velo < 3000.0f) {
            p2.changeSpeed(1.10f);
        } else if( event.pressed_and_clr(pixel::input_event_type_t::P2_DOWN) && p2.velo > 1.0f) {
            p2.changeSpeed(0.90f);
        } else if( event.pressed_and_clr(pixel::input_event_type_t::P2_LEFT) ) {
            p2.rotate(M_PI_2);
        } else if( event.pressed_and_clr(pixel::input_event_type_t::P2_RIGHT) ) {
            p2.rotate(-M_PI_2);
        }
    }
    float dbs = 2000;
    if(animating){
        if(p1.velo > 10.0f && p1.velo < dbs){
            p1.velo -= 0.01;
        } else if(p1.velo > dbs){
            p1.velo -= 1;
        }

        if(p2.velo > 10.0f && p1.velo < dbs){
            p2.velo -= 0.01;
        } else if(p2.velo > dbs){
            p2.velo -= 1;
        }

        p1.tick(dt);
        if(!p1.body.on_screen()){
            //  std::cout << "Exited P1: " << p1.toString() << std::endl;
            p1.reset();
            a2 += 100;
        }

        if(p1.velo < dbs){
            if( p1.intersects(p2) ) {
                //     std::cout << "Crash P1: " << p1.toString() << std::endl;
                p1.reset();
                a2 += p2.velo;
            }
        }
        p2.tick(dt);
        if(!p2.body.on_screen()){
            //   std::cout << "Exited P2: " << p2.toString() << std::endl;
            p2.reset();
            a1 += 100;
        }

        if(p2.velo < dbs){
            if( p2.intersects(p1) ) {
                //  std::cout << "Crash P2: " << p2.toString() << std::endl;
                p2.reset();
                a1 += p1.velo;
            }
        }
    }


    pixel::set_pixel_color(0, 0, 255, 255);
    p1.draw();

    pixel::set_pixel_color(255, 0, 0, 255);
    p2.draw();

    pixel::swap_pixel_fb(false);
    if( nullptr != hud_text ) {
        const int dx = ( pixel::fb_width - pixel::round_to_int(hud_text->width*hud_text->dest_sx) ) / 2;
        hud_text->draw(dx, 0);
    }
    pixel::swap_gpu_buffer();
}

int main(int argc, char *argv[])
{
    unsigned int win_width = 1920, win_height = 1000;
    #if defined(__EMSCRIPTEN__)
        win_width = 1024, win_height = 576; // 16:9
    #endif
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
    pixel::log_printf(0, "XX %s\n", pixel::cart_coord.toString().c_str());
    {
        float w = pixel::cart_coord.width();
        float h = pixel::cart_coord.height();
        float r01 = h/w;
        float a = w / h;
        printf("-w %f [x]\n-h %f [y]\n-r1 %f [y/x]\n-r2 %f [x/y]\n", w, h, r01, a);
    }
    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
}
