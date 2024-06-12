#define PHYSIKS_HPP_

#include <cinttypes>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <iostream>
#include <cctype>

#include <pixel/pixel4f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"

namespace physiks {
    using namespace pixel::f2;
    
    constexpr static const float earth_accel = 9.81f; // [m/s^2]
    constexpr static const float rho_default = 0.75f;

    class ball_t;
    typedef std::shared_ptr<ball_t> ball_ref_t;

    class ball_t : public disk_t {
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
         
        // constexpr static const float max_velocity = 5.6f; // m/s
        // constexpr static const float diff_bounce = 0.075f;
        // constexpr static const float pad_accel = 1.0f+diff_bounce; // factor
        constexpr static const float diff_bounce = 0.075f;
        constexpr static const float rho_deaccel = 1.0f-diff_bounce; // factor
        constexpr static const float pad_accel = 1.0f+diff_bounce; // factor

        std::string id;
        point_t start_pos; // [m]
        float velocity_start; // [m/s] @ angle
        float start_angle; // rad
        float m_gravity; // [m/s^2]
        bool has_gravity;
        float start_velocity_max; // [m/s], derived from drop_height
        bool m_debug_gfx;
        bool m_make_do_reset;

        float velocity_max; // [m/s], derived from drop_height
        bool use_velocity_max; // only use velo_max if drop_height > 0
        float m_rho;
        vec_t velocity; // [m/s]
        float medium_accel = -0.08f; // [m/s*s]
        float min_velocity;

      private:
        std::vector<rect_ref_t> player_pads;
        
      public:
        /** Actually private, a secret to render public ctor also private. */
        class secret_t {
          public:
            int lala;            
                        
          private:
            friend ball_t;
            secret_t(int lili) : lala(lili) {}  
        };
        
        /**
         * Actually a private ctor, use ball_t::create()
         * @param id_
         * @param center position in [m] within coordinate system 0/0 origin
         * @param r_m
         * @param velocity_
         * @param v_angle_rad
         * @param gravity
         * @param drop_height total height this ball will fall in [m]
         * @param debug_gfx
         */
        ball_t(secret_t s, std::string id_, const point_t& center_, const float r_m,
               const float velocity_, const float v_angle_rad,
               const float gravity, const float drop_height,
               const bool debug_gfx, bool make_do_reset = true)
        : disk_t(center_, r_m),
          id(std::move(id_)), start_pos(center_),
          velocity_start(velocity_),
          start_angle(v_angle_rad),
          m_gravity(gravity), has_gravity(true),
          start_velocity_max( std::sqrt( 2 * gravity * std::abs(drop_height) ) ),
          m_debug_gfx(debug_gfx),
          m_make_do_reset(make_do_reset),
          velocity_max(start_velocity_max),
          use_velocity_max( !pixel::is_zero(velocity_max) ),
          m_rho(rho_default), 
          velocity( vec_t::from_length_angle(velocity_start, start_angle) ), min_velocity(0.1f)
        {
            (void)s;
            rotate(start_angle); // direction of velocity -> this->dir_angle
            if( m_debug_gfx ) {
                const uint64_t elapsed_ms = pixel::getElapsedMillisecond();
                pixel::log_printf(elapsed_ms, "ball %s-i, %s\n", id.c_str(), toString().c_str());
                pixel::log_printf(elapsed_ms, "Ball %s-i: v %s, |%f| m/s, %s\n",
                        id.c_str(), velocity.toString().c_str(), velocity.length(), box().toString().c_str());
            }
        }
        /**
         * Public factory ctor for ball exposed to gravity.
         * @param id_
         * @param center position in [m] within coordinate system 0/0 origin
         * @param r_m
         * @param velocity_
         * @param v_angle_rad
         * @param gravity
         * @param drop_height total height this ball will fall in [m]
         * @param debug_gfx
         */
        static ball_ref_t create(std::string id_, const point_t& center_, const float r_m,
               const float velocity_, const float v_angle_rad,
               const float gravity, const float drop_height,
               const bool debug_gfx, bool make_do_reset = true) {
            return std::make_shared<ball_t>(secret_t(42), id_, center_, r_m, velocity_, v_angle_rad,
                                            gravity, drop_height, debug_gfx, make_do_reset);
        }
        
