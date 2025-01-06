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
#include <cstddef>
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
#include <pixel/unit.hpp>
#include <vector>

using namespace pixel::literals;

static pixel::input_event_t event;

/**
 * Space Invaders (1978 by Taito)
 *
 * - Space   224 x 260
 * - Field   204 x 184  (w: min-max, h: base -> mothership)
 * - Alien-1  12 x   8
 * - Alien-2  11 x   8
 * - Alien-3   8 x   8
 * - Alien-X  13 x   8
 * - Alien-S   3 x   7  (shot sets 2x3)
 * - Red-X     8 x   8
 * - Base     13 x   8  (incl. explosions 1-3)
 * - peng      1 x   4
 * - Bunker   22 x  16  (aspect 1.375) 1: 32/192, 2: 77/192, 3: 122/192, 4: 176/192
 * - Aline-M  16 x   8  (mothership)
 * - Peng      1 x   4
*/
constexpr static float space_height = 260.0f; // [m]
constexpr static float space_width = 224.0f; // [m]
constexpr static float space_width_pct = space_width / space_height;

constexpr static float field_height = 188.0f; // [m]
constexpr static float field_width = 204.0f; // [m]
static const pixel::f2::aabbox_t field_box( { -field_width/2.0, -field_height/2.0 }, { field_width/2.0, field_height/2.0 } ); // gcc bug 93413 (constexpr, solved in gcc 13)

constexpr static float base_width = 13.0f; // [m]
constexpr static float base_height = 8.0f; // [m]

constexpr static float bunk_width = 22.0f;
// constexpr static float bunk_height = 16.0f;

constexpr static float base_peng_velo = field_height / 1.5f; // [m/s]
constexpr static int base_peng_inventory_max = 1;

constexpr static float alien_hstep = 2;
constexpr static float alien_vstep = 8;
constexpr static int aliens_per_row = 11;

constexpr static pixel::f2::point_t bunk1_tl( -80, -62 );
constexpr static pixel::f2::point_t bunk2_tl( -35, -62 );
constexpr static pixel::f2::point_t bunk3_tl(  10, -62 );
constexpr static pixel::f2::point_t bunk4_tl(  55, -62 );

static const pixel::f2::aabbox_t base_box( { bunk1_tl.x-base_width, -field_height/2.0 }, { bunk4_tl.x+bunk_width+base_width, field_height/2.0 } );

//
//
//

constexpr static int base_id = 1;
constexpr static int alien_id = 2;

// static const uint8_t rgba_white[/*4*/] = { 255, 255, 255, 255 };
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
static pixel::texture_ref tex_alienm;
static int level = 1;

using namespace jau::audio;

static std::vector<audio_sample_ref> audio_aliens;
static audio_sample_ref audio_peng, audio_alienX, audio_baseX;

static bool load_samples() {
    audio_aliens.clear();
    if( jau::audio::is_audio_subsystem_initialized() ) {
        audio_aliens.push_back( std::make_shared<jau::audio::audio_sample_t>("resources/spaceinv/alien1.wav", false, MIX_MAX_VOLUME) );
        audio_aliens.push_back( std::make_shared<jau::audio::audio_sample_t>("resources/spaceinv/alien2.wav", false, MIX_MAX_VOLUME) );
        audio_aliens.push_back( std::make_shared<jau::audio::audio_sample_t>("resources/spaceinv/alien3.wav", false, MIX_MAX_VOLUME) );
        audio_aliens.push_back( std::make_shared<jau::audio::audio_sample_t>("resources/spaceinv/alien4.wav", false, MIX_MAX_VOLUME) );
        audio_peng = std::make_shared<jau::audio::audio_sample_t>("resources/spaceinv/peng.wav", false, MIX_MAX_VOLUME/4);
        audio_alienX = std::make_shared<jau::audio::audio_sample_t>("resources/spaceinv/alienX.wav", false, MIX_MAX_VOLUME/4);
        audio_baseX = std::make_shared<jau::audio::audio_sample_t>("resources/spaceinv/baseX.wav", false, MIX_MAX_VOLUME);
        return true;
    } else {
        audio_aliens.push_back( std::make_shared<jau::audio::audio_sample_t>() );
        audio_peng = std::make_shared<jau::audio::audio_sample_t>();
        audio_alienX = std::make_shared<jau::audio::audio_sample_t>();
        audio_baseX = std::make_shared<jau::audio::audio_sample_t>();
        return false;
    }
}

