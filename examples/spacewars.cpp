/*
 * Author: Sven Gothel <sgothel@jausoft.com>
 * Copyright (c) 2023 Gothel Software e.K.
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
#include <pixel/pixel4f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"

#include <algorithm>
#include <random>

const float field_width = 4.0f;
const float field_height = 3.0f;
const float diff_bounce = 0.075f;
const float rho_deaccel = 1.0f-diff_bounce; // factor
const float pad_accel = 1.0f+diff_bounce; // factor

const float ship_height = 0.10f; // [m]
std::random_device rng;

bool debug_gfx = false;

class fragment_t : public pixel::f2::linestrip_t {
    public:
        pixel::f2::vec_t velocity; // [m/s]
        float roration_velocity; // [angle_radians/s]
        //pixel::f4::vec_t color(1.0f, 1.0f, 1.0f, 1.0f);

        /**
         *
         * @param center center position
         * @param angle angle in radians
         * @param v velocity in meter per seconds
         * @param rot_v rotation velocity in radians per seconds
         */
        fragment_t(const pixel::f2::point_t& center,
                   const float angle, const float v, const float rot_v) noexcept
        : linestrip_t(center, angle)
        {
            velocity = pixel::f2::vec_t::from_length_angle(v, angle);
            roration_velocity = rot_v;
        }

        /**
         *
         * @param center center position
         * @param angle angle in radians
         * @param v velocity in meter per seconds
         * @param rot_v rotation velocity in radians per seconds
         */
        fragment_t(const pixel::f2::point_t& center,
                const pixel::f2::vec_t& velocity_, const float rot_v) noexcept
        : linestrip_t(center, velocity_.angle())
        {
            velocity = velocity_;
            roration_velocity = rot_v;
        }

        bool tick(const float dt) noexcept {
            move(velocity * dt);
            rotate(roration_velocity * dt);

            if( p_center.x < pixel::cart_coord.min_x() ) {
                return false;
            }
            if( p_center.x > pixel::cart_coord.max_x() ) {
                return false;
            }
            if( p_center.y < pixel::cart_coord.min_y() ) {
                return false;
            }
            if( p_center.y > pixel::cart_coord.max_y() ) {
                return false;
            }
            return true;
        }
};
typedef std::shared_ptr<fragment_t> fragment_ref_t;

void make_fragments(std::vector<fragment_ref_t>& dest,
                   const pixel::f2::linestrip_ref_t& ls, const float v, const float rot_v)
{
    if( ls->p_list.size() < 2 ) {
        return;
    }
    pixel::f2::point_t p0 = ls->p_list[0];
    for(size_t i=1; i<ls->p_list.size(); ++i) {
        const pixel::f2::point_t& pc = ls->p_center;
        const pixel::f2::point_t& p1 = ls->p_list[i];
        const pixel::f2::point_t p_v = p0 + ( p1 - p0 ) / 2.0f;
        const pixel::f2::vec_t v_v0 = ( p_v - pc ).normalize() * v;
        fragment_ref_t f = std::make_shared<fragment_t>(pc, v_v0, rot_v);
        f->p_list.push_back(pc);
        f->p_list.push_back(p0);
        f->p_list.push_back(p1);
        f->p_list.push_back(pc);
        dest.push_back(f);
        p0 = p1;
    }
}

class asteroid_t : public pixel::f2::linestrip_t {
    public:
        pixel::f2::vec_t velocity; // [m/s]
        float roration_velocity; // [angle_radians/s]

        /**
         *
         * @param center center position
         * @param angle angle in radians
         * @param v velocity in meter per seconds
         * @param rot_v rotation velocity in radians per seconds
         */
        asteroid_t(const pixel::f2::point_t& center,
                   const float angle, const float v, const float rot_v) noexcept
        : linestrip_t(center, angle)
        {
            velocity = pixel::f2::vec_t::from_length_angle(v, angle);
            roration_velocity = rot_v;
        }

        void tick(const float dt) noexcept {
            move(velocity * dt);
            rotate(roration_velocity * dt);
            if( p_center.x < pixel::cart_coord.min_x() ) {
                move(pixel::cart_coord.max_x()-p_center.x, 0.0f);
            }
            if( p_center.x > pixel::cart_coord.max_x() ) {
                move(pixel::cart_coord.min_x()-p_center.x, 0.0f);
            }
            if( p_center.y < pixel::cart_coord.min_y() ) {
                move(0.0f, pixel::cart_coord.max_y()-p_center.y);
            }
            if( p_center.y > pixel::cart_coord.max_y() ) {
                move(0.0f, pixel::cart_coord.min_y()-p_center.y);
            }
        }
};
typedef std::shared_ptr<asteroid_t> asteroid_ref_t;

