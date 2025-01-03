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
#include <cstddef>
#include <pixel/pixel3f.hpp>
#include <pixel/pixel4f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"

#include <cinttypes>
#include <cstdio>
#include <cmath>
#include <vector>

using namespace pixel::literals;

static pixel::input_event_t event;

/**
 * Space Invaders Metrics
 * - Space   224 x 260
 * - Field   204 x 184  (w: min-max, h: base -> mothership)
 * - Alien-1  12 x   8
 * - Alien-2  11 x   8
 * - Alien-3   8 x   8
 * - Base     13 x   8
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
static const pixel::f2::aabbox_t field_box( { -field_width/2.0, -field_height/2.0 }, { field_width/2.0, field_height/2.0 } );
// static const pixel::f2::vec_t alien_dim( 172, 72 );

constexpr static const float base_width = 13.0f; // [m]
constexpr static const float base_height = 8.0f; // [m]

constexpr static float bunk_width = 22.0f;
// constexpr static float bunk_height = 16.0f;

constexpr static int ship_id = 1;
constexpr static int alien_id = 2;

static const pixel::f2::point_t bunk1_tl( -80, -62 );
static const pixel::f2::point_t bunk2_tl( -35, -62 );
static const pixel::f2::point_t bunk3_tl(  10, -62 );
static const pixel::f2::point_t bunk4_tl(  55, -62 );

static const pixel::f2::aabbox_t base_box( { bunk1_tl.x-base_width, -field_height/2.0 }, { bunk4_tl.x+bunk_width+base_width, field_height/2.0 } );

static const uint8_t rgba_white[/*4*/] = { 255, 255, 255, 255 };
static const uint8_t rgba_yellow[/*4*/] = { 255, 255, 0, 255 };
// static const uint8_t rgba_red[/*4*/] = { 255, 0, 0, 255 };
static const uint8_t rgba_green[/*4*/] = { 0, 255, 0, 255 };
// static const uint8_t rgba_blue[/*4*/] = { 0, 0, 255, 255 };
static const float text_lum = 0.75f;
static const pixel::f4::vec_t vec4_text_color(text_lum, text_lum, text_lum, 1.0f);
static bool debug_gfx = true;

static pixel::bitmap_ref bmp_bunk;
static pixel::texture_ref all_images;
static std::vector<pixel::texture_ref> tex_alien1, tex_alien2, tex_alien3;
static pixel::texture_ref tex_base, tex_peng, tex_alienm;
static pixel::texture_ref tex_bunk[] { nullptr, nullptr, nullptr, nullptr };
int level = 1;

static bool load_textures() {
    {
        pixel::bitmap_ref empty = std::make_shared<pixel::bitmap_t>(64, 64);
        pixel::log_printf(0, "XX empty: %s\n", empty->toString().c_str());
    }
    bmp_bunk = std::make_shared<pixel::bitmap_t>("resources/spaceinv/spaceinv-bunk.png");
    pixel::log_printf(0, "XX bmp bunk: %s\n", bmp_bunk->toString().c_str());

    constexpr int dy1 = 8;  // aliens, base
    constexpr int dy1b = 4; // peng
    constexpr int dy2 = 16; // bunk
    constexpr int dy3 = 7;  // mothership
    constexpr int dx1 = 12; // alien1
    constexpr int dx2 = 11; // alien2
    constexpr int dx3 =  8; // alien3
    constexpr int dx4 = 13; // base
    constexpr int dx5 =  1; // peng
    // constexpr int dx6 = 22; // bunk
    constexpr int dx7 = 16; // mothership
    int y_off=0;
    all_images = std::make_shared<pixel::texture_t>("resources/spaceinv/spaceinv-sprites.png");
    if( !all_images->handle() ) {
        return false;
    }
    pixel::add_sub_textures(tex_alien1, all_images, 0, y_off, dx1, dy1, { {  0*dx1, 0 }, {  1*dx1, 0 }, });
    y_off+=dy1;
    pixel::add_sub_textures(tex_alien2, all_images, 0, y_off, dx2, dy1, { {  0*dx2, 0 }, {  1*dx2, 0 }, });
    y_off+=dy1;
    pixel::add_sub_textures(tex_alien3, all_images, 0, y_off, dx3, dy1, { {  0*dx3, 0 }, {  1*dx3, 0 }, });
    y_off+=dy1;
    tex_base = pixel::add_sub_texture(all_images,   0, y_off, dx4, dy1);
    tex_peng = pixel::add_sub_texture(all_images,  13, y_off, dx5, dy1b);
    y_off+=dy1;
    tex_bunk[0] = std::make_shared<pixel::texture_t>(bmp_bunk);
    tex_bunk[1] = std::make_shared<pixel::texture_t>(bmp_bunk);
    tex_bunk[2] = std::make_shared<pixel::texture_t>(bmp_bunk);
    tex_bunk[3] = std::make_shared<pixel::texture_t>(bmp_bunk);
    y_off+=dy2;
    tex_alienm = pixel::add_sub_texture(all_images, 0, y_off, dx7, dy3);

    pixel::log_printf(0, "XX base: %s\n", tex_base->toString().c_str());
    pixel::log_printf(0, "XX peng: %s\n", tex_peng->toString().c_str());
    pixel::log_printf(0, "XX bunk: %s\n", tex_bunk[0]->toString().c_str());
    pixel::log_printf(0, "XX alien MS: %s\n", tex_alienm->toString().c_str());
    pixel::log_printf(0, "XX alien1: %d textures\n", tex_alien1.size());
    for(const pixel::texture_ref& t : tex_alien1) {
        pixel::log_printf(0, "XX alien1: %s\n", t->toString().c_str());
    }
    pixel::log_printf(0, "XX alien2: %d textures\n", tex_alien2.size());
    for(const pixel::texture_ref& t : tex_alien2) {
        pixel::log_printf(0, "XX alien2: %s\n", t->toString().c_str());
    }
    pixel::log_printf(0, "XX alien3: %d textures\n", tex_alien3.size());
    for(const pixel::texture_ref& t : tex_alien3) {
        pixel::log_printf(0, "XX alien3: %s\n", t->toString().c_str());
    }
    return true;
}

