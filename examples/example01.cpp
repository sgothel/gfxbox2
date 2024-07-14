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
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"

// #include <random>

void rebuild_objects() {
    pixel::f2::ageom_list_t& list = pixel::f2::agobjects();
    list.clear();
    
    pixel::f2::point_t bl = { pixel::cart_coord.min_x(), pixel::cart_coord.min_y() };
    const float sz = 50;
    const float radius = sz/2.0f;
    const float spacing = 10;
    for(int i=0; i<20; ++i) {
        pixel::f2::point_t p = { bl.x + (float)i * ( sz + spacing ), bl.y + (float)i * ( sz + spacing ) };
        if( 0 == i % 2 ) { // modulo (divisionsrest) mit zwei, d.h. durch zwei teilbar?
            const pixel::f2::vec_t c_diff(-sz/2.0f, sz/2.0f);
            std::shared_ptr<pixel::f2::rect_t> o = std::make_shared<pixel::f2::rect_t>(p+c_diff, sz, sz);
            o->rotate(pixel::adeg_to_rad(45.0f));
            list.push_back(o);
        } else {
            std::shared_ptr<pixel::f2::disk_t> o = std::make_shared<pixel::f2::disk_t>(p, radius);
            list.push_back(o);
        }
        printf("[%d]: Added %s\n", i, list.back()->toString().c_str());
    }
    {
        const float sz2 = 100;
        pixel::f2::point_t a = { 0, sz2 };
        pixel::f2::point_t b = { -sz2, -sz2 };
        pixel::f2::point_t c = { sz2, -sz2 };
        std::shared_ptr<pixel::f2::triangle_t> o = std::make_shared<pixel::f2::triangle_t>(a, b, c);
        list.push_back(o);
    }    
}

