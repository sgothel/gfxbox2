#include <pixel/pixel3f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"

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
        ball_t(std::string id_, float x_m, float y_m, const float r_m, const float velocity_, const float v_angle_rad)
        : pixel::f2::disk_t(x_m, y_m, r_m),
          id(std::move(id_)), start_xpos(x_m), start_ypos(y_m), velocity_start(velocity_), total_fall(drop_height),
          velocity_max(std::sqrt( 2 * earth_accel * total_fall )), velocity()
        {
            rotate(v_angle_rad); // direction of velocity
            velocity = pixel::f2::vec_t::from_length_angle(velocity_start, this->dir_angle);
            const uint64_t elapsed_ms = pixel::getElapsedMillisecond();
            pixel::log_printf(elapsed_ms, "ball %s-i, %s\n", id.c_str(), toString().c_str());
            pixel::log_printf(elapsed_ms, "Ball %s-i: v %s, |%f| / %f m/s, %s\n",
                    id.c_str(), velocity.toString().c_str(), velocity.length(), velocity_max, box().toString().c_str());
        }

        void reset() noexcept {
            velocity_max = std::sqrt( 2 * earth_accel * total_fall );
            if( velocity_start > 0.0f ) {
                // shoot back ;-)
                this->dir_angle = M_PI - this->dir_angle; // 180 - adeg
            } else {
                center.x = start_xpos;
                center.y = start_ypos-radius;
            }
            velocity = pixel::f2::vec_t::from_length_angle(velocity_start, this->dir_angle);
        }

        void tick(const float dt) noexcept {
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
                // Extend move size to cover radius in moving direction p1 and -p0
                pixel::f2::vec_t l_move_diff = pixel::f2::vec_t::from_length_angle(radius, a_move);
                l_move.p0 -= l_move_diff;
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
                this->draw(false);
                l_move.draw();
                if( nullptr != coll_obj ) {
                    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
                    this->draw(false);

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
                    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
                }
            }
            if( nullptr != coll_obj ) {
                pixel::log_printf(elapsed_ms, "\n");
                pixel::log_printf(elapsed_ms, "Ball %s-e-a: v %s, |%f| / %f m/s, ds %s [m/s], move[angle %f, len %f, %s]\n",
                        id.c_str(), velocity.toString().c_str(), velocity.length(), velocity_max, ds_m_dir.toString().c_str(),
                        pixel::rad_to_adeg(a_move), l_move.length(), l_move.toString().c_str());
                pixel::log_printf(elapsed_ms, "Ball %s-e-a: coll[point %s, out[%s, angle %f]]]\n",
                        id.c_str(),
                        coll_point.toString().c_str(), coll_out.toString().c_str(), pixel::rad_to_adeg(coll_out.angle()));
                pixel::log_printf(elapsed_ms, "Ball %s-e-a: %s\n", id.c_str(), coll_obj->toString().c_str());

                // bounce velocity: current velocity * 0.75 (rho) in collision reflection angle
                velocity_max *= 0.75f; // rho
                velocity = pixel::f2::vec_t::from_length_angle(velocity.length() * 0.75f, coll_out.angle()); // cont using simulated velocity
                // adjust position out of collision space
                if( true ) {
                    // Simple & safe, but not giving an accurate continuous animation
                    center = good_position;
                } else {
                    // Could be off-screen already, literal corner cases on high speed and/or low-fps
                    // center = coll_point + coll_out;
                    center = coll_point + pixel::f2::vec_t::from_length_angle((coll_point-good_position).length(), coll_out.angle());
                }
                // const bool do_reset = velocity.norm() <= min_velocity;
                const bool do_reset = velocity_max <= min_velocity;
                pixel::log_printf(elapsed_ms, "Ball %s-e-b: v %s, |%f| / %f m/s, reset %d, %s, %s\n",
                        id.c_str(), velocity.toString().c_str(), velocity.length(), velocity_max,
                        do_reset, toString().c_str(), box().toString().c_str());
                if( do_reset ) {
                    reset();
                }
            } else if( !this->on_screen() ) {
                pixel::log_printf(elapsed_ms, "Ball %s-off: v %s, |%f| / %f m/s, %s, %s\n",
                        id.c_str(), velocity.toString().c_str(), velocity.length(), velocity_max, toString().c_str(), box().toString().c_str());
                reset();
            }
        }
};