std::vector<asteroid_ref_t> asteroids;
std::vector<fragment_ref_t> fragments;

class spaceship_t : public pixel::f2::linestrip_t {
    public:
        pixel::f2::vec_t velocity; // [m/s]

        spaceship_t(const pixel::f2::point_t& center, const float angle) noexcept
        : linestrip_t(center, angle), velocity()
        {}

        bool tick(const float dt) noexcept {
            move(velocity * dt);
            if( p_center.x < pixel::cart_coord.min_x() ) {
                move(pixel::cart_coord.max_x()-p_center.x, 0.0f);
            }
            if( p_center.x > pixel::cart_coord.max_x() ) {
                move(pixel::cart_coord.min_x()-p_center.x, 0.0f);
            }
            if( p_center.y < pixel::cart_coord.min_y() ) {
                move(0.0f, pixel::cart_coord.max_y()-p_center.y);
            }
            if( p_center.y > pixel::cart_coord.max_y() ) {
                move(0.0f, pixel::cart_coord.min_y()-p_center.y);
            }
            bool keep_alive = true;
            for(int i=(int)asteroids.size()-1; i>=0; --i) {
                std::vector<asteroid_ref_t>::iterator it = asteroids.begin() + i;
                asteroid_ref_t a = *it;
                if( intersects(a->box()) ) {
                    keep_alive = false;
                    make_fragments(fragments, a, a->velocity.length(), a->roration_velocity*2.0f);
                    asteroids.erase(it);
                }
            }
            return keep_alive;
        }

};
typedef std::shared_ptr<spaceship_t> spaceship_ref_t;
std::vector<spaceship_ref_t> spaceship;


/**
 * Unrotated:
 *
 *       (a)
 *      /  \
 *     / (c)\
 *    /  / \ \
 *   (d)/   \(b)
 */
spaceship_ref_t make_spaceship1( const pixel::f2::point_t& m, const float h) noexcept
{
    spaceship_ref_t lf = std::make_shared<spaceship_t>(m, pixel::adeg_to_rad(90.0f));

    // a
    pixel::f2::point_t p = m;
    p.y += h/2.0f;
    lf->p_list.push_back(p);

    // lf->lala = 4;

    // b
    p.y -= h;
    const float width = 4.0f/5.0f * h;;
    p.x += width/2.0f;
    lf->p_list.push_back(p);

    // c
    lf->p_list.push_back(m);

    // d
    p.x -= width;
    lf->p_list.push_back(p);

    p = m;
    p.y += h/2.0f;
    lf->p_list.push_back(p);
    return lf;
}

asteroid_ref_t make_asteroid(const pixel::f2::point_t& center, float height,
                             const float angle,
                             const float velocity,
                             const float jitter=1.0f/8.0f) noexcept
{
    const float max_height = 6.0f*ship_height;
    const float min_height = 1.0f*ship_height;
    height = std::max(min_height, std::min(height, 6*max_height));
    const float s = 1.0f - ( height - min_height ) / (max_height - min_height );
    const float rot_velocity = pixel::adeg_to_rad(5.0f) + s * pixel::adeg_to_rad(15.0f); // angle/s
    asteroid_ref_t lf = std::make_shared<asteroid_t>(center, angle, velocity, rot_velocity);

    const float w = height;
    const float j = height * jitter;

    // a
    pixel::f2::point_t p = center;
    p.x += -w/4.0f + j;
    p.y +=  height/2.0f - j;
    lf->p_list.push_back(p);
    pixel::f2::point_t a = p;

    // lf->lala = 4;

    // b
    p.x += w/2.0f + j;
    p.y += j;
    lf->p_list.push_back(p);

    // c
    p.x +=  w/4.0f - j;
    p.y += -height/4.0f + j;
    lf->p_list.push_back(p);

    // d
    p.x +=  j;
    p.y += -height/2.0f + j;
    lf->p_list.push_back(p);

    // e
    p.x += -w/4.0f + j;
    p.y += -height/4.0f + j;
    lf->p_list.push_back(p);

    // f
    p.x += -w/2.0f;
    p.y += -j-j;
    lf->p_list.push_back(p);

    // g
    p.x += -w/4.0f - j;
    p.y += height/4.0f - j;
    lf->p_list.push_back(p);

    // height
    p.x += j;
    p.y += height/2.0f - j;
    lf->p_list.push_back(p);

    // a
    lf->p_list.push_back(a);
    return lf;
}

/**
 * A bouncing ball w/ initial velocity in given direction plus gravity exposure (falling)
 */