        /** Actually a private ctor, use ball_t::create() */
        ball_t(secret_t s, std::string id_, const point_t& center_, const float r_m,
               const float velocity_, const float v_angle_rad,
               const float velocity_max_,
               const bool debug_gfx, std::vector<rect_ref_t> playerpads)
        : disk_t(center_, r_m),
          id(std::move(id_)), start_pos(center_),
          velocity_start(velocity_),
          start_angle(v_angle_rad),
          m_gravity(0), has_gravity(false),
          start_velocity_max( velocity_max_ ),
          m_debug_gfx(debug_gfx),
          m_make_do_reset(false),
          velocity_max(start_velocity_max),
          use_velocity_max( false ),
          m_rho(rho_default),
          velocity(), min_velocity(3.0f), player_pads(std::move(playerpads))
        {
            (void)s;
            rotate(start_angle); // direction of velocity -> this->dir_angle
            velocity = vec_t::from_length_angle(velocity_start, this->dir_angle);
            if( m_debug_gfx ) {
                const uint64_t elapsed_ms = pixel::getElapsedMillisecond();
                pixel::log_printf(elapsed_ms, "ball %s-i, %s\n", id.c_str(), toString().c_str());
                pixel::log_printf(elapsed_ms, "Ball %s-i: v %s, |%f| m/s, %s\n",
                        id.c_str(), velocity.toString().c_str(), velocity.length(), box().toString().c_str());
            }
        }
        /**
         * Public factory ctor for ball not exposed to gravity.
         * @param id_
         * @param center position in [m] within coordinate system 0/0 origin
         * @param r_m
         * @param velocity_
         * @param v_angle_rad
         * @param gravity
         * @param drop_height total height this ball will fall in [m]
         * @param debug_gfx
         */
        static ball_ref_t create(std::string id_, const point_t& center_, 
               const float r_m, const float velocity_, const float v_angle_rad,
               const float velocity_max_,
               const bool debug_gfx, std::vector<rect_ref_t> playerpads) {
            return std::make_shared<ball_t>(secret_t(42), id_, center_, r_m, velocity_, v_angle_rad,
                                            velocity_max_, debug_gfx, playerpads);
        }

        float get_roh() { return m_rho; }

        void set_roh(const float &v) { m_rho = v; }

        ball_t & operator=(const ball_t &o) = default;
        
        void reset(bool force_start_pos=false){
            velocity_max = start_velocity_max;
            if( !force_start_pos && velocity_start > 0.0f && this->on_screen() ) {
                // shoot back ;-)
                this->dir_angle = M_PI - this->dir_angle; // 180 - adeg
            } else {
                this->center.x = start_pos.x;
                this->center.y = start_pos.y-radius;
                this->dir_angle = start_angle; // direction of velocity
            }
            velocity = vec_t::from_length_angle(velocity_start, this->dir_angle);
            pixel::log_printf("Ball %s-res: v %s, |%f| m/s, %s, %s\n",
                    id.c_str(), velocity.toString().c_str(), velocity.length(),
                    toString().c_str(), box().toString().c_str());
        }