int main(int argc, char *argv[])
{
    int win_width = 1920, win_height = 1080;
    std::string record_bmpseq_basename;
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
            }
        }
    }
    {
        const uint64_t elapsed_ms = pixel::getElapsedMillisecond();
        pixel::log_printf(elapsed_ms, "\n%s\n\n", argv[0]);
        pixel::log_printf(elapsed_ms, "- win size %d x %d\n", win_width, win_height);
        pixel::log_printf(elapsed_ms, "- record %s\n", record_bmpseq_basename.size()==0 ? "disabled" : record_bmpseq_basename.c_str());
        pixel::log_printf(elapsed_ms, "- debug_gfx %d\n", debug_gfx);
    }

    {
        const float origin_norm[] = { 0.5f, 0.5f };
        pixel::init_gfx_subsystem("freefall01", win_width, win_height, origin_norm);
    }

    const float ball_height = 0.05f; // [m] .. diameter
    const float ball_radius = ball_height/2.0f; // [m]
    const float thickness = 1.0f * ball_height;
    const float small_gap = ball_radius;

    pixel::cart_coord.set_height(0.0f, drop_height+2.0f*thickness);

    const int thickness_pixel = pixel::cart_coord.to_fb_dy(thickness);
    const int small_gap_pixel = pixel::cart_coord.to_fb_dy(small_gap);

    std::shared_ptr<ball_t> ball_1 = std::make_shared<ball_t>( "one", -4.0f*ball_height, drop_height-ball_radius, ball_radius,
                    0.0f /* [m/s] */, pixel::adeg_to_rad(90));
    std::shared_ptr<ball_t> ball_2 = std::make_shared<ball_t>( "two", +2.0f*ball_height, drop_height-ball_radius, ball_radius,
                    0.0f /* [m/s] */, pixel::adeg_to_rad(90));
    std::shared_ptr<ball_t> ball_3 = std::make_shared<ball_t>( "can",  pixel::cart_coord.min_x()+2*ball_height, pixel::cart_coord.min_y()+small_gap+thickness+ball_height, ball_radius,
                    6.8f /* [m/s] */, pixel::adeg_to_rad(64));
                    // 6.1f /* [m/s] */, pixel::adeg_to_rad(78));
    {
        const uint64_t elapsed_ms = pixel::getElapsedMillisecond();
        pixel::log_printf(elapsed_ms, "XX %s\n", pixel::cart_coord.toString().c_str());
        pixel::f2::geom_list_t& list = pixel::f2::gobjects();
        list.push_back(ball_1);
        list.push_back(ball_2);
        list.push_back(ball_3);
        {
            // top horizontal bounds
            pixel::f2::point_t tl = { pixel::cart_coord.min_x()+small_gap, pixel::cart_coord.max_y()-small_gap };
            pixel::f2::geom_ref_t r = std::make_shared<pixel::f2::rect_t>(tl, pixel::cart_coord.width()-2.0f*small_gap, thickness);
            list.push_back(r);
            pixel::log_printf(elapsed_ms, "XX RT %s\n", r->toString().c_str());
        }
        {
            // bottom horizontal bounds
            pixel::f2::point_t tl = { pixel::cart_coord.min_x()+small_gap, pixel::cart_coord.min_y()+small_gap+thickness };
            pixel::f2::geom_ref_t r = std::make_shared<pixel::f2::rect_t>(tl, pixel::cart_coord.width()-2.0f*small_gap, thickness);
            list.push_back(r);
            pixel::log_printf(elapsed_ms, "XX RB %s\n", r->toString().c_str());
        }
        if(true) {
            // left vertical bounds
            pixel::f2::point_t tl = { pixel::cart_coord.min_x()+small_gap, pixel::cart_coord.max_y()-2.0f*small_gap-thickness };
            pixel::f2::geom_ref_t r = std::make_shared<pixel::f2::rect_t>(tl, thickness, pixel::cart_coord.height()-4.0f*small_gap-2.0f*thickness);
            list.push_back(r);
            pixel::log_printf(elapsed_ms, "XX RL %s\n", r->toString().c_str());
        }
        if(true) {
            // right vertical bounds
            pixel::f2::point_t tl = { pixel::cart_coord.max_x()-small_gap-thickness, pixel::cart_coord.max_y()-2.0f*small_gap-thickness };
            pixel::f2::geom_ref_t r = std::make_shared<pixel::f2::rect_t>(tl, thickness, pixel::cart_coord.height()-4.0f*small_gap-2.0f*thickness);
            list.push_back(r);
            pixel::log_printf(elapsed_ms, "XX RR %s\n", r->toString().c_str());
        }
    }

    bool close = false;
    bool resized = false;
    bool set_dir = false;
    pixel::direction_t dir = pixel::direction_t::UP;
    pixel::texture_ref hud_text;
    uint64_t frame_count_total = 0;

    uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]

    while(!close) {
        handle_events(close, resized, set_dir, dir);

        // white background
        pixel::clear_pixel_fb(255, 255, 255, 255);

        const uint64_t t1 = pixel::getElapsedMillisecond(); // [ms]
        const float dt = (float)( t1 - t_last ) / 1000.0f; // [s]
        const float dt_exp = 1.0f / (float)pixel::frames_per_sec; // [s]
        const float dt_diff = (float)( dt_exp - dt ) * 1000.0f; // [ms]
        t_last = t1;

        hud_text = pixel::make_text_texture("td "+pixel::to_decstring(t1, ',', 9)+", fps "+std::to_string(pixel::get_gpu_fps()));

        // move ball_1
        ball_1->tick(dt);

        // move ball_2
        ball_2->tick(dt);

        // move ball_3
        ball_3->tick(dt);

        pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
        {
            pixel::f2::geom_list_t& list = pixel::f2::gobjects();
            if( debug_gfx ) {
                for(pixel::f2::geom_ref_t g : list) {
                    if( g.get() != ball_1.get() &&
                        g.get() != ball_2.get() &&
                        g.get() != ball_3.get() )
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
        if( dt_diff > 1.0f ) {
            pixel::milli_sleep( (uint64_t)dt_diff );
        }
        pixel::swap_pixel_fb(false);
        if( nullptr != hud_text ) {
            const int text_height = thickness_pixel - 2;
            const float sy = (float)text_height / (float)hud_text->height;
            hud_text->draw(small_gap_pixel*2.0f, small_gap_pixel+1, sy, sy);
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
    exit(0);
}