class ball_t : public pixel::f2::disk_t {
    public:
        std::string id;
        float start_xpos; // [m]
        float start_ypos; // [m]
        float velocity_start; // [m/s] @ angle
        pixel::f2::vec_t velocity; // [m/s]
        float medium_accel = -0.08f; // [m/s*s]

        /**
         *
         * @param x_m x position in [m] within coordinate system 0/0 center
         * @param y_m y position in [m] within coordinate system 0/0 center
         * @param r_m
         */
        ball_t(std::string id_, float x_m, float y_m, const float r_m, const float velocity_, const float v_angle_rad)
        : pixel::f2::disk_t(x_m, y_m, r_m),
          id(std::move(id_)), start_xpos(x_m), start_ypos(y_m), velocity_start(velocity_),
          velocity()
        {
            rotate(v_angle_rad); // direction of velocity
            velocity = pixel::f2::vec_t::from_length_angle(velocity_start, this->dir_angle);
            if( debug_gfx ) {
                const uint64_t elapsed_ms = pixel::getElapsedMillisecond();
                pixel::log_printf(elapsed_ms, "ball %s-i, %s\n", id.c_str(), toString().c_str());
                pixel::log_printf(elapsed_ms, "Ball %s-i: v %s, |%f| m/s, %s\n",
                        id.c_str(), velocity.toString().c_str(), velocity.length(), box().toString().c_str());
            }
        }

        void reset() noexcept {
            center.x = start_xpos;
            center.y = start_ypos-radius;
            velocity = pixel::f2::vec_t::from_length_angle(velocity_start, this->dir_angle);
            pixel::log_printf("Ball %s-res: v %s, |%f| m/s, %s, %s\n",
                    id.c_str(), velocity.toString().c_str(), velocity.length(),
                    toString().c_str(), box().toString().c_str());
        }

        void tick(const float dt) noexcept {
            const float min_velocity = 3.0f;
            const float max_velocity = 5.6f; // m/s
            {
                const float v_abs = velocity.length();
                if( v_abs < min_velocity ) {
                    velocity *= 1.5f; // boost
                    medium_accel = std::abs(medium_accel); // medium speedup
                } else if( v_abs >= max_velocity ) {
                    velocity *= 0.9f; // break
                    medium_accel = -1.0f*std::abs(medium_accel); // medium slowdown
                }
            }

            const pixel::f2::vec_t good_position = center;

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

            // medium_deaccel after move
            velocity = pixel::f2::vec_t::from_length_angle(velocity.length() + medium_accel * dt, a_move);

            const uint64_t elapsed_ms = pixel::getElapsedMillisecond();

            pixel::f2::geom_ref_t coll_obj = nullptr;
            pixel::f2::point_t coll_point;
            pixel::f2::vec_t coll_normal;
            pixel::f2::vec_t coll_out;
            #if 0
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
            #endif
            if( debug_gfx ) {
                pixel::set_pixel_color(255 /* r */, 255 /* g */, 255 /* b */, 255 /* a */);
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

                    pixel::set_pixel_color(255 /* r */, 255 /* g */, 0 /* b */, 255 /* a */);
                    pixel::f2::lineseg_t l_out(coll_point, coll_point+coll_out);
                    l_out.draw();
                    pixel::set_pixel_color(255 /* r */, 255 /* g */, 255 /* b */, 255 /* a */);
                }
            }
            if( nullptr != coll_obj ) {
                if( debug_gfx ) {
                    pixel::log_printf(elapsed_ms, "\n");
                    pixel::log_printf(elapsed_ms, "Ball %s-e-a: v %s, |%f| m/s, ds %s [m/s], move[angle %f, len %f, %s]\n",
                            id.c_str(), velocity.toString().c_str(), velocity.length(), ds_m_dir.toString().c_str(),
                            pixel::rad_to_adeg(a_move), l_move.length(), l_move.toString().c_str());
                    pixel::log_printf(elapsed_ms, "Ball %s-e-a: coll[point %s, out[%s, angle %f]]]\n",
                            id.c_str(),
                            coll_point.toString().c_str(), coll_out.toString().c_str(), pixel::rad_to_adeg(coll_out.angle()));
                    pixel::log_printf(elapsed_ms, "Ball %s-e-a: %s\n", id.c_str(), coll_obj->toString().c_str());
                }
                float accel_factor = rho_deaccel; // rho deaccel;
                velocity = pixel::f2::vec_t::from_length_angle(velocity.length() * accel_factor, coll_out.angle()); // cont using simulated velocity
                center = coll_point + coll_out;
                if( !this->on_screen() ) {
                    center = good_position;
                }
                if( debug_gfx ) {
                    pixel::log_printf(elapsed_ms, "Ball %s-e-b: v %s, |%f| m/s, %s, %s\n",
                            id.c_str(), velocity.toString().c_str(), velocity.length(),
                            toString().c_str(), box().toString().c_str());
                }
            } else if( !this->on_screen() ) {
                if( debug_gfx ) {
                    pixel::log_printf(elapsed_ms, "Ball %s-off: v %s, |%f| m/s, %s, %s\n",
                            id.c_str(), velocity.toString().c_str(), velocity.length(), toString().c_str(), box().toString().c_str());
                }
                reset();
            }
        }
};

