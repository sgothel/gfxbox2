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

const float rho_default = 0.75f;
const float drop_height = 2.0f; // [m]
const float earth_accel = 9.81f; // [m/s*s]
bool debug_gfx = false;

/**
 * A bouncing ball w/ initial velocity in given direction plus gravity exposure (falling)
 */
class ball_t : public pixel::f2::disk_t {
    public:
        /**
         *
         * v = s / t
         *   s = v * t
         *
         * a = v / t
         *   a = s / t^2
         *   s = a * t^2
         *
         * PE = m * a * s;      * [J] = [Kg] * [m/s]^2
         *    = m * v/t * s
         *    = m * ( s^2 ) / ( t^2 )
         *
         * KE = 1/2 * m * v^2;  * [J] = [Kg] * [m/s]^2
         *
         * PE == KE (conservation of energy)
         *   m * a * s = 1/2 * m * v^2
         *   a * s = 1/2 * v^2
         *   v^2 = 2 * a * s
         *
         *   s = 1/2 * 1/a * v^2
         *   s = 1/2 * t/v * v^2
         *   s = 1/2 * v * t
         *
         *
         */
        std::string id;
        float rho;
        float start_xpos; // [m]
        float start_ypos; // [m]
        float velocity_start; // [m/s] @ angle
        float total_fall; // [m]
        float velocity_max; // [m/s]
        pixel::f2::vec_t velocity; // [m/s]

        /**
         *
         * @param x_m x position in [m] within coordinate system 0/0 center
         * @param y_m y position in [m] within coordinate system 0/0 center
         * @param r_m
         */
        ball_t(std::string id_, float rho_, float x_m, float y_m, const float r_m, const float velocity_, const float v_angle_rad)
        : pixel::f2::disk_t(x_m, y_m, r_m),
          id(std::move(id_)), rho(rho_), start_xpos(x_m), start_ypos(y_m), velocity_start(velocity_), total_fall(drop_height),
          velocity_max(std::sqrt( 2 * earth_accel * total_fall )), velocity()
        {
            rotate(v_angle_rad); // direction of velocity
            velocity = pixel::f2::vec_t::from_length_angle(velocity_start, this->dir_angle);
            if( debug_gfx ) {
                const uint64_t elapsed_ms = pixel::getElapsedMillisecond();
                pixel::log_printf(elapsed_ms, "ball %s-i, %s\n", id.c_str(), toString().c_str());
                pixel::log_printf(elapsed_ms, "Ball %s-i: v %s, |%f| / %f m/s, %s\n",
                        id.c_str(), velocity.toString().c_str(), velocity.length(), velocity_max, box().toString().c_str());
            }
        }

        void reset() noexcept {
            velocity_max = std::sqrt( 2 * earth_accel * total_fall );
            if( velocity_start > 0.0f && this->on_screen() ) {
                // shoot back ;-)
                this->dir_angle = M_PI - this->dir_angle; // 180 - adeg
            } else {
                center.x = start_xpos;
                center.y = start_ypos-radius;
            }
            velocity = pixel::f2::vec_t::from_length_angle(velocity_start, this->dir_angle);
        }

