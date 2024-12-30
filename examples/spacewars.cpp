/*
 * Author: Svenson Han Göthel und Sven Göthel
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
#include <iostream>
#include <memory>
#include <pixel/pixel3f.hpp>
#include <pixel/pixel4f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"

#include <algorithm>
#include <random>
#include <cinttypes>

using namespace pixel::literals;

static pixel::input_event_t event;
constexpr static const int player_id_1 = 1;
constexpr static const int player_id_2 = 2;
constexpr static const int player_id_3 = 3;
constexpr static const float spaceship_height = 10.0f; // [m]
constexpr static const float space_height = spaceship_height*30.0f; // [m]
constexpr static const float sun_gravity = 28 * 8.0f; // [m/s^2]
static const uint8_t rgba_white[/*4*/] = { 255, 255, 255, 255 };
static const uint8_t rgba_yellow[/*4*/] = { 255, 255, 0, 255 };
static const uint8_t rgba_red[/*4*/] = { 255, 0, 0, 255 };
static const uint8_t rgba_green[/*4*/] = { 0, 255, 0, 255 };
// static const uint8_t rgba_blue[/*4*/] = { 0, 0, 255, 255 };
static const float text_lum = 0.75f;
static const pixel::f4::vec_t vec4_text_color(text_lum, text_lum, text_lum, 1.0f);
static bool debug_gfx = false;
static bool show_ship_velo = false;
static bool cloak_enabled = false;

class idscore_t {
    public:
        constexpr static const int score_frag =  5;
        constexpr static const int score_ship = 10 * score_frag;

    private:
        int m_id;
        int m_score;

    public:
        idscore_t(const int id)
        : m_id(id), m_score(0) {}

        constexpr int id() const noexcept { return m_id; }
        constexpr int score() const noexcept { return m_score; }
        void add_score(int diff) noexcept { m_score += diff; }
        void reset() noexcept { m_score = 0; }
};
static idscore_t world_id(0);

class star_t {
    public:
        const float r0;
        const float g0_env, g0_ships; // [m/s^2]
        pixel::f2::disk_t body;
        float dr_dir = 1;

        star_t(const pixel::f2::point_t& p0, const float r, const float g_env, const float g_ships)
        : r0(r), g0_env(g_env), g0_ships(g_ships), body(p0, r) {}

        bool tick(const float dt) noexcept {
            const float dr_min = r0 * 0.95f;
            const float dr_max = r0 * 1.05f;
            const float r = body.radius + r0 * 0.1f * dt * dr_dir;
            if( r <= dr_min ) {
                dr_dir = 1;
                body.radius = dr_min;
            } else if( r >= dr_max ) {
                dr_dir = -1;
                body.radius = dr_max;
            } else {
                body.radius = r;
            }
            return true;
        }
        void draw() {
            body.draw(true);
        }

        /** Returns star's environmental gravity [m/s^2] impact on given position. */
        pixel::f2::vec_t gravity_env(const pixel::f2::point_t& p) {
            return gravity(p, g0_env);
        }
        /** Returns star's spaceship's gravity [m/s^2] impact on given position. */
        pixel::f2::vec_t gravity_ships(const pixel::f2::point_t& p) {
            return gravity(p, g0_ships);
        }
        /** Returns star's gravity [m/s^2] impact on given position. */
        pixel::f2::vec_t gravity(const pixel::f2::point_t& p, const float g0) {
            pixel::f2::vec_t v_d = body.center - p;
            const float d = v_d.length();
            if( pixel::is_zero(d) ) {
                return pixel::f2::vec_t();
            } else {
                // v.normalize() -> v / d
                // return v.normalize() * ( g0 / ( d * d ) );
                return ( v_d /= d ) *= ( g0 / ( d * d ) ); // same as above but reusing vector 'v'
            }
        }

        bool hit(const pixel::f2::point_t& c) const noexcept {
            return (c - body.center).length() <= body.radius;
        }
};

typedef std::shared_ptr<star_t> star_ref_t;

star_ref_t sun;

static std::random_device rng;
static const float rng_range = (float)std::random_device::max() - (float)std::random_device::min() + 1.0f;

static float next_rnd() noexcept {
    if constexpr (false) {
        const float r0 = (float)rng();
        const float r = r0 / rng_range;
        std::cout << "rnd: r0 " << r0 << " / " << rng_range << " = " << r << std::endl;
        return r;
    } else {
        return (float)rng() / rng_range;
    }
}

class fragment_t : public pixel::f2::linestrip_t {
    public:
        pixel::f2::vec_t velocity; // [m/s]
        float rotation_velocity; // [angle_radians/s]
        bool leave_on_screen_exit;

        /**
         *
         * @param center center position
         * @param angle angle in radians
         * @param v velocity in meter per seconds
         * @param rot_v rotation velocity in radians per seconds
         */
        fragment_t(const pixel::f2::point_t& center,
                   const float angle, const float v, const float rot_v, bool leave_on_screen_exit_) noexcept
        : linestrip_t(center, angle),
          velocity( pixel::f2::vec_t::from_length_angle(v, angle) ),
          rotation_velocity(rot_v), leave_on_screen_exit(leave_on_screen_exit_)
        { }

        /**
         *
         * @param center center position
         * @param v velocity vector in meter per seconds
         * @param rot_v rotation velocity in radians per seconds
         */
        fragment_t(const pixel::f2::point_t& center,
                const pixel::f2::vec_t& v, const float rot_v, bool leave_on_screen_exit_) noexcept
        : linestrip_t(center, v.angle()),
          velocity(v),
          rotation_velocity(rot_v), leave_on_screen_exit(leave_on_screen_exit_)
        { }

