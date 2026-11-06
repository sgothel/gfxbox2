/*
 * Author: Hermann Kai Gothel <hkg@jausoft.com>,
 *         Svenson Han Gothel <shg@jausoft.com> and
 *         Sven Gothel <sgothel@jausoft.com>
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
#include <cstddef>
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
static constexpr bool debug_gfx1 = false;
static constexpr bool debug_gfx2 = false;

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

enum class direction_t : int {
    RIGHT = 0,
    DOWN = 1,
    LEFT = 2,
    UP = 3
};

constexpr si_angle_f32 dir_to_angle(direction_t dir) noexcept {
    switch(dir) {
        case direction_t::DOWN: return -90_deg;
        case direction_t::RIGHT: return 0_deg;
        case direction_t::UP: return 90_deg;
        case direction_t::LEFT: return 180_deg;
        default: return 0_deg;
    }
}
constexpr char dir_to_char(direction_t dir) noexcept {
    switch(dir) {
        case direction_t::DOWN: return 'D';
        case direction_t::RIGHT: return 'R';
        case direction_t::UP: return 'U';
        case direction_t::LEFT: return 'L';
        default: return ' ';
    }
}
direction_t next_rnd_dir() noexcept {
    int r = (int)std::round(next_rnd() * float(4 - 0) + float(0)) % 4;
    return (direction_t)r;
}

enum class wall_t : int {
    EMPTY = 0,
    JOINT = 1,
    HORZ = 2,
    VERT = 3
};

static char getChar(wall_t t) noexcept {
    switch(t) {
        case wall_t::HORZ: return '-';
        case wall_t::VERT: return '|';
        case wall_t::JOINT: return '+';
        default: return ' ';
    }
}

// Orientation: 0/0 bottom-left
static DynArray2D<wall_t> walls;
point_t fritzchen_start, erna_start;
static pixel::f2::aabbox_t maze_box1, maze_box2;
static std::vector<pixel::f2::lineseg_t> h_lines;
static std::vector<pixel::f2::lineseg_t> v_lines;

bool mazeWalls(size_t x, size_t y) noexcept {
    return x<walls.columns() && y<walls.rows() && wall_t::EMPTY != walls[x, y];
}

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
                    walls = DynArray2D<wall_t>(width, height);
                    maze_box1.reset();
                    maze_box1.resize(0.0f, 0.0f);
                    maze_box1.resize((float)width, (float)height);
                    maze_box2.reset();
                    maze_box2.resize(-0.5f, -0.5f);
                    maze_box2.resize((float)width+0.5f, (float)height+0.5f);
                    walls.clear(wall_t::EMPTY);
                    if( DEBUG_ON ) {
                        jau::log_printf("maze: read dimension: %dx%d\n", width, height);
                    }
                } else if( 1 == linenum ) {
                    digest_fposition_line("fritz pos", fritzchen_start, line);
                } else if( 2 == linenum ) {
                    digest_fposition_line("erna pos", erna_start, line);
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
                                break;
                            case '|':
                                walls[x, height-1-y] = wall_t::VERT;
                                break;
                            case '+':
                                walls[x, height-1-y] = wall_t::JOINT;
                                break;
                            case '-':
                                walls[x, height-1-y] = wall_t::HORZ;
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
                                // jau::log_printf("maze error: unknown tile @ %d / %d: '%c'\n", x, field_line_iter, c);
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
                bool has_pre = false;
                for(size_t x = 0; x < walls.columns(); ++x) {
                    const wall_t t = walls[x, y];
                    if(wall_t::JOINT == t) {
                        if(!has_pre) {
                            left = {(float)x, (float)y};
                            has_pre = true;
                        } else {
                            point_t right = {(float)x, (float)y};
                            h_lines.emplace_back(left, right);
                            has_pre = false;
                        }
                    } else if(wall_t::HORZ != t) {
                        has_pre = false;
                    }
                }
            }
            for(size_t x = 0; x < walls.columns(); ++x) {
                point_t bottom;
                bool has_pre = false;
                for(size_t y = 0; y < walls.rows(); ++y) {
                    const wall_t t = walls[x, y];
                    if(wall_t::JOINT == t) {
                        if(!has_pre) {
                            bottom = {(float)x, (float)y};
                            has_pre = true;
                        } else {
                            point_t top = {(float)x, (float)y};
                            v_lines.emplace_back(bottom, top);
                            has_pre = false;
                        }
                    } else if(wall_t::VERT != t) {
                        has_pre = false;
                    }
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
    constexpr static si_length_f32 height = 1.2f;
    constexpr static si_length_f32 width = 1.8f;
    constexpr static si_length_f32 shoulder_height = height*0.6f;
    constexpr static si_length_f32 arm_width = width*0.20f;
    constexpr static si_length_f32 leg_width = width*0.25f;
    constexpr static float max_ausdauer = 10;
    constexpr static float recovery_time = 10; // [s] of max_ausdauer
    constexpr static float recovery_per_second = max_ausdauer / recovery_time; // [ausdauer/s]
    constexpr static direction_t start_dir = direction_t::UP;
    // constexpr static si_length_f32 step_width = height/2;
    constexpr static si_length_f32 nose_length = height/6;
    constexpr static si_velo_f32 def_velocity = 3.0f; // [m/s]

    point_t m_start;
    disk_t m_head;
    rect_t m_shoulders;
    lineseg_t m_nose;
    aabbox_t m_aabbox;
    point_t m_center;
    vec_t m_velocity; // [m/s]
    direction_t m_dir;
    float m_ausdauer;
    bool m_run;

  private:
    bool colision(vec_t){
        // if(man.pos==maze2.pos){
        {
        }
        return false;
    }
    void rotate(si_angle_f32 rad) noexcept {
        m_shoulders.rotate(rad);
        m_nose.rotate(rad, m_head.center);
        m_velocity.rotate(rad);
        // m_aabbox.rotate(rad);
        m_aabbox.reset();
        m_aabbox.resize(m_head.box());
        m_aabbox.resize(m_shoulders.box());
        m_center = m_aabbox.center();
    }

  public:

    Person(const point_t position)
    : m_start(position), m_head(position, height / 2), m_aabbox(), m_dir(start_dir),
      m_ausdauer(max_ausdauer), m_run(false)
    {
        const point_t tl = point_t(position.x - width/2, position.y + shoulder_height/2);
        m_shoulders = rect_t(tl, width, shoulder_height);
        m_nose = lineseg_t(m_head.center+vec_t(0, m_head.radius), m_head.center+vec_t(0, m_head.radius + nose_length));
        m_aabbox.resize(m_head.box());
        m_aabbox.resize(m_shoulders.box());
        m_center = m_aabbox.center();
        move(m_start - pos());
        // gobjects().push_back(std::make_shared<disk_t>(m_head));
        // gobjects().push_back(std::make_shared<rect_t>(m_shoulders));
        // gobjects().push_back(std::make_shared<lineseg_t>(m_nose));
    }

    Person() = default;

    const point_t& pos() const noexcept { return m_center; }
    const point_t& nasenpos() const noexcept { return m_nose.p1; }
    direction_t dir() const noexcept { return m_dir; }
    bool is_running() const noexcept { return m_run; }
    float ausdauer() const noexcept { return m_ausdauer; }

    void reset() noexcept {
        m_velocity = vec_t();
        m_ausdauer = max_ausdauer;
        m_run = false;
        rotateTo(dir_to_angle(start_dir));
        move(m_start - pos());
    }

    void setVelo(const vec_t &v) noexcept { m_velocity = v; }
    void setVelo(direction_t dir, const float speed=def_velocity) noexcept {
        si_angle_f32 angle = dir_to_angle(dir);
        rotateTo(angle);
        m_velocity = vec_t::from_length_angle(speed, angle);
        m_dir = dir;
    }

    bool tick(const float dt) noexcept { // override {
        // pixel::f2::vec_t g = sun->gravity_ships(p_center);
        // velocity += g * dt; // [m/s^2] * [s] = [m/s]
        vec_t d = m_velocity * dt; // [m/s] * [s] = [m]
        if(m_run) {
            m_ausdauer -= 2.0f*recovery_per_second * dt;
            if( m_ausdauer > 0 ) {
                d = ( float(m_run) + 1.0f ) * m_velocity * dt; // [m/s] * [s] = [m]
            } else {
                m_ausdauer = 0;
            }
        } else if (m_ausdauer < max_ausdauer) {
            m_ausdauer += recovery_per_second * dt;
        }
        if (d.is_zero()) {
            return true;
        }
        if (!step(m_dir, d)) {
            return false;
        }
        move(d);
        return true;
    }
    void setRun(bool v) { m_run = v; }

    bool step(const direction_t dir, vec_t &d) noexcept {
        vec_t pos2 = pos() + d;
        float x1 = pos().x;
        float y1 = pos().y;
        float x2 = pos2.x;
        float y2 = pos2.y;
        if (x2 <= maze_box1.bl.x) {
            x2 = maze_box1.tr.x-1;
            d.x = x2 - x1;
        } else if(x2 >= maze_box1.tr.x) {
            x2 = maze_box1.bl.x;
            d.x = x2 - x1;
        }
        if (y2 <= maze_box1.bl.y) {
            y2 = maze_box1.tr.y-1;
            d.y = y2 - y1;
        } else if(y2 >= maze_box1.tr.y) {
            y2 = maze_box1.bl.y;
            d.y = y2 - y1;
        }
        size_t xi2=0, yi2=0; // forward tile position
        bool hit = false;
        switch(dir) {
            case direction_t::DOWN:
                xi2 = (size_t)(x2+0.5f); // NOLINT
                yi2 = (size_t)(y2);      // NOLINT
                if (mazeWalls(xi2, yi2)) {
                    const float frac = y1 - std::trunc(y1);
                    if( 0.0 < frac && frac < 0.5f) {
                        d.y = -frac;
                    } else {
                        hit = true;
                    }
                }
                break;
            case direction_t::LEFT:
                xi2 = (size_t)(x2);           // NOLINT
                yi2 = (size_t)(y2+0.5f);      // NOLINT
                if (mazeWalls(xi2, yi2)) {
                    const float frac = x1 - std::trunc(x1);
                    if( 0.0 < frac && frac < 0.5f) {
                        d.x = -frac;
                    } else {
                        hit = true;
                    }
                }
                break;
            case direction_t::RIGHT:
                xi2 = (size_t)(x2+1.0f);      // NOLINT
                yi2 = (size_t)(y2+0.5f);      // NOLINT
                if (mazeWalls(xi2, yi2)) {
                    const float frac = x1 - std::trunc(x1);
                    if( 0.5 < frac && frac < 1.0f) {
                        d.x = 1-frac;
                    } else {
                        hit = true;
                    }
                }
                break;
            case direction_t::UP:
            default:
                xi2 = (size_t)(x2+0.5f);      // NOLINT
                yi2 = (size_t)(y2+1.0f);      // NOLINT
                if (mazeWalls(xi2, yi2)) {
                    const float frac = y1 - std::trunc(y1);
                    if( 0.5 < frac && frac < 1.0f) {
                        d.y = 1-frac;
                    } else {
                        hit = true;
                    }
                }
                break;
        }
        if (hit) {
            m_velocity = vec_t();
            printf("XXX: %2.2f/%2.2f -> %zu / %zu; pos %s, pos2 %s; aabbox %s, center %s\n",
                x2, y2, xi2, yi2, pos().toString().c_str(), pos2.toString().c_str(),
                m_aabbox.toString().c_str(), m_aabbox.center().toString().c_str());
            return false;
        }
        return true;
    }

    void move(const vec_t d) noexcept {
        m_head.move(d);
        m_shoulders.move(d);
        m_nose.move(d);
        m_aabbox.move(d);
        m_center = m_aabbox.center();
    }

    void rotateTo(const si_angle_f32 rad) noexcept {
        rotate(rad - dir_to_angle(m_dir));
    }
    void draw() noexcept {
        m_head.draw(false);
        m_shoulders.draw(false);
        m_nose.draw();
        if (debug_gfx2) {
            m_aabbox.draw();
            lineseg_t::draw(m_aabbox.bl, m_aabbox.tr);
            point_t br = { m_aabbox.bl.x, m_aabbox.tr.y };
            point_t tl = { m_aabbox.tr.x, m_aabbox.bl.y };
            lineseg_t::draw(br, tl);
        }
    }
};
constexpr size_t max_balls = 10;
Person fritzchen;
Person erna;
std::vector<physiks::ball_ref_t> balls;

void draw_grid(pixel::f4::vec_t color) {
    pixel::set_pixel_color(color);
    // const float dxy = 0.5f;
    const float dxy = 0;
    for (float x = maze_box1.bl.x; x < maze_box1.tr.x; x+=1.0f) { // NOLINT
        lineseg_t::draw(point_t(x - dxy, maze_box1.bl.y - dxy), point_t(x + dxy, maze_box1.tr.y + dxy));
    }
    for (float y = maze_box1.bl.y; y < maze_box1.tr.y; y+=1.0f) { // NOLINT
        lineseg_t::draw(point_t(maze_box1.bl.x - dxy, y - dxy), point_t(maze_box1.tr.x + dxy, y + dxy));
    }
}

void mainloop() {
    static uint64_t t_last = getElapsedMillisecond();  // [ms]
    static pixel::input_event_t event;
    static bool animating = true;

    const uint64_t t1 = getElapsedMillisecond();      // [ms]
    const float dt = (float)(t1 - t_last) / 1000.0f;  // [s]
    t_last = t1;

    while (pixel::handle_one_event(event)) {

        if (event.pressed_and_clr(pixel::input_event_type_t::WINDOW_CLOSE_REQ)) {
            printf("Exit Application\n");
#if defined(__EMSCRIPTEN__)
            emscripten_cancel_main_loop();
#else
            exit(0);
#endif
        } else if (event.pressed_and_clr(pixel::input_event_type_t::WINDOW_RESIZED)) {
            pixel::cart_coord.set_height(0, (float)walls.rows());
        } else if (event.pressed(pixel::input_event_type_t::P1_UP)) {
            fritzchen.setVelo(direction_t::UP);  // , dt, event.pressed( pixel::input_event_type_t::P1_ACTION1 ));
        } else if (event.pressed(pixel::input_event_type_t::P1_DOWN)) {
            fritzchen.setVelo(direction_t::DOWN);  // , dt, event.pressed( pixel::input_event_type_t::P1_ACTION1 ));
        } else if (event.pressed(pixel::input_event_type_t::P1_LEFT)) {
            fritzchen.setVelo(direction_t::LEFT);  // , dt, event.pressed( pixel::input_event_type_t::P1_ACTION1 ));
        } else if (event.pressed(pixel::input_event_type_t::P1_RIGHT)) {
            fritzchen.setVelo(direction_t::RIGHT);  // , dt, event.pressed( pixel::input_event_type_t::P1_ACTION1 ));
        } else if (event.pressed(pixel::input_event_type_t::RESET)) {
            fritzchen.reset();
            erna.reset();
            balls.clear();
        } else if (event.released_and_clr_mask(pixel::p1_cursor_mask)) {
            fritzchen.setVelo(fritzchen.dir(), 0);
            //fritz.setRun(false);
        }
        if (event.released_and_clr(pixel::input_event_type_t::P2_ACTION2) && balls.size() < max_balls) {
            balls.push_back( physiks::ball_t::createZeroG("ballFritz",
                fritzchen.nasenpos(),
                20_cm, // radius
                15_m_s, // velocity (scalar)
                dir_to_angle(fritzchen.dir()) + jau::adeg_to_rad(45), // angle/direction of velocity: Fritz + 45deg
                15_m_s, // max velocity (scalar)
                false /* reset */) );
            balls.back()->boost_factor = 1;
            balls.back()->break_factor = 1;
        }
        if (event.pressed(pixel::input_event_type_t::P2_ACTION3)) {
            fritzchen.setRun(true);
        } else if (event.released_and_clr(pixel::input_event_type_t::P2_ACTION3)) {
            fritzchen.setRun(false);
        }
        if (event.paused()) {
            animating = false;
        } else {
            if (!animating) {
                t_last = getElapsedMillisecond();  // [ms]
            }
            animating = true;
        }
    }

    [[maybe_unused]] constexpr int lala = 1U << 2U;

    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
    pixel::texture_ref hud_text = pixel::make_text("td %s, fps %2.2f, %s, %s, ausdauer %2.2f, dir %c, pos %2.2f/%2.2f",
            to_decstring(t1, ',', 9).c_str(), pixel::gpu_avg_fps(),
            animating ? "animating" : "paused",
            fritzchen.is_running() ? "run" : "walk",
            fritzchen.ausdauer(), dir_to_char(fritzchen.dir()), fritzchen.pos().x, fritzchen.pos().y);

    if (animating) {
        fritzchen.tick(dt);
        if(!erna.tick(dt)) {
            direction_t new_dir;
            #if 0
            switch(erna.dir()) {
                case direction_t::DOWN:
                    new_dir = direction_t::RIGHT;
                    break;
                case direction_t::RIGHT:
                    new_dir = direction_t::UP;
                    break;
                case direction_t::UP:
                    new_dir = direction_t::LEFT;
                    break;
                case direction_t::LEFT:
                default:
                    new_dir = direction_t::DOWN;
                    break;
            }
            #else
                new_dir = next_rnd_dir();
            #endif
            erna.setVelo(new_dir);
        }
        for(const auto& i : balls) {
            i->tick(dt);
        }
        for(auto i = balls.begin(); i < balls.end();) {
            physiks::ball_ref_t &b = *i;
            if (!b->box().inside(maze_box2)) {
                // delete i
                balls.erase(i);
                std::cout << "maze-box: " << maze_box2.toString() << "\n";
                std::cout << "ball-box: " << b->box().toString() << "\n";
            } else if (b->velocity.length() < 3) {
                // delete i
                balls.erase(i);
                std::cout << "ball-velo: " << b->velocity.toString() << "\n";
            } else {
                ++i;
            }
        }
    }

    // white background
    pixel::clear_pixel_fb(255, 255, 255, 255);

    if (debug_gfx1) {
        draw_grid(pixel::f4::vec_t(0, 255, 255, 255));
    }

    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
    for (auto &l : h_lines) { l.draw(); }
    pixel::set_pixel_color(255 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
    for (auto &l : v_lines) { l.draw(); }

    pixel::set_pixel_color(    255 /* r */,   0 /* g */, 255 /* b */, 255 /* a */);
    erna.draw();
    pixel::set_pixel_color(    0 /* r */,   0 /* g */, 255 /* b */, 255 /* a */);
    fritzchen.draw();
    for(auto &i : balls) {
        i->draw(true);
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
    std::string maze_name = "fritzchen/maze3.txt";
    int window_width = 1920, window_height = 1080;
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
        if( !pixel::init_gfx_subsystem(argv[0], "Fritzchen", window_width, window_height, origin_norm, true, true /* subsys primitives */) ) {
            return 1;
        }
    }
    read_maze(maze_name);
    {
        printf("Wall bitmap %zux%zu:\n", walls.columns(), walls.rows());
        printf("Maze box1 %s:\n", maze_box1.toString().c_str());
        printf("Maze box2 %s:\n", maze_box2.toString().c_str());
        for(size_t y=walls.rows(); y-->0; ) {
            for(size_t x=0; x<walls.columns(); ++x) {
                printf("%c", getChar(walls[x,y]));
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
    fritzchen = Person(fritzchen_start);
    erna = Person(erna_start);
    erna.setVelo(direction_t::UP);
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

/**
 * DONE
 * - run (ausdauer etc)
 * - ball reparieren
 * - n baelle erlauben (-> vector)
 * - evt bewegung einschraenken (-> pacman)
 *
 * TODO - Erna
 * - abbiegen
 * - weglaufen
 * - jagen
 */
