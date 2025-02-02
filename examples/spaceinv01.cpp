/*
 * Author: Hermann Kai Göthel, Svenson Han Göthel und Sven Göthel
 * Copyright (c) 2025 Gothel Software e.K.
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
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <pixel/pixel3f.hpp>
#include <pixel/pixel4f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"
#include "pixel/audio.hpp"

#include <cinttypes>
#include <cstdio>
#include <cmath>
#include <vector>

using namespace jau;
using namespace jau::si_f32_literals;

static pixel::input_event_t event;

/**
 * Space Invaders (1978 by Taito)
 *
 * - Playfield Area
 *   - Origin bottom-left (orig hardware coordinate system)
 *   - Screen  224 x 256
 *   - Field   204 x 216  (w: min-max, h: base -> mothership)
 *   - Base    [16..185]  base horizontal range
 *   - Saucer  x=[9..192], y=206
 *   - Bunker  1: 32/50, 2: 77/50, 3: 122/50, 4: 167/50
 * - Object Dimension
 *   - Alien-1  12 x   8
 *   - Alien-2  11 x   8
 *   - Alien-3   8 x   8
 *   - Alien-X  13 x   8
 *   - Alien-S   3 x   7  (shot sets 2x3)
 *   - Red-X     8 x   8
 *   - Base     13 x   8  (incl. explosions 1-3)
 *   - peng      1 x   4
 *   - Bunker   22 x  16  (aspect 1.375)
 *   - Saucer   16 x   8  (mothership at y=206)
 *   - Peng      1 x   4
 * - Timings
 *   - Saucer appears every ~25.6s
 * - Velocity
 *   - Rendering 1 frame is done at 60Hz (fps) after vsync
 *   - Alien   2*60/N (2/N per frame with N = alient count, see below)
 *   - Saucer  183/4  (183 in 4s, 183 saucer distance)
 *   - Base    1*60   (1 per frame, i.e. 60 in 1s)
 *   -
 *
 * Orig hardware vs this code:
 * - Alien velovity
 *   - Original hardware renders only 1 alien per frame
 *   - Single alien moves at max-speed 2 pixel/frame @ 60fps -> 120 pixel/s
 *   - N alien move at 2*60/N, i.e. 2.18 pixel/s
 *   - This `emulation` renders all at once, but paused 1/60*N for N aliens,
 *     giving proper group movement, removes artifacts and simplifies code
*/
constexpr static float screen_width = 224.0f;
constexpr static float screen_height = 256.0f;
constexpr static float screen_width_pct = screen_width / screen_height;
constexpr float scrn2cartx(const float v) noexcept { return v - screen_width/2.0f; }
constexpr float scrn2carty(const float v) noexcept { return v - screen_height/2.0f; }
constexpr static float field_width = 204.0f;
constexpr static float field_height = 216.0f;
static constexpr float field_yshift = 0;
static const pixel::f2::aabbox_t field_box( { -field_width/2.0, -field_height/2.0 +field_yshift}, { field_width/2.0, field_height/2.0 +field_yshift} ); // gcc bug 93413 (constexpr, solved in gcc 13)
static const pixel::f2::vec_t field_tl( -field_width/2.0, field_height/2.0 +field_yshift );

constexpr static float base_width = 13.0f;
constexpr static float base_height = 8.0f;

// constexpr static float bunk_width = 22.0f;
// constexpr static float bunk_height = 16.0f;

constexpr static float base_velo_h = 1.0f*60.0f; // 1 pixel per frame, half of max single alien speed
constexpr static float base_peng_velo = field_height / 1.0f; // approx 1s for field
constexpr static int base_peng_inventory_max = 1;

constexpr static int aliens_per_row = 11;
constexpr static int aliens_rows = 5;
constexpr static int aliens_total_default = aliens_rows * aliens_per_row;

constexpr static float alien_saucer_period = 25.6f;
constexpr static pixel::f2::vec_t saucer_lpos(scrn2cartx(9), scrn2carty(206));
constexpr static pixel::f2::vec_t saucer_rpos(scrn2cartx(192), scrn2carty(206));
constexpr static pixel::f2::vec_t saucer_velo( (saucer_rpos.x - saucer_lpos.x)/4.0f, 0.0f); // ([1/s], [1]) 183 in 4s

/// horizontal: [1/s], single-alien full-speed 2 pixel/frame @ 60fps -> 120 pixel/s
///             Due to orig hardware limitation, must be divided by current alien_count,
///             hence slowest speed is 2*60/55=2.18 pixel/s.
/// vertical:   only 8 pixel per step-down
constexpr static pixel::f2::vec_t alien_max_velo( 2.0f*60.0f, -8.0f); // ([1/s], [1])
constexpr static float alien_explosion_d = 16.0f/60.f; // [s], 16 frames @ 60fps

constexpr static pixel::f2::point_t bunk1_bl( scrn2cartx( 32), scrn2carty(50) );
constexpr static pixel::f2::point_t bunk2_bl( scrn2cartx( 77), scrn2carty(50) );
constexpr static pixel::f2::point_t bunk3_bl( scrn2cartx(122), scrn2carty(50) );
constexpr static pixel::f2::point_t bunk4_bl( scrn2cartx(167), scrn2carty(50) );

static const pixel::f2::aabbox_t base_box( { scrn2cartx(16), -field_height/2.0f +base_height*2.0f }, { scrn2cartx(185)+base_width, field_height/2.0f } );

//
//
//
static size_t aliens_total = aliens_total_default;
static float alien_velo_amp = 1.0f;

constexpr static int base_id = 1;
constexpr static int alien_id = 2;

static const uint8_t rgba_white[/*4*/] = { 255, 255, 255, 255 };
static const uint8_t rgba_yellow[/*4*/] = { 255, 255, 0, 255 };
// static const uint8_t rgba_red[/*4*/] = { 255, 0, 0, 255 };
static const uint8_t rgba_green[/*4*/] = { 0, 255, 0, 255 };
// static const uint8_t rgba_blue[/*4*/] = { 0, 0, 255, 255 };
static const float text_lum = 0.75f;
static const pixel::f4::vec_t vec4_text_color(text_lum, text_lum, text_lum, 1.0f);
static bool debug_gfx = false;

static pixel::bitmap_ref bmp_bunk;
static pixel::texture_ref all_images;

// row-1
static std::vector<pixel::texture_ref> tex_alien1; // 2x
static std::vector<pixel::texture_ref> tex_alienX, tex_redX;

// row-2
static std::vector<pixel::texture_ref> tex_alien2; // 2x