        bool tick(const float dt) noexcept override {
            pixel::f2::vec_t g = sun->gravity_env(p_center);
            velocity += g * dt;
            move(velocity * dt);
            rotate(rotation_velocity * dt);
            if( p_center.x < pixel::cart_coord.min_x() ) {
                if( leave_on_screen_exit ) {
                    return false;
                }
                move(pixel::cart_coord.max_x()-p_center.x, 0.0f);
            }
            if( p_center.x > pixel::cart_coord.max_x() ) {
                if( leave_on_screen_exit ) {
                    return false;
                }
                move(pixel::cart_coord.min_x()-p_center.x, 0.0f);
            }
            if( p_center.y < pixel::cart_coord.min_y() ) {
                if( leave_on_screen_exit ) {
                    return false;
                }
                move(0.0f, pixel::cart_coord.max_y()-p_center.y);
            }
            if( p_center.y > pixel::cart_coord.max_y() ) {
                if( leave_on_screen_exit ) {
                    return false;
                }
                move(0.0f, pixel::cart_coord.min_y()-p_center.y);
            }
            return true;
        }

        void draw() const noexcept override {
            linestrip_t::draw();
            if( debug_gfx ) {
                pixel::set_pixel_color(rgba_yellow);
                pixel::f2::lineseg_t::draw(p_center, p_center+velocity);
                pixel::set_pixel_color(rgba_white);
            }
        }
};
typedef std::shared_ptr<fragment_t> fragment_ref_t;

std::vector<fragment_ref_t> fragments;

void make_fragments(std::vector<fragment_ref_t>& dest,
                   const pixel::f2::linestrip_ref_t& ls, const float v, const float rot_v)
{
    if( ls->p_list.size() <= 3+1 ) {
        return; // drop simple fragment
    }
    pixel::f2::point_t p0 = ls->p_list[0];
    for(size_t i=1; i<ls->p_list.size(); ++i) {
        const pixel::f2::point_t& pc = ls->p_center;
        const pixel::f2::point_t& p1 = ls->p_list[i];
        // const double area = std::abs( pixel::f2::tri_area(pc, p0, p1) );
        // if( area >= 0.5f ) {
        {
            const pixel::f2::point_t p_v = p0 + ( p1 - p0 ) / 2.0f;
            const pixel::f2::vec_t v_v0 = ( p_v - pc ).normalize() * v;
            fragment_ref_t f = std::make_shared<fragment_t>(pc, v_v0, rot_v, true);
            f->p_list.push_back(pc);
            f->p_list.push_back(p0);
            f->p_list.push_back(p1);
            f->p_list.push_back(pc);
            f->normalize_center();
            dest.push_back(f);
        }
        p0 = p1;
    }
}

fragment_ref_t make_asteroid(const pixel::f2::point_t& center, float height,
                             const float angle,
                             const float velocity,
                             const float rot_velocity,
                             const float jitter=1.0f/8.0f) noexcept
{
    const float max_height = 6.0f*height;
    const float min_height = 1.0f*height;
    height = std::max(min_height, std::min(height, 6*max_height));
    fragment_ref_t lf = std::make_shared<fragment_t>(center, angle, velocity, rot_velocity, false);

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

    lf->normalize_center();
    return lf;
}

class peng_t {
  private:
    idscore_t* m_owner;
    float m_fuse; // [s]
    constexpr static float fuse2_max = 30_s;
    float m_fuse2 = fuse2_max;

    bool hits_fragment() noexcept {
        bool hit = false;
        std::vector<fragment_ref_t> new_fragments;
        for(auto it = fragments.begin(); it != fragments.end(); ) {
            fragment_ref_t f = *it;
            if( m_peng.box().intersects(f->box()) ) {
                m_owner->add_score(idscore_t::score_frag);
                hit = true;
                it = fragments.erase(it);
                make_fragments(new_fragments, f,
                               f->velocity.length() + m_velo.length()/4,
                               f->rotation_velocity*2.0f);
            } else {
                ++it;
            }
        }
        fragments.insert(fragments.end(), new_fragments.begin(), new_fragments.end());
        return hit;
    }

  public:
    pixel::f2::vec_t m_velo; // [m/s]
    pixel::f2::rect_t m_peng;

    peng_t(idscore_t* owner,
           const pixel::f2::point_t& p0, const float diag, const pixel::f2::vec_t& v,
           const float fuse = 0.25f) noexcept
    : m_owner(owner), m_fuse(fuse), m_velo( v ),
      m_peng(p0 + pixel::f2::point_t(-diag/2, +diag/2), diag, diag, v.angle())
    { }

    idscore_t& owner() noexcept { return *m_owner; }

    bool tick(const float dt) noexcept {
        if(!m_velo.is_zero()){
            pixel::f2::vec_t g = sun->gravity_env(m_peng.p_center);
            m_velo += g * dt;
            m_peng.move( m_velo * dt );
        }
        if( armed() ) {
            m_fuse2 -= dt;
        }
        m_peng.rotate(std::numbers::pi_v<float> * dt); // 180 degrees (180_deg)
        m_fuse = std::max(0.0f, m_fuse - dt);
        return m_fuse2 > 0 && !sun->hit( m_peng.p_center ) &&
               !hits_fragment();
    }
    bool armed() const noexcept { return pixel::is_zero(m_fuse); }
    float fuse2() const noexcept { return m_fuse2; }