static bool load_textures() {
    {
        pixel::bitmap_ref empty = std::make_shared<pixel::bitmap_t>(64, 64);
        pixel::log_printf(0, "XX empty: %s\n", empty->toString().c_str());
    }
    bmp_bunk = std::make_shared<pixel::bitmap_t>("resources/spaceinv/spaceinv-bunk.png");
    pixel::log_printf(0, "XX bmp bunk: %s\n", bmp_bunk->toString().c_str());

    int y_off=0;
    all_images = std::make_shared<pixel::texture_t>("resources/spaceinv/spaceinv-sprites.png");
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
    tex_alienm = pixel::add_sub_texture(all_images, 0, y_off, 16, 7);

    // row-1
    pixel::log_printf(0, "XX alien1: %d textures\n", tex_alien1.size());
    for(const pixel::texture_ref& t : tex_alien1) {
        pixel::log_printf(0, "XX alien1: %s\n", t->toString().c_str());
    }
    pixel::log_printf(0, "XX alienX: %s\n", tex_alienX[0]->toString().c_str());
    pixel::log_printf(0, "XX redX:   %s\n", tex_redX[0]->toString().c_str());

    // row-2
    pixel::log_printf(0, "XX alien2: %d textures\n", tex_alien2.size());
    for(const pixel::texture_ref& t : tex_alien2) {
        pixel::log_printf(0, "XX alien2: %s\n", t->toString().c_str());
    }

    // row-3
    pixel::log_printf(0, "XX alien3: %d textures\n", tex_alien3.size());
    for(const pixel::texture_ref& t : tex_alien3) {
        pixel::log_printf(0, "XX alien3: %s\n", t->toString().c_str());
    }
    pixel::log_printf(0, "XX ashot1: %d textures\n", tex_ashot1.size());
    for(const pixel::texture_ref& t : tex_ashot1) {
        pixel::log_printf(0, "XX ashot1: %s\n", t->toString().c_str());
    }
    pixel::log_printf(0, "XX ashot2: %d textures\n", tex_ashot2.size());
    for(const pixel::texture_ref& t : tex_ashot2) {
        pixel::log_printf(0, "XX ashot2: %s\n", t->toString().c_str());
    }

    // row-4
    pixel::log_printf(0, "XX base: %s\n", tex_base[0]->toString().c_str());
    pixel::log_printf(0, "XX baseX: %d textures\n", tex_baseX.size());
    for(const pixel::texture_ref& t : tex_baseX) {
        pixel::log_printf(0, "XX baseX: %s\n", t->toString().c_str());
    }

    // row-5
    pixel::log_printf(0, "XX bunk: %s\n", tex_bunk[0]->toString().c_str());
    pixel::log_printf(0, "XX peng: %s\n", tex_peng[0]->toString().c_str());

    // row-6
    pixel::log_printf(0, "XX alien MS: %s\n", tex_alienm->toString().c_str());
    return true;
}

class alient_t {
  private:
    int m_value;
    pixel::animtex_t m_atex;
    pixel::f2::vec_t m_dim;
    float dt_kill = 0;

  public:
    pixel::f2::point_t m_tl;

    alient_t(int value, pixel::animtex_t atex_alien, const pixel::f2::point_t& center) noexcept
    : m_value(value), m_atex(std::move(atex_alien)),
      m_dim((float)m_atex.width(), (float)m_atex.height()),
      m_tl( center + pixel::f2::point_t(-m_dim.x/2, +m_dim.y/2) )
    { }