// row-3
static std::vector<pixel::texture_ref> tex_alien3; // 2x
static std::vector<pixel::texture_ref> tex_ashot1, tex_ashot2; // 3x

// row-4
static std::vector<pixel::texture_ref> tex_base;
static std::vector<pixel::texture_ref> tex_baseX; // 3x

// row-5
static pixel::texture_ref tex_bunk[] { nullptr, nullptr, nullptr, nullptr };
static std::vector<pixel::texture_ref> tex_peng;

// row-6
static std::vector<pixel::texture_ref> tex_saucer;
static int level = 1;

using namespace jau::audio;

static std::vector<audio_sample_ref> audio_aliens;
static audio_sample_ref audio_saucer, audio_peng, audio_alienX, audio_baseX;

static bool load_samples() {
    audio_aliens.clear();
    if( jau::audio::is_audio_subsystem_initialized() ) {
        audio_aliens.push_back( std::make_shared<jau::audio::audio_sample_t>("spaceinv/alien1.wav", false, MIX_MAX_VOLUME) );
        audio_aliens.push_back( std::make_shared<jau::audio::audio_sample_t>("spaceinv/alien2.wav", false, MIX_MAX_VOLUME) );
        audio_aliens.push_back( std::make_shared<jau::audio::audio_sample_t>("spaceinv/alien3.wav", false, MIX_MAX_VOLUME) );
        audio_aliens.push_back( std::make_shared<jau::audio::audio_sample_t>("spaceinv/alien4.wav", false, MIX_MAX_VOLUME) );
        audio_saucer = std::make_shared<jau::audio::audio_sample_t>("spaceinv/alienM.ogg", false, MIX_MAX_VOLUME/4);
        audio_peng = std::make_shared<jau::audio::audio_sample_t>("spaceinv/peng.wav", false, MIX_MAX_VOLUME/4);
        audio_alienX = std::make_shared<jau::audio::audio_sample_t>("spaceinv/alienX.wav", false, MIX_MAX_VOLUME/4);
        audio_baseX = std::make_shared<jau::audio::audio_sample_t>("spaceinv/baseX.wav", false, MIX_MAX_VOLUME);
        return true;
    } else {
        audio_aliens.push_back( std::make_shared<jau::audio::audio_sample_t>() );
        audio_saucer = std::make_shared<jau::audio::audio_sample_t>();
        audio_peng = std::make_shared<jau::audio::audio_sample_t>();
        audio_alienX = std::make_shared<jau::audio::audio_sample_t>();
        audio_baseX = std::make_shared<jau::audio::audio_sample_t>();
        return false;
    }
}

static bool load_textures() {
    {
        pixel::bitmap_ref empty = std::make_shared<pixel::bitmap_t>(64, 64);
        log_printf(0, "XX empty: %s\n", empty->toString().c_str());
    }
    bmp_bunk = std::make_shared<pixel::bitmap_t>("spaceinv/spaceinv-bunk.png");
    log_printf(0, "XX bmp bunk: %s\n", bmp_bunk->toString().c_str());

    int y_off=0;
    all_images = std::make_shared<pixel::texture_t>("spaceinv/spaceinv-sprites.png");
    if( !all_images->handle() ) {
        return false;
    }
    // row-1
    pixel::add_sub_textures(tex_alien1, all_images, 0*12,  y_off, 12, 8, { {  0*12, 0 }, {  1*12, 0 }, });
    tex_alienX.push_back( pixel::add_sub_texture(all_images, 2*12,  y_off, 13, 8) );
    tex_redX.push_back( pixel::add_sub_texture(all_images, 24+13, y_off,  8, 8) );
    y_off+= 8;

    // row-2
    pixel::add_sub_textures(tex_alien2, all_images, 0*11, y_off, 11, 8, { {  0*11, 0 }, {  1*11, 0 }, });
    y_off+= 8;

    // row-3
    pixel::add_sub_textures(tex_alien3, all_images, 0* 8, y_off,  8, 8, { {  0* 8, 0 }, {  1* 8, 0 }, });
    pixel::add_sub_textures(tex_ashot1, all_images, 2* 8, y_off,  3, 7, { {  0* 3, 0 }, {  1* 3, 0 }, {  2* 3, 0 } });
    pixel::add_sub_textures(tex_ashot2, all_images, 16+9, y_off,  3, 7, { {  0* 3, 0 }, {  1* 3, 0 }, {  2* 3, 0 } });
    y_off+= 8;

    // row-4
    tex_base.push_back( pixel::add_sub_texture(all_images,   0*13, y_off, 13, 8) );
    pixel::add_sub_textures(tex_baseX, all_images,  1*13, y_off, 13, 8, { {  0* 3, 0 }, {  1* 3, 0 }, {  2* 3, 0 } });
    y_off+= 8;

    // row-5
    tex_bunk[0] = std::make_shared<pixel::texture_t>(bmp_bunk); // 22x16
    tex_bunk[1] = std::make_shared<pixel::texture_t>(bmp_bunk);
    tex_bunk[2] = std::make_shared<pixel::texture_t>(bmp_bunk);
    tex_bunk[3] = std::make_shared<pixel::texture_t>(bmp_bunk);
    tex_peng.push_back( pixel::add_sub_texture(all_images,  22, y_off,  1, 4) );
    y_off+=16;

    // row-6
    tex_saucer.push_back( pixel::add_sub_texture(all_images, 0, y_off, 16, 7) );

    // row-1
    log_printf(0, "XX alien1: %d textures\n", tex_alien1.size());
    for(const pixel::texture_ref& t : tex_alien1) {
        log_printf(0, "XX alien1: %s\n", t->toString().c_str());
    }
    log_printf(0, "XX alienX: %s\n", tex_alienX[0]->toString().c_str());
    log_printf(0, "XX redX:   %s\n", tex_redX[0]->toString().c_str());

    // row-2
    log_printf(0, "XX alien2: %d textures\n", tex_alien2.size());
    for(const pixel::texture_ref& t : tex_alien2) {
        log_printf(0, "XX alien2: %s\n", t->toString().c_str());
    }

    // row-3
    log_printf(0, "XX alien3: %d textures\n", tex_alien3.size());
    for(const pixel::texture_ref& t : tex_alien3) {
        log_printf(0, "XX alien3: %s\n", t->toString().c_str());
    }
    log_printf(0, "XX ashot1: %d textures\n", tex_ashot1.size());
    for(const pixel::texture_ref& t : tex_ashot1) {
        log_printf(0, "XX ashot1: %s\n", t->toString().c_str());
    }
    log_printf(0, "XX ashot2: %d textures\n", tex_ashot2.size());
    for(const pixel::texture_ref& t : tex_ashot2) {
        log_printf(0, "XX ashot2: %s\n", t->toString().c_str());
    }

    // row-4
    log_printf(0, "XX base: %s\n", tex_base[0]->toString().c_str());
    log_printf(0, "XX baseX: %d textures\n", tex_baseX.size());
    for(const pixel::texture_ref& t : tex_baseX) {
        log_printf(0, "XX baseX: %s\n", t->toString().c_str());
    }

    // row-5
    log_printf(0, "XX bunk: %s\n", tex_bunk[0]->toString().c_str());
    log_printf(0, "XX peng: %s\n", tex_peng[0]->toString().c_str());

    // row-6
    log_printf(0, "XX saucer: %s\n", tex_saucer[0]->toString().c_str());
    return true;
}