void mainloop() {
    // static uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]
    static pixel::input_event_t event;
    static bool animating = true;
    
    static const pixel::i2::point_t p0_i = { 0, 0 };
    static const pixel::f2::point_t p0_f = { 0, 0 };
    static pixel::f2::lineseg_t la = { p0_f, { 0.0f, pixel::cart_coord.max_y()*2.0f } };
    static pixel::i2::blob_t blob1 = { { 0, 0 }, 100 };
    static bool blob1_grow = true;

    static pixel::f2::point_t blob0_home { pixel::cart_coord.min_x()+300.0f, pixel::cart_coord.min_y()+200.0f };
    static pixel::f2::rect_t hero(blob0_home, 200, 100);
    static int blob0_speed = 1;
        
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
                // t_last = pixel::getElapsedMillisecond(); // [ms]
            }
            animating = true;
        }
    }
    {
        pixel::f2::point_t p_old = hero.m_tl;
        if( event.pressed(pixel::input_event_type_t::P1_UP) ) {
            hero.move_dir((float)blob0_speed);
            blob0_speed += 1;            
        } else if( event.pressed(pixel::input_event_type_t::P1_DOWN) ) {
            hero.move_dir((float)-blob0_speed);
            blob0_speed += 1;
        } else {
            blob0_speed = 1;            
        }            
        if( event.pressed(pixel::input_event_type_t::P1_LEFT) ) {
            hero.rotate(pixel::adeg_to_rad(2.0f));
            // hero.move(-blob0_speed, 0);
        } else if( event.pressed(pixel::input_event_type_t::P1_RIGHT) ) {
            hero.rotate(pixel::adeg_to_rad(-2.0f));
            // hero.move(blob0_speed, 0);
        }
        if( !hero.on_screen() ) {
            hero.set_top_left( p_old );
            if( true ) {
                printf("XXX offscreen %s\n", hero.toString().c_str());
            }
        }
    }
    
    // const uint64_t t1 = pixel::getElapsedMillisecond(); // [ms]
    // const float dt = (float)( t1 - t_last ) / 1000.0f; // [s]
    // t_last = t1;
    
    bool blob1_hit = false;
       
    pixel::set_pixel_color(255 /* r */, 255 /* g */, 255 /* b */, 255 /* a */);
    pixel::texture_ref hud_text = pixel::make_text_texture("fps "+std::to_string(pixel::get_gpu_fps()));

    // black background
    pixel::clear_pixel_fb(0, 0, 0, 255);

    pixel::draw_grid(50,
            100 /* r */, 100 /* g */, 100 /* b */, 255 /* a */,
              0 /* r */, 100 /* g */, 100 /* b */, 255 /* a */);

    bool hero_hit_la = false;
    bool hero_hit_geom = false;

    {
        pixel::set_pixel_color(255 /* r */, 255 /* g */, 255 /* b */, 255 /* a */);
        pixel::f2::ageom_list_t& list = pixel::f2::agobjects();
        for(const pixel::f2::ageom_ref_t& g : list) {
            if( animating ) {
                g->rotate(pixel::adeg_to_rad(1.0f));
            }
            g->draw();

            if( hero.intersects(*g) ) {
                hero_hit_geom = true;
            }
        }
    }
    {
        const pixel::i2::lineseg_t l0 = { p0_i, { (int) pixel::cart_coord.max_x(), (int) pixel::cart_coord.max_y() } };
        const pixel::i2::lineseg_t l1 = { p0_i, { (int)-pixel::cart_coord.max_x(), (int) pixel::cart_coord.max_y() } };
        const pixel::i2::lineseg_t l2 = { p0_i, { (int)-pixel::cart_coord.max_x(), (int)-pixel::cart_coord.max_y() } };
        const pixel::i2::lineseg_t l3 = { p0_i, { (int) pixel::cart_coord.max_x(), (int)-pixel::cart_coord.max_y() } };
    
        pixel::set_pixel_color(255 /* r */, 255 /* g */, 255 /* b */, 255 /* a */);
        l0.draw();
    
        pixel::set_pixel_color(255 /* r */,   0 /* g */,   0 /* b */, 255 /* a */);
        l1.draw();
    
        pixel::set_pixel_color(  0 /* r */,   0 /* g */, 255 /* b */, 255 /* a */);
        l2.draw();
    
        pixel::set_pixel_color(  0 /* r */, 255 /* g */,   0 /* b */, 255 /* a */);
        l3.draw();
        pixel::set_pixel_color(255 /* r */, 255 /* g */, 255 /* b */, 255 /* a */);
    }

    if( animating ) {            
        blob1_hit = false; // blob1.intersects(hero);
        if( blob1_hit ) {
            printf("XXX coll.blob %d, 0: %s, 1: %s\n",
                    blob1_hit, hero.toString().c_str(), blob1.toString().c_str());
        }
        la.rotate(pixel::adeg_to_rad(1.0f));
    }
    pixel::set_pixel_color(255 /* r */, 255 /* g */, 255 /* b */, 255 /* a */);
    la.draw();

    if( hero.intersects(la) ) {
        hero_hit_la = true;
    }

    if( !blob1_hit && animating ) {
        if( blob1_grow ) {
            blob1.size += 10;
            if( blob1.size >= 200 ) {
                blob1_grow = false;
            }
        } else {
            blob1.size -= 10;
            if( blob1.size <= 50 ) {
                blob1_grow = true;
            }
        }
    }
    blob1.draw();

    pixel::set_pixel_color(200 /* r */, 200 /* g */, 200 /* b */, 255 /* a */);
    pixel::i2::draw_circle(   0,    0, 100, pixel::i2::CircleDrawType::OUTLINE);
    pixel::i2::draw_circle( 200,  200, 100, pixel::i2::CircleDrawType::FILLED);
    pixel::i2::draw_circle(-200, -200, 100, pixel::i2::CircleDrawType::BB_INVERTED);
    if( hero_hit_la ) {
        pixel::set_pixel_color(255 /* r */, 255 /* g */, 0 /* b */, 255 /* a */);
    } else if( hero_hit_geom ) {
        pixel::set_pixel_color(  0 /* r */, 255 /* g */, 0 /* b */, 255 /* a */);
    } else {
        pixel::set_pixel_color(255 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
    }
    hero.draw();
    hero.box().draw();
    pixel::swap_pixel_fb(false);
    if( nullptr != hud_text ) {
        hud_text->draw(0, 0);
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
        if( !pixel::init_gfx_subsystem("gfxbox example01", window_width, window_height, origin_norm) ) {
            return 1;
        }
    }

    rebuild_objects();
    
    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
}