        bool tick(const float dt) noexcept override {
            // const float min_velocity = 0.02f;
            if(!has_gravity){
                const float v_abs = velocity.length();
                if( v_abs < min_velocity ) {
                    velocity *= 1.5f; // boost
                    medium_accel = std::abs(medium_accel); // medium speedup
                } else if( v_abs >= velocity_max ) {
                    velocity *= 0.9f; // break
                    medium_accel = -1.0f*std::abs(medium_accel); // medium slowdown
                }
            }

            const vec_t good_position = center;

            if(has_gravity){
                // Leave horizontal velocity untouched, will be reduced at bounce
                velocity.y -= m_gravity * dt;
            }
            // directional velocity
            vec_t ds_m_dir = velocity * dt; // [m/s] * [s] = [m] -> matches velocity_max roughly in this simulation
            // move vector
            lineseg_t l_move;
            float a_move;
            {
                // Setup move from p0 -> p1
                l_move.p0 = this->center;
                l_move.p1 = l_move.p0 + ds_m_dir;
                a_move = l_move.angle();
                // Extend move size to cover radius in moving direction p1 and -p0
                const vec_t l_move_diff = vec_t::from_length_angle(radius, a_move);
                // l_move.p0 -= l_move_diff;
                l_move.p1 += l_move_diff;
            }
            this->move( ds_m_dir ); 

            // medium_deaccel after move
            if( ds_m_dir.length_sq() > 0 && !has_gravity) {
                velocity = vec_t::from_length_angle(velocity.length() + medium_accel * dt, a_move);
            }

            const uint64_t elapsed_ms = pixel::getElapsedMillisecond();

            geom_ref_t coll_obj = nullptr;
            point_t coll_point;
            vec_t coll_normal;
            vec_t coll_out;
            {
                // detect a collision
                geom_list_t& list = gobjects();
                for(geom_ref_t &g : list) {
                    if( g.get() != this ) {
                        if( g->intersection(coll_out, coll_normal, coll_point, l_move) ) {
                            coll_obj = g;
                            break;
                        }
                    }
                }
            }
            if( m_debug_gfx ) {
                pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
                if(!has_gravity){
                    this->draw(false);
                }
                l_move.draw();
                if( nullptr != coll_obj ) {
                    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
                    if(!has_gravity){
                        this->draw(false);
                    }

                    pixel::set_pixel_color(0 /* r */, 255 /* g */, 0 /* b */, 255 /* a */);
                    vec_t p_dir_angle = vec_t::from_length_angle(2.0f*radius, a_move);
                    lineseg_t l_in(coll_point-p_dir_angle, coll_point);
                    l_in.draw();

                    pixel::set_pixel_color(255 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
                    lineseg_t l_CN(coll_point, coll_point+coll_normal);
                    l_CN.draw();

                    pixel::set_pixel_color(0 /* r */, 0 /* g */, 255 /* b */, 255 /* a */);
                    lineseg_t l_out(coll_point, coll_point+coll_out);
                    l_out.draw();

                    pixel::set_pixel_color(255 /* r */, 255 /* g */, 0 /* b */, 255 /* a */);
                    // reconstruct distance post collision, minimum to surface
                    vec_t vec_post_coll = l_move.p1 - coll_point;
                    const float s_post_coll = std::max( radius, vec_post_coll.length() * m_rho );
                    // reconstruct position post collision from collision point
                    vec_t coll_out2 = coll_out;
                    vec_t new_center = coll_point + ( coll_out2.normalize() * s_post_coll );
                    lineseg_t l_new_center(coll_point, new_center);
                    l_new_center.draw();

                    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);

                    pixel::log_printf(elapsed_ms, "\n");
                    pixel::log_printf(elapsed_ms, "Ball %s-e-a: v %s, |%f| / %f m/s\n",
                            id.c_str(), velocity.toString().c_str(), velocity.length(), velocity_max);                            
                    pixel::log_printf(elapsed_ms, "Ball %s-e-a: ds %s [m/s], move[angle %f, len %f, %s]\n",
                            id.c_str(), ds_m_dir.toString().c_str(),
                            pixel::rad_to_adeg(a_move), l_move.length(), l_move.toString().c_str());                            
                    pixel::log_printf(elapsed_ms, "Ball %s-e-a: coll[point %s, normal[%s, angle %f]]]\n",
                            id.c_str(),
                            coll_point.toString().c_str(), 
                            coll_normal.toString().c_str(), pixel::rad_to_adeg(coll_normal.angle()));                            
                    pixel::log_printf(elapsed_ms, "Ball %s-e-a: coll[out[%s, angle %f]]]\n",
                            id.c_str(),
                            coll_out.toString().c_str(), pixel::rad_to_adeg(coll_out.angle()));
                    pixel::log_printf(elapsed_ms, "Ball %s-e-a: %s\n", id.c_str(), coll_obj->toString().c_str());
                } else {
                    this->draw(false);
                }
            }
            if( nullptr != coll_obj ) {
                // collision
                if(has_gravity){
                    // reconstruct distance post collision, minimum to surface
                    vec_t vec_post_coll = l_move.p1 - coll_point;
                    const float s_post_coll = std::max( radius, vec_post_coll.length() * m_rho );

                    // vec_t v_move = l_move.p1 - l_move.p0;
                    // adjust position out of collision space
                    // center = coll_point + vec_t::from_length_angle(l_move.length()/2.0f, coll_out.angle());
                    // center = coll_point + coll_out/2.0f;

                    // reconstruct position post collision from collision point
                    vec_t coll_out2 = coll_out;
                    center = coll_point + ( coll_out2.normalize() * s_post_coll );
                    if( m_debug_gfx ) {
                        this->draw(false);
                    }

                    // bounce velocity: current velocity * 0.75 (rho) in collision reflection angle
                    if( use_velocity_max ) {
                        velocity_max *= m_rho;
                        velocity = vec_t::from_length_angle(velocity_max, coll_out.angle()); // cont using calculated velocity
                    } else {
                        velocity = vec_t::from_length_angle(velocity.length() * m_rho, coll_out.angle()); // cont using simulated velocity
                    }
                } else {
                    float accel_factor;
                    if( std::find(player_pads.begin(), player_pads.end(), coll_obj) != player_pads.end() ) {
                        accel_factor = pad_accel; // speedup
                    } else {
                        accel_factor = rho_deaccel; // rho deaccel
                    }
                    velocity = vec_t::from_length_angle(velocity.length() * accel_factor, coll_out.angle()); // cont using simulated velocity
                    center = coll_point + coll_out;
                }
                if( !this->on_screen() ) {
                    center = good_position;
                }
                if( has_gravity  ) {
                    // const bool do_reset = velocity.norm() <= min_velocity;
                    // bool do_reset = velocity_max <= min_velocity;
                    bool do_reset = m_make_do_reset && velocity.length() <= min_velocity;                
                    if( m_debug_gfx ) {
                        pixel::log_printf(elapsed_ms, "Ball %s-e-b: v %s, |%f| / %f m/s, reset %d, %s, %s\n",
                                id.c_str(), velocity.toString().c_str(), velocity.length(), velocity_max,
                                do_reset, toString().c_str(), box().toString().c_str());
                    }
                    if( do_reset ) {
                        reset();
                    }
                }
                // exit(1);
                return true;
            } else {
                // no collision
                if( !this->on_screen() ) {
                    if (m_debug_gfx) {
                        pixel::log_printf(elapsed_ms, "Ball %s-off: reset %d, v %s, |%f| m/s, %s, %s\n",
                                          id.c_str(), m_make_do_reset, velocity.toString().c_str(), 
                                          velocity.length(), toString().c_str(), 
                                          box().toString().c_str());
                    }
                    if( m_make_do_reset ) {
                        reset();
                        return true;
                    } else {
                        return false; // left screen and no reset
                    }
                } else {
                    return true;
                }
            }
        }

        void draw(const bool filled) const noexcept {        
            disk_t::draw(filled);
        }
    };
};