    pixel::f2::aabbox_t box() const noexcept {
        return pixel::f2::aabbox_t().resize(m_tl).resize(m_tl.x + m_dim.x, m_tl.y - m_dim.y);
    }

    constexpr int value() const noexcept { return m_value; }

    void step(const pixel::f2::vec_t& step) noexcept {
        m_atex.next();
        m_tl += step;
    }

    void notify_killed() {
        m_atex = pixel::animtex_t("AlienX", 1.0f, tex_alienX);
        dt_kill = 0.5f;
    }

    bool killanim(const float dt) {
        if( 0 < dt_kill ) {
            dt_kill = std::max( 0.0f, dt_kill - dt );
            m_atex.tick(dt);
            return true;
        } else {
            return false;
        }
    }
    void draw() const noexcept {
        m_atex.draw(m_tl.x, m_tl.y);
    }

    bool on_screen(){
        return box().inside(field_box);
    }

    bool intersection(const alient_t& o) const {
        return box().intersects(o.box());
    }

    pixel::animtex_t& atex() { return m_atex; }
};

class alien_group_t {
  private:
    pixel::f2::aabbox_t m_box;
    pixel::f2::vec_t m_step = pixel::f2::vec_t(alien_hstep, 0);
    float m_sec_per_step = 1.0f;
    float m_dt_step_left = m_sec_per_step;
    std::vector<alient_t> m_killed;
    bool m_pause = false;

  public:
    std::vector<alient_t> actives;

    alien_group_t() = default;
    alien_group_t(const alien_group_t&) = default;
    alien_group_t(alien_group_t&&)      = default;
    alien_group_t& operator=(const alien_group_t&) = default;
    alien_group_t& operator=(alien_group_t&&)      = default;

    void reset() {
        const float cell_height = (float)tex_alien1[0]->height;
        const float cell_width = (float)tex_alien1[0]->width;
        actives.clear();
        m_box.reset();
        m_sec_per_step = std::max(700_ms, 1_s-(level-1)/40);
        pixel::f2::point_t p = field_box.bl;
        p.y =  0.0f;
        for(int y=0; y<2; ++y) {
            p.x = field_box.bl.x + cell_width/2.0f;
            for(int x=0; x<aliens_per_row; ++x) {
                actives.emplace_back(10, pixel::animtex_t("Alien1", m_sec_per_step, tex_alien1), p);
                p.x += cell_width * 1.5f;
            }
            p.y +=  2.0f * cell_height;
        }
        for(int y=0; y<2; ++y) {
            p.x = field_box.bl.x + cell_width/2.0f;
            for(int x=0; x<aliens_per_row; ++x) {
                actives.emplace_back(20, pixel::animtex_t("Alien2", m_sec_per_step, tex_alien2), p);
                m_box.resize(actives[actives.size()-1].box());
                p.x += cell_width * 1.5f;
            }
            p.y +=  2.0f * cell_height;
        }
        for(int y=0; y<1; ++y) {
            p.x = field_box.bl.x + cell_width/2.0f;
            for(int x=0; x<aliens_per_row; ++x) {
                actives.emplace_back(30, pixel::animtex_t("Alien3", m_sec_per_step, tex_alien3), p);
                m_box.resize(actives[actives.size()-1].box());
                p.x += cell_width * 1.5f;
            }
            p.y +=  2.0f * cell_height;
        }
        m_dt_step_left = m_sec_per_step;
    }

    void scale_pace(float faktor) {
        const float old = m_sec_per_step;
        m_sec_per_step *= faktor;
        pixel::log_printf(0, "XX ag scale pace %f * %f = %f\n", old, faktor, m_sec_per_step);
    }