    void draw() const noexcept {
        if( !armed() ) {
            pixel::set_pixel_color(rgba_green);
        } else {
            const float pct = m_fuse2 / fuse2_max;
            pixel::set_pixel_color4f(1.0f, pct, pct, 1.0f);
        }
        m_peng.draw(false);
        pixel::set_pixel_color(rgba_white);
    }

    void changeSpeed(float a){
        m_velo *= a;
    }

    bool on_screen(){
        return m_peng.on_screen();
    }
    bool intersection(const peng_t& o) const {
        return m_peng.intersects(o.m_peng);
    }
};

std::vector<peng_t> pengs;

class spaceship_t : public pixel::f2::linestrip_t {
    public:
        constexpr static const float height = spaceship_height; // [m]
        constexpr static const float vel_step = 5.0f; // [m/s]
        constexpr static const float vel_max = 100.0f + vel_step; // [m/s]
        constexpr static const float rot_step = 180.0f; // [ang-degrees / s]

        constexpr static const float peng_diag = 0.15f*height;
        constexpr static const float peng_velo_0 = vel_max / 2;
        constexpr static const int peng_inventory_max = 4000;
        constexpr static const int mine_inventory_max = 2000;

        constexpr static const float shield_radius = spaceship_height * 0.9f;
        constexpr static const float shield_time_max = 10; // [s]

    private:
        idscore_t* m_owner;

        float m_shield_time;
        bool m_shield;

        bool hits_fragment(const pixel::f2::aabbox_t& box) noexcept {
            bool hit = false;
            std::vector<fragment_ref_t> new_fragments;
            for(auto it = fragments.begin(); it != fragments.end(); ) {
                fragment_ref_t f = *it;
                if( box.intersects(f->box()) ) {
                    if( !m_shield ) {
                        m_owner->add_score(-idscore_t::score_ship);
                    }
                    hit = true;
                    it = fragments.erase(it);
                    make_fragments(new_fragments, f,
                            f->velocity.length() + velocity.length(),
                            f->rotation_velocity*2.0f);
                } else {
                    ++it;
                }
            }
            fragments.insert(fragments.end(), new_fragments.begin(), new_fragments.end());
            return hit;
        }
        bool hits_peng(const pixel::f2::aabbox_t& box) noexcept {
            bool hit = false;
            for(auto it = pengs.begin(); it != pengs.end(); ) {
                peng_t& p = *it;
                if( p.armed() && box.intersects(p.m_peng.box()) ) {
                    hit = true;
                    it = pengs.erase(it);
                    if( !m_shield ) {
                        if( m_owner->id() == p.owner().id() ) {
                            m_owner->add_score(-idscore_t::score_ship);
                        } else {
                            p.owner().add_score(idscore_t::score_ship);
                        }
                    }
                } else {
                    ++it;
                }
            }
            return hit;
        }

    public:
        pixel::f2::disk_t shield_body = pixel::f2::disk_t(p_center, shield_radius);
        pixel::f2::vec_t velocity; // [m/s]
        int peng_inventory;
        int mine_inventory;

        spaceship_t(idscore_t* owner,
                    const pixel::f2::point_t& center, const float angle) noexcept
        : linestrip_t(center, angle), m_owner(owner),
          m_shield_time(shield_time_max), m_shield(false),
          velocity(), peng_inventory(peng_inventory_max), mine_inventory(mine_inventory_max)
        {}

        void peng() noexcept {
            if(peng_inventory > 0){
                pixel::f2::point_t p0;
                // adjust start posision to geometric ship model
                if(m_owner->id() == player_id_3){
                    p0 = p_list[4];
                } else {
                    p0 = p_list[0];
                }
                pixel::f2::vec_t v_p = velocity + pixel::f2::vec_t::from_length_angle(peng_velo_0, dir_angle);
                pengs.emplace_back(m_owner, p0, peng_diag, v_p);
                --peng_inventory;
            }
        }

        void mine() noexcept {
            if(mine_inventory <= 0){
                return;
            }
            if( true ) {
                pengs.emplace_back(m_owner,
                    pixel::f2::point_t(p_center.x, p_center.y-peng_diag)-
                    5*velocity.copy().normalize(),
                    peng_diag, pixel::f2::vec_t(), 2);
            } else {
                pengs.emplace_back(m_owner,
                    p_center, peng_diag, -velocity, 2);
            }
            --mine_inventory;
        }

        void velo_up(const float dv = vel_step) noexcept {
            pixel::f2::vec_t v = velocity;
            v += pixel::f2::vec_t::from_length_angle(dv, dir_angle);
            if( v.length() < vel_max ) {
                velocity = v;
            }
        }
        void rotate_adeg(const float da_adeg) {
            rotate(pixel::adeg_to_rad(da_adeg));
        }

        bool shield() const noexcept { return m_shield; }
        float shield_time() const noexcept { return m_shield_time; }

        void toggle_shield() noexcept {
            set_shield(!m_shield);
        }
        void set_shield(bool v) noexcept {
            if( m_shield_time <= 0 ) {
                m_shield = false;
            } else {
                m_shield = v;
            }
        //    m_shield = true; // FIXME
        }

