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

std::random_device rng;
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

bool debug_gfx = false;

class fragment_t : public pixel::f2::linestrip_t {
    public:
        constexpr static const float accel = 0.999f;
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
            // velocity *= accel;
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
        const pixel::f2::point_t p_v = p0 + ( p1 - p0 ) / 2.0f;
        const pixel::f2::vec_t v_v0 = ( p_v - pc ).normalize() * v;
        fragment_ref_t f = std::make_shared<fragment_t>(pc, v_v0, rot_v, true);
        f->p_list.push_back(pc);
        f->p_list.push_back(p0);
        f->p_list.push_back(p1);
        f->p_list.push_back(pc);
        dest.push_back(f);
        p0 = p1;
    }
}

fragment_ref_t make_asteroid(const pixel::f2::point_t& center, float height,
                             const float angle,
                             const float velocity,
                             const float jitter=1.0f/8.0f) noexcept
{
    const float max_height = 6.0f*height;
    const float min_height = 1.0f*height;
    height = std::max(min_height, std::min(height, 6*max_height));
    const float s = 1.0f - ( height - min_height ) / (max_height - min_height );
    const float rot_velocity = pixel::adeg_to_rad(5.0f) + s * pixel::adeg_to_rad(15.0f); // angle/s
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
    return lf;
}

bool hits_fragment(const pixel::f2::aabbox_t& box, const float velocity) noexcept {
    bool hit = false;
    std::vector<fragment_ref_t> new_asteroids;
    for(auto it = fragments.begin(); it != fragments.end(); ) {
        fragment_ref_t a = *it;
        if( box.intersects(a->box()) ) {
            hit = true;
            make_fragments(new_asteroids, a, a->velocity.length() + velocity, a->rotation_velocity*2.0f);
            it = fragments.erase(it);
        } else {
            ++it;
        }
    }
    fragments.insert(fragments.end(), new_asteroids.begin(), new_asteroids.end());
    return hit;
}

class peng_t {
public:
    constexpr static const pixel::f2::vec_t accel = pixel::f2::vec_t(-0.0001f, -0.0001f); // [m/s]
    pixel::f2::vec_t velo; // [m/s]
    pixel::f2::rect_t peng;

    peng_t(const pixel::f2::point_t& p0, const float diag, const float v, const float angle)
    : velo( pixel::f2::point_t::from_length_angle(v, angle) ),
      peng(p0 + pixel::f2::point_t(-diag/2, +diag/2), diag, diag, angle)
    { }
    peng_t(const pixel::f2::point_t& p0, const float diag, const pixel::f2::vec_t& v)
    : velo( v ),
      peng(p0 + pixel::f2::point_t(-diag/2, +diag/2), diag, diag, v.angle())
    { }

    bool tick(const float dt) noexcept {
        velo += accel;
        peng.move( velo * dt );
        peng.rotate(pixel::adeg_to_rad(180.0f) * dt);
        return !hits_fragment(peng.box(), velo.length()/4);
    }

    void draw() const noexcept {
        peng.draw(false);
    }

    void changeSpeed(float a){
        velo *= a;
    }

    bool on_screen(){
        return peng.on_screen();
    }
    bool intersection(const peng_t& o) const {
        return peng.intersects(o.peng);
    }
};

class spaceship_t : public pixel::f2::linestrip_t {
    public:
        constexpr static const float height = 0.10f; // [m]
        constexpr static const float vel_step = 0.1f; // [m/s]
        constexpr static const float vel_max = 2.0f + vel_step; // [m/s]
        constexpr static const float rot_step = 3.0f; // ang-degrees

        constexpr static const float peng_diag = 0.3f*height;
        constexpr static const float peng_velo_0 = vel_max / 2;
        constexpr static const int peng_inventory_max = 5000;

        pixel::f2::vec_t velocity; // [m/s]
        std::vector<peng_t> pengs;
        int peng_inventory;

        spaceship_t(const pixel::f2::point_t& center, const float angle) noexcept
        : linestrip_t(center, angle), velocity(), peng_inventory(peng_inventory_max)
        {}

        void peng() noexcept {
            if(peng_inventory > 0){
                pixel::f2::point_t p0 = p_list[0];
                pixel::f2::vec_t v_p = pixel::f2::vec_t::from_length_angle(velocity.length() + peng_velo_0, dir_angle);
                pengs.push_back( peng_t(p0, peng_diag, v_p ) );
                --peng_inventory;
            }
        }
        size_t peng_count() const noexcept { return pengs.size(); }

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
            bool hit = hits_fragment(box(), velocity.length());