class gobject_t {
  public:
    virtual ~gobject_t() noexcept = default;

    /// Returns bottom-left position
    virtual pixel::f2::point_t pos() const noexcept = 0;

    /// returns the AA bounding box
    virtual pixel::f2::aabbox_t box() const noexcept = 0;

    virtual bool tick(const float dt) noexcept = 0;

    virtual void draw() noexcept = 0;

    virtual bool on_screen() const noexcept = 0;

    virtual bool intersection(const pixel::f2::aabbox_t& o) const noexcept = 0;
};

class sprite_t : public gobject_t {
  private:
    pixel::animtex_t m_atex;
    float m_duration;
    pixel::f2::point_t m_bl;
    pixel::f2::vec_t m_dim;

  public:
    sprite_t(const pixel::f2::point_t& bl, pixel::animtex_t atex, float duration) noexcept
    : m_atex(std::move(atex)), m_duration(duration),
      m_bl( bl ), m_dim((float)m_atex.width(), (float)m_atex.height())
    { }

    /// Returns bottom-left position
    pixel::f2::point_t pos() const noexcept override { return m_bl; }

    pixel::f2::aabbox_t box() const noexcept override {
        return pixel::f2::aabbox_t().resize(m_bl).resize(m_bl.x + m_dim.x, m_bl.y + m_dim.y);
    }

    bool tick(const float dt) noexcept override {
        m_atex.tick(dt);
        if( 0 < m_duration ) {
            m_duration -= dt;
            if( 0 >= m_duration ) {
                m_atex.clear();
                return false;
            }
        }
        return true;
    }

    void draw() noexcept override {
        m_atex.draw(m_bl.x, m_bl.y+m_dim.y);
    }

    bool on_screen() const noexcept override {
        return box().inside(field_box);
    }

    bool intersection(const pixel::f2::aabbox_t& o) const noexcept override {
        return box().intersects(o);
    }

    pixel::animtex_t& atex() { return m_atex; }
};
std::vector<sprite_t> extra_sprites;

class alient_t : public gobject_t {
  private:
    int m_value;
    pixel::animtex_t m_atex;
    pixel::f2::point_t m_bl;
    pixel::f2::vec_t m_dim;
    float m_killanim_d = 0;
    bool m_killed = false;

  public:

    alient_t(int value, const pixel::f2::point_t& bl, pixel::animtex_t atex_alien) noexcept
    : m_value(value), m_atex(std::move(atex_alien)),
      m_bl( bl ), m_dim((float)m_atex.width(), (float)m_atex.height())
    { }

    /// Returns bottom-left position
    pixel::f2::point_t pos() const noexcept override { return m_bl; }

    pixel::f2::aabbox_t box() const noexcept override {
        return pixel::f2::aabbox_t().resize(m_bl).resize(m_bl.x + m_dim.x, m_bl.y + m_dim.y);
    }

    bool tick(const float dt) noexcept override {
        if( 0 < m_killanim_d ) {
            m_killanim_d -= dt;
            if( 0 >= m_killanim_d ) {
                audio_alienX->stop();
                m_atex.clear();
                return false;
            }
        }
        return true;
    }

    void draw() noexcept override {
        m_atex.draw(m_bl.x, m_bl.y+m_dim.y);
        if( debug_gfx ) {
            pixel::set_pixel_color(rgba_yellow);
            pixel::f2::rect_t r({m_bl.x, m_bl.y+m_dim.y}, m_dim.x, m_dim.y);
            r.draw();
        }
    }

    bool on_screen() const noexcept override {
        return box().inside(field_box);
    }

    bool intersection(const pixel::f2::aabbox_t& o) const noexcept override {
        return box().intersects(o);
    }

    constexpr int value() const noexcept { return m_value; }

    void step(const pixel::f2::vec_t& d) noexcept {
        m_atex.next();
        m_bl += d;
    }

    void notify_killed() noexcept {
        m_atex = pixel::animtex_t("AlienX", 1.0f, tex_alienX);
        m_killanim_d = alien_explosion_d;
        audio_alienX->play();
        m_killed = true;
    }
    bool is_killed() noexcept { return m_killed; }

    pixel::animtex_t& atex() { return m_atex; }
};
typedef std::shared_ptr<alient_t> alien_ref;

class alien_group_t : public gobject_t {
  private:
    constexpr static float m_sec_step_delay = 1.0f/60.0f; // orig 1 alien per frame moves
    pixel::f2::aabbox_t m_box;
    pixel::f2::vec_t m_alien_velo = alien_max_velo;
    pixel::f2::vec_t m_saucer_velo;
    float m_step_delay_left;
    float m_saucer_delay_left;
    size_t m_current_idx = 0;
    bool m_pause = false;
    std::vector<alien_ref> m_actives;
    std::vector<alien_ref> m_killed;
    alien_ref m_saucer = nullptr;

  public:
    alien_group_t() = default;
    alien_group_t(const alien_group_t&) = default;
    alien_group_t(alien_group_t&&)      = default;
    alien_group_t& operator=(const alien_group_t&) = default;
    alien_group_t& operator=(alien_group_t&&)      = default;

    /// Returns bottom-left position
    pixel::f2::point_t pos() const noexcept override { return m_box.bl; }

    pixel::f2::aabbox_t box() const noexcept override { return m_box; }

    bool on_screen() const noexcept override {
        return m_box.inside(field_box);
    }

    bool intersection(const pixel::f2::aabbox_t& o) const noexcept override {
        return m_box.intersects(o);
    }