        bool tick(const float dt) noexcept override {
            const float min_velocity = 0.00005f;
            // const float min_velocity = 0.02f;

            const pixel::f2::vec_t good_position = center;

            // Leave horizontal velocity untouched, will be reduced at bounce
            velocity.y += -earth_accel * dt;

            // directional velocity
            pixel::f2::vec_t ds_m_dir = velocity * dt; // [m/s] * [s] = [m] -> matches velocity_max roughly in this simulation
            // move vector
            pixel::f2::lineseg_t l_move;
            float a_move;
            {
                // Setup move from p0 -> p1
                l_move.p0 = this->center;
                l_move.p1 = l_move.p0 + ds_m_dir;
                a_move = l_move.angle();
                // Extend move size to cover radius in moving direction p1 // and -p0
                pixel::f2::vec_t l_move_diff = pixel::f2::vec_t::from_length_angle(radius, a_move);
                // l_move.p0 -= l_move_diff;
                l_move.p1 += l_move_diff;
            }
            this->move( ds_m_dir );

            const uint64_t elapsed_ms = pixel::getElapsedMillisecond();

            pixel::f2::geom_ref_t coll_obj = nullptr;
            pixel::f2::point_t coll_point;
            pixel::f2::vec_t coll_normal;
            pixel::f2::vec_t coll_out;
            {
                // detect a collision
                pixel::f2::geom_list_t& list = pixel::f2::gobjects();
                for(pixel::f2::geom_ref_t g : list) {
                    if( g.get() != this ) {
                        if( g->intersection(coll_out, coll_normal, coll_point, l_move) ) {
                            coll_obj = g;
                            break;
                        }
                    }
                }
            }
            if( debug_gfx ) {
                pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
                l_move.draw();
                if( nullptr != coll_obj ) {
                    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
                    // this->draw(false);

                    pixel::set_pixel_color(0 /* r */, 255 /* g */, 0 /* b */, 255 /* a */);
                    pixel::f2::vec_t p_dir_angle = pixel::f2::vec_t::from_length_angle(2.0f*radius, a_move);
                    pixel::f2::lineseg_t l_in(coll_point-p_dir_angle, coll_point);
                    l_in.draw();

                    pixel::set_pixel_color(255 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
                    pixel::f2::lineseg_t l_CN(coll_point, coll_point+coll_normal);
                    l_CN.draw();

                    pixel::set_pixel_color(0 /* r */, 0 /* g */, 255 /* b */, 255 /* a */);
                    pixel::f2::lineseg_t l_out(coll_point, coll_point+coll_out);
                    l_out.draw();

                    pixel::set_pixel_color(255 /* r */, 255 /* g */, 0 /* b */, 255 /* a */);
                    // reconstruct distance post collision, minimum to surface
                    pixel::f2::vec_t vec_post_coll = l_move.p1 - coll_point;
                    const float s_post_coll = std::max( radius, vec_post_coll.length() * rho );
                    // reconstruct position post collision from collision point
                    pixel::f2::vec_t new_center = coll_point + ( coll_out.normalize() * s_post_coll );
                    pixel::f2::lineseg_t l_new_center(coll_point, new_center);
                    l_new_center.draw();

                    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
                } else {
                    this->draw(false);
                }
            }
            if( nullptr != coll_obj ) {
                if( debug_gfx ) {
                    pixel::log_printf(elapsed_ms, "\n");
                    pixel::log_printf(elapsed_ms, "Ball %s-e-a: v %s, |%f| / %f m/s, ds %s [m/s], move[angle %f, len %f, %s]\n",
                            id.c_str(), velocity.toString().c_str(), velocity.length(), velocity_max, ds_m_dir.toString().c_str(),
                            pixel::rad_to_adeg(a_move), l_move.length(), l_move.toString().c_str());
                    pixel::log_printf(elapsed_ms, "Ball %s-e-a: coll[point %s, out[%s, angle %f]]]\n",
                            id.c_str(),
                            coll_point.toString().c_str(), coll_out.toString().c_str(), pixel::rad_to_adeg(coll_out.angle()));
                    pixel::log_printf(elapsed_ms, "Ball %s-e-a: %s\n", id.c_str(), coll_obj->toString().c_str());
                }
                // reconstruct distance post collision, minimum to surface
                pixel::f2::vec_t vec_post_coll = l_move.p1 - coll_point;
                const float s_post_coll = std::max( radius, vec_post_coll.length() * rho );

                // pixel::f2::vec_t v_move = l_move.p1 - l_move.p0;
                // adjust position out of collision space
                // center = coll_point + pixel::f2::vec_t::from_length_angle(l_move.length()/2.0f, coll_out.angle());
                // center = coll_point + coll_out/2.0f;

                // reconstruct position post collision from collision point
                center = coll_point + ( coll_out.normalize() * s_post_coll );
                if( debug_gfx ) {
                    this->draw(false);
                }

                // bounce velocity: current velocity * 0.75 (rho) in collision reflection angle
                velocity_max *= rho;
                // velocity = pixel::f2::vec_t::from_length_angle(velocity.length() * rho, coll_out.angle()); // cont using simulated velocity
                velocity = pixel::f2::vec_t::from_length_angle(velocity_max, coll_out.angle()); // cont using calculated velocity
                if( !this->on_screen() ) {
                    center = good_position;
                }
                // const bool do_reset = velocity.norm() <= min_velocity;
                const bool do_reset = velocity_max <= min_velocity;
                if( debug_gfx ) {
                    pixel::log_printf(elapsed_ms, "Ball %s-e-b: v %s, |%f| / %f m/s, reset %d, %s, %s\n",
                            id.c_str(), velocity.toString().c_str(), velocity.length(), velocity_max,
                            do_reset, toString().c_str(), box().toString().c_str());
                }
                if( do_reset ) {
                    reset();
                }
            } else if( !this->on_screen() ) {
                if( debug_gfx ) {
                    pixel::log_printf(elapsed_ms, "Ball %s-off: v %s, |%f| / %f m/s, %s, %s\n",
                            id.c_str(), velocity.toString().c_str(), velocity.length(), velocity_max, toString().c_str(), box().toString().c_str());
                }
                reset();
            }
            return true;
        }
};

static const float ball_height = 0.05f; // [m] .. diameter
static const float ball_radius = ball_height/2.0f; // [m]
static const float small_gap = ball_radius;
static const float thickness = 1.0f * ball_height;

static float rho = rho_default;
static int forced_fps = -1;
static std::string record_bmpseq_basename;

typedef std::shared_ptr<ball_t> ball_ref_t;
typedef std::vector<ball_ref_t> ball_list_t;
static ball_list_t ball_list;

void mainloop() {
    static uint64_t frame_count_total = 0;

    static uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]
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
        pixel::cart_coord.set_height(0.0f, drop_height+6.0f*thickness);
    }
    const bool animating = !event.paused();

    // white background
    pixel::clear_pixel_fb(255, 255, 255, 255);

    const uint64_t t1 = pixel::getElapsedMillisecond(); // [ms]
    const float dt = (float)( t1 - t_last ) / 1000.0f; // [s]
    t_last = t1;

    pixel::texture_ref hud_text = pixel::make_text_texture("td %s, fps %2.2f",
            pixel::to_decstring(t1, ',', 9).c_str(), pixel::get_gpu_fps());

    if( animating ) {
        for(ball_ref_t g : ball_list) {
            g->tick(dt);
        }
    }

    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
    {
        pixel::f2::geom_list_t& list = pixel::f2::gobjects();
        if( debug_gfx ) {
            for(pixel::f2::geom_ref_t g : list) {
                if( g.get() != ball_list[0].get() &&
                    g.get() != ball_list[1].get() &&
                    g.get() != ball_list[2].get() )
                {
                    g->draw();
                }
            }
        } else {
            for(pixel::f2::geom_ref_t g : list) {
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
        hud_text->draw(small_gap_pixel*2.0f, small_gap_pixel+1, sy, sy);
    }
    pixel::swap_gpu_buffer(forced_fps);
    if( record_bmpseq_basename.size() > 0 ) {
        std::string snap_fname(128, '\0');
        const int written = std::snprintf(&snap_fname[0], snap_fname.size(), "%s-%7.7" PRIu64 ".bmp", record_bmpseq_basename.c_str(), frame_count_total);
        snap_fname.resize(written);
        pixel::save_snapshot(snap_fname);
    }

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
            if( 0 == strcmp("-width", argv[i]) && i+1<argc) {
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
                forced_fps = atoi(argv[i+1]);
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
        const uint64_t elapsed_ms = pixel::getElapsedMillisecond();
        pixel::log_printf(elapsed_ms, "Usage %s -width <int> -height <int> -record <bmp-files-basename> -debug_gfx -fps <int>\n", argv[0]);
        pixel::log_printf(elapsed_ms, "- win size %d x %d\n", win_width, win_height);
        pixel::log_printf(elapsed_ms, "- record %s\n", record_bmpseq_basename.size()==0 ? "disabled" : record_bmpseq_basename.c_str());
        pixel::log_printf(elapsed_ms, "- debug_gfx %d\n", debug_gfx);
        pixel::log_printf(elapsed_ms, "- enable_vsync %d\n", enable_vsync);
        pixel::log_printf(elapsed_ms, "- forced_fps %d\n", forced_fps);
        pixel::log_printf(elapsed_ms, "- rho %f\n", rho);
    }

    {
        const float origin_norm[] = { 0.5f, 0.5f };
        pixel::init_gfx_subsystem("freefall01", win_width, win_height, origin_norm, enable_vsync, true /* subsys primitives */);
    }

    pixel::cart_coord.set_height(0.0f, drop_height+6.0f*thickness);

    {
        const uint64_t elapsed_ms = pixel::getElapsedMillisecond();
        if( debug_gfx ) {
            pixel::log_printf(elapsed_ms, "XX %s\n", pixel::cart_coord.toString().c_str());
        }
        std::shared_ptr<ball_t> ball_1 = std::make_shared<ball_t>( "one", rho, -4.0f*ball_height, drop_height-ball_radius, ball_radius,
                        0.0f /* [m/s] */, pixel::adeg_to_rad(90));
        std::shared_ptr<ball_t> ball_2 = std::make_shared<ball_t>( "two", rho, +2.0f*ball_height, drop_height-ball_radius, ball_radius,
                        0.0f /* [m/s] */, pixel::adeg_to_rad(90));
        std::shared_ptr<ball_t> ball_3 = std::make_shared<ball_t>( "can", rho, pixel::cart_coord.min_x()+2*ball_height, pixel::cart_coord.min_y()+small_gap+thickness+ball_height, ball_radius,
                        6.8f /* [m/s] */, pixel::adeg_to_rad(64));
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
                pixel::log_printf(elapsed_ms, "XX RT %s\n", r->toString().c_str());
            }
        }
        {
            // bottom horizontal bounds
            pixel::f2::point_t tl = { pixel::cart_coord.min_x()+small_gap, pixel::cart_coord.min_y()+small_gap+thickness };
            pixel::f2::geom_ref_t r = std::make_shared<pixel::f2::rect_t>(tl, pixel::cart_coord.width()-2.0f*small_gap, thickness);
            list.push_back(r);
            if( debug_gfx ) {
                pixel::log_printf(elapsed_ms, "XX RB %s\n", r->toString().c_str());
            }
        }
        if(true) {
            // left vertical bounds
            pixel::f2::point_t tl = { pixel::cart_coord.min_x()+small_gap, pixel::cart_coord.max_y()-2.0f*small_gap-thickness };
            pixel::f2::geom_ref_t r = std::make_shared<pixel::f2::rect_t>(tl, thickness, pixel::cart_coord.height()-4.0f*small_gap-2.0f*thickness);
            list.push_back(r);
            if( debug_gfx ) {
                pixel::log_printf(elapsed_ms, "XX RL %s\n", r->toString().c_str());
            }
        }
        if(true) {
            // right vertical bounds
            pixel::f2::point_t tl = { pixel::cart_coord.max_x()-small_gap-thickness, pixel::cart_coord.max_y()-2.0f*small_gap-thickness };
            pixel::f2::geom_ref_t r = std::make_shared<pixel::f2::rect_t>(tl, thickness, pixel::cart_coord.height()-4.0f*small_gap-2.0f*thickness);
            list.push_back(r);
            if( debug_gfx ) {
                pixel::log_printf(elapsed_ms, "XX RR %s\n", r->toString().c_str());
            }
        }
    }

    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
}
