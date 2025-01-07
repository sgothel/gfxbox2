/*
 * Author: Svenson Han Göthel und Sven Göthel
 * Copyright (c) 2022 Gothel Software e.K.
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
#include <pixel/pixel3f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include <vector>
#include "pixel/pixel.hpp"
#include "physics.hpp"

#include <cinttypes>

using namespace jau;

const float rho_default = 0.75f;
const float drop_height = 2.0f; // [m]
const float earth_accel = 9.81f; // [m/s*s]

static float rho = rho_default;
static bool debug_gfx = false;

extern "C" {
    EMSCRIPTEN_KEEPALIVE void set_debug_gfx(bool v) noexcept { debug_gfx = v; }
    EMSCRIPTEN_KEEPALIVE void set_rho(float v) noexcept { rho = v; }
}

/**
 * A bouncing ball w/ initial velocity in given direction plus gravity exposure (falling)
 */

static const float ball_height = 0.05f; // [m] .. diameter
static const float ball_radius = ball_height/2.0f; // [m]
static const float small_gap = ball_radius;
static const float thickness = 1.0f * ball_height;

static std::string record_bmpseq_basename;

typedef std::shared_ptr<physiks::ball_t> ball_ref_t;
typedef std::vector<ball_ref_t> ball_list_t;
static ball_list_t ball_list;

void mainloop() {
    static uint64_t frame_count_total = 0;

    static uint64_t t_last = getElapsedMillisecond(); // [ms]
    static pixel::input_event_t event;
    static bool animating = true;

    while(pixel::handle_one_event(event)){
        if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_CLOSE_REQ ) ) {
            printf("Exit Application\n");
            #if defined(__EMSCRIPTEN__)
                emscripten_cancel_main_loop();
            #else
                exit(0);
            #endif
        } else if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_RESIZED ) ) {
            pixel::cart_coord.set_height(0.0f, drop_height+6.0f*thickness);
        }
        if( event.released_and_clr(pixel::input_event_type_t::RESET) ) {
            for(ball_ref_t &g : ball_list) {
                g->reset(true);
            }
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
    uint64_t t1;
    if(animating){
        t1 = getElapsedMillisecond(); // [ms]
    } else {
        t1 = t_last;
        if (event.has_any_p1()) {
            if (event.pressed(pixel::input_event_type_t::P1_RIGHT)) {
                t1 += 1;
            } else if (event.pressed(pixel::input_event_type_t::P1_UP)) {
                t1 += 10;
            }
        }
    }
    const float dt = (float)( t1 - t_last ) / 1000.0f; // [s]
    t_last = t1;

    // white background
    pixel::clear_pixel_fb(255, 255, 255, 255);

    pixel::texture_ref hud_text = pixel::make_text_texture("td %s, fps %2.2f, rho %.2f",
            to_decstring(t1, ',', 9).c_str(), pixel::gpu_avg_fps(), rho);

    for(ball_ref_t &g : ball_list) {
        g->tick(dt);
    }
    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
    {
        pixel::f2::geom_list_t& list = pixel::f2::gobjects();
        if( debug_gfx ) {
            for(pixel::f2::geom_ref_t &g : list) {
                if( g.get() != ball_list[0].get() &&
                    g.get() != ball_list[1].get() &&
                    g.get() != ball_list[2].get() )
                {
                    g->draw();
                }
            }
        } else {
            for(pixel::f2::geom_ref_t &g : list) {
                g->draw();
            }
        }
    }

    fflush(nullptr);
    pixel::swap_pixel_fb(false);
    if( nullptr != hud_text ) {
        const int thickness_pixel = pixel::cart_coord.to_fb_dy(thickness);
        const int small_gap_pixel = pixel::cart_coord.to_fb_dy(small_gap);
        const int text_height = thickness_pixel - 2;
        const float sy = (float)text_height / (float)hud_text->height;
        hud_text->draw_fbcoord(small_gap_pixel*2, small_gap_pixel+1, sy, sy);
    }
    pixel::swap_gpu_buffer();
    if( record_bmpseq_basename.size() > 0 ) {
        std::string snap_fname(128, '\0');
        const int written = std::snprintf(&snap_fname[0], snap_fname.size(), "%s-%7.7" PRIu64 ".bmp", record_bmpseq_basename.c_str(), frame_count_total);
        snap_fname.resize(written);
        pixel::save_snapshot(snap_fname);
    }
    ++frame_count_total;
}

