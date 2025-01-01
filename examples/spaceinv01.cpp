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

#include <algorithm>
#include <random>
#include <cinttypes>
#include <cstdio>
#include <cmath>
#include <iostream>

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
 * - Bunker   22 x  16  1: 32/192, 2: 77/192, 3: 122/192, 4: 176/192
 * - Aline-M  16 x   8  (mothership)
 * - Peng      1 x   4
*/
constexpr static float space_height = 260.0f; // [m]
constexpr static float space_width = 224.0f; // [m]
constexpr static float space_width_pct = space_width / space_height;
constexpr static float field_height = 184.0f; // [m]
constexpr static float field_width = 204.0f; // [m]
static const pixel::f2::aabbox_t field_box( { -field_width/2.0, -field_height/2.0 }, { field_width/2.0, field_height/2.0 } );
static const pixel::f2::vec_t alien_dim( 172, 72 );

constexpr static const float base_width = 13.0f; // [m]
constexpr static const float base_height = 8.0f; // [m]

constexpr static float bunk_width = 22.0f;
constexpr static float bunk_height = 16.0f;

static const pixel::f2::point_t bunk1_tl( -80, -62 );
static const pixel::f2::point_t bunk2_tl( -35, -62 );
static const pixel::f2::point_t bunk3_tl(  10, -62 );
static const pixel::f2::point_t bunk4_tl(  55, -62 );

static const pixel::f2::aabbox_t base_box( { bunk1_tl.x-base_width, -field_height/2.0 }, { bunk4_tl.x+bunk_width+base_width, field_height/2.0 } );

static const uint8_t rgba_white[/*4*/] = { 255, 255, 255, 255 };
// static const uint8_t rgba_yellow[/*4*/] = { 255, 255, 0, 255 };
static const uint8_t rgba_red[/*4*/] = { 255, 0, 0, 255 };
static const uint8_t rgba_green[/*4*/] = { 0, 255, 0, 255 };
// static const uint8_t rgba_blue[/*4*/] = { 0, 0, 255, 255 };
static const float text_lum = 0.75f;
static const pixel::f4::vec_t vec4_text_color(text_lum, text_lum, text_lum, 1.0f);
// static bool debug_gfx = false;

static pixel::texture_ref all_images;
static std::vector<pixel::texture_ref> tex_alien1, tex_alien2, tex_alien3;
static pixel::texture_ref tex_base, tex_peng, tex_bunk, tex_alienm;

static void load_textures() {
    constexpr int dy1 = 8;  // aliens, base, mothership
    constexpr int dy1b = 4; // peng
    constexpr int dy2 = 16; // bunk
    constexpr int dx1 = 12; // alien1
    constexpr int dx2 = 11; // alien2
    constexpr int dx3 =  8; // alien3
    constexpr int dx4 = 13; // base
    constexpr int dx5 =  1; // peng
    constexpr int dx6 = 22; // bunk
    constexpr int dx7 = 16; // mothership
    int y_off=0;
    all_images = std::make_shared<pixel::texture_t>("resources/spaceinv/spaceinv-sprites.png");
    pixel::add_sub_textures(tex_alien1, all_images, 0, y_off, dx1, dy1, { {  0*dx1, 0 }, {  1*dx1, 0 }, });
    y_off+=dy1;
    pixel::add_sub_textures(tex_alien2, all_images, 0, y_off, dx2, dy1, { {  0*dx2, 0 }, {  1*dx2, 0 }, });
    y_off+=dy1;
    pixel::add_sub_textures(tex_alien3, all_images, 0, y_off, dx3, dy1, { {  0*dx3, 0 }, {  1*dx3, 0 }, });
    y_off+=dy1;
    tex_base = pixel::add_sub_texture(all_images,   0, y_off, dx4, dy1);
    tex_peng = pixel::add_sub_texture(all_images,  13, y_off, dx5, dy1b);
    y_off+=dy1;
    tex_bunk = pixel::add_sub_texture(all_images,   0, y_off, dx6, dy2);
    y_off+=dy2;
    tex_alienm = pixel::add_sub_texture(all_images, 0, y_off, dx7, dy1);

    pixel::log_printf(0, "XX base: %s\n", tex_base->toString().c_str());
    pixel::log_printf(0, "XX peng: %s\n", tex_peng->toString().c_str());
    pixel::log_printf(0, "XX bunk: %s\n", tex_bunk->toString().c_str());
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
}