    bool check_hit(const pixel::f2::aabbox_t& b, int& value) {
        for(auto it = actives.begin(); it != actives.end(); ) {
            alient_t& a = *it;
            if( !a.box().intersects(b) ) {
                ++it;
            } else {
                value = a.value();
                a.notify_killed();
                m_killed.push_back(a);
                it = actives.erase(it);
                return true;
            }
        }
        return false;
    }

    void set_pause(bool v) noexcept { m_pause = v; }

    void tick(const float dt) {
        static int sound_idx = 0;
        static audio_sample_ref audio_sample = nullptr;

        if( m_pause ) {
            return;
        }
        for(auto it = m_killed.begin(); it != m_killed.end(); ) {
            if( it->killanim(dt) ) {
                ++it;
            } else {
                it = m_killed.erase(it);
            }
        }
        m_dt_step_left -= dt;
        if( m_dt_step_left > 0 || m_killed.size() > 0 ) {
            return;
        }
        {
            if( audio_sample ) {
                audio_sample->stop();
            }
            audio_sample = audio_aliens[sound_idx];
            sound_idx = ( sound_idx + 1 ) % audio_aliens.size();
            audio_sample->play(1);
        }
        m_dt_step_left = m_sec_per_step;
        if( !m_box.inside(field_box) ) {
            m_step.x *= -1;
            m_step.y = -alien_vstep;
            scale_pace(1.0f-1.0f/20.f);
        }
        for(alient_t& a : actives){
            a.step(m_step);
        }
        m_step.y = 0;
    }

    void draw(){
        m_box.reset();
        for(alient_t& a : actives){
            a.draw();
            m_box.resize(a.box());
        }
        for(alient_t& a : m_killed){
            a.draw();
        }
    }
};
alien_group_t alien_group;

class bunker_t {
  private:
    size_t idx;
    pixel::f2::vec_t m_dim;
    pixel::f2::point_t m_tl;
    pixel::bitmap_ref m_bunk;

    void put_bunk(const pixel::f2::aabbox_t& box, uint32_t abgr) noexcept {
        if(!m_bunk->pixels() || 0 == m_bunk->width || 0 == m_bunk->height ) {
            return;
        }
        const uint32_t width = m_bunk->width;
        const uint32_t height = m_bunk->height;
        const uint32_t y1 = std::max<uint32_t>(0, pixel::floor_to_uint32(box.bl.y));
        const uint32_t y2 = std::min<uint32_t>(height, pixel::ceil_to_uint32(box.tr.y));
        for(uint32_t y=y1; y<y2; ++y) {
            const uint32_t o = pixel::round_to_uint32(pixel::next_rnd()*4);
            const uint32_t x1 = std::max<uint32_t>(0, pixel::floor_to_uint32(box.bl.x)+1-o);
            const uint32_t x2 = std::min<uint32_t>(width, pixel::ceil_to_uint32(box.tr.x)-1+o);
            for(uint32_t x=x1; x<x2; ++x) {
                uint32_t * const target_pixel = std::bit_cast<uint32_t *>(m_bunk->pixels() + static_cast<size_t>((m_bunk->height - y - 1) * m_bunk->stride) + static_cast<size_t>(x * m_bunk->bpp));
                *target_pixel = abgr;
            }
        }
    }
  public:
    bunker_t(size_t idx_, const pixel::f2::point_t& tl) noexcept
    : idx(idx_), m_dim((float)tex_bunk[idx]->width, (float)tex_bunk[idx]->height),
      m_tl(tl), m_bunk(bmp_bunk->clone())
    {
        tex_bunk[idx]->update(m_bunk);
    }