    void reset() {
        const float cell_height = (float)tex_alien1[0]->height;
        const float cell_width = (float)tex_alien1[0]->width;
        m_actives.clear();
        m_actives.reserve(aliens_total);
        m_current_idx = 0;
        m_box.reset();
        pixel::f2::point_t bl = field_box.bl;
        bl.y = 2.0f * cell_height * aliens_rows - 2.5f * cell_height;
        for(int y=0; y<1; ++y) {
            // bl.x = field_box.bl.x + float( tex_alien1[0]->width - tex_alien3[0]->width )/2.0f;
            bl.x = field_box.bl.x + 1.0f;
            for(int x=0; x<aliens_per_row && m_actives.size()<aliens_total; ++x) {
                m_actives.emplace_back(std::make_shared<alient_t>(30, bl, pixel::animtex_t("Alien3", m_sec_step_delay, tex_alien3)));
                m_box.resize(m_actives[m_actives.size()-1]->box());
                bl.x += cell_width * 1.5f;
            }
            bl.y -=  2.0f * cell_height;
        }
        for(int y=0; y<2; ++y) {
            bl.x = field_box.bl.x;
            for(int x=0; x<aliens_per_row && m_actives.size()<aliens_total; ++x) {
                m_actives.emplace_back(std::make_shared<alient_t>(20, bl, pixel::animtex_t("Alien2", m_sec_step_delay, tex_alien2)));
                m_box.resize(m_actives[m_actives.size()-1]->box());
                bl.x += cell_width * 1.5f;
            }
            bl.y -=  2.0f * cell_height;
        }
        for(int y=0; y<2; ++y) {
            bl.x = field_box.bl.x;
            for(int x=0; x<aliens_per_row && m_actives.size()<aliens_total; ++x) {
                m_actives.emplace_back(std::make_shared<alient_t>(10, bl, pixel::animtex_t("Alien1", m_sec_step_delay, tex_alien1)));
                bl.x += cell_width * 1.5f;
            }
            bl.y -=  2.0f * cell_height;
        }
        m_saucer = nullptr;
        m_step_delay_left = m_sec_step_delay * float(active_count());
        m_saucer_delay_left = alien_saucer_period;
    }

    size_t active_count() const noexcept { return m_actives.size(); }

    alien_ref get_random() const noexcept {
        if( 0 == active_count() ) {
            return nullptr;
        }
        if( 1 == active_count() ) {
            return m_actives[0];
        }
        return m_actives[ next_rnd((size_t)0, m_actives.size()-1) ];
    }

    /// return the hit alien value or zero if non hit
    int check_hit(const pixel::f2::aabbox_t& b) noexcept {
        for(auto it = m_actives.begin(); it != m_actives.end(); ) {
            const alien_ref& a = *it;
            if( !a->box().intersects(b) ) {
                ++it;
            } else {
                a->notify_killed();
                m_killed.push_back(a);
                it = m_actives.erase(it);
                return a->value();
            }
        }
        if( m_saucer ) {
            if( m_saucer->box().intersects(b) ) {
                audio_saucer->stop();
                m_saucer->notify_killed();
                return m_saucer->value();
            }
        }
        return 0;
    }

    void set_pause(bool v) noexcept { m_pause = v; }

    bool tick(const float /*dt*/) noexcept override {
        static int sound_idx = 0;
        static audio_sample_ref audio_sample = nullptr;
        const float avg_fd = float( pixel::gpu_avg_framedur().to_double() );

        if( m_pause ) {
            return true;
        }
        if( m_saucer ) {
            pixel::f2::vec_t dxy = avg_fd * m_saucer_velo;
            if( (m_alien_velo.x > 0 && m_saucer->pos().x > saucer_rpos.x+1) ||
                (m_alien_velo.x < 0 && m_saucer->pos().x < saucer_lpos.x-1) ) {
                // exit
                m_saucer = nullptr;
                audio_saucer->stop();
            } else {
                if( m_saucer->is_killed() ) {
                    m_saucer->tick(avg_fd);
                } else {
                    m_saucer->step(dxy);
                }
            }
        }
        if( m_killed.size() > 0 ) {
            for(auto it = m_killed.begin(); it != m_killed.end(); ) {
                if( (*it)->tick(avg_fd) ) {
                    ++it;
                } else {
                    it = m_killed.erase(it);
                }
            }
            return true;
        }
        m_saucer_delay_left -= avg_fd;
        if( m_saucer_delay_left <= 0 ) {
            if( !m_saucer ) {
                const uint8_t lr = next_rnd<uint8_t>(0, 1);
                m_saucer = std::make_shared<alient_t>(90, lr ? saucer_rpos : saucer_lpos, pixel::animtex_t("Saucer", 1.0f, tex_saucer));
                m_saucer_velo = { saucer_velo.x * ( lr ? -1.0f : +1.0f), 0 };
                audio_saucer->play(1);
                // pixel::log_printf("XX saucer: lr %u, velo %s, bl %s\n", lr, m_saucer_velo.toString().c_str(), m_saucer->pos().toString().c_str());
            }
            m_saucer_delay_left = alien_saucer_period;
        }
        m_step_delay_left -= avg_fd;
        if( m_step_delay_left > 0 ) {
            return true;
        }
        m_step_delay_left = m_sec_step_delay * float(active_count());
        {
            audio_sample = audio_aliens[sound_idx];
            sound_idx = ( sound_idx + 1 ) % int(audio_aliens.size());
            audio_sample->play(1);
        }
        {
            pixel::f2::vec_t dxy = { alien_velo_amp * avg_fd * m_alien_velo.x, 0 };

            constexpr float lx_adjust = 1;
            if( (m_alien_velo.x > 0 && m_box.tr.x > field_box.tr.x) ||
                (m_alien_velo.x < 0 && m_box.bl.x < field_box.bl.x - lx_adjust) ) {
                m_alien_velo.x *= -1;
                dxy.x *= -1;
                dxy.y = m_alien_velo.y;
            }
            for(const alien_ref& a : m_actives){
                a->step(dxy);
            }
        }
        return true;
    }

    void draw() noexcept override {
        m_box.reset();
        if( m_saucer ) {
            m_saucer->draw();
        }
        for(const alien_ref& a : m_actives){
            a->draw();
            m_box.resize(a->box());
        }
        if( debug_gfx ) {
            pixel::set_pixel_color(rgba_white);
            pixel::f2::rect_t r({m_box.bl.x, m_box.bl.y+m_box.height()}, m_box.width(), m_box.height());
            r.draw();
        }
        for(const alien_ref& a : m_killed){
            a->draw();
        }
    }
};
alien_group_t alien_group;