class alient_t {
  private:
    pixel::animtex_t m_atex;
    pixel::f2::vec_t m_dim;
    pixel::f2::point_t m_tl;

  public:
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
        if(!m_velo.is_zero()){
            m_tl += m_velo * dt;
            if( !box().inside(field_box) ) {
                m_velo.x *= -1;
                m_tl += 2 * m_velo * dt;
            }
        }
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
};
std::vector<alient_t> aliens;

class bunker_t {
  private:
    pixel::f2::vec_t m_dim;
    pixel::f2::point_t m_tl;

    bool hits_fragment() noexcept {
        bool hit = false;
        return hit;
    }

  public:
    bunker_t(const pixel::f2::point_t& tl) noexcept
    : m_dim((float)tex_bunk->width, (float)tex_bunk->height),
      m_tl(tl)
    { }

    pixel::f2::aabbox_t box() const noexcept {
        return pixel::f2::aabbox_t().resize(m_tl).resize(m_tl.x + m_dim.x, m_tl.y - m_dim.y);
    }

    void draw() const noexcept {
        tex_bunk->draw(m_tl.x, m_tl.y, m_dim.x, m_dim.y);
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
    bunks.emplace_back(bunk1_tl);
    bunks.emplace_back(bunk2_tl);
    bunks.emplace_back(bunk3_tl);
    bunks.emplace_back(bunk4_tl);
}

void reset_aliens() {
    const float cell_height = (float)tex_alien1[0]->height;
    const float cell_width = (float)tex_alien1[0]->width;
    const float sec_per_atex = 1.0f;
    aliens.clear();
    pixel::f2::vec_t velo(3, 0);
    // pixel::f2::point_t p(-(field_box.width() - alien_dim.x)/2, -(field_box.height() - alien_dim.y)/2);
    pixel::f2::point_t p = field_box.bl;
    p.y =  -2.0f * cell_height;
    for(int y=0; y<2; ++y) {
        const pixel::animtex_t atex_alien("Alien1", sec_per_atex, tex_alien1);
        p.x = field_box.bl.x + cell_width/2.0f;
        for(int x=0; x<11; ++x) {
            aliens.emplace_back(atex_alien, p, velo);
            p.x += cell_width * 1.5f;
        }
        p.y +=  2.0f * cell_height;
    }
    for(int y=0; y<2; ++y) {
        const pixel::animtex_t atex_alien("Alien2", sec_per_atex, tex_alien2);
        p.x = field_box.bl.x + cell_width/2.0f;
        for(int x=0; x<11; ++x) {
            aliens.emplace_back(atex_alien, p, velo);
            p.x += cell_width * 1.5f;
        }
        p.y +=  2.0f * cell_height;
    }
    for(int y=0; y<1; ++y) {
        const pixel::animtex_t atex_alien("Alien3", sec_per_atex, tex_alien3);
        p.x = field_box.bl.x + cell_width/2.0f;
        for(int x=0; x<11; ++x) {
            aliens.emplace_back(atex_alien, p, velo);
            p.x += cell_width * 1.5f;
        }
        p.y +=  2.0f * cell_height;
    }
}

void reset_items() {
    reset_bunks();
    reset_aliens();
}

class peng_t {
  private:
    pixel::f2::vec_t m_dim;
    pixel::f2::point_t m_tl;

    bool hits_aliens() {
        pixel::f2::aabbox_t b = box();
        for(auto it = aliens.begin(); it != aliens.end(); ) {
            alient_t& a = *it;
            if( !a.box().intersects(b) ) {
                ++it;
            } else {
                it = aliens.erase(it);
                printf("XXX: Peng: Killed ALien\n");
                return true;
            }
        }
        return false;
    }
  public:
    pixel::f2::vec_t m_velo; // [m/s]

