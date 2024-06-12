/*
 * Author: Sven Gothel <sgothel@jausoft.com>
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
#include "pixel/pixel.hpp"
#include "physics.hpp"
#include "pixel/pixel4f.hpp"

#include <algorithm>
#include <string>
#include <vector>
#include <random>
#include <cstdio>
#include <cmath>
#include <iostream>

// const float field_width = 4.0f;
const float field_height = 3.0f;
const float diff_bounce = 0.075f;
const float rho_deaccel = 1.0f-diff_bounce; // factor
const float pad_accel = 1.0f+diff_bounce; // factor
const float max_velocity = 5.6f; // m/s

bool debug_gfx = false;

extern "C" {
    EMSCRIPTEN_KEEPALIVE void set_debug_gfx(bool v) noexcept { debug_gfx = v; }
}

std::vector<pixel::f2::rect_ref_t> player_pads;

/**
 * A bouncing ball w/ initial velocity in given direction plus gravity exposure (falling)
 */

static const float ball_height = 0.05f; // [m] .. diameter
static const float ball_radius = ball_height/2.0f; // [m]
static const float pad_height = 0.25f * 1.2f; // [m]
static const pixel::f2::vec_t pad_step_up(0.0f, 1.5f*ball_height); // [m]
static const pixel::f2::vec_t pad_step_down(0.0f, -1.5f*ball_height); // [m]
static const float pad_rot_step = 3.0f; // ang-degrees
static const float pad_thickness = 0.07f; // [m]

static bool big_pads = false;
static bool one_player = true;
static std::string record_bmpseq_basename;
static pixel::f2::rect_ref_t pad_l, pad_r;
static std::shared_ptr<physiks::ball_t> ball;