class bunker_t : public gobject_t {
  private:
    size_t idx;
    pixel::f2::vec_t m_dim;
    pixel::f2::point_t m_bl;
    pixel::bitmap_ref m_bunk;

    void put_bunk(const pixel::f2::aabbox_t& box, uint32_t abgr) noexcept {
        if(!m_bunk->pixels() || 0 == m_bunk->width || 0 == m_bunk->height ) {
            return;
        }
        const uint32_t width = m_bunk->width;
        const uint32_t height = m_bunk->height;
        const uint32_t y1 = std::max<uint32_t>(0, floor_to_uint32(box.bl.y));
        const uint32_t y2 = std::min<uint32_t>(height, ceil_to_uint32(box.tr.y));
        for(uint32_t y=y1; y<y2; ++y) {
            const uint32_t o = next_rnd((uint32_t)0, (uint32_t)4);
            const uint32_t x1 = std::max<uint32_t>(0, floor_to_uint32(box.bl.x)+1-o);
            const uint32_t x2 = std::min<uint32_t>(width, ceil_to_uint32(box.tr.x)-1+o);
            for(uint32_t x=x1; x<x2; ++x) {
                uint32_t * const target_pixel = std::bit_cast<uint32_t *>(m_bunk->pixels() + static_cast<size_t>((m_bunk->height - y - 1) * m_bunk->stride) + static_cast<size_t>(x * m_bunk->bpp));
                *target_pixel = abgr;
            }
        }
    }
  public:
    bunker_t(size_t idx_, const pixel::f2::point_t& bl) noexcept
    : idx(idx_), m_dim((float)tex_bunk[idx]->width, (float)tex_bunk[idx]->height),
      m_bl(bl), m_bunk(bmp_bunk->clone())
    {
        tex_bunk[idx]->update(m_bunk);
    }

    /// Returns bottom-left position
    pixel::f2::point_t pos() const noexcept override { return m_bl; }

    pixel::f2::aabbox_t box() const noexcept override {
        return pixel::f2::aabbox_t().resize(m_bl).resize(m_bl.x + m_dim.x, m_bl.y + m_dim.y);
    }

    bool on_screen() const noexcept override {
        return box().inside(field_box);
    }

    bool intersection(const pixel::f2::aabbox_t& o) const noexcept override {
        return box().intersects(o);
    }

    bool tick(const float /*dt*/) noexcept override { return true; }

    bool hit(pixel::f2::aabbox_t& hitbox, pixel::f2::vec_t amp={0, 0} ) {
        const uint32_t clrpix = 0x00000000;
        const pixel::f2::aabbox_t mybox = box();
        pixel::f2::aabbox_t ibox = hitbox.intersection(mybox);
        if( false ) {
            printf("XXX bunk.hit:\n");
            printf("XXX bunk.hit: bunk   %s\n", mybox.toString().c_str());
            printf("XXX bunk.hit: hit    %s\n", hitbox.toString().c_str());
            printf("XXX bunk.hit: ibox1.0 %s\n", ibox.toString().c_str());
        }
        if( ibox.width() == 0 ) {
            // printf("XXX bunk.hit: ibox empty -> false\n");
            return false;
        }
        ibox.bl -= mybox.bl;
        ibox.tr -= mybox.bl;
        // printf("XXX bunk.hit: ibox1.1 %s\n", ibox.toString().c_str());
        ibox.bl.x = std::max(0.0f, ibox.bl.x - ibox.width() * amp.x);
        ibox.bl.y = std::max(0.0f, ibox.bl.y - ibox.height() * amp.y);
        ibox.tr.x += ibox.width() * amp.x;
        ibox.tr.y += ibox.height() * amp.y;
        // printf("XXX bunk.hit: ibox1.2 %s, amp %s\n", ibox.toString().c_str(), amp.toString().c_str());
        if( m_bunk->equals(ibox, clrpix) ) {
            // printf("XXX bunk.hit: clear -> false\n");
            return false;
        }
        put_bunk(ibox, clrpix);
        tex_bunk[idx]->update(m_bunk);
        return true;
    }
    void draw() noexcept override {
        tex_bunk[idx]->draw(m_bl.x, m_bl.y + m_dim.y);
    }
};
std::vector<bunker_t> bunks;

struct hit_result{
    bool hit;
    bool live;
};

class peng_t : public gobject_t {
  private:
    pixel::animtex_t m_atex;
    pixel::f2::point_t m_bl;
    pixel::f2::vec_t m_dim;
    pixel::f2::vec_t m_velo; // [m/s]
    int m_alien_hit_value;

  public:
    constexpr static float anim_period = 0.03f; // 0.025f;
    int m_owner;

  private:
    bool check_alien_hit() {
        if( m_owner == alien_id ) {
            return false;
        }
        m_alien_hit_value = alien_group.check_hit( box() );
        return 0 != m_alien_hit_value;
    }

    bool check_bunker_hit() {
        pixel::f2::aabbox_t b = box();
        for(auto it = bunks.begin(); it != bunks.end(); ) {
            bunker_t& bunk = *it;
            if( bunk.box().intersects(b) &&
                bunk.hit(b, m_owner == alien_id ? pixel::f2::vec_t{ 0.6f, 2.5f } : pixel::f2::vec_t{ 0.6f, 0.6f }) )
            {
                return true;
            } else {
                ++it;
            }
        }
        return false;
    }

  public:
    peng_t(const pixel::f2::point_t& bl, const pixel::f2::vec_t& v, const int owner, pixel::animtex_t atex) noexcept
    : m_atex(std::move(atex)), m_bl( bl ),
      m_dim((float)m_atex.width(), (float)m_atex.height()), m_velo( v ),
      m_alien_hit_value(0), m_owner(owner)
    { }

    /// Returns bottom-left position
    pixel::f2::point_t pos() const noexcept override { return m_bl; }

    pixel::f2::aabbox_t box() const noexcept override {
        return pixel::f2::aabbox_t().resize(m_bl).resize(m_bl.x + m_dim.x, m_bl.y + m_dim.y);
    }

    bool on_screen() const noexcept override {
        return box().inside(field_box);
    }

    bool intersection(const pixel::f2::aabbox_t& o) const noexcept override {
        return box().intersects(o);
    }