    pixel::f2::aabbox_t box() const noexcept {
        return pixel::f2::aabbox_t().resize(m_tl).resize(m_tl.x + m_dim.x, m_tl.y - m_dim.y);
    }

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
        ibox.tr.x += ibox.width() * amp.x ;
        ibox.tr.y += ibox.height() * amp.y ;
        // printf("XXX bunk.hit: ibox1.2 %s, amp %s\n", ibox.toString().c_str(), amp.toString().c_str());
        if( m_bunk->equals(ibox, clrpix) ) {
            // printf("XXX bunk.hit: clear -> false\n");
            return false;
        }
        put_bunk(ibox, clrpix);
        tex_bunk[idx]->update(m_bunk);
        return true;
    }
    void draw() const noexcept {
        tex_bunk[idx]->draw(m_tl.x, m_tl.y, m_dim.x, m_dim.y);
    }

    bool on_screen(){
        return box().inside(field_box);
    }
    bool intersection(const pixel::f2::aabbox_t& o) const {
        return box().intersects(o);
    }
};
std::vector<bunker_t> bunks;

struct hit_result{
    bool hit;
    bool live;
};

class peng_t {
  private:
    pixel::animtex_t m_atex;
    pixel::f2::vec_t m_dim;
    pixel::f2::point_t m_tl;
    int m_alien_value;

  public:
    constexpr static float anim_period = 0.03f; // 0.025f;
    pixel::f2::vec_t m_velo; // [m/s]
    int m_owner;

  private:
    bool check_alien_hit() {
        if( m_owner == alien_id ) {
            return false;
        }
        m_alien_value = 0;
        if( alien_group.check_hit( box(), m_alien_value ) ) {
            audio_alienX->play();
            return true;
        } else {
            return false;
        }
    }

    bool check_bunker_hit() {
        pixel::f2::aabbox_t b = box();
        for(auto it = bunks.begin(); it != bunks.end(); ) {
            bunker_t& bunk = *it;
            if( bunk.box().intersects(b) &&
                bunk.hit(b, m_owner == alien_id ? pixel::f2::vec_t{ 0.6f, 2.5f } : pixel::f2::vec_t{ 0.0f, 0.0f }) )
            {
                return true;
            } else {
                ++it;
            }
        }
        return false;
    }

  public:
    peng_t(const pixel::f2::point_t& center, const pixel::f2::vec_t& v, const int owner, pixel::animtex_t atex) noexcept
    : m_atex(std::move(atex)),
      m_dim((float)m_atex.width(), (float)m_atex.height()),
      m_tl( center + pixel::f2::point_t(-m_dim.x/2, +m_dim.y/2) ),
      m_alien_value(0), m_velo( v ), m_owner(owner)
    { }

    pixel::f2::aabbox_t box() const noexcept {
        return pixel::f2::aabbox_t().resize(m_tl).resize(m_tl.x + m_dim.x, m_tl.y - m_dim.y);
    }

    bool tick(const float dt) noexcept {
        m_atex.tick(dt);
        if( !box().inside(field_box) ) {
            return false;
        }
        if(!m_velo.is_zero()){
            m_tl += m_velo * dt;
        }
        return !check_alien_hit() && !check_bunker_hit();
    }

    void draw() const noexcept {
        m_atex.draw(m_tl.x, m_tl.y, m_dim.x, m_dim.y);
    }

    bool on_screen(){
        return box().inside(field_box);
    }

    bool intersection(const peng_t& o) const {
        return box().intersects(o.box());
    }

    int alien_hit_value() const { return m_alien_value; }
};
std::vector<peng_t> pengs;

void reset_items() {
    bunks.clear();
    bunks.emplace_back(0, bunk1_tl);
    bunks.emplace_back(1, bunk2_tl);
    bunks.emplace_back(2, bunk3_tl);
    bunks.emplace_back(3, bunk4_tl);
    alien_group.reset();
    pengs.clear();
}

class base_t {
    public:
        constexpr static const float height = base_height; // [m]

    private:
        pixel::animtex_t m_atex;
        pixel::f2::vec_t m_dim;
        pixel::f2::point_t m_tl, m_tc;
        bool m_killed;

