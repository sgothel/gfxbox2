/*
 * Author: Hermann Kai Gothel <hkg@jausoft.com>
 * Copyright (c) 2026 Gothel Software e.K.
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
#include <memory>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include <pixel/pixel.hpp>
#include <pixel/dynarray2d.hpp>

#include <iostream>
#include <fstream>

#include <cstdio>
#include <cmath>
#include <pixel/pixel4f.hpp>
#include <physics.hpp>
#include <vector>
#include <jau/float_si_types.hpp>
#include <jau/utils.hpp>
using namespace jau;
using namespace jau::si_f32_literals;
using namespace pixel::f2;

static constexpr bool DEBUG_ON = false;

//
// tile_t
//
enum class tile_t : int {
    EMPTY = 0,
    WALL = 1,
    GATE = 2,
    PELLET = 3,
    PELLET_POWER = 4,
    CHERRY = 5,
    STRAWBERRY = 6,
    PEACH = 7,
    APPLE = 8,
    MELON = 9,
    GALAXIAN = 10,
    BELL = 11,
    KEY = 12
};

// Orientation: 0/0 bottom-left
static DynArray2D<bool> walls;
point_t fritz_start;
static std::vector<pixel::f2::lineseg_t> h_lines;
static std::vector<pixel::f2::lineseg_t> v_lines;

bool digest_fposition_line(const std::string& name, point_t& dest, const std::string& line) noexcept {
    float x_pos = 0.0f, y_pos = 0.0f;
    sscanf(line.c_str(), "%f %f", &x_pos, &y_pos);
    dest.set(x_pos, y_pos);
    if( DEBUG_ON ) {
        jau::log_printf("maze: read %s position: %s\n", name.c_str(), dest.toString().c_str());
    }
    return true;
}

void read_maze(const std::string& fname0) noexcept
{
    h_lines.clear();
    v_lines.clear();
    size_t width = 0, height = 0;
    const std::string fname1 = pixel::resolve_asset(fname0);
    if( !fname1.size() ) {
        jau::log_printf("maze_t: Could locate file '%s' in asset dir '%s'\n", fname0.c_str(), pixel::asset_dir().c_str());
        return;
    }
    {
        size_t field_line_iter = 0;
        std::fstream file;
        file.open(fname1, std::ios::in);
        if( file.is_open() ) {
            std::string line;
            size_t linenum=0;
            int y = 0;
            while( std::getline(file, line) ) {
                if( 0 == linenum ) {
                    int w=-1, h=-1;
                    int visual_width=-1, visual_height=-1;
                    sscanf(line.c_str(), "%d %d %d %d", &w, &h, &visual_width, &visual_height);
                    width = w;
                    height = h;
                    walls = DynArray2D<bool>(width, height);
                    walls.clear(false);
                    if( DEBUG_ON ) {
                        jau::log_printf("maze: read dimension: %dx%d\n", width, height);
                    }
                } else if( 1 == linenum ) {
                    digest_fposition_line("fritz pos", fritz_start, line);
                } else if( field_line_iter < height ) {
                    if( DEBUG_ON ) {
                        jau::log_printf("maze: read line y = %d, len = %zd: %s\n", field_line_iter, line.length(), line.c_str());
                    }
                    const size_t maxlen = std::min(line.length(), width);
                    for(size_t x=0; x<maxlen; ++x) {
                        const char c = line[x];
                        switch( c ) {
                            case '_':
                                [[fallthrough]];
                            case ' ':
                                // original.add_tile(tile_t::EMPTY);
                                //printf("_");
                                break;
                            case '|':
                                [[fallthrough]];
                            case '+':
                                [[fallthrough]];
                            case '-':
                                // original.add_tile(tile_t::WALL);
                                walls[x, height-1-y] = true;
                                //printf("|");
                                break;
                            case '.':
                                // original.add_tile(tile_t::PELLET);
                                //printf(".");
                                break;
                            case '*':
                                // original.add_tile(tile_t::PELLET_POWER);
                                //printf("*");
                                break;
                            default:
                                jau::log_printf("maze error: unknown tile @ %d / %d: '%c'\n", x, field_line_iter, c);
                                break;
                        }
                    }
                    ++field_line_iter;
                    ++y;
                }
                ++linenum;
            }
            file.close();
        } else {
            jau::log_printf("Could not open maze file: %s\n", fname0.c_str());
        }
        {
            using pixel::f2::point_t;
            for(size_t y = 0; y < walls.rows(); ++y) {
                point_t left;
                point_t right;
                bool has_pre = false;
                for(size_t x = 0; x < walls.columns(); ++x) {
                    if(walls[x, y]) {
                        if(!has_pre) {
                            left = {(float)x, (float)y};
                            has_pre = true;
                        }
                        right = {(float)x, (float)y};
                    } else if (has_pre) {
                        h_lines.emplace_back(left, right);
                        has_pre = false;
                    }
                }
                if (has_pre) {
                    h_lines.emplace_back(left, right);
                    has_pre = false;
                }
            }
            for(size_t x = 0; x < walls.columns(); ++x) {
                point_t bottom;
                point_t top;
                bool has_pre = false;
                for(size_t y = 0; y < walls.rows(); ++y) {
                    if(walls[x, y]) {
                        if(!has_pre) {
                            bottom = {(float)x, (float)y};
                            has_pre = true;
                        }
                        top = {(float)x, (float)y};
                    } else if (has_pre) {
                        v_lines.emplace_back(bottom, top);
                        has_pre = false;
                    }
                }
                if (has_pre) {
                    v_lines.emplace_back(bottom, top);
                    has_pre = false;
                }
            }
        }
    }
}

void kreis(pixel::f2::point_t m, float r) {
    using pixel::f2::point_t;
    point_t p0 {m.x - r, m.y - r};
    point_t p1 {m.x + r, m.y + r};
    for (float y = p0.y; y <= p1.y; ++y) { // NOLINT
        for (float x = p0.x; x <= p1.x; ++x) { // NOLINT
            float a = m.y - y;
            float b = m.x - x;
            float c = sqrtf(a * a + b * b);
            if (c <= r) {
                point_t p = { x, y};
                p.draw();
            }
        }
    }
}

class Person {
  private:
    constexpr static si_length_f32 height = 1;
    constexpr static si_length_f32 width = height * (8.0f/5.0f);
    constexpr static si_length_f32 shoulder_height = height*0.6f;
    constexpr static si_length_f32 arm_width = width*0.20f;
    constexpr static si_length_f32 leg_width = width*0.25f;
    constexpr static float max_ausdauer = 10;
    constexpr static si_angle_f32 start_angle = 0_deg;
    constexpr static si_length_f32 step = height/2;
    constexpr static si_length_f32 nose_length = height/4;
    constexpr static si_velo_f32 def_velocity = 2; // [m/s]

    disk_t m_head;
    rect_t m_shoulders;
    lineseg_t m_nose;
    aabbox_t m_aabbox;
    vec_t m_velocity; // [m/s]
    si_velo_f32 m_velo_scalar;
    si_angle_f32 m_angle;
    float m_ausdauer;

  private:
    void rotateInt(si_angle_f32 rad) {
        if((int)rad % (int)360_deg == 0) {
            return;
        }
        m_shoulders.rotate(rad);
        m_nose.rotate(rad, m_head.center);
        m_velocity.rotate(rad);
        m_angle += rad;
    }

    bool colision(vec_t){
        // if(man.pos==maze2.pos){
        {
        }
        return false;
    }

  public:

    Person(const point_t pos)
    : m_head(pos, height / 2), m_aabbox(), m_angle(start_angle), m_ausdauer(max_ausdauer) {
        const point_t tl = point_t(pos.x - width/2, pos.y + shoulder_height/2);
        m_shoulders = rect_t(tl, width, shoulder_height);
        rotateInt(-90_deg); // links
        m_angle -= 90_deg; // links
        m_nose = lineseg_t(m_head.center+vec_t(-m_head.radius, 0), m_head.center+vec_t(-(m_head.radius + nose_length), 0));
        m_aabbox.resize(m_head.box());
        m_aabbox.resize(m_shoulders.box());
        m_aabbox.resize(m_nose.box());
        gobjects().push_back(std::make_shared<disk_t>(m_head));
        gobjects().push_back(std::make_shared<rect_t>(m_shoulders));
        gobjects().push_back(std::make_shared<lineseg_t>(m_nose));
    }

    Person() = default;

    float angle() { return m_angle; }

    void setVelo(const vec_t &v) { m_velocity = v; m_velo_scalar = v.length(); }
    void setVelo(const float angle, const float speed=def_velocity) noexcept {
        m_velocity = vec_t::from_length_angle(speed, angle);
        m_velo_scalar = speed;
        m_angle = angle;
    }

    bool tick(const float dt) noexcept { // override {
        // pixel::f2::vec_t g = sun->gravity_ships(p_center);
        // velocity += g * dt; // [m/s^2] * [s] = [m/s]

        move(m_velocity * dt); // [m/s] * [s] = [m]
        if (m_velo_scalar > def_velocity) {
            m_ausdauer -= dt;
            if (m_ausdauer < 0) {
                m_ausdauer = 0;
                setVelo(m_angle, def_velocity);
            }
            m_ausdauer = std::max(m_ausdauer-dt, 0.0f);
        }
        return true;
    }

    bool move(const vec_t move_vec) {
        aabbox_t aabbox = m_aabbox;
        aabbox.move(move_vec);
#if 0
        {
            const int l = int(aabbox.bl.x+0.5f); // NOLINT
            const int b = int(aabbox.bl.y+0.5f); // NOLINT
            const int r = int(aabbox.tr.x+0.5f); // NOLINT
            const int t = int(aabbox.tr.y+0.5f); // NOLINT
            bool hlb = walls[l, b];
            bool hrb = walls[r, b];
            bool hlt = walls[l, t];
            bool hrt = walls[r, t];
            // if (walls[l, b] || walls[r, b] || walls[l, t] || walls[r, t]) {
            if (hlb || hrb || hlt || hrt) {
                printf("XXX: aabbox %s, walls %zu x %zu: l %d, b %d, r %d, t %d; hlb %d, hrb %d, hlt %d, hrt %d\n",
                    aabbox.toString().c_str(), walls.columns(), walls.rows(), l, b, r, t,
                    hlb, hrb, hlt, hrt);
                return false;
            }
        }
#else
        {
            point_t c = aabbox.center();
            const int x = int(c.x+0.5f); // NOLINT
            const int y = int(c.y+0.5f); // NOLINT
            bool hit = walls[x, y];
            // if (walls[l, b] || walls[r, b] || walls[l, t] || walls[r, t]) {
            if (hit) {
                m_velocity = vec_t();
                printf("XXX: aabbox %s, walls %zu x %zu: %d / %d\n",
                    aabbox.toString().c_str(), walls.columns(), walls.rows(), x, y);
                return false;
            }
        }
#endif
        m_aabbox = std::move(aabbox);

        m_head.move(move_vec);
        m_shoulders.move(move_vec);
        m_nose.move(move_vec);
        return true;
    }

    void rotate(si_angle_f32 rad) {
        if((int)rad % (int)360_deg == 0) {
            return;
        }
        m_shoulders.rotate(rad);
        m_nose.rotate(rad, m_head.center);
        m_velocity.rotate(rad);
        m_angle += rad;
        // m_aabbox.rotate(rad);
        m_aabbox.reset();
        m_aabbox.resize(m_head.box());
        m_aabbox.resize(m_shoulders.box());
        m_aabbox.resize(m_nose.box());
    }
    void rotateTo(const si_angle_f32 rad) {
        rotate(rad-m_angle);
    }
    point_t pos() { return m_head.center; }
    void draw() {
        m_head.draw(false);
        m_shoulders.draw(false);
        m_nose.draw();
        m_aabbox.draw();
    }
};

Person fritz;
physiks::ball_ref_t ball;

void draw_grid(const float size, pixel::f4::vec_t color) {
    pixel::set_pixel_color(color);
    for(int i = -1; i < 28; ++i) {
        lineseg_t::draw(point_t(size * float(i)+0.5f, 0+0.5f), point_t(size*float(i)+0.5f, pixel::cart_coord.max_y()+0.5f));
    }
    for(int i = 0; float(i) < pixel::cart_coord.max_y(); ++i) {
        lineseg_t::draw(point_t(0-0.5f, size*float(i)-0.5f), point_t(28-0.5f, size*float(i)-0.5f));
    }
}
void mainloop() {
    static uint64_t t_last = getElapsedMillisecond(); // [ms]
    static pixel::input_event_t event;
    static bool animating = true;

    const uint64_t t1 = getElapsedMillisecond(); // [ms]
    const float dt = (float)( t1 - t_last ) / 1000.0f; // [s]
    t_last = t1;

    while(pixel::handle_one_event(event)){
        if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_CLOSE_REQ ) ) {
            printf("Exit Application\n");
            #if defined(__EMSCRIPTEN__)
                emscripten_cancel_main_loop();
            #else
                exit(0);
            #endif
        } else if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_RESIZED ) ) {
            pixel::cart_coord.set_height(0, (float)walls.rows());
        } else if( event.pressed( pixel::input_event_type_t::P1_UP ) ) {
            fritz.setVelo( 90_deg ); // , dt, event.pressed( pixel::input_event_type_t::P1_ACTION1 )) || fritzBadMove;
        } else if( event.pressed( pixel::input_event_type_t::P1_DOWN ) ) {
            fritz.setVelo(-90_deg ); // , dt, event.pressed( pixel::input_event_type_t::P1_ACTION1 )) || fritzBadMove;
        } else if( event.pressed( pixel::input_event_type_t::P1_LEFT ) ) {
            fritz.setVelo(180_deg ); // , dt, event.pressed( pixel::input_event_type_t::P1_ACTION1 )) || fritzBadMove;
        } else if( event.pressed( pixel::input_event_type_t::P1_RIGHT ) ) {
            fritz.setVelo(  0_deg ); // , dt, event.pressed( pixel::input_event_type_t::P1_ACTION1 )) || fritzBadMove;
        } else if( event.released_and_clr_mask( pixel::p1_cursor_mask ) ) {
            fritz.setVelo(  fritz.angle(), 0 );
        } else if( event.released_and_clr( pixel::input_event_type_t::P1_ACTION2 ) ) {
            std::vector<rect_ref_t> p;
            ball = physiks::ball_t::create("ballFritz", fritz.pos() + vec_t(2, 0), 20_cm,
                   15_m_s, jau::next_rnd(30_deg, 60_deg), 0, 15_m_s);

        }
        if( event.paused() ) {
            animating = false;
        } else {
            if( !animating ) {
                t_last = getElapsedMillisecond(); // [ms]
            }
            animating = true;
        }
    }
    
    [[maybe_unused]] constexpr int lala = 1U << 2U;

    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
    pixel::texture_ref hud_text = pixel::make_text("td %s, fps %2.2f, dt %d ms, %s",
            to_decstring(t1, ',', 9).c_str(), pixel::gpu_avg_fps(), (int)(dt*1000), animating?"animating":"paused");
    fritz.tick(dt);
    if (ball) {
        ball->tick(dt);
    }

    // white background
    pixel::clear_pixel_fb(255, 255, 255, 255);

    draw_grid(1, pixel::f4::vec_t(0, 255, 255, 255));

    pixel::set_pixel_color(  0 /* r */,   0 /* g */, 255 /* b */, 255 /* a */);
    for(auto &l : h_lines) { l.draw(); }
    pixel::set_pixel_color(  255 /* r */,   0 /* g */, 0 /* b */, 255 /* a */);
    for(auto &l : v_lines) { l.draw(); }

    pixel::set_pixel_color(    0 /* r */,   0 /* g */, 0 /* b */, 255 /* a */);
    fritz.draw();
    if (ball) {
        ball->draw(true);
    }
    // kreis({0, 0}, pixel::cart_coord.height()/4.0f);

    pixel::swap_pixel_fb(false);
    if( nullptr != hud_text ) {
        hud_text->draw_fbcoord(0, 0);
    }
    pixel::swap_gpu_buffer();
}