void mainloop() {
    static uint64_t frame_count_total = 0;
    static uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]
    static pixel::input_event_t event;
    static bool animating = true;

    pixel::handle_events(event);
    if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_CLOSE_REQ ) ) {
        printf("Exit Application\n");
        #if defined(__EMSCRIPTEN__)
            emscripten_cancel_main_loop();
        #else
            exit(0);
        #endif
    } else if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_RESIZED ) ) {
        pixel::cart_coord.set_height(-field_height/2.0f, field_height/2.0f);
    }
    if( event.paused() ) {
        animating = false;
    } else {
        if( !animating ) {
            t_last = pixel::getElapsedMillisecond(); // [ms]
        }
        animating = true;
    }

    uint64_t t1;
    if( animating ) {
        t1 = pixel::getElapsedMillisecond(); // [ms]
        if( event.has_any_p1() ){
            if( event.pressed(pixel::input_event_type_t::P1_UP) ) {
                pad_r->move(pad_step_up);
                if( !pad_r->on_screen() ) {
                    pad_r->move(pad_step_down);
                }
            } else if( event.pressed(pixel::input_event_type_t::P1_DOWN) ) {
                pad_r->move(pad_step_down);
                if( !pad_r->on_screen() ) {
                    pad_r->move(pad_step_up);
                }
            } else if( event.pressed(pixel::input_event_type_t::P1_LEFT) ) {
                pad_r->rotate(pixel::adeg_to_rad(pad_rot_step));
            } else if( event.pressed(pixel::input_event_type_t::P1_RIGHT) ) {
                pad_r->rotate(pixel::adeg_to_rad(-pad_rot_step));
            }
        }
    } else {
        t1 = t_last;
        if( event.has_any_p1() ) {
            if( event.pressed(pixel::input_event_type_t::P1_RIGHT) ){
                t1 +=  1;
            } else if( event.pressed(pixel::input_event_type_t::P1_UP) ){
                t1 += 10;
            }
        }
    }
    const float dt = (float)( t1 - t_last ) / 1000.0f; // [s]
    t_last = t1;

    // white background
    pixel::clear_pixel_fb(0, 0, 0, 255);
    static std::vector<pixel::texture_ref> texts;
    pixel::texture_ref hud_text;
    {
        std::string hud_s = pixel::to_string("td %s, %5.2f m/s",
                pixel::to_decstring(t1, ',', 9).c_str(), ball->velocity.length());
        if( one_player ) {
            hud_s.append( pixel::to_string(", angle %6.2f deg", pixel::rad_to_adeg(pad_l->dir_angle)) );
        }
        hud_s.append( pixel::to_string(", fps %2.2f", pixel::get_gpu_fps()) );
        hud_text = pixel::make_text_texture(hud_s);
    }
    
    // move ball_1
    ball->tick(dt);

    pixel::set_pixel_color(255 /* r */, 255 /* g */, 255 /* b */, 255 /* a */);
    {
        pixel::f2::geom_list_t& list = pixel::f2::gobjects();
        if( debug_gfx ) {
            for(pixel::f2::geom_ref_t &g : list) {
                if( g.get() != ball.get() ) {
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
        const int thickness_pixel = pixel::cart_coord.to_fb_dy(pad_thickness);
        const int text_height = thickness_pixel;
        const float sy = (float)text_height / (float)hud_text->height;
        hud_text->draw(pixel::fb_width/2.0f-((float)hud_text->width*sy/2.0f), thickness_pixel, sy, sy);
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
    int win_width = 1920, win_height = 1080;
    bool enable_vsync = true;
    #if defined(__EMSCRIPTEN__)
        win_width = 1024, win_height = 576; // 16:9
    #endif
    {
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-2p", argv[i]) ) {
                // one_player = false;
            } else if( 0 == strcmp("-big_pads", argv[i])){
                big_pads = true;
            } else if( 0 == strcmp("-width", argv[i]) && i+1<argc) {
                win_width = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-height", argv[i]) && i+1<argc) {
                win_height = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-record", argv[i]) && i+1<argc) {
                record_bmpseq_basename = argv[i+1];
                ++i;
            } else if( 0 == strcmp("-debug_gfx", argv[i]) ) {
                debug_gfx = true;
            } else if( 0 == strcmp("-fps", argv[i]) && i+1<argc) {
                pixel::forced_fps = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-no_vsync", argv[i]) ) {
                enable_vsync = false;
            }
        }
    }
    {
        const uint64_t elapsed_ms = pixel::getElapsedMillisecond();
        pixel::log_printf(elapsed_ms, "Usage %s -2p -width <int> -height <int> -record <bmp-files-basename> -debug_gfx -fps <int>\n", argv[0]);
        pixel::log_printf(elapsed_ms, "- win size %d x %d\n", win_width, win_height);
        pixel::log_printf(elapsed_ms, "- record %s\n", record_bmpseq_basename.size()==0 ? "disabled" : record_bmpseq_basename.c_str());
        pixel::log_printf(elapsed_ms, "- debug_gfx %d\n", debug_gfx);
        pixel::log_printf(elapsed_ms, "- enable_vsync %d\n", enable_vsync);
        pixel::log_printf(elapsed_ms, "- forced_fps %d\n", pixel::forced_fps);
    }

    {
        const float origin_norm[] = { 0.5f, 0.5f };
        pixel::init_gfx_subsystem("pong01", win_width, win_height, origin_norm, enable_vsync, true /* subsys primitives */);
    }
    pixel::cart_coord.set_height(-field_height/2.0f, field_height/2.0f);

    ball = physiks::ball_t::create("one", pixel::f2::point_t(0.0f, 0.0f), ball_radius,
                                    4.0f /* [m/s] */, pixel::adeg_to_rad(0), 
                                    max_velocity, false, player_pads);

    const pixel::f2::point_t tl = { pixel::cart_coord.min_x()+4.0f*pad_thickness, pixel::cart_coord.max_y()-pad_thickness };
    const pixel::f2::point_t br = { pixel::cart_coord.max_x()-4.0f*pad_thickness, pixel::cart_coord.min_y()+pad_thickness };

    if( one_player || big_pads) {
        pad_l = std::make_shared<pixel::f2::rect_t>(pixel::f2::vec_t(tl.x, pixel::cart_coord.max_y()-2.0f*pad_thickness),
                                                    pad_thickness, pixel::cart_coord.height() - 4*pad_thickness);
    } else {
        pad_l = std::make_shared<pixel::f2::rect_t>(pixel::f2::vec_t(tl.x, 0.0f+pad_height/2.0f),
                                                    pad_thickness, pad_height);
    }
    player_pads.push_back(pad_l);

    if( big_pads ){
        pad_r = std::make_shared<pixel::f2::rect_t>(pixel::f2::vec_t(br.x, pixel::cart_coord.max_y()-2.0f*pad_thickness),
                                                    pad_thickness, pixel::cart_coord.height() - 4*pad_thickness);
    } else {
        pad_r = std::make_shared<pixel::f2::rect_t>(pixel::f2::vec_t(br.x, 0.0f+pad_height/2.0f),
                                                    pad_thickness, pad_height);
    }
    player_pads.push_back(pad_r);

    {
        const uint64_t elapsed_ms = pixel::getElapsedMillisecond();
        if( debug_gfx ) {
            pixel::log_printf(elapsed_ms, "XX %s\n", pixel::cart_coord.toString().c_str());
        }
        pixel::f2::geom_list_t& list = pixel::f2::gobjects();
        list.push_back(ball);
        list.push_back(pad_r);
        list.push_back(pad_l);
        {
            // top horizontal bounds
            pixel::f2::geom_ref_t r = std::make_shared<pixel::f2::rect_t>(tl, pixel::cart_coord.width()-7.0f*pad_thickness, pad_thickness);
            list.push_back(r);
            if( debug_gfx ) {
                pixel::log_printf(elapsed_ms, "XX RT %s\n", r->toString().c_str());
            }
        }
        {
            // bottom horizontal bounds
            pixel::f2::point_t bottom_tl = { tl.x, pixel::cart_coord.min_y()+2.0f*pad_thickness };
            pixel::f2::geom_ref_t r = std::make_shared<pixel::f2::rect_t>(bottom_tl, pixel::cart_coord.width()-7.0f*pad_thickness, pad_thickness);
            list.push_back(r);
            if( debug_gfx ) {
                pixel::log_printf(elapsed_ms, "XX RB %s\n", r->toString().c_str());
            }
        }
    }

    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
}
