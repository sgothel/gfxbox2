/*
 * Author: Hermann Kai Gothel <hkg@jausoft.com>
 * Copyright (c) 2026 Gothel Software e.K.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"

#include <cstdio>
#include <cmath>

using namespace jau;

void kreis(pixel::f2::point_t m, float r) {
    using pixel::f2::point_t;
    point_t p0 {m.x - r, m.y - r};
    point_t p1 {m.x + r, m.y + r};
    for (float y = p0.y; y <= p1.y; ++y) {
        for (float x = p0.x; x <= p1.x; ++x) {
            float a = m.y - y;
            float b = m.x - x;
            float c = sqrtf(a * a + b * b);
            if (c <= r) {
                point_t p = { x, y};
                p.draw();
            }
        }
    }
}
void donute(pixel::f2::point_t m, float r, float r2) {
    using pixel::f2::point_t;
    point_t p0 {m.x - r, m.y - r};
    point_t p1 {m.x + r, m.y + r};
    for (float y = p0.y; y <= p1.y; ++y) {
        for (float x = p0.x; x <= p1.x; ++x) {
            float a = m.y - y;
            float b = m.x - x;
            float c = sqrtf(a * a + b * b);
            if (c <= r && c>r2) {
                point_t p = { x, y};
                p.draw();
            }
        }
    }
}

#if 0
void donut(punkt m, int r1, int r2) {
    if(r2>r1){
        std::cout<<"r2>r1";
    }
    punkt p0 = {.x = m.x - r1, .y = m.y - r1};
    punkt p1 = {.x = m.x + r1, .y = m.y + r1};
    for (int i = 0; i < p0.y; ++i) {
        std::cout << "\n";
    }
    for (int y = p0.y; y <= p1.y; ++y) {
        for (int i = 0; i < p0.x; ++i) {
            std::cout << " ";
        }
        for (int x = p0.x; x <= p1.x; ++x) {
            int a = m.y - y;
            int b = m.x - x;
            int c = sqrt(a * a + b * b);
            if (c <= r1 && c>r2) {
                std::cout << "x";
            } else {
                std::cout << " ";
            }
        }
        std::cout << "\n";
    }
}
#endif

void mainloop() {
    static uint64_t t_last = getElapsedMillisecond(); // [ms]
    static pixel::input_event_t event;
    static bool animating = true;

    const float grid_gap = 50;

    while(pixel::handle_one_event(event)){
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
        if( event.paused() ) {
            animating = false;
        } else {
            if( !animating ) {
                t_last = getElapsedMillisecond(); // [ms]
            }
            animating = true;
        }
    }
    const uint64_t t1 = getElapsedMillisecond(); // [ms]
    const float dt = (float)( t1 - t_last ) / 1000.0f; // [s]
    t_last = t1;

    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
    pixel::texture_ref hud_text = pixel::make_text("td %s, fps %2.2f, dt %d ms, %s",
            to_decstring(t1, ',', 9).c_str(), pixel::gpu_avg_fps(), (int)(dt*1000), animating?"animating":"paused");

    // white background
    pixel::clear_pixel_fb(255, 255, 255, 255);
    pixel::draw_grid(grid_gap,
            225 /* r */, 225 /* g */, 225 /* b */, 255 /* a */,
            200 /* r */, 200 /* g */, 200 /* b */, 255 /* a */);

    pixel::set_pixel_color(  0 /* r */,   0 /* g */, 255 /* b */, 255 /* a */);
    // kreis({0, 0}, pixel::cart_coord.height()/4.0f);
    donute({0, 0}, pixel::cart_coord.height()/4.0f, pixel::cart_coord.height()/5.0f);
    
    pixel::swap_pixel_fb(false);
    if( nullptr != hud_text ) {
        hud_text->draw_fbcoord(0, 0);
    }
    pixel::swap_gpu_buffer();
}

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
        if( !pixel::init_gfx_subsystem(argv[0], "sandbox01", window_width, window_height, origin_norm, true, true /* subsys primitives */) ) {
            return 1;
        }
    }

    log_printf(0, "XX %s\n", pixel::cart_coord.toString().c_str());
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