rect_ref_t screen = std::make_shared<rect_t>(point_t(pixel::cart_coord.min_x(), pixel::cart_coord.max_y()), pixel::cart_coord.height(), pixel::cart_coord.width());
int main(int argc, char *argv[])
{
    std::string maze_name = "jagd/maze2.txt";
    int window_width = 1920/2, window_height = 1080/2;
    #if defined(__EMSCRIPTEN__)
        window_width = 1024, window_height = 576; // 16:9
    #endif
    {
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-width", argv[i]) && i+1<argc) {
                window_width = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-height", argv[i]) && i+1<argc) {
                window_height = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-maze", argv[i]) && i+1<argc) {
                maze_name= argv[i+1];
                ++i;
            }
        }
    }
    {
        const float origin_norm[] = { 0.5f, 0.5f };
        if( !pixel::init_gfx_subsystem(argv[0], "sandbox01", window_width, window_height, origin_norm, true, true /* subsys primitives */) ) {
            return 1;
        }
    }
    read_maze(maze_name);
    {
        printf("Wall bitmap:\n");
        for(size_t y=walls.rows(); y-->0; ) {
            for(size_t x=0; x<walls.columns(); ++x) {
                if(walls[x,y]) {
                    printf("X");
                } else {
                    printf(" ");
                }
            }
            printf("\n");
        }
        printf("\n");
    }

    log_printf(0, "XX %s\n", pixel::cart_coord.toString().c_str());
    {
        float w = pixel::cart_coord.width();
        float h = pixel::cart_coord.height();
        float r01 = h/w;
        float a = w / h;
        printf("-w %f [x]\n-h %f [y]\n-r1 %f [y/x]\n-r2 %f [x/y]\n", w, h, r01, a);
    }

    pixel::cart_coord.set_height(0, (float)walls.rows());
    fritz = Person(fritz_start);
    for(const lineseg_t& line0 : h_lines) {
        const lineseg_ref_t& line = std::make_shared<lineseg_t>(line0);
        gobjects().push_back(line);
    }
    for(const lineseg_t& line0 : v_lines) {
        const lineseg_ref_t line = std::make_shared<lineseg_t>(line0);
        gobjects().push_back(line);
    }

    gobjects().push_back(screen);
    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
    return 0;
}