    bool tick(const float /*dt*/) noexcept override {
        const float avg_fd = float( pixel::gpu_avg_framedur().to_double() );
        m_atex.tick(avg_fd);
        if( !box().inside(field_box) ) {
            if( m_owner == base_id ) {
                pixel::f2::point_t bl = {m_bl.x + m_dim.x/2.0f - float(tex_redX[0]->width)/2.0f, m_bl.y };
                extra_sprites.emplace_back( bl, pixel::animtex_t("peng", 1.0f, tex_redX), 0.5f );
            }
            return false;
        }
        if(!m_velo.is_zero()){
            const pixel::f2::vec_t dxy = m_velo * avg_fd;
            m_bl += dxy;
            if( false && m_owner == base_id ) {
                log_printf("XX: peng: v %s, dt %f, dxy %s\n", m_velo.toString().c_str(), avg_fd, dxy.toString().c_str());
            }
        }
        return !check_alien_hit() && !check_bunker_hit();
    }

    void draw() noexcept override {
        m_atex.draw(m_bl.x, m_bl.y + m_dim.y);
    }

    constexpr int alien_hit_value() const { return m_alien_hit_value; }

};
std::vector<peng_t> pengs;

static void reset_items() {
    extra_sprites.clear();
    bunks.clear();
    bunks.emplace_back(0, bunk1_bl);
    bunks.emplace_back(1, bunk2_bl);
    bunks.emplace_back(2, bunk3_bl);
    bunks.emplace_back(3, bunk4_bl);
    alien_group.reset();
    pengs.clear();
}

class base_t : public gobject_t {
    public:
        constexpr static const float height = base_height; // [m]

    private:
        pixel::animtex_t m_atex;
        pixel::f2::point_t m_bl;
        pixel::f2::vec_t m_dim;
        bool m_killed;

        bool check_hit() {
            if( m_killed ) {
                return false;
            }
            pixel::f2::aabbox_t b = box();
            for(auto it = pengs.begin(); it != pengs.end(); ) {
                peng_t& a = *it;
                if( a.m_owner == base_id || !a.box().intersects(b) ) {
                    ++it;
                } else {
                    it = pengs.erase(it);
                    m_atex = pixel::animtex_t("pengX", 0.3f, tex_baseX);
                    audio_baseX->play();
                    m_killed = true;
                    return true;
                }
            }
            return false;
        }
    public:
        int peng_inventory;

        base_t(const pixel::f2::point_t& bl) noexcept
        : m_atex(pixel::animtex_t("peng", 1.0f, tex_base)), m_bl(bl),
          m_dim((float)tex_base[0]->width, (float)tex_base[0]->height),
          m_killed(false),
          peng_inventory(base_peng_inventory_max)
        {}

        /// Returns bottom-left position
        pixel::f2::point_t pos() const noexcept override { return m_bl; }

        pixel::f2::aabbox_t box() const noexcept override {
            return pixel::f2::aabbox_t().resize(m_bl).resize(m_bl.x + m_dim.x, m_bl.y + m_dim.y);
        }

        bool on_screen() const noexcept override {
            return box().inside(field_box);
        }

        bool intersection(const pixel::f2::aabbox_t& o) const noexcept override {
            return box().intersects(o);
        }

        void reset(const pixel::f2::point_t& bl){
            peng_inventory = base_peng_inventory_max;
            m_bl = bl;
        }

        constexpr bool is_killed() const noexcept { return m_killed; }

        bool peng() noexcept {
            if(peng_inventory > 0){
                // adjust start posision to geometric base model
                constexpr float x_adjust = 0.5f;
                pixel::f2::point_t bl = {m_bl.x + m_dim.x/2.0f + x_adjust - float(tex_peng[0]->width), m_bl.y };
                pixel::f2::vec_t v_p = pixel::f2::vec_t::from_length_angle(base_peng_velo, 90_deg);
                pengs.emplace_back(bl, v_p, base_id, pixel::animtex_t("peng", peng_t::anim_period, tex_peng));
                --peng_inventory;
                audio_peng->play();
                return true;
            }
            return false;
        }

        void move(const pixel::f2::point_t& d) noexcept {
            m_bl += d;
            if( !box().inside(base_box) ) {
                // pixel::log_printf(0, "XX %s -> %s, %s\n", d.toString().c_str(), box().toString().c_str(), base_box.toString().c_str());
                m_bl -= d;
            }
        }
        bool tick(const float dt) noexcept override {
            m_atex.tick(dt);
            return !check_hit();
        }

        void draw() noexcept override {
            m_atex.draw(m_bl.x, m_bl.y + m_dim.y);
            if( debug_gfx ) {
                pixel::set_pixel_color(rgba_yellow);
                pixel::f2::rect_t r({m_bl.x, m_bl.y+m_dim.y}, m_dim.x, m_dim.y);
                r.draw();
            }
        }
};
typedef std::shared_ptr<base_t> base_ref_t;

class player_t : public gobject_t {
    private:
        int m_lives;
        base_ref_t m_base;
        constexpr static int start_live = 3;
        float m_respawn_timer;
        int m_score;
        int shoot_number;

        void base_dtor() noexcept {
            --m_lives;
            m_respawn_timer = 2; // [s]
            alien_group.set_pause(true);
        }

        /// Returns base bottom-left start-pos
        pixel::f2::point_t base_start_pos_bl() { return base_box.bl; }

        void respawn_base() noexcept {
            if(m_lives <= 0){
                return;
            }
            m_respawn_timer = 0;
            m_base = std::make_shared<base_t>(base_start_pos_bl());
            alien_group.set_pause(false);
        }

    public:
        player_t() noexcept
        : m_lives(start_live),
          m_base(nullptr), m_respawn_timer(0), m_score(0), shoot_number(0)
        { respawn_base(); }

        /// Returns bottom-left position
        pixel::f2::point_t pos() const noexcept override { return m_base->pos(); }

        pixel::f2::aabbox_t box() const noexcept override { return m_base->box(); }

        bool on_screen() const noexcept override { return m_base->on_screen(); }

        bool intersection(const pixel::f2::aabbox_t& o) const noexcept override { return m_base->intersection(o); }

        void reset() noexcept {
            m_score = 0;
            m_lives = start_live;
            respawn_base();
        }

        void next_level(){
            if(!m_base){
                respawn_base();
            } else {
                m_base->reset(base_start_pos_bl());
            }
        }

        bool is_killed() const noexcept { return m_base->is_killed(); }
        constexpr int lives() const noexcept { return m_lives; }