        bool check_hit() {
            if( m_killed ) {
                return false;
            }
            pixel::f2::aabbox_t b = box();
            for(auto it = pengs.begin(); it != pengs.end(); ) {
                peng_t& a = *it;
                if( !a.box().intersects(b) ) {
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

        base_t(const pixel::f2::point_t& top_center) noexcept
        : m_atex(pixel::animtex_t("peng", 1.0f, tex_base)),
          m_dim((float)tex_base[0]->width, (float)tex_base[0]->height),
          m_tl(top_center.x - m_dim.x/2.0f, top_center.y), m_tc(top_center),
          m_killed(false),
          peng_inventory(base_peng_inventory_max)
        {}

        void reset(const pixel::f2::point_t& top_center){
            peng_inventory = base_peng_inventory_max;
            m_tl = {top_center.x - m_dim.x/2.0f, top_center.y};
            m_tc = top_center;
        }

        constexpr bool is_killed() const noexcept { return m_killed; }

        pixel::f2::aabbox_t box() const noexcept {
            return pixel::f2::aabbox_t().resize(m_tl).resize(m_tl.x + m_dim.x, m_tl.y - m_dim.y);
        }

        void peng() noexcept {
            if(peng_inventory > 0){
                // adjust start posision to geometric base model
                pixel::f2::point_t p0 = {m_tc.x, m_tc.y + (float)tex_peng[0]->height/2 + 0.05f};
                pixel::f2::vec_t v_p = pixel::f2::vec_t::from_length_angle(base_peng_velo + 10.0f * (float)level, 90_deg);
                pengs.emplace_back(p0, v_p, base_id, pixel::animtex_t("peng", peng_t::anim_period, tex_peng));
                --peng_inventory;
                audio_peng->play();
            }
        }

        void move(const pixel::f2::point_t& d) noexcept {
            m_tl += d;
            m_tc += d;
            if( !box().inside(base_box) ) {
                // pixel::log_printf(0, "XX %s -> %s, %s\n", d.toString().c_str(), box().toString().c_str(), base_box.toString().c_str());
                m_tl -= d;
                m_tc -= d;
            }
        }
        bool tick(const float dt) noexcept {
            m_atex.tick(dt);
            return !check_hit();
        }

        void draw() const noexcept {
            m_atex.draw(m_tl.x, m_tl.y, m_dim.x, m_dim.y);
        }
};
typedef std::shared_ptr<base_t> base_ref_t;

class player_t {
    private:
        int m_lives;
        base_ref_t m_base;
        constexpr static int start_live = 3;
        float m_respawn_timer;
        int m_score;

        void base_dtor() noexcept {
            --m_lives;
            m_respawn_timer = 2; // [s]
            alien_group.set_pause(true);
        }

        pixel::f2::point_t base_startpos() {
            return {base_box.bl.x + base_width/2, base_box.bl.y + base_height};
        }

        void respawn_base() noexcept {
            if(m_lives <= 0){
                return;
            }
            m_respawn_timer = 0;
            m_base = std::make_shared<base_t>(base_startpos());
            alien_group.set_pause(false);
        }

    public:
        player_t() noexcept
        : m_lives(start_live),
          m_base(nullptr), m_respawn_timer(0), m_score(0)
        { respawn_base(); }

        void reset() noexcept {
            m_score = 0;
            m_lives = start_live;
            respawn_base();
        }

        void next_level(){
            if(!m_base){
                respawn_base();
            } else {
                m_base->reset(base_startpos());
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

        bool tick(const float dt) noexcept {
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
        void draw() const noexcept {
            m_base->draw();
        }
        void handle_event0() noexcept {
            if( !m_base->is_killed() && event.has_any_p1() ) {
                if( event.released_and_clr(pixel::input_event_type_t::P2_ACTION2) ) {
                    m_base->peng();
                }
            }
        }
        void handle_event1(const float dt /* [s] */) noexcept {
            const float v = 35.0f;
            if( !m_base->is_killed() && event.has_any_p1() ) {
                if( event.pressed(pixel::input_event_type_t::P1_LEFT) ){
                    m_base->move( { -v * dt, 0 } );
                } else if( event.pressed(pixel::input_event_type_t::P1_RIGHT) ){
                    m_base->move( { v * dt, 0 } );
                }
            }
        }
};
void peng_from_alien(alient_t alien){
    static int alt = 0;
    std::vector<pixel::texture_ref>& vtex = 0 == alt ? tex_ashot1 : tex_ashot2;
    // adjust start posision to geometric alien model
    pixel::f2::point_t p0 = {alien.m_tl.x + (float)alien.atex().width()/2,
                             alien.m_tl.y - (float)alien.atex().height() - 0.05f};
    p0.add(0, (float)vtex[0]->height/2);
    pixel::f2::vec_t v_p = pixel::f2::vec_t::from_length_angle(field_height/2 + 10.0f * (float)level, 270_deg);
    pengs.emplace_back(p0, v_p, alien_id, pixel::animtex_t("ashot", peng_t::anim_period, vtex));
    alt = (alt + 1)%2;
}

void peng_alien() {
    //printf("XXX: Alien shooted");
    if(alien_group.actives.size() <= 0){
        return;
    }
    peng_from_alien(alien_group.actives[(size_t)(pixel::next_rnd() * (float)(alien_group.actives.size()-1))]);
}

#if defined(__EMSCRIPTEN__)
    static void init_audio() {
        jau::audio::init_audio_subsystem();
        load_samples();
    }
    extern "C" {
        EMSCRIPTEN_KEEPALIVE void start_audio() noexcept { init_audio(); }
    }
#endif

static pixel::f2::point_t tl_text;
static std::string record_bmpseq_basename;
static bool raster = false;
static int high_score = 1000;

void mainloop() {
    static player_t p1;
    static uint64_t frame_count_total = 0;
    static uint64_t snap_count = 0;
    static uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]
    static const int text_height = 24;
    static bool animating = true;
    static bool game_over = false;
    constexpr static float min_peng_time = 300_ms;
    constexpr static float max_peng_time = 2_s;
    static float peng_time = min_peng_time + pixel::next_rnd() * (max_peng_time-min_peng_time);
    static const pixel::f2::aabbox_t p1_base_box = p1.base()->box();
    static pixel::si_time_t level_time = 0;
    bool do_snapshot = false;

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
        } else if( event.released_and_clr(pixel::input_event_type_t::RESET) ) {
            reset_items();
            p1.reset();
            game_over = false;
            level_time = 0;
            level = 1;
        } else if( event.released_and_clr(pixel::input_event_type_t::F12) ) {
            do_snapshot = true;
        }
        if( event.paused() ) {
            animating = false;
        } else {
            if( !animating ) {
                t_last = pixel::getElapsedMillisecond(); // [ms]
            }
            animating = true;
        }

        // Pass events to all animated objects
        if( animating ) {
            p1.handle_event0();
        }
    }
    const uint64_t t1 = animating ? pixel::getElapsedMillisecond() : t_last; // [ms]
    const float dt = (float)(t1 - t_last) / 1000.0f; // [s]
    t_last = t1;
    level_time += dt;

    if(animating) {
        {
            p1.handle_event1(dt);
            p1.tick(dt);
        }
        if (raster) {
            pixel::draw_grid(50, 255, 0, 0, 0, 255, 0, 0, 0);
        }
        // alien tick
        alien_group.tick(dt);

        // pengs tick
        {
            for(auto it = pengs.begin(); it != pengs.end(); ) {
                peng_t& p = *it;
                if( p.tick(dt) ) {
                    ++it;
                } else {
                    if(p.m_owner == base_id){
                        p1.peng_completed(p);
                    }
                    it = pengs.erase(it);
                }
            }
        }
        if( !p1.is_killed() ) {
            peng_time -= dt;
            if(peng_time <= 0){
                peng_alien();
                peng_time = min_peng_time +
                        pixel::next_rnd() * (max_peng_time-min_peng_time-(float)level*50_ms-level_time/2_min);
            }
        }
    }
    pixel::clear_pixel_fb(0, 0, 0, 255);

    // Draw all objects
    if( debug_gfx ) {
        {
            const float w = space_width - 2;
            const float h = space_height - 2;
            pixel::set_pixel_color(rgba_yellow);
            pixel::f2::rect_t r({-w/2.0f, +h/2.0f}, w, h);
            r.draw();
        }
        {
            pixel::set_pixel_color(rgba_green);
            pixel::f2::rect_t r({-field_width/2.0f, +field_height/2.0f}, field_width, field_height);
            r.draw();
        }
    }
    {
        const float abstand_zsl = 5;
        for(int i = 0; i < p1.lives()-1; ++i){
            const float w = space_width - 2;
            const float h = space_height - 2;
            pixel::f2::rect_t r({-w/2.0f, +h/2.0f}, w, h);
            base_t l = {{r.m_bl.x + abstand_zsl*(float)(i + 1) +
            p1_base_box.width()*(float)i + p1_base_box.width()/2,
            r.m_bl.y + abstand_zsl + p1_base_box.height()}};
            l.draw();
        }
    }
    p1.draw();
    for(auto & bunk : bunks) {
        bunk.draw();
    }
    // printf("XXX: aliens %zu\n", aliens.size());
    alien_group.draw();
    for(auto & peng : pengs) {
        peng.draw();
    }

    if(alien_group.actives.size() == 0){
        reset_items();
        p1.next_level();
        ++level;
        level_time = 0;
    }
    pixel::set_pixel_color(255, 255, 255, 255);

    float fps = pixel::get_gpu_fps();
    tl_text.set(pixel::cart_coord.min_x(), pixel::cart_coord.max_y());
    pixel::texture_ref hud_text = pixel::make_text(tl_text, 0, vec4_text_color, text_height, "%s s, fps %4.2f, score %4d, high score %d, level %d",
                    pixel::to_decstring(t1/1000, ',', 5).c_str(), // 1d limit
                    fps, p1.score(), high_score, level);
    for(alient_t& a : alien_group.actives){
        if(a.box().bl.y < -62-bunks[0].box().height() || p1.lives() <= 0){
            game_over = true;
            animating = false;
        }
    }

    if(p1.score() > high_score){
        high_score = p1.score();
    }

    pixel::swap_pixel_fb(false);
    {
        const int dx = ( pixel::fb_width - pixel::round_to_int((float)hud_text->width*(float)hud_text->dest_sx) ) / 2;
        hud_text->draw_fbcoord(dx, 0);
    }
    if(game_over){
        tl_text.set(pixel::cart_coord.min_x(), 1.0f*pixel::cart_coord.height()/4.0f);
        pixel::texture_ref game_over_text = pixel::make_text(tl_text, 0, {1, 0, 0, 1}, text_height*5 , "GAME OVER");
        const int dx = ( pixel::fb_width - pixel::round_to_int((float)game_over_text->width*game_over_text->dest_sx) ) / 2;
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
    int window_height = 1024;
    int window_width = (int)std::round( (float)window_height * space_width_pct );
    {
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-height", argv[i]) && i+1<argc) {
                window_height = atoi(argv[i+1]);
                window_width = (int)std::round( (float)window_height * space_width_pct );
                ++i;
            }
        }
    }
    {
        const float origin_norm[] = { 0.5f, 0.5f };
        if( !pixel::init_gfx_subsystem("Space Invaders", window_width, window_height, origin_norm, true, true /* subsys primitives */) ) {
            return 1;
        }
    }
    pixel::cart_coord.set_height(-space_height/2.0f, space_height/2.0f);

    pixel::log_printf(0, "XX %s\n", pixel::cart_coord.toString().c_str());
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
        jau::audio::init_audio_subsystem();
    #endif
    load_samples();
    reset_items();
    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
    return 0;
}