int main(int argc, char *argv[])
{
    int win_width = 1920, win_height = 1080;
    std::string record_bmpseq_basename;
    bool enable_vsync = true;
    int forced_fps = -1;
    bool one_player = true;
    pixel::log_printf("X1\n");
    {
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-2p", argv[i]) ) {
                // one_player = false;
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
                forced_fps = atoi(argv[i+1]);
                enable_vsync = false;
                ++i;
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
        pixel::log_printf(elapsed_ms, "- forced_fps %d\n", forced_fps);
    }

    {
        const float origin_norm[] = { 0.5f, 0.5f };
        pixel::init_gfx_subsystem("spacewars", win_width, win_height, origin_norm, enable_vsync);
    }

    // const float bullet_height = 0.02f; // [m] .. diameter
    // const float ball_radius = ball_height/2.0f; // [m]
    const float ship_vel_step = 0.1f; // [m/s]
    const float ship_rot_step = 3.0f; // ang-degrees
    const float ship_thickness = 0.07f; // [m]

    pixel::cart_coord.set_height(-field_height/2.0f, field_height/2.0f);

    const pixel::f2::point_t p0_ss1 = { pixel::cart_coord.min_x()+pixel::cart_coord.width()/4.0f, 0.0f };
    const pixel::f2::point_t p0_ss2 = { pixel::cart_coord.min_x()+pixel::cart_coord.width()*3.0f/4.0f, 0.0f };

    spaceship_ref_t ship_l;
    if( !one_player ) {
        ship_l = make_spaceship1(p0_ss1, ship_height);
    }
    spaceship_ref_t ship_r = make_spaceship1(p0_ss2, ship_height);

    {
        pixel::f2::point_t p0 = { 0.0f, 0.0f };
        float height = ship_height*4.0f;
        for(int i = 0; i < 5; ++i) {
            const float angle = pixel::adeg_to_rad(i * 360.0f / 10.0f);
            const float velocity = 0.3f; // m/s
            p0 -= pixel::f2::point_t(height, 0);
            asteroid_ref_t asteroid1 = make_asteroid(p0, height,
                    angle, velocity, 1.0f/(2.0f+i));
            asteroids.push_back(asteroid1);
        }

    }
    {
        const uint64_t elapsed_ms = pixel::getElapsedMillisecond();
        if( debug_gfx ) {
            pixel::log_printf(elapsed_ms, "XX %s\n", pixel::cart_coord.toString().c_str());
        }
    }

    bool close = false;
    bool resized = false;
    bool set_dir = false;
    pixel::direction_t dir = pixel::direction_t::UP;
    pixel::texture_ref hud_text;
    uint64_t frame_count_total = 0;

    uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]
    uint64_t t_fps_last = pixel::getCurrentMilliseconds();

    while(!close) {
        handle_events(close, resized, set_dir, dir);

        if( resized ) {
            pixel::cart_coord.set_height(-field_height/2.0f, field_height/2.0f);
        }

        // white background
        pixel::clear_pixel_fb(0, 0, 0, 255);

        const uint64_t t1 = pixel::getElapsedMillisecond(); // [ms]
        const float dt = (float)( t1 - t_last ) / 1000.0f; // [s]
        const float dt_exp = 1.0f / (float)pixel::frames_per_sec; // [s]
        const float dt_diff = (float)( dt_exp - dt ) * 1000.0f; // [ms]
        t_last = t1;

        if(nullptr != ship_r){
            const float ship_r_v_abs = ship_r->velocity.length();
            hud_text = pixel::make_text_texture("td "+pixel::to_decstring(t1, ',', 9)+
                          ", v "+std::to_string(ship_r_v_abs)+
                          " [m/s], fps "+std::to_string(pixel::get_gpu_fps()));
        } else {
            hud_text = pixel::make_text_texture("td "+pixel::to_decstring(t1, ',', 9)+
                          ", KAPUTT, fps "+std::to_string(pixel::get_gpu_fps()));
        }

        if( nullptr != ship_r && set_dir ) {
            switch( dir ) {
                case pixel::direction_t::UP: {
                        pixel::f2::vec_t v = ship_r->velocity;
                        v += pixel::f2::vec_t::from_length_angle(ship_vel_step, ship_r->dir_angle);
                        if( v.length() < 2.0f + ship_vel_step) {
                            ship_r->velocity = v;
                        }
                    }
                    break;
                case pixel::direction_t::DOWN:
                    //ship_r->velocity += pixel::f2::vec_t::from_length_angle(-ship_vel_step, ship_r->dir_angle);
                    break;
                case pixel::direction_t::LEFT:
                    ship_r->rotate(pixel::adeg_to_rad(ship_rot_step));
                    break;
                case pixel::direction_t::RIGHT:
                    ship_r->rotate(pixel::adeg_to_rad(-ship_rot_step));
                    break;
                default:
                    break;
            }
        }

        for(asteroid_ref_t a : asteroids) {
            a->tick(dt);
        }
        for(fragment_ref_t a : fragments) {
            a->tick(dt);
        }
        for(int i=(int)fragments.size()-1; i>=0; --i) {
            std::vector<fragment_ref_t>::iterator it = fragments.begin() + i;
            // fragment_ref_t& e = fragments[i];
            if( !(*it)->tick(dt) ) {
                fragments.erase(it);
            }
        }
        /**
        for(auto i=fragments.end()-1; i>=fragments.begin(); --i) {
            if( !(*i)->tick(dt) ) {
                fragments.erase(i);
            }
        } */
        if( nullptr != ship_r && !ship_r->tick(dt) ) {
            make_fragments(fragments, ship_r, ship_r->velocity.length() + ship_vel_step, 0.003f);
            ship_r = nullptr;
        }

        // move ball_1
        // ball_1->tick(dt);

        pixel::set_pixel_color(255 /* r */, 255 /* g */, 255 /* b */, 255 /* a */);
        for(asteroid_ref_t a : asteroids) {
            a->draw();
        }
        for(fragment_ref_t a : fragments) {
            a->draw();
        }
        if( nullptr != ship_r ) {
            ship_r->draw();
        }



        fflush(nullptr);
        pixel::swap_pixel_fb(false);
        if( nullptr != hud_text ) {
            const int thickness_pixel = pixel::cart_coord.to_fb_dy(ship_thickness);
            const int text_height = thickness_pixel;
            const float sy = (float)text_height / (float)hud_text->height;
            hud_text->draw(pixel::fb_width/2.0f-(hud_text->width*sy/2.0f), thickness_pixel, sy, sy);
        }
        pixel::swap_gpu_buffer();
        if( record_bmpseq_basename.size() > 0 ) {
            std::string snap_fname(128, '\0');
            const int written = std::snprintf(&snap_fname[0], snap_fname.size(), "%s-%7.7" PRIu64 ".bmp", record_bmpseq_basename.c_str(), frame_count_total);
            snap_fname.resize(written);
            pixel::save_snapshot(snap_fname);
        }
        ++frame_count_total;
        if( 0 < forced_fps ) {
            const int64_t fudge_ns = pixel::NanoPerMilli / 4;
            const uint64_t ms_per_frame = (uint64_t)std::round(1000.0 / forced_fps);
            const uint64_t ms_last_frame = pixel::getCurrentMilliseconds() - t_fps_last;
            int64_t td_ns = int64_t( ms_per_frame - ms_last_frame ) * pixel::NanoPerMilli;
            if( td_ns > fudge_ns )
            {
                if( true ) {
                    const int64_t td_ns_0 = td_ns%pixel::NanoPerOne;
                    struct timespec ts { td_ns/pixel::NanoPerOne, td_ns_0 - fudge_ns };
                    nanosleep( &ts, NULL );
                    // pixel::log_printf("soft-sync [exp %zd > has %zd]ms, delay %" PRIi64 "ms (%lds, %ldns)\n", ms_per_frame, ms_last_frame, td_ns/pixel::NanoPerMilli, ts.tv_sec, ts.tv_nsec);
                } else {
                    pixel::milli_sleep( td_ns / pixel::NanoPerMilli );
                    // pixel::log_printf("soft-sync [exp %zd > has %zd]ms, delay %" PRIi64 "ms\n", ms_per_frame, ms_last_frame, td_ns/pixel::NanoPerMilli);
                }
            }
        } else if( dt_diff > 1.0f ) {
            pixel::milli_sleep( (uint64_t)dt_diff );
        }
        t_fps_last = pixel::getCurrentMilliseconds();
    }
    printf("Exit");
    exit(0);
}