    peng_t(const pixel::f2::point_t& center, const pixel::f2::vec_t& v) noexcept
    : m_dim((float)tex_peng->width, (float)tex_peng->height),
      m_tl( center + pixel::f2::point_t(-m_dim.x/2, +m_dim.y/2) ), m_velo( v )
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
        return !hits_aliens();
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
};
std::vector<peng_t> pengs;

class spaceship_t {
    public:
        constexpr static const float height = base_height; // [m]

        constexpr static const float peng_velo_0 = field_height / 1; // [m/s]
        constexpr static const int peng_inventory_max = 4000;

    private:
        pixel::f2::vec_t m_dim;
        pixel::f2::point_t m_tl, m_tc;

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
                pixel::f2::point_t p0 = m_tc;
                pixel::f2::vec_t v_p = pixel::f2::vec_t::from_length_angle(peng_velo_0, 90_deg);
                pengs.emplace_back(p0, v_p);
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
            return true;
        }

        void draw() const noexcept {
            tex_base->draw(m_tl.x, m_tl.y, m_dim.x, m_dim.y);
        }
};
typedef std::shared_ptr<spaceship_t> spaceship_ref_t;
std::vector<spaceship_ref_t> spaceship;

class player_t {
    private:
        float m_respawn_timer;
        spaceship_ref_t m_ship;
        int m_score;

        void ship_dtor() noexcept {
            m_ship = nullptr;
            m_respawn_timer = 5; // [s]
        }

        void respawn_ship() noexcept {
            m_respawn_timer = 0;
            m_ship = std::make_shared<spaceship_t>( pixel::f2::point_t{base_box.bl.x + base_width/2, base_box.bl.y + base_height} );
        }

    public:
        player_t() noexcept
        : m_respawn_timer(0),
          m_ship(nullptr), m_score(0)
        { respawn_ship(); }

        void reset() noexcept {
            respawn_ship();
            m_score = 0;
        }
        spaceship_ref_t ship() noexcept { return m_ship; }
        bool has_ship() noexcept { return nullptr != m_ship; }

        int peng_inventory() const noexcept { return nullptr != m_ship ? m_ship->peng_inventory : 0; }

        constexpr int score() const noexcept { return m_score; }
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

static pixel::f2::point_t tl_text;
static std::string record_bmpseq_basename;
static bool raster = false;


void mainloop() {
    static player_t p1;

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
            reset_items();
            p1.reset();
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
/*
        for(size_t i = 0; i < aliens.size(); ++i){
            alient_t& alien = aliens[i];
            if(i == 0){
                continue;
            }
            if(!aliens[i-1].on_screen()){
                alien.m_velo.x *= -1;
            }
        }
*/
        // alien tick
        for(alient_t& a: aliens) {
            a.tick(dt);
        }

        // pengs tick
        {
            for(auto it = pengs.begin(); it != pengs.end(); ) {
                peng_t& p = *it;
                if( p.tick(dt) ) {
                    ++it;
                } else {
                    it = pengs.erase(it);
                }
            }
        }
    }
    pixel::clear_pixel_fb(0, 0, 0, 255);

    // Draw all objects
    pixel::set_pixel_color(rgba_white);
    p1.draw();
    for(auto & bunk : bunks) {
        bunk.draw();
    }
    // printf("XXX: aliens %zu\n", aliens.size());
    for(auto & alien : aliens) {
        alien.draw();
    }
    for(auto & peng : pengs) {
        peng.draw();
    }
    pixel::set_pixel_color(255, 255, 255, 255);

    float fps = pixel::get_gpu_fps();
    tl_text.set(pixel::cart_coord.min_x(), pixel::cart_coord.max_y());
    pixel::texture_ref hud_text = pixel::make_text(tl_text, 0, vec4_text_color, text_height, "%s s, fps %4.2f, score %4d",
                    pixel::to_decstring(t1/1000, ',', 5).c_str(), // 1d limit
                    fps, p1.score());
    pixel::swap_pixel_fb(false);
    {
        const int dx = ( pixel::fb_width - pixel::round_to_int((float)hud_text->width*hud_text->dest_sx) ) / 2;
        hud_text->draw_fbcoord(dx, 0);
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
    load_textures();
    reset_items();

    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
}