        int peng_inventory() const noexcept { return m_base->peng_inventory; }
        base_ref_t& base() noexcept { return m_base; }
        constexpr int score() const noexcept { return m_score; }
        void peng_completed(const peng_t& p) noexcept {
            if(!is_killed())  {
                ++m_base->peng_inventory;
            }
            add_score(p.alien_hit_value());
        }
        void add_score(int diff) noexcept { m_score += diff; }

        bool tick(const float dt) noexcept override {
            if( !m_base->tick(dt) ) {
                base_dtor();
            }
            if( m_respawn_timer > 0 ) {
                m_respawn_timer -= dt;
                if( 0 >= m_respawn_timer ) {
                    respawn_base();
                }
            }
            return true;
        }
        void draw() noexcept override {
            m_base->draw();
        }
        void handle_event0() noexcept {
        }
        void handle_event1(const float /* dt [s] */) noexcept {
            if( m_base->is_killed() ) {
                return;
            }
            const float avg_fd = float( pixel::gpu_avg_framedur().to_double() );
            if( event.has_any_p2() ) {
                if( event.pressed_and_clr(pixel::input_event_type_t::P2_ACTION2) ) {
                    if(m_base->peng()){
                        ++shoot_number;
                    }
                }
            }
            if( event.has_any_p1() ) {
                if( event.pressed(pixel::input_event_type_t::P1_LEFT) ){
                    m_base->move( { -base_velo_h * avg_fd, 0 } );
                } else if( event.pressed(pixel::input_event_type_t::P1_RIGHT) ){
                    m_base->move( { base_velo_h * avg_fd, 0 } );
                }
            }
        }
};
static void peng_from_alien() {
    const alien_ref& alien = alien_group.get_random();
    if( nullptr == alien ) {
        return;
    }
    static int alt = 0;
    std::vector<pixel::texture_ref>& vtex = 0 == alt ? tex_ashot1 : tex_ashot2;
    // adjust start posision to geometric alien model
    constexpr float x_adjust = 1.5f;
    pixel::f2::point_t bl = {alien->pos().x + (float)alien->atex().width()/2.0f + x_adjust - float(vtex[0]->width),
                             alien->pos().y - (float)vtex[0]->height};
    pixel::f2::vec_t v_p = pixel::f2::vec_t::from_length_angle(field_height/2 + 10.0f * (float)level, 270_deg);
    pengs.emplace_back(bl, v_p, alien_id, pixel::animtex_t("ashot", peng_t::anim_period, vtex));
    alt = (alt + 1)%2;
}

static pixel::f2::point_t tl_text;
static std::string record_bmpseq_basename;
static int high_score = 1000;
static uint64_t t_start = 0; // [ms]
static std::shared_ptr<player_t> player;
static bool game_over = false;
static si_time_f32 level_time = 0;

static void reset_all() {
    t_start = jau::getElapsedMillisecond();
    reset_items();
    if( player ) {
        player->reset();
    } else {
        player = std::make_shared<player_t>();
    }
    game_over = false;
    level_time = 0;
    level = 1;
}

#if defined(__EMSCRIPTEN__)
    static void init_audio() {
        jau::audio::init_audio_subsystem(MIX_INIT_OGG, MIX_INIT_OGG);
        load_samples();
    }
    extern "C" {
        EMSCRIPTEN_KEEPALIVE void reset_game() noexcept { reset_all(); }
        EMSCRIPTEN_KEEPALIVE void start_audio() noexcept { init_audio(); reset_game(); }
        EMSCRIPTEN_KEEPALIVE void set_game_fps(int v) noexcept { pixel::set_gpu_forced_fps(v); reset_game(); }
    }
#endif

