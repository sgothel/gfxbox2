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

void draw_circle_seg(const pixel::f2::point_t& pm, float r, const float thickness, float const alpha1, const float alpha2){
    float x;
    float y;
    float i = alpha1;
    for(; i <= alpha2; i += 0.01){
        x = std::cos(i) * r;
        y = std::sin(i) * r;
        pixel::f2::point_t p0 = pixel::f2::point_t(x, y);
        p0 += pm;
        if( 1 >= thickness ) {
            p0.draw();
        } else {
            const float t = thickness*2.0f/3.0f;
            pixel::f2::rect_t(p0, t, t, true).draw(true);
        }
    }
}

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
        void tick(const float dt) noexcept {
            pixel::f2::point_t p1_move = pixel::f2::point_t::from_length_angle(velo * dt, angle);
            // std::cout << "tick " << p1_move.toString() << " + " << head.toString();
            head += p1_move;
            // std::cout << " = " << head.toString() << std::endl;
            body.center = head;
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
            if(velo > 2.0f) {
                velo *= a;
            }
        }

        std::string toString() const noexcept {
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
pixel::texture_ref make_text(const pixel::f2::point_t& pos, const std::string& text, const pixel::f4::vec_t& color) {
    pixel::set_pixel_color4f(color.x, color.y, color.z, color.w);
    pixel::texture_ref tex = pixel::make_text_texture(text.c_str());
    tex->dest_x = pixel::cart_coord.to_fb_x(pos.x);
    tex->dest_y = pixel::cart_coord.to_fb_y(pos.y);
    return tex;
}

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

    bool close = false;
    bool resized = false;
    bool set_dir = false;
    bool set_dir2 = false;
    pixel::direction_t dir = pixel::direction_t::UP;
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
    Tron p1(pixel::f2::point_t(ax1, ay1));
    Tron p2(pixel::f2::point_t(ax2, ay1));
    uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]
    int a1 = 0;
    int a2 = 0;

    while( !close ) {
        pixel::handle_events2(close, resized, set_dir, set_dir2, dir);
        const bool animating = pixel::direction_t::PAUSE != dir;

        float fps = pixel::get_gpu_fps();
        texts.push_back( make_text(
                pixel::f2::point_t(pixel::cart_coord.min_x(), pixel::cart_coord.max_y()),
                "fps "+std::to_string(fps)+", "+(animating?"animating":"paused"), text_color));

        // white background
        pixel::clear_pixel_fb( 255, 255, 255, 255);

        const uint64_t t1 = pixel::getElapsedMillisecond(); // [ms]
        const float dt = (float)( t1 - t_last ) / 1000.0f; // [s]
        t_last = t1;
        if(animating){
            if( set_dir ) {
                set_dir = false;
                if( pixel::direction_t::UP == dir ) {
                    p1.changeSpeed(1.10f);
                } else if( pixel::direction_t::DOWN == dir ) {
                    p1.changeSpeed(0.90f);
                } else if( pixel::direction_t::LEFT == dir ) {
                    p1.rotate(M_PI_2);
                } else if( pixel::direction_t::RIGHT == dir ) {
                    p1.rotate(-M_PI_2);
                } else if( pixel::direction_t::RESET == dir ){
                    p1.reset();
                    p2.reset();
                    a1 = 0;
                    a2 = 0;
                }

            }

            if( set_dir2 ) {
                set_dir2 = false;
                if( pixel::direction_t::UP2 == dir ) {
                    p2.changeSpeed(1.10f);
                } else if( pixel::direction_t::DOWN2 == dir ) {
                    p2.changeSpeed(0.90f);
                } else if( pixel::direction_t::LEFT2 == dir ) {
                    p2.rotate(M_PI_2);
                } else if( pixel::direction_t::RIGHT2 == dir ) {
                    p2.rotate(-M_PI_2);
                }
            }
        }
        if(animating){
            p1.tick(dt);
            if(!p1.body.on_screen()){
                std::cout << "Exited P1: " << p1.toString() << std::endl;
                p1.reset();
                a1 += 100;
            }

            if( p1.intersects(p2) ) {
                std::cout << "Crash P1: " << p1.toString() << std::endl;
                p1.reset();
                a1 += p2.velo;
            }

            p2.tick(dt);
            if(!p2.body.on_screen()){
                std::cout << "Exited P2: " << p2.toString() << std::endl;
                p2.reset();
                a2 += 100;
            }

            if( p2.intersects(p1) ) {
                std::cout << "Crash P2: " << p2.toString() << std::endl;
                p2.reset();
                a2 += p1.velo;
            }
        }
        texts.push_back( make_text(
                pixel::f2::point_t(pixel::cart_coord.min_x() + 1000, pixel::cart_coord.max_y()), "Tron: "+std::to_string(a1)+
                ", Master Controll Programm: "+std::to_string(a2), text_color));
        pixel::set_pixel_color(255, 0, 0, 255);
        p1.draw();

        pixel::set_pixel_color(0, 0, 255, 255);
        p2.draw();

        pixel::swap_pixel_fb(false);
        for(pixel::texture_ref tex : texts) {
            tex->draw(0, 0);
        }
        texts.clear();
        pixel::swap_gpu_buffer();
    }
    printf("Exit\n");
    exit(0);
}