int main(int argc, char *argv[])
{
    int window_width = 1920, window_height = 1080;
    bool enable_vsync = true;
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
            } else if( 0 == strcmp("-record", argv[i]) && i+1<argc) {
                record_bmpseq_basename = argv[i+1];
                ++i;
            } else if( 0 == strcmp("-debug_gfx", argv[i]) ) {
                debug_gfx = true;
            } else if( 0 == strcmp("-fps", argv[i]) && i+1<argc) {
                pixel::set_gpu_forced_fps(atoi(argv[i+1]));
                ++i;
            } else if( 0 == strcmp("-no_vsync", argv[i]) ) {
                enable_vsync = false;
            } else if( 0 == strcmp("-rho", argv[i]) && i+1<argc) {
                rho = atof(argv[i+1]);
                ++i;
            }
        }
    }
    {
        const uint64_t elapsed_ms = getElapsedMillisecond();
        log_printf(elapsed_ms, "Usage %s -width <int> -height <int> -record <bmp-files-basename> -debug_gfx -fps <int>\n", argv[0]);
        log_printf(elapsed_ms, "- win size %d x %d\n", window_width, window_height);
        log_printf(elapsed_ms, "- record %s\n", record_bmpseq_basename.size()==0 ? "disabled" : record_bmpseq_basename.c_str());
        log_printf(elapsed_ms, "- debug_gfx %d\n", debug_gfx);
        log_printf(elapsed_ms, "- enable_vsync %d\n", enable_vsync);
        log_printf(elapsed_ms, "- forced_fps %d\n", pixel::gpu_forced_fps());
        log_printf(elapsed_ms, "- rho %f\n", rho);
    }

    {
        const float origin_norm[] = { 0.5f, 0.5f };
        if( !pixel::init_gfx_subsystem("freefall01", window_width, window_height, origin_norm, enable_vsync, true /* subsys primitives */) ) {
            return 1;
        }
    }

    pixel::cart_coord.set_height(0.0f, drop_height+6.0f*thickness);

    {
        const uint64_t elapsed_ms = getElapsedMillisecond();
        if( debug_gfx ) {
            log_printf(elapsed_ms, "XX %s\n", pixel::cart_coord.toString().c_str());
        }

        std::shared_ptr<physiks::ball_t> ball_1 = physiks::ball_t::create(
            "one",
            pixel::f2::point_t(-4.0f*ball_height, drop_height-ball_radius),
            ball_radius,
            0.0f /* [m/s] */,
            adeg_to_rad(90),
            earth_accel,
            drop_height,
            debug_gfx);
        std::shared_ptr<physiks::ball_t> ball_2 = physiks::ball_t::create(
            "two",
            pixel::f2::point_t(+2.0f*ball_height, drop_height-ball_radius),
            ball_radius,
            0.0f /* [m/s] */,
            adeg_to_rad(90),
            earth_accel,
            drop_height,
            debug_gfx);
        std::shared_ptr<physiks::ball_t> ball_3 = physiks::ball_t::create(
            "can",
            pixel::f2::point_t(pixel::cart_coord.min_x()+2*ball_height, pixel::cart_coord.min_y()+small_gap+thickness+ball_height),
            ball_radius,
            6.8f /* [m/s] */,
            adeg_to_rad(64),
            earth_accel,
            0,
            debug_gfx);
                        // 6.1f /* [m/s] */, pixel::adeg_to_rad(78));
        pixel::f2::geom_list_t& list = pixel::f2::gobjects();
        ball_list.push_back(ball_1);
        ball_list.push_back(ball_2);
        ball_list.push_back(ball_3);
        list.push_back(ball_1);
        list.push_back(ball_2);
        list.push_back(ball_3);
        {
            // top horizontal bounds
            pixel::f2::point_t tl = { pixel::cart_coord.min_x()+small_gap, pixel::cart_coord.max_y()-small_gap };
            pixel::f2::geom_ref_t r = std::make_shared<pixel::f2::rect_t>(tl, pixel::cart_coord.width()-2.0f*small_gap, thickness);
            list.push_back(r);
            if( debug_gfx ) {
                log_printf(elapsed_ms, "XX RT %s\n", r->toString().c_str());
            }
        }
        {
            // bottom horizontal bounds
            pixel::f2::point_t tl = { pixel::cart_coord.min_x()+small_gap, pixel::cart_coord.min_y()+small_gap+thickness };
            pixel::f2::geom_ref_t r = std::make_shared<pixel::f2::rect_t>(tl, pixel::cart_coord.width()-2.0f*small_gap, thickness);
            list.push_back(r);
            if( debug_gfx ) {
                log_printf(elapsed_ms, "XX RB %s\n", r->toString().c_str());
            }
        }
        if(true) {
            // left vertical bounds
            pixel::f2::point_t tl = { pixel::cart_coord.min_x()+small_gap, pixel::cart_coord.max_y()-1.0f*small_gap-thickness };
            pixel::f2::geom_ref_t r = std::make_shared<pixel::f2::rect_t>(tl, thickness, pixel::cart_coord.height()-2.0f*small_gap-2.0f*thickness);
            list.push_back(r);
            if( debug_gfx ) {
                log_printf(elapsed_ms, "XX RL %s\n", r->toString().c_str());
            }
        }
        if(true) {
            // right vertical bounds
            pixel::f2::point_t tl = { pixel::cart_coord.max_x()-small_gap-thickness, pixel::cart_coord.max_y()-1.0f*small_gap-thickness };
            pixel::f2::geom_ref_t r = std::make_shared<pixel::f2::rect_t>(tl, thickness, pixel::cart_coord.height()-2.0f*small_gap-2.0f*thickness);
            list.push_back(r);
            if( debug_gfx ) {
                log_printf(elapsed_ms, "XX RR %s\n", r->toString().c_str());
            }
        }
    }

    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
}