        pixel::f2::aabbox_t box() const noexcept override {
            if( m_shield ) {
                return shield_body.box();
            } else {
                return linestrip_t::box();
            }
        }
        void move(const pixel::f2::point_t& d) noexcept override {
            linestrip_t::move(d);

            if( p_center.x < pixel::cart_coord.min_x() ) {
                linestrip_t::move(pixel::cart_coord.max_x()-p_center.x, 0.0f);
            }
            if( p_center.x > pixel::cart_coord.max_x() ) {
                linestrip_t::move(pixel::cart_coord.min_x()-p_center.x, 0.0f);
            }
            if( p_center.y < pixel::cart_coord.min_y() ) {
                linestrip_t::move(0.0f, pixel::cart_coord.max_y()-p_center.y);
            }
            if( p_center.y > pixel::cart_coord.max_y() ) {
                linestrip_t::move(0.0f, pixel::cart_coord.min_y()-p_center.y);
            }
            shield_body.center = this->p_center;
        }
        bool tick(const float dt) noexcept override {
            pixel::f2::vec_t g = sun->gravity_ships(p_center);
            velocity += g * dt;

            move(velocity * dt);

            if( m_shield ) {
                m_shield_time -= dt;
                if( m_shield_time <= 0 ) {
                    m_shield = false;
                    m_shield_time = 0;
                }
            }
            if( sun->hit(p_center) ) {
                m_owner->add_score(-m_owner->score_ship);
                return false;
            }
            pixel::f2::aabbox_t b = box();
            return ( !hits_fragment(b) && !hits_peng(b) ) || m_shield;
        }

        void set_orbit_velocity() noexcept {
            velocity = orbit_velocity();
        }
        pixel::f2::vec_t orbit_velocity() const noexcept {
            // g = v^2 / d
            const float d = ( sun->body.center - p_center ).length();
            pixel::f2::vec_t g = sun->gravity_ships(p_center);
            const float v0 = std::sqrt( g.length() * d );
            g.normalize().rotate(-M_PI_2) *= v0; // becomes velocity
            if( g.angle(velocity) > M_PI_2 ) {
                g.rotate(M_PI); // rotate to velocity direction
            }
            return g;
        }