            for(auto it = pengs.begin(); it != pengs.end(); ) {
                peng_t& p = *it;
                if(p.on_screen() && p.velo.length_sq() > 0){
                    if( p.tick(dt) ) {
                        ++it;
                        continue;
                    }
                }
                it = pengs.erase(it);
            }
            return !hit;
        }

        void draw() const noexcept override {
            linestrip_t::draw();
            for(auto it = pengs.begin(); it != pengs.end(); ++it) {
                (*it).draw();
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
spaceship_ref_t make_spaceship1( const pixel::f2::point_t& m, const float h=spaceship_t::height) noexcept
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

void reset_asteroids(int count) {
    fragments.clear();
    for(int i = 0; i < count; ++i) {
        const float height_h = spaceship_t::height*2.0f;
        const float height = height_h + height_h*next_rnd();
        const float angle = pixel::adeg_to_rad(next_rnd() * 360.0f);
        const float velocity = 0.1f + next_rnd() * 0.1f; // m/s
        pixel::f2::point_t p0(pixel::cart_coord.min_x()+(int)(pixel::cart_coord.width()*next_rnd()),
                              i%2 == 0 ? pixel::cart_coord.min_y()+height/2 : pixel::cart_coord.max_y()-height/2);
        fragment_ref_t asteroid1 = make_asteroid(p0, height,
                angle, velocity,  1.0f / ( 2.0f + 6.0f * next_rnd() ) ); // 1.0f/(2.0f+i) ); // 1.0f / ( 2+next_rnd() ) );
        fragments.push_back(asteroid1);
    }

}

int main(int argc, char *argv[])
{
    int win_width = 1920, win_height = 1080;
    std::string record_bmpseq_basename;
    bool enable_vsync = true;
    int forced_fps = 30;
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
        pixel::log_printf(elapsed_ms, "- forced_fps %d\n", forced_fps);
    }

    {
        const float origin_norm[] = { 0.5f, 0.5f };
        pixel::init_gfx_subsystem("spacewars", win_width, win_height, origin_norm, enable_vsync);
    }

    // const float bullet_height = 0.02f; // [m] .. diameter
    // const float ball_radius = ball_height/2.0f; // [m]
    const float ship_thickness = 0.07f; // [m]

    pixel::cart_coord.set_height(-field_height/2.0f, field_height/2.0f);

    const pixel::f2::point_t p0_ss1 = { pixel::cart_coord.min_x()+pixel::cart_coord.width()/4.0f, 0.0f };
    const pixel::f2::point_t p0_ss2 = { pixel::cart_coord.min_x()+pixel::cart_coord.width()*3.0f/4.0f, 0.0f };

    spaceship_ref_t ship_l;
    if( !one_player ) {
        ship_l = make_spaceship1(p0_ss1);
    }
    spaceship_ref_t ship_r = make_spaceship1(p0_ss2);

    reset_asteroids(5);
    {
        const uint64_t elapsed_ms = pixel::getElapsedMillisecond();
        if( debug_gfx ) {
            pixel::log_printf(elapsed_ms, "XX %s\n", pixel::cart_coord.toString().c_str());
        }
    }

    pixel::texture_ref hud_text;
    uint64_t frame_count_total = 0;

    uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]

    pixel::input_event_t event;
    while( !event.pressed_and_clr( pixel::input_event_type_t::WINDOW_CLOSE_REQ ) ) {
        pixel::handle_events(event);
        if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_RESIZED ) ) {
            pixel::cart_coord.set_height(-field_height/2.0f, field_height/2.0f);
        }

        // white background
        pixel::clear_pixel_fb(0, 0, 0, 255);

        const uint64_t t1 = pixel::getElapsedMillisecond(); // [ms]
        const float dt = (float)( t1 - t_last ) / 1000.0f; // [s]
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

        if( event.released_and_clr(pixel::input_event_type_t::RESET) ) {
            ship_r = make_spaceship1(p0_ss2);
            if( !one_player ) {
                ship_l = make_spaceship1(p0_ss1);
            }
            reset_asteroids(5);
        }
        if( nullptr != ship_r && event.has_any_p1() ) {
            if( event.pressed(pixel::input_event_type_t::P1_UP) ) {
                ship_r->velo_up(spaceship_t::vel_step);
            } else if( event.pressed(pixel::input_event_type_t::P1_LEFT) ){
                ship_r->rotate_adeg(spaceship_t::rot_step);
            } else if( event.pressed(pixel::input_event_type_t::P1_RIGHT) ){
                ship_r->rotate_adeg(-spaceship_t::rot_step);
            } else if( event.pressed_and_clr(pixel::input_event_type_t::P1_ACTION1) ) {
                ship_r->peng();
            }
        }

        for(auto it=fragments.begin(); it != fragments.end(); ) {
            if( !(*it)->tick(dt) ) {
                it = fragments.erase( it );
            } else {
                ++it;
            }
        }
        if( nullptr != ship_r && !ship_r->tick(dt) ) {
            make_fragments(fragments, ship_r, ship_r->velocity.length() + spaceship_t::vel_step, 0.003f);
            ship_r = nullptr;
        }

        if( 0 == fragments.size() ) {
            reset_asteroids(5);
        }

        pixel::set_pixel_color(255 /* r */, 255 /* g */, 255 /* b */, 255 /* a */);
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
        pixel::swap_gpu_buffer(forced_fps);
        if( record_bmpseq_basename.size() > 0 ) {
            std::string snap_fname(128, '\0');
            const int written = std::snprintf(&snap_fname[0], snap_fname.size(), "%s-%7.7" PRIu64 ".bmp", record_bmpseq_basename.c_str(), frame_count_total);
            snap_fname.resize(written);
            pixel::save_snapshot(snap_fname);
        }
        ++frame_count_total;
    }
    printf("Exit");
    exit(0);
}
