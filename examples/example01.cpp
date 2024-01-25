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
#include <random>

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

    const pixel::i2::point_t p0_i = { 0, 0 };
    //pixel::i2::lineseg_t la = { p0, { max_x, max_y } };
    // const pixel::f2::point_t p0_f = { pixel::cart_coord.min_x(), pixel::cart_coord.min_y() }; // { 0, 0 };
    const pixel::f2::point_t p0_f = { 0, 0 };
    pixel::f2::lineseg_t la = { p0_f, { pixel::cart_coord.max_x(), pixel::cart_coord.max_y() } };

    int anim_step = 1;

    // if using hardware RNG
    // std::unique_ptr<std::random_device> rng_hw = std::make_unique<std::random_device>();
    // if using predictable NG C++
    // std::unique_ptr<std::mt19937> rng_p0 std::make_unique<std::mt19937>();

    // std::vector<geom2df*>& gobjects()
    {
        pixel::f2::ageom_list_t& list = pixel::f2::agobjects();
        pixel::f2::point_t bl = { pixel::cart_coord.min_x(), pixel::cart_coord.min_y() };
        const float sz = 50;
        const float radius = sz/2.0f;
        const float spacing = 10;
        if(true){
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
        }
        {
            const float sz2 = 1000;
            pixel::f2::point_t a = { 0, sz2/2.0f };
            pixel::f2::point_t b = { -sz2/2.0f, -sz2/2.0f };
            pixel::f2::point_t c = { sz2/2.0f, -sz2/2.0f };
            std::shared_ptr<pixel::f2::triangle_t> o = std::make_shared<pixel::f2::triangle_t>(a, b, c);
            list.push_back(o);
        }
    }
    pixel::i2::blob_t blob1 = { { 0, 0 }, 100 };
    bool blob1_grow = true;
    bool blob1_hit = false;

    //pointi_t blob0_home { -max_x+100, -max_y+100 };
    //recti_t hero(blob0_home, 200, 100);
    pixel::f2::point_t blob0_home { pixel::cart_coord.min_x()+300.0f, pixel::cart_coord.min_y()+200.0f };
    pixel::f2::rect_t hero(blob0_home, 200, 100);
    int blob0_speed = 2;

    bool close = false;
    bool resized = false;
    bool set_dir = false;
    pixel::direction_t dir = pixel::direction_t::UP;
    pixel::texture_ref hud_text;
    float last_fps = -1.0f;

    while(!close) {
        handle_events(close, resized, set_dir, dir);
        const bool animating = pixel::direction_t::PAUSE != dir;
        {
            float fps = pixel::get_gpu_fps();
            if( fps != last_fps ) {
                pixel::set_pixel_color(255 /* r */, 255 /* g */, 255 /* b */, 255 /* a */);
                hud_text = pixel::make_text_texture("fps "+std::to_string(fps));
                // printf("XX: fps %f -> %f, hud %d\n", last_fps, fps, nullptr!=hud_text);
                last_fps = fps;
            }
        }

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
            for(pixel::f2::ageom_ref_t g : list) {
                if( false && animating ) {
                    g->rotate(pixel::adeg_to_rad(1.0f));
                }
                g->draw();

                if( hero.intersects(*g) ) {
                    hero_hit_geom = true;
                }
            }
        }
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

        if( animating ) {
            anim_step = 10;
            if( la.p1.x > -pixel::cart_coord.max_x() && la.p1.y == pixel::cart_coord.max_y()) {
                // ph 1
                la.p1.x = std::max<float>( -pixel::cart_coord.max_x(), la.p1.x - (float)anim_step );
                ++anim_step;
            } else if( la.p1.x == -pixel::cart_coord.max_x() && la.p1.y > -pixel::cart_coord.max_y() ) {
                // ph 2
                la.p1.y = std::max<float>( -pixel::cart_coord.max_y(), la.p1.y - (float)anim_step ) ;
                ++anim_step;
            } else if( la.p1.x < pixel::cart_coord.max_x() && la.p1.y == -pixel::cart_coord.max_y() ) {
                // ph 3
                la.p1.x = std::min<float>( pixel::cart_coord.max_x(), la.p1.x + (float)anim_step ) ;
                ++anim_step;
            } else if( la.p1.x == pixel::cart_coord.max_x() && la.p1.y < pixel::cart_coord.max_y() ) {
                // ph 4
                la.p1.y = std::min<float>( pixel::cart_coord.max_y(), la.p1.y + (float)anim_step ) ;
                ++anim_step;
            }
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

        if( set_dir && animating ) {
            blob0_speed += 2;
            pixel::f2::point_t p_old = hero.p_a;
            switch( dir ) {
                case pixel::direction_t::UP:
                    hero.move_dir((float)blob0_speed);
                    break;
                case pixel::direction_t::DOWN:
                    hero.move_dir((float)-blob0_speed);
                    break;
                case pixel::direction_t::LEFT:
                    hero.rotate(pixel::adeg_to_rad(2.0f));
                    // hero.move(-blob0_speed, 0);
                    break;
                case pixel::direction_t::RIGHT:
                    hero.rotate(pixel::adeg_to_rad(-2.0f));
                    // hero.move(blob0_speed, 0);
                    break;
                default:
                    break;
            }
            if( !hero.on_screen() ) {
                hero.set_top_left( p_old );
                if( true ) {
                    printf("XXX offscreen %s\n", hero.toString().c_str());
                }
            }
            blob1_hit = false; // blob1.intersects(hero);
            if( blob1_hit ) {
                printf("XXX coll.blob %d, 0: %s, 1: %s\n",
                        blob1_hit, hero.toString().c_str(), blob1.toString().c_str());
            }
        } else {
            blob0_speed = 2;
        }
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
        // sleep(2);
        // break;
    }
    exit(0);
}