void mainloop() {
    static uint64_t frame_count_total = 0;
    static uint64_t snap_count = 0;
    static uint64_t t_last = getElapsedMillisecond(); // [ms]
    static const int text_height = 24;
    static bool animating = true;
    constexpr static float min_peng_time = 300_ms;
    constexpr static float max_peng_time = 2_s;
    static float peng_time = next_rnd(min_peng_time, max_peng_time);
    bool do_snapshot = false;

    if( 0 == frame_count_total ) {
        t_start = jau::getElapsedMillisecond();
        player = std::make_shared<player_t>();
    }

    while( pixel::handle_one_event(event) ) {
        if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_CLOSE_REQ ) ) {
            printf("Exit Application\n");
            #if defined(__EMSCRIPTEN__)
                emscripten_cancel_main_loop();
            #else
                exit(0);
            #endif
        } else if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_RESIZED ) ) {
            pixel::cart_coord.set_height(-screen_height/2.0f, screen_height/2.0f);
        } else if( event.released_and_clr(pixel::input_event_type_t::RESET) ) {
            reset_all();
        } else if( event.released_and_clr(pixel::input_event_type_t::F12) ) {
            do_snapshot = true;
        } else if( event.released_and_clr(pixel::input_event_type_t::F9) ) {
            debug_gfx = !debug_gfx;
        }
        if( event.paused() ) {
            animating = false;
        } else {
            if( !animating ) {
                t_last = getElapsedMillisecond(); // [ms]
            }
            animating = true;
        }

        // Pass events to all animated objects
        if( animating ) {
            player->handle_event0();
        }
    }
    const uint64_t t1 = animating ? getElapsedMillisecond() : t_last; // [ms]
    const float dt = (float)(t1 - t_last) / 1000.0f; // [s]
    t_last = t1;
    level_time += dt;

    if(animating) {
        {
            player->handle_event1(dt);
            player->tick(dt);
        }
        // alien tick
        alien_group.tick(dt);

        // pengs tick
        for(auto it = pengs.begin(); it != pengs.end(); ) {
            peng_t& p = *it;
            if( p.tick(dt) ) {
                ++it;
            } else {
                if(p.m_owner == base_id){
                    player->peng_completed(p);
                }
                it = pengs.erase(it);
            }
        }
        if( !player->is_killed() ) {
            peng_time -= dt;
            if(peng_time <= 0){
                peng_from_alien();
                peng_time = next_rnd(min_peng_time, max_peng_time-(float)level*50_ms-level_time/2_min);
            }
        }
        for(auto it = extra_sprites.begin(); it != extra_sprites.end(); ) {
            sprite_t& s = *it;
            if( s.tick(dt) ) {
                ++it;
            } else {
                it = extra_sprites.erase(it);
            }
        }
    }
    pixel::clear_pixel_fb(0, 0, 0, 255);

    // Draw all objects
    if( debug_gfx ) {
        {
            const float w = screen_width - 2;
            const float h = screen_height - 2;
            pixel::set_pixel_color(rgba_yellow);
            pixel::f2::rect_t r({-w/2.0f, +h/2.0f}, w, h);
            r.draw();
        }
        {
            pixel::set_pixel_color(rgba_green);
            pixel::f2::rect_t r(field_tl, field_width, field_height);
            r.draw();
        }
    }

    player->draw();
    for(auto & bunk : bunks) {
        bunk.draw();
    }
    alien_group.draw();
    for(auto & peng : pengs) {
        peng.draw();
    }
    for(auto & s : extra_sprites) {
        s.draw();
    }

    {
        pixel::set_pixel_color(rgba_green);
        pixel::f2::rect_t r1(field_box.bl, field_width, 1.0f);
        r1.draw(false);

        const float abstand_zsl = 5;
        for(int i = 0; i < player->lives()-1; ++i){
            const float w = screen_width - 2;
            const float h = screen_height - 2;
            const pixel::f2::aabbox_t box = player->base()->box();
            pixel::f2::rect_t r2( {-w/2.0f, +h/2.0f }, w, h);
            base_t l = { { r2.m_bl.x + abstand_zsl*(float)(i + 1) +
                           box.width()*(float)i + box.width()/2,
                           r2.m_bl.y + abstand_zsl}};
            l.draw();
        }
    }

    //
    //
    //

    if(alien_group.active_count() == 0){
        reset_items();
        player->next_level();
        ++level;
        level_time = 0;
    }
    pixel::set_pixel_color(255, 255, 255, 255);

    float fps = pixel::gpu_avg_fps();
    tl_text.set(pixel::cart_coord.min_x(), pixel::cart_coord.max_y());
    pixel::texture_ref hud_text = pixel::make_text(tl_text, 0, vec4_text_color, text_height, "%s s, fps %4.2f/%d, score %4d, high score %d, level %d",
                    to_decstring((t1-t_start)/1000, ',', 5).c_str(),
                    fps, pixel::expected_fps(), player->score(), high_score, level);
    if( alien_group.box().bl.y < -62-bunks[0].box().height() || player->lives() <= 0) {
        game_over = true;
        animating = false;
    }

    if(player->score() > high_score){
        high_score = player->score();
    }

    pixel::swap_pixel_fb(false);
    {
        const int dx = ( pixel::fb_width - round_to_int((float)hud_text->width*(float)hud_text->dest_sx) ) / 2;
        hud_text->draw_fbcoord(dx, 0);
    }
    if(game_over){
        tl_text.set(pixel::cart_coord.min_x(), 1.0f*pixel::cart_coord.height()/4.0f);
        pixel::texture_ref game_over_text = pixel::make_text(tl_text, 0, {1, 0, 0, 1}, text_height*5 , "GAME OVER");
        const int dx = ( pixel::fb_width - round_to_int((float)game_over_text->width*game_over_text->dest_sx) ) / 2;
        game_over_text->draw_fbcoord(dx, 0);
    }
    pixel::swap_gpu_buffer();
    if( record_bmpseq_basename.size() > 0 ) {
        std::string snap_fname(128, '\0');
        const int written = std::snprintf(&snap_fname[0], snap_fname.size(), "%s-%7.7" PRIu64 ".bmp", record_bmpseq_basename.c_str(), frame_count_total);
        snap_fname.resize(written);
        pixel::save_snapshot(snap_fname);
    }
    if( do_snapshot ) {
        std::string snap_fname(128, '\0');
        const int written = std::snprintf(&snap_fname[0], snap_fname.size(), "spaceinv-%7.7" PRIu64 ".bmp", snap_count++);
        snap_fname.resize(written);
        pixel::save_snapshot(snap_fname);
        printf("Snapshot written to %s\n", snap_fname.c_str());
    }
    ++frame_count_total;
}

int main(int argc, char *argv[])
{
    bool use_audio = true;
    int window_height = 1024;
    int window_width = (int)std::round( (float)window_height * screen_width_pct );
    {
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-height", argv[i]) && i+1<argc) {
                window_height = atoi(argv[i+1]);
                window_width = (int)std::round( (float)window_height * screen_width_pct );
                ++i;
            } else if( 0 == strcmp("-record", argv[i]) && i+1<argc) {
                record_bmpseq_basename = argv[i+1];
                ++i;
            } else if( 0 == strcmp("-debug_gfx", argv[i]) ) {
                debug_gfx = true;
            } else if( 0 == strcmp("-fps", argv[i]) && i+1<argc) {
                pixel::set_gpu_forced_fps(atoi(argv[i+1]));
                ++i;
            } else if( 0 == strcmp("-noaudio", argv[i])) {
                use_audio = false;
            } else if( 0 == strcmp("-aliens", argv[i]) && i+1<argc) {
                aliens_total = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-alienva", argv[i]) && i+1<argc) {
                alien_velo_amp = (float)atof(argv[i+1]);
                ++i;
            }

        }
        const uint64_t elapsed_ms = getElapsedMillisecond();
        log_printf(elapsed_ms, "- record %s\n", record_bmpseq_basename.size()==0 ? "disabled" : record_bmpseq_basename.c_str());
        log_printf(elapsed_ms, "- forced_fps %d\n", pixel::gpu_forced_fps());
        log_printf(elapsed_ms, "- debug_gfx %d\n", debug_gfx);
        log_printf(elapsed_ms, "- aliens %zu\n", aliens_total);
        log_printf(elapsed_ms, "- alien velo amp %f\n", alien_velo_amp);
    }
    {
        const float origin_norm[] = { 0.5f, 0.5f };
        if( !pixel::init_gfx_subsystem(argv[0], "Space Invaders", window_width, window_height, origin_norm, true, true /* subsys primitives */) ) {
            return 1;
        }
    }
    pixel::cart_coord.set_height(-screen_height/2.0f, screen_height/2.0f);

    log_printf(0, "XX %s\n", pixel::cart_coord.toString().c_str());
    {
        float w = pixel::cart_coord.width();
        float h = pixel::cart_coord.height();
        float r01 = h/w;
        float a = w / h;
        printf("-w %f [x]\n-h %f [y]\n-r1 %f [y/x]\n-r2 %f [x/y]\n", w, h, r01, a);
    }
    if( !load_textures() ) {
        return 1;
    }
    #if !defined(__EMSCRIPTEN__)
        if( use_audio ) {
            jau::audio::init_audio_subsystem(MIX_INIT_OGG, MIX_INIT_OGG);
        }
    #endif
    load_samples();
    reset_items();
    #if defined(__EMSCRIPTEN__)
        (void)use_audio;
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
    return 0;
}
