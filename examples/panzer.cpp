/**
 * Author: Svenson Han Göthel
 * Funktion Name: Panzer.cpp
 */
#include <pixel/pixel4f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"
#include <tron.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <thread>

#include <random>
#include <cstdio>
#include <cmath>
#include <iostream>

int main(int argc, char *argv[])
{
    int window_width = 1920, window_height = 1000;
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
            }
        }
    }
    {
        const float origin_norm[] = { 0.5f, 0.5f };
        if( !pixel::init_gfx_subsystem("panzer", window_width, window_height, origin_norm) ) {
            return 1;
        }
    }

    std::vector<pixel::texture_ref> texts;
    pixel::f2::point_t origin(0, 0);
    const pixel::f4::vec_t text_color(0, 0, 0, 1);

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
    Tron::Panzer p1(pixel::f2::point_t(ax1, ay1));
    Tron::Panzer p2(pixel::f2::point_t(ax2, ay1));
    uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]
    int a1 = 0;
    int a2 = 0;
    pixel::input_event_t event;

    while( !event.pressed_and_clr( pixel::input_event_type_t::WINDOW_CLOSE_REQ ) ) {
        const pixel::f2::point_t tl_text(pixel::cart_coord.min_x(), pixel::cart_coord.max_y());
        while( pixel::handle_one_event(event) ) {
            // std::cout << "Event " << pixel::to_string(event) << std::endl;
            if( !event.paused() ) {
                if( event.released_and_clr(pixel::input_event_type_t::RESET) ) {
                    p1.reset();
                    p2.reset();
                    a1 = 0;
                    a2 = 0;
                }
                if( event.has_any_p1() ) {
                    if( event.released_and_clr(pixel::input_event_type_t::P1_ACTION2) ) {
                        p1.peng();
                    }
                }
                if( event.has_any_p2() ) {
                    if( event.released_and_clr(pixel::input_event_type_t::P2_ACTION2) ) {
                        p2.peng();
                    }
                }
            }
        }
        // resized = event.has_and_clr( pixel::input_event_type_t::WINDOW_RESIZED );

        if( true ) {
            float fps = pixel::gpu_avg_fps();
            texts.push_back( pixel::make_text(tl_text, 0, text_color,
                    "fps "+std::to_string(fps)+", "+(event.paused()?"paused":"animating")));
            texts.push_back( pixel::make_text(tl_text, 1, text_color,
                    "Pengs Velocity [pixel pro sec] = Velocity + 100 | Pengs: Tron "
                    +std::to_string(p1.peng_inventory)+
                    ", MCP "+std::to_string(p2.peng_inventory)));
            texts.push_back( pixel::make_text(tl_text, 2, text_color,
                    "Velocity [pixel pro sec]: Tron "+std::to_string(p1.velo)+", MCP "+std::to_string(p2.velo)+
                    " | Score: Tron "+std::to_string(a1)+", MCP "+std::to_string(a2)));
        }

        // white background
        pixel::clear_pixel_fb( 255, 255, 255, 255);
        const uint64_t t1 = pixel::getElapsedMillisecond(); // [ms]
        const float dt = (float)( t1 - t_last ) / 1000.0f; // [s]
        t_last = t1;
        if (!event.paused()) {
            if (event.pressed(pixel::input_event_type_t::P1_UP)) {
                p1.changeSpeed(1.05f);
            } else if (event.pressed(pixel::input_event_type_t::P1_DOWN)) {
                p1.changeSpeed(0.95f);
            } else if (event.pressed(pixel::input_event_type_t::P1_LEFT)) {
                if (event.pressed(pixel::input_event_type_t::P1_ACTION1)) {
                    p1.rotate_barrel(M_PI / 200);
                } else {
                    p1.rotate(M_PI / 100);
                }
            } else if (event.pressed(pixel::input_event_type_t::P1_RIGHT)) {
                if (event.pressed(pixel::input_event_type_t::P1_ACTION1)) {
                    p1.rotate_barrel(-(M_PI / 200));
                } else {
                    p1.rotate(-(M_PI / 100));
                }
            }
            if (event.pressed(pixel::input_event_type_t::P2_UP)) {
                p2.changeSpeed(1.05f);
            } else if (event.pressed(pixel::input_event_type_t::P2_DOWN)) {
                p2.changeSpeed(0.95f);
            } else if (event.pressed(pixel::input_event_type_t::P2_LEFT)) {
                if (event.pressed(pixel::input_event_type_t::P2_ACTION1)) {
                    p2.rotate_barrel((M_PI / 200));
                } else {
                    p2.rotate((M_PI / 100));
                }
            } else if (event.pressed(pixel::input_event_type_t::P2_RIGHT)) {
                if (event.pressed(pixel::input_event_type_t::P2_ACTION1)) {
                    p2.rotate_barrel(-(M_PI / 200));
                } else {
                    p2.rotate(-(M_PI / 100));
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
            tex->draw_fbcoord(0, 0);
        }
        texts.clear();
        pixel::swap_gpu_buffer(30);
    }
    printf("Exit\n");
    exit(0);
}