        void draw() const noexcept override {
            pixel::set_pixel_color(rgba_white);
            if( m_shield ) {
                shield_body.draw(false);
            }
            linestrip_t::draw();
            if( show_ship_velo ) {
                pixel::set_pixel_color(rgba_yellow);
                pixel::f2::lineseg_t::draw(p_center, p_center+velocity);
                pixel::set_pixel_color(rgba_red);
                pixel::f2::vec_t v_o = orbit_velocity();
                pixel::f2::lineseg_t::draw(p_center, p_center+v_o);
                // pixel::set_pixel_color(rgba_green);
                // pixel::f2::disk_t o(sun->body.center, (sun->body.center - p_center).length());
                // o.draw(false);
                pixel::set_pixel_color(rgba_white);
            }
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
spaceship_ref_t make_spaceship1(idscore_t* owner, const pixel::f2::point_t& m, const float h=spaceship_t::height) noexcept
{
    spaceship_ref_t lf = std::make_shared<spaceship_t>(owner, m, 90_deg);

    const float width = 4.0f/5.0f * h;;

    // a
    pixel::f2::point_t p = m;
    p.y += h/2.0f;
    lf->p_list.push_back(p);

    // lf->lala = 4;

    // b
    p.y -= h;
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
    lf->normalize_center();
    return lf;
}

/**
 * Unrotated:
 *
 *      (a)
 *      | |
 *    | | | |
 *    | | | |
 *    |=|c|=|
 *
 */
spaceship_ref_t make_spaceship2(idscore_t* owner, const pixel::f2::point_t& m, const float h=spaceship_t::height) noexcept
{
    spaceship_ref_t lf = std::make_shared<spaceship_t>(owner, m, 90_deg);

    const float w = 4.0f / 5.0f * h;
    const float w_s = w / 4.0f;
    const float h_s = h / 5.0f;
    pixel::f2::point_t p = m;

    p.x -= 0.25f * w_s;
    p.y += h / 2;
    lf->p_list.push_back(p);

    p.y -= 4 * h_s;
    lf->p_list.push_back(p);

    p.x -= ( 2.0f - 0.25f ) * w_s;
    lf->p_list.push_back(p);

    p.y += 2 * h_s;
    lf->p_list.push_back(p);

    p.y -= 3 * h_s;
    lf->p_list.push_back(p);

    p.y += 1 * h_s;
    lf->p_list.push_back(p);

    p.x += w;
    lf->p_list.push_back(p);

    p.y -= 1 * h_s;
    lf->p_list.push_back(p);

    p.y += 3 * h_s;
    lf->p_list.push_back(p);

    p.y -= 2 * h_s;
    lf->p_list.push_back(p);

    p.x -= ( 2.0f - 0.25f ) * w_s;
    lf->p_list.push_back(p);

    p.y += 4 * h_s;
    lf->p_list.push_back(p);

    p.x -= 0.50f * w_s;
    lf->p_list.push_back(p);

    lf->normalize_center();
    return lf;
}

/**
 * Unrotated:
 *
 *    /\
 *   /  \
 *|  |  |  |
 *----------
 */
spaceship_ref_t make_spaceship3(idscore_t* owner, const pixel::f2::point_t& m, const float h=spaceship_t::height) noexcept
{
    spaceship_ref_t lf = std::make_shared<spaceship_t>(owner, m, 90_deg);

    const float w = 4.0f / 5.0f * h;
    pixel::f2::point_t p = {m.x - w / 2, m.y - h / 2};

    lf->p_list.push_back(p);

    p.y -= h / 2;
    lf->p_list.push_back(p);

    p.x += w / 3;
    lf->p_list.push_back(p);

    p.y += h / 2;
    lf->p_list.push_back(p);

    p.y += h / 2;
    p.x += w / 6;
    lf->p_list.push_back(p);

    p.x += w / 6;
    p.y -= h / 2;
    lf->p_list.push_back(p);

    p.y -= h / 2;
    lf->p_list.push_back(p);

    p.x -= w / 3;
    lf->p_list.push_back(p);

    p.x += w / 3 * 2;
    lf->p_list.push_back(p);

    p.y += h / 2;
    lf->p_list.push_back(p);

    lf->normalize_center();
    return lf;
}

void reset_asteroids(int count) {
    fragments.clear();
    for(int i = 0; i < count; ++i) {
        const float height_h = spaceship_t::height*2.0f;
        const float height = height_h + height_h*next_rnd();
        const float angle = pixel::adeg_to_rad(next_rnd() * 360.0f);
        const float velocity = 10.0f + next_rnd() * 10.0f; // m/s
        const float rot_velocity = ( 15_deg +
                                     next_rnd() * 15_deg )
                                   * ( i%2 == 0 ? 1.0f : -1.0f ); // angle/s
        const float jitter = 1.0f / ( 4.0f + 4.0f * next_rnd() );
        pixel::f2::point_t p0(pixel::cart_coord.min_x()+(pixel::cart_coord.width()*next_rnd()),
                              i%2 == 0 ? pixel::cart_coord.min_y()+height/2 : pixel::cart_coord.max_y()-height/2);
        fragment_ref_t asteroid1 = make_asteroid(p0, height,
                angle, velocity, rot_velocity, jitter );
        fragments.push_back(asteroid1);
    }

}

class player_t : public idscore_t {
    private:
        constexpr static const pixel::f2::point_t p0_ss1 = {  6 * spaceship_height,  6 * spaceship_height };
        constexpr static const pixel::f2::point_t p0_ss2 = { -6 * spaceship_height, -6 * spaceship_height };
        constexpr static const pixel::f2::point_t p0_ss3 = {  6 * spaceship_height, -6 * spaceship_height };
        float m_respawn_timer;
        spaceship_ref_t m_ship;
        bool m_cloak;

        void ship_dtor() noexcept {
            make_fragments(fragments, m_ship, m_ship->velocity.length() + spaceship_t::vel_step, 0.003f);
            m_ship = nullptr;
            m_respawn_timer = 5; // [s]
        }

        void respawn_ship() noexcept {
            m_cloak = false;
            m_respawn_timer = 0;
            if( player_id_1 == id() ) {
                m_ship = make_spaceship1(this, p0_ss1);
            } else if(player_id_2 == id()){
                m_ship = make_spaceship2(this, p0_ss2);
            } else {
                m_ship = make_spaceship3(this, p0_ss3);
            }
            m_ship->set_orbit_velocity();
        }

    public:
        static void collision(player_t& player1, player_t& player2) noexcept {
            spaceship_ref_t& ship1 = player1.m_ship;
            spaceship_ref_t& ship2 = player2.m_ship;
            if( nullptr != ship1 && nullptr != ship2 && ship1->intersects(*ship2)) {
                if( !ship2->shield() && !ship1->shield() ) {
                    player1.ship_dtor();
                    player2.ship_dtor();
                } else if( !ship2->shield() && ship1->shield() ) {
                    player2.ship_dtor();
                    player2.add_score(-score_ship);
                } else if( ship2->shield() && !ship1->shield() ) {
                    player1.add_score(-score_ship);
                    player1.ship_dtor();
                } else if( ship2->shield() && ship1->shield() ) {
                    /**
                    pixel::f2::vec_t cross_normal;
                    pixel::f2::point_t cross_point;
                    pixel::f2::vec_t tangente_r = pr.m_ship->velocity.normal_ccw();
                    pixel::f2::vec_t tangente_l = pl.m_ship->velocity.normal_ccw();
                            pl.m_ship->velocity.normal_ccw();
                    // pr.m_ship->shield_body.intersection(pr.m_ship->velocity, cross_normal, cross_point, tangente_r);
                    // pl.m_ship->shield_body.intersection(pl.m_ship->velocity, cross_normal, cross_point, tangente_l);
                     */
                    {
                        pixel::f2::vec_t cross_normal, reflect_out;
                        pixel::f2::point_t cross_point;
                        pixel::f2::vec_t v_norm(ship2->velocity);
                        v_norm.normalize();
                        pixel::f2::point_t p0 = ship2->p_center - v_norm * ship2->shield_radius;
                        pixel::f2::point_t p1 = ship2->p_center + v_norm * ship2->shield_radius;
                        pixel::f2::lineseg_t l12(p0, p1);
                        if( ship1->shield_body.intersection(reflect_out, cross_normal, cross_point, l12) ) {
                            std::cout << "XXX3.0: pr.c " << ship2->p_center
                                      << ", v " << ship2->velocity
                                      << ", cross " << cross_point
                                      << ", r-out " << reflect_out << std::endl;
                            {
                                pixel::f2::vec_t d = reflect_out; //  - ship2->p_center;
                                ship2->move(d);
                                ship2->velocity = ship2->velocity.length() * reflect_out.normalize();
                                std::cout << "XXX3.X: pr.c " << ship2->p_center
                                          << ", d " << d
                                          << ", v " << ship2->velocity << std::endl;
                            }
                            if ( false ) {
                                pixel::f2::vec_t d = reflect_out - ship1->p_center;
                                ship1->move(d);
                                ship1->velocity = ship1->velocity.length() * d.normalize();
                            }
                            event.set_paused(false);
                        }
                    }
                    if( true ){
                        pixel::f2::vec_t cross_normal, reflect_out;
                        pixel::f2::point_t cross_point;
                        pixel::f2::vec_t v_norm(ship1->velocity);
                        v_norm.normalize();
                        pixel::f2::point_t p0 = ship1->p_center - v_norm * ship1->shield_radius;
                        pixel::f2::point_t p1 = ship1->p_center + v_norm * ship1->shield_radius;
                        pixel::f2::lineseg_t l12(p0, p1);
                        if( ship2->shield_body.intersection(reflect_out, cross_normal, cross_point, l12) ) {
                            std::cout << "XXX3.0: pr.c " << ship1->p_center
                                      << ", v " << ship2->velocity
                                      << ", cross " << cross_point
                                      << ", r-out " << reflect_out << std::endl;
                            {
                                pixel::f2::vec_t d = reflect_out; //  - ship2->p_center;
                                ship1->move(d);
                                ship1->velocity = ship1->velocity.length() * reflect_out.normalize();
                                std::cout << "XXX3.X: pr.c " << ship2->p_center
                                          << ", d " << d
                                          << ", v " << ship1->velocity << std::endl;
                            }
                            if ( false ) {
                                pixel::f2::vec_t d = reflect_out - ship2->p_center;
                                ship2->move(d);
                                ship2->velocity = ship1->velocity.length() * d.normalize();
                            }
                            event.set_paused(false);
                        }
                    }
                }
            }
        }

        player_t(const int id) noexcept
        : idscore_t(id), m_respawn_timer(0),
          m_ship(nullptr)
        { respawn_ship(); }

        void reset() noexcept {
            idscore_t::reset();
            respawn_ship();
        }
        spaceship_ref_t ship() noexcept { return m_ship; }
        bool has_ship() noexcept { return nullptr != m_ship; }

        float velocity() const noexcept { return nullptr != m_ship ? m_ship->velocity.length() : 0; }

        int peng_inventory() const noexcept { return nullptr != m_ship ? m_ship->peng_inventory : 0; }

        int mine_inventory() const noexcept { return nullptr != m_ship ? m_ship->mine_inventory : 0; }

        float shield_time() const noexcept { return nullptr != m_ship ? m_ship->shield_time() : 0; }

        bool cloak() const noexcept { return m_cloak; }

        void set_cloak( bool v ) noexcept { m_cloak = cloak_enabled && v; }

        pixel::f2::point_t center() {
            if(m_ship != nullptr){
                return m_ship->p_center;
            } else {
                return pixel::f2::point_t(0, 0);
            }
        }

        bool tick(const float dt) noexcept {
            if( nullptr != m_ship ) {
                if( !m_ship->tick(dt) ) {
                    ship_dtor();
                }
            } else {
                if( m_respawn_timer > 0 ) {
                    m_respawn_timer -= dt;
                }
                if( 0 >= m_respawn_timer ) {
                    respawn_ship();
                }
            }
            return true;
        }
        void draw() const noexcept {
            if( m_ship != nullptr && !m_cloak ) {
                m_ship->draw();
            }
        }
        void handle_event0() noexcept {
            if( nullptr != m_ship && event.has_any_pn(id()) ) {
                if( event.released_and_clr(pixel::to_input_event(id(), pixel::player_event_type_t::ACTION1)) ) {
                    m_ship->peng();
                } else if( event.released_and_clr(pixel::to_input_event(id(), pixel::player_event_type_t::ACTION2)) ) {
                    m_ship->set_orbit_velocity();
                } else if( event.released_and_clr(pixel::to_input_event(id(), pixel::player_event_type_t::ACTION3)) ) {
                    set_cloak(!cloak());
                } else if( event.pressed(pixel::to_input_event(id(), pixel::player_event_type_t::ACTION4))) {
                    m_ship->mine();
                }
            }
        }
        void handle_event1(const float dt /* [s] */) noexcept {
            if( nullptr != m_ship && event.has_any_pn(id()) ) {
                if( event.pressed(pixel::to_input_event(id(), pixel::player_event_type_t::UP)) ) {
                    m_ship->velo_up(spaceship_t::vel_step);
                } else if( event.pressed(pixel::to_input_event(id(), pixel::player_event_type_t::LEFT)) ){
                    m_ship->rotate_adeg(spaceship_t::rot_step * dt);
                } else if( event.pressed(pixel::to_input_event(id(), pixel::player_event_type_t::RIGHT)) ){
                    m_ship->rotate_adeg(-spaceship_t::rot_step * dt);
                }
                m_ship->set_shield( event.pressed(pixel::to_input_event(id(), pixel::player_event_type_t::DOWN)) );
            }
        }
};

static int player_count = 2;
static int asteroid_count = 6;
static pixel::f2::point_t tl_text;
static std::string record_bmpseq_basename;
static bool raster = false;

extern "C" {
    EMSCRIPTEN_KEEPALIVE void set_showvelo(bool v) noexcept { show_ship_velo = v; }
}

void mainloop() {
    static player_t p1(player_id_1);
    static player_t p2(player_id_2);
    static player_t p3(player_id_3);

    static pixel::texture_ref hud_text;
    static uint64_t frame_count_total = 0;
    static uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]
    static const int text_height = 24;
    static bool animating = true;

    while( pixel::handle_one_event(event) ) {
        if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_CLOSE_REQ ) ) {
            printf("Exit Application\n");
            #if defined(__EMSCRIPTEN__)
                emscripten_cancel_main_loop();
            #else
                exit(0);
            #endif
        } else if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_RESIZED ) ) {
            pixel::cart_coord.set_height(-space_height/2.0f, space_height/2.0f);
        }
        if( event.paused() ) {
            animating = false;
        } else {
            if( !animating ) {
                t_last = pixel::getElapsedMillisecond(); // [ms]
            }
            animating = true;
        }

        if( event.released_and_clr(pixel::input_event_type_t::RESET) ) {
            pengs.clear();
            reset_asteroids(asteroid_count);
            p1.reset();
            if(1 < player_count) {
                p2.reset();
            }
            if(2 < player_count) {
                p3.reset();
            }
        }

        // Pass events to all animated objects
        if( animating ) {
            p1.handle_event0();
            
            if(1 < player_count) {
                p2.handle_event0();
            }
            if(2 < player_count) {
                p3.handle_event0();
            }
        }
    }
    const uint64_t t1 = animating ? pixel::getElapsedMillisecond() : t_last; // [ms]
    const float dt = (float)(t1 - t_last) / 1000.0f; // [s]
    t_last = t1;

    if(animating) {
        if(2 < player_count) {
            p3.handle_event1(dt);
            p3.tick(dt);
            player_t::collision(p1, p3);
            player_t::collision(p2, p3);
        }
        if(1 < player_count) {
            p2.handle_event1(dt);
            p2.tick(dt);
            player_t::collision(p1, p2);
        }
        {
            p1.handle_event1(dt);
            p1.tick(dt);
        }
        if (raster) {
            pixel::draw_grid(50, 255, 0, 0, 0, 255, 0, 0, 0);
        }

        // fragments tick
        {
            std::vector<fragment_ref_t> new_fragments;
            for(auto it=fragments.begin(); it != fragments.end(); ) {
                fragment_ref_t f = *it;
                if( !f->tick(dt) ) {
                    it = fragments.erase( it );
                } else if( sun->hit( f->p_center ) ) {
                    it = fragments.erase( it );
                    make_fragments(new_fragments, f, f->velocity.length() * 0.75f, f->rotation_velocity*2.0f);
                } else {
                    ++it;
                }
            }
            fragments.insert(fragments.end(), new_fragments.begin(), new_fragments.end());
        }
        // pengs tick
        {
            for(auto it = pengs.begin(); it != pengs.end(); ) {
                peng_t& p = *it;
                if(p.on_screen()){
                    if( p.tick(dt) ) {
                        ++it;
                        continue;
                    }
                }
                it = pengs.erase(it);
            }
        }
        if( 0 == fragments.size() ) {
            reset_asteroids(asteroid_count);
        }
        sun->tick(dt);
    }
    pixel::clear_pixel_fb(0, 0, 0, 255);

    // Draw all objects
    pixel::set_pixel_color(rgba_white);
    p1.draw();
    if(1 < player_count) {
        p2.draw();
    }
    if(2 < player_count) {
        p3.draw();
    }
    for(fragment_ref_t &a : fragments) {
        a->draw();
    }
    for(auto & peng : pengs) {
        peng.draw();
    }
    pixel::set_pixel_color(255, 255, 255, 255);
    sun->draw();

    float fps = pixel::get_gpu_fps();
    tl_text.set(pixel::cart_coord.min_x(), pixel::cart_coord.max_y());
    {
        std::string sp1, sp2, sp3;
        {
            std::string c;
            if( cloak_enabled ) {
                c = pixel::to_string(", %6.2f / %6.2f", p1.center().x, p1.center().y);
            }
            sp1 = pixel::to_string("S1 %4d (%4d pengs, %2d mines, %.1f s shield, %4.2f m/s%s)",
                p1.score(), p1.peng_inventory(), p1.mine_inventory(), p1.shield_time(), p1.velocity(), c.c_str());
            }
        if(1 < player_count) {
            std::string c;
            if( cloak_enabled ) {
                c = pixel::to_string(", %6.2f / %6.2f", p2.center().x, p2.center().y);
            }
            sp2 = pixel::to_string(", S2 %4d (%4d pengs, %2d mines, %.1f s shield, %4.2f m/s)",
                p2.score(), p2.peng_inventory(), p2.mine_inventory(), p2.shield_time(), p2.velocity(), c.c_str());
        }
        if(2 < player_count) {
            std::string c;
            if( cloak_enabled ) {
                c = pixel::to_string(", %6.2f / %6.2f", p3.center().x, p3.center().y);
            }
            sp3 = pixel::to_string(", S3 %4d (%4d pengs, %2d mines, %.1f s shield, %4.2f m/s)",
                p3.score(), p3.peng_inventory(), p3.mine_inventory(), p3.shield_time(), p3.velocity(), c.c_str());
        }
        hud_text = pixel::make_text(tl_text, 0, vec4_text_color, text_height, "%s s, fps %4.2f, %s%s%s",
                        pixel::to_decstring(t1/1000, ',', 5).c_str(), // 1d limit
                        fps, sp1.c_str(), sp2.c_str(), sp3.c_str());
    }
    pixel::swap_pixel_fb(false);
    if( nullptr != hud_text ) {
        const int dx = ( pixel::fb_width - pixel::round_to_int((float)hud_text->width*hud_text->dest_sx) ) / 2;
        hud_text->draw(dx, 0);
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
    int window_width = 1920, window_height = 1080; // 16:9
    bool enable_vsync = true;
    int sun_gravity_scale_env = 200;    //  20 x 280 =  5600
    int sun_gravity_scale_ships = 200; // 200 x 280 = 56000
    bool use_subsys_primitives = true;
    #if defined(__EMSCRIPTEN__)
        window_width = 1024, window_height = 576; // 16:9
    #endif
    player_count = 2;
    {
        bool fps_set = false;
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-1p", argv[i]) ) {
                player_count = 1;
            } else if( 0 == strcmp("-3p", argv[i]) ) {
                player_count = 3;
            } else if( 0 == strcmp("-width", argv[i]) && i+1<argc) {
                window_width = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-height", argv[i]) && i+1<argc) {
                window_height = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-record", argv[i]) && i+1<argc) {
                record_bmpseq_basename = argv[i+1];
                ++i;
            } else if( 0 == strcmp("-fps", argv[i]) && i+1<argc) {
                fps_set = true;
                pixel::forced_fps = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-no_vsync", argv[i]) ) {
                enable_vsync = false;
            } else if( 0 == strcmp("-debug_gfx", argv[i]) ) {
                debug_gfx = true;
                show_ship_velo = true;
            } else if( 0 == strcmp("-show_velo", argv[i]) ) {
                show_ship_velo = true;
            } else if( 0 == strcmp("-asteroids", argv[i]) && i+1<argc) {
                asteroid_count = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-sung_env", argv[i]) && i+1<argc) {
                sun_gravity_scale_env = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-sung_ships", argv[i]) && i+1<argc) {
                sun_gravity_scale_ships = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-with_cloak", argv[i]) ) {
                cloak_enabled = true;
            } else if( 0 == strcmp("-soft_prim", argv[i]) ) {
                use_subsys_primitives = false;
            }
        }
        if( !fps_set && !use_subsys_primitives ) {
            pixel::forced_fps = 30;
        }
    }
    {
        const uint64_t elapsed_ms = pixel::getElapsedMillisecond();
        pixel::log_printf(elapsed_ms, "Usage %s -1p -width <int> -height <int> -record <bmp-files-basename> -fps <int> -no_vsync"
                                      " -debug_gfx -show_velo -asteroids <int> -sung_env <int> -sung_ships <int>\n", argv[0]);
        pixel::log_printf(elapsed_ms, "- win size %d x %d\n", window_width, window_height);
        pixel::log_printf(elapsed_ms, "- record %s\n", record_bmpseq_basename.size()==0 ? "disabled" : record_bmpseq_basename.c_str());
        pixel::log_printf(elapsed_ms, "- subsys_primitives %d\n", use_subsys_primitives);
        pixel::log_printf(elapsed_ms, "- enable_vsync %d\n", enable_vsync);
        pixel::log_printf(elapsed_ms, "- forced_fps %d\n", pixel::forced_fps);
        pixel::log_printf(elapsed_ms, "- debug_gfx %d\n", debug_gfx);
        pixel::log_printf(elapsed_ms, "- show_ship_velo %d\n", show_ship_velo);
        pixel::log_printf(elapsed_ms, "- players %d\n", player_count);
        pixel::log_printf(elapsed_ms, "- raster %d\n", raster);
        pixel::log_printf(elapsed_ms, "- asteroid_count %d\n", asteroid_count);
        pixel::log_printf(elapsed_ms, "- sun_gravity_scale_env %d -> %f [m/s^2]\n", sun_gravity_scale_env, sun_gravity * (float)sun_gravity_scale_env);
        pixel::log_printf(elapsed_ms, "- sun_gravity_scale_ships %d -> %f [m/s^2]\n", sun_gravity_scale_ships, sun_gravity * (float)sun_gravity_scale_ships);
        pixel::log_printf(elapsed_ms, "- cloak enabled %d\n", cloak_enabled);
    }

    {
        const float origin_norm[] = { 0.5f, 0.5f };
        if( !pixel::init_gfx_subsystem("spacewars", window_width, window_height, origin_norm, enable_vsync, use_subsys_primitives) ) {
            return 1;
        }
    }
    pixel::cart_coord.set_height(-space_height/2.0f, space_height/2.0f);

    sun = std::make_shared<star_t>(pixel::f2::point_t(0, 0), spaceship_height,
                                   sun_gravity * (float)sun_gravity_scale_env,
                                   sun_gravity * (float)sun_gravity_scale_ships);
    /*
    const float peng_diag = spaceship_height;
    for(int i = 0; i < 5; ++i){
        const float x = next_rnd() * pixel::cart_coord.width();
        const float y = next_rnd() * pixel::cart_coord.height();
        peng_t peng(&world_id, pixel::f2::point_t(x, y), peng_diag, 0.0f, 0.0f);
        pengs.emplace_back(peng);
        if(!peng.on_screen()){
            printf("peng %d is not on screen", i);
        }
    }
    */
    reset_asteroids(asteroid_count);

    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
}