class alient_t {
  private:
    pixel::animtex_t m_atex;
    pixel::f2::vec_t m_dim;

  public:
    pixel::f2::point_t m_tl;
    pixel::f2::vec_t m_velo; // [m/s]

    alient_t(pixel::animtex_t atex_alien, const pixel::f2::point_t& center, const pixel::f2::vec_t& v) noexcept
    : m_atex(std::move(atex_alien)),
      m_dim((float)m_atex.width(), (float)m_atex.height()),
      m_tl( center + pixel::f2::point_t(-m_dim.x/2, +m_dim.y/2) ),
      m_velo( v )
    { }

    pixel::f2::aabbox_t box() const noexcept {
        return pixel::f2::aabbox_t().resize(m_tl).resize(m_tl.x + m_dim.x, m_tl.y - m_dim.y);
    }

    void tick(const float dt) noexcept {
        m_atex.tick(dt);
        m_tl += m_velo * dt;
        /*
        if( !box().inside(field_box) ) {
            m_velo.x *= -1;
            m_tl += 2 * m_velo * dt;
        }
        */
    }

    void draw() const noexcept {
        // printf("XXX: alien %s\n", m_atex.toString().c_str());
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
    pixel::f2::vec_t m_velo;
  public:
    std::vector<alient_t> items;

    alien_group_t() = default;
    alien_group_t(const class alien_group_t&)            = default;
    alien_group_t(class alien_group_t&&)                 = default;
    class alien_group_t& operator=(const class alien_group_t&) = default;
    class alien_group_t& operator=(class alien_group_t&&)      = default;

    void reset() {
        const float cell_height = (float)tex_alien1[0]->height;
        const float cell_width = (float)tex_alien1[0]->width;
        const float sec_per_atex = 1.0f;
        items.clear();
        m_velo = pixel::f2::vec_t(3, 0);
        pixel::f2::point_t p = field_box.bl;
        p.y =  0.0f;
        for(int y=0; y<2; ++y) {
            const pixel::animtex_t atex_alien("Alien1", sec_per_atex, tex_alien1);
            p.x = field_box.bl.x + cell_width/2.0f;
            for(int x=0; x<11; ++x) {
                items.emplace_back(atex_alien, p, m_velo);
                p.x += cell_width * 1.5f;
            }
            p.y +=  2.0f * cell_height;
        }
        for(int y=0; y<2; ++y) {
            const pixel::animtex_t atex_alien("Alien2", sec_per_atex, tex_alien2);
            p.x = field_box.bl.x + cell_width/2.0f;
            for(int x=0; x<11; ++x) {
                items.emplace_back(atex_alien, p, m_velo);
                p.x += cell_width * 1.5f;
            }
            p.y +=  2.0f * cell_height;
        }
        for(int y=0; y<1; ++y) {
            const pixel::animtex_t atex_alien("Alien3", sec_per_atex, tex_alien3);
            p.x = field_box.bl.x + cell_width/2.0f;
            for(int x=0; x<11; ++x) {
                items.emplace_back(atex_alien, p, m_velo);
                p.x += cell_width * 1.5f;
            }
            p.y +=  2.0f * cell_height;
        }
    }

    void tick(const float dt){
        m_velo.y = 0;
        for(alient_t& a : items){
            if( !a.box().inside(field_box) ) {
                if(pixel::get_gpu_fps() > 60){
                    m_velo.x *= -1;
                    m_velo.y = -(float)a.atex().height();
                }
                m_velo.x *= -1;
                m_velo.y = -(float)a.atex().height() * pixel::get_gpu_fps();
                goto mark;
            }
        }
        mark:
        int h=0;
        for(alient_t& a : items){
            a.m_velo = m_velo;
            a.tick(dt);
            h = a.atex().height();
        }
        if(m_velo.y != 0){ // FIXME: debug code to be deleted
            printf("texheight: %d, Velo.y: %f, fps: %fs\n", h, m_velo.y, pixel::get_gpu_fps());
        }
        m_velo.y = 0;
    }

    void draw(){
        for(alient_t& a : items){
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

    void hit(pixel::f2::aabbox_t& hitbox) {
        const pixel::f2::aabbox_t myself = box();
        pixel::f2::aabbox_t ibox = hitbox.intersection(myself);
        if( ibox.width() > 0 ) {
            printf("XXX bunk.hit: hit    %s\n", hitbox.toString().c_str());
            printf("XXX bunk.hit: bunk   %s\n", myself.toString().c_str());
            printf("XXX bunk.hit: ibox.0 %s\n", ibox.toString().c_str());
            pixel::f2::point_t bl { m_tl.x, m_tl.y - m_dim.y };
            ibox.bl -= bl;
            ibox.tr -= bl;
            printf("XXX bunk.hit: ibox.1 %s\n", ibox.toString().c_str());
            for(uint32_t y=(uint32_t)ibox.bl.y; y<(uint32_t)ibox.tr.y; ++y) {
                for(uint32_t x=(uint32_t)ibox.bl.x; x<(uint32_t)ibox.tr.x; ++x) {
                    m_bunk->put(x, y, 0xffffffff);
                }
            }
            tex_bunk[idx]->update(m_bunk);
        }
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

void reset_bunks() {
    bunks.clear();
    bunks.emplace_back(0, bunk1_tl);
    bunks.emplace_back(1, bunk2_tl);
    bunks.emplace_back(2, bunk3_tl);
    bunks.emplace_back(2, bunk4_tl);
}


void reset_items() {
    reset_bunks();
    alien_group.reset();
}

struct hit_result{
    bool hit;
    bool live;
};

class peng_t {
  private:
    pixel::f2::vec_t m_dim;
    pixel::f2::point_t m_tl;
    bool m_alien_hit;

    bool check_alien_hit() {
        if( m_owner == alien_id ) {
            return false;
        }
        pixel::f2::aabbox_t b = box();
        for(auto it = alien_group.items.begin(); it != alien_group.items.end(); ) {
            alient_t& a = *it;
            if( !a.box().intersects(b) ) {
                ++it;
            } else {
                it = alien_group.items.erase(it);
                m_alien_hit = true;
                return true;
            }
        }
        return false;
    }

    bool check_bunker_hit() {
        pixel::f2::aabbox_t b = box();
        for(auto it = bunks.begin(); it != bunks.end(); ) {
            bunker_t& bunk = *it;
            if( !bunk.box().intersects(b) ) {
                ++it;
            } else {
                bunk.hit(b);
                return true;
            }
        }
        return false;
    }
  public:
    pixel::f2::vec_t m_velo; // [m/s]
    int m_owner;

    peng_t(const pixel::f2::point_t& center, const pixel::f2::vec_t& v, const int owner) noexcept
    : m_dim((float)tex_peng->width, (float)tex_peng->height),
      m_tl( center + pixel::f2::point_t(-m_dim.x/2, +m_dim.y/2) ),
      m_alien_hit(false), m_velo( v ), m_owner(owner)
    { }

    pixel::f2::aabbox_t box() const noexcept {
        return pixel::f2::aabbox_t().resize(m_tl).resize(m_tl.x + m_dim.x, m_tl.y - m_dim.y);
    }

    bool tick(const float dt) noexcept {
        if( !box().inside(field_box) ) {
            return false;
        }
        if(!m_velo.is_zero()){
            m_tl += m_velo * dt;
        }
        return !check_alien_hit() && !check_bunker_hit();
    }

    void draw() const noexcept {
        tex_peng->draw(m_tl.x, m_tl.y, m_dim.x, m_dim.y);
    }

    bool on_screen(){
        return box().inside(field_box);
    }

    bool intersection(const peng_t& o) const {
        return box().intersects(o.box());
    }

    bool alien_hit() const { return m_alien_hit; }
};
std::vector<peng_t> pengs;

class spaceship_t {
    public:
        constexpr static const float height = base_height; // [m]

        constexpr static const float peng_velo_0 = field_height / 2; // [m/s]
        constexpr static const int peng_inventory_max = 3;

    private:
        pixel::f2::vec_t m_dim;
        pixel::f2::point_t m_tl, m_tc;

        bool check_hit() {
            pixel::f2::aabbox_t b = box();
            for(auto it = pengs.begin(); it != pengs.end(); ) {
                peng_t& a = *it;
                if( !a.box().intersects(b) ) {
                    ++it;
                } else {
                    it = pengs.erase(it);
                    // printf("XXX: Peng: Killed Ship\n");
                    return true;
                }
            }
            return false;
        }
    public:
        int peng_inventory;

        spaceship_t(const pixel::f2::point_t& top_center) noexcept
        : m_dim((float)tex_base->width, (float)tex_base->height),
          m_tl(top_center.x - m_dim.x/2.0f, top_center.y), m_tc(top_center),
          peng_inventory(peng_inventory_max)
        {}

        pixel::f2::aabbox_t box() const noexcept {
            return pixel::f2::aabbox_t().resize(m_tl).resize(m_tl.x + m_dim.x, m_tl.y - m_dim.y);
        }

        void peng() noexcept {
            if(peng_inventory > 0){
                // adjust start posision to geometric ship model
                pixel::f2::point_t p0 = {m_tc.x, m_tc.y + (float)tex_peng->height/2 + 0.05f};
                pixel::f2::vec_t v_p = pixel::f2::vec_t::from_length_angle(peng_velo_0 + 10.0f * (float)level, 90_deg);
                pengs.emplace_back(p0, v_p, ship_id);
                --peng_inventory;
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
        bool tick(const float /*dt*/) noexcept {
            return !check_hit();
        }

        void draw() const noexcept {
            tex_base->draw(m_tl.x, m_tl.y, m_dim.x, m_dim.y);
        }
};
typedef std::shared_ptr<spaceship_t> spaceship_ref_t;
std::vector<spaceship_ref_t> spaceship;

class player_t {
    public:
        int m_live;
        spaceship_ref_t m_ship;
    private:
        constexpr static int start_live = 3;
        float m_respawn_timer;
        int m_score;

        void ship_dtor() noexcept {
            --m_live;
            m_ship = nullptr;
            m_respawn_timer = 3; // [s]
        }

        void respawn_ship() noexcept {
            if(m_live <= 0){
                return;
            }
            m_respawn_timer = 0;
            m_ship = std::make_shared<spaceship_t>( pixel::f2::point_t{base_box.bl.x + base_width/2, base_box.bl.y + base_height} );
        }

    public:
        player_t() noexcept
        : m_live(start_live),
          m_ship(nullptr), m_respawn_timer(0), m_score(0)
        { respawn_ship(); }

        void reset() noexcept {
            respawn_ship();
            m_score = 0;
            m_live = start_live;
        }
        spaceship_ref_t& ship() noexcept { return m_ship; }
        bool has_ship() noexcept { return nullptr != m_ship; }

        int peng_inventory() const noexcept { return nullptr != m_ship ? m_ship->peng_inventory : 0; }

        constexpr int score() const noexcept { return m_score; }
        void peng_completed(const peng_t& p) noexcept {
            if(has_ship())  {
                ++m_ship->peng_inventory;
            }
            if(p.alien_hit()){
                add_score(1);
            }
        }
        void add_score(int diff) noexcept { m_score += diff; }

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
            if( m_ship != nullptr ) {
                m_ship->draw();
            }
        }
        void handle_event0() noexcept {
            if( nullptr != m_ship && event.has_any_p1() ) {
                if( event.released_and_clr(pixel::input_event_type_t::P2_ACTION2) ) {
                    m_ship->peng();
                }
            }
        }
        void handle_event1(const float dt /* [s] */) noexcept {
            const float v = 35.0f;
            if( nullptr != m_ship && event.has_any_p1() ) {
                if( event.pressed(pixel::input_event_type_t::P1_LEFT) ){
                    m_ship->move( { -v * dt, 0 } );
                } else if( event.pressed(pixel::input_event_type_t::P1_RIGHT) ){
                    m_ship->move( { v * dt, 0 } );
                }
            }
        }
};
void peng_from_alien(alient_t alien){
    // adjust start posision to geometric alien model
    pixel::f2::point_t p0 = {alien.m_tl.x + (float)alien.atex().width()/2,
                             alien.m_tl.y - (float)alien.atex().height() - 0.05f};
    p0.add(0, (float)tex_peng->height/2);
    pixel::f2::vec_t v_p = pixel::f2::vec_t::from_length_angle(field_height/2 + 10.0f * (float)level, 270_deg);
    pengs.emplace_back(p0, v_p, alien_id);
    //printf("peng");
}

void peng_alien() {
    //printf("XXX: Alien shooted");
    if(alien_group.items.size() <= 0){
        return;
    }
    peng_from_alien(alien_group.items[(size_t)(pixel::next_rnd() * (float)(alien_group.items.size()-1))]);
}
static pixel::f2::point_t tl_text;
static std::string record_bmpseq_basename;
static bool raster = false;

void mainloop() {
    static player_t p1;
    static uint64_t frame_count_total = 0;
    static uint64_t snap_count = 0;
    static uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]
    static const int text_height = 24;
    static bool animating = true;
    static pixel::texture_ref game_over_text = pixel::make_text(tl_text, 0, {1, 0, 0, 1}, text_height*5 , "GAME OVER");
    static bool game_over = false;
    constexpr static float max_peng_time = 1_s;
    static float peng_time = pixel::next_rnd() * max_peng_time;
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
            pengs.clear();
            reset_items();
            p1.reset();
            game_over = false;
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
                    if(p.m_owner == ship_id){
                        p1.peng_completed(p);
                    }
                    it = pengs.erase(it);
                }
            }
        }
        peng_time -= dt;
        if(peng_time <= 0){
            peng_alien();
            if((float)level < max_peng_time){
                peng_time = pixel::next_rnd() * (max_peng_time - ((float)level-1));
            } else {
                peng_time = pixel::next_rnd() * (max_peng_time - (float)level+1);
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
    p1.draw();
    for(auto & bunk : bunks) {
        bunk.draw();
    }
    // printf("XXX: aliens %zu\n", aliens.size());
    alien_group.draw();
    for(auto & peng : pengs) {
        peng.draw();
    }

    if(alien_group.items.size() == 0){
        pengs.clear();
        reset_items();
        p1.reset();
        ++level;
    }
    pixel::set_pixel_color(255, 255, 255, 255);

    float fps = pixel::get_gpu_fps();
    tl_text.set(pixel::cart_coord.min_x(), pixel::cart_coord.max_y());
    pixel::texture_ref hud_text = pixel::make_text(tl_text, 0, vec4_text_color, text_height, "%s s, fps %4.2f, score %4d, lives %d, level %d",
                    pixel::to_decstring(t1/1000, ',', 5).c_str(), // 1d limit
                    fps, p1.score(), p1.m_live, level);
    for(alient_t& a : alien_group.items){
        if(a.box().bl.y < -62-bunks[0].box().height() || p1.m_live <= 0){
            game_over = true;
            animating = false;
        }
    }
    pixel::swap_pixel_fb(false);
    {
        const int dx = ( pixel::fb_width - pixel::round_to_int((float)hud_text->width*(float)hud_text->dest_sx) ) / 2;
        hud_text->draw_fbcoord(dx, 0);
    }
    if(game_over){
        const int dx = ( pixel::fb_width - pixel::round_to_int((float)game_over_text->width*game_over_text->dest_sx) ) / 2;
        game_over_text->draw_fbcoord(dx, 0);
    }
    pixel::swap_gpu_buffer(60);
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
    reset_items();
    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
    return 0;
}
