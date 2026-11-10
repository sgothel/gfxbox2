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
    WALL_JOINT = 1,
    WALL_HORZ = 2,
    WALL_VERT = 3,
    PELLET_POWER = 20,
    CHERRY = 80,
    STRAWBERRY = 81,
    PEACH = 82,
    APPLE = 83,
    MELON = 84,
    GALAXIAN = 85,
    BELL = 86,
    KEY = 87
};

static char getChar(tile_t t) noexcept {
    switch(t) {
        case tile_t::EMPTY: return ' ';
        case tile_t::WALL_HORZ: return '-';
        case tile_t::WALL_VERT: return '|';
        case tile_t::WALL_JOINT: return '+';
        default: return '*';
    }
}

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

class Maze {
  public:
    vec_t screen_dim;
    vec_t field_offset;
    vec_t field_dim;
    DynArray2D<tile_t> tiles; ///< Orientation: 0/0 bottom-left, Dimension of screen_dim
    point_t fritzchen_start, erna_start;
    pixel::f2::aabbox_t box1; ///< Matching field bounding box
    pixel::f2::aabbox_t box2; ///< Outer field bounding box

    bool hasWall(size_t x, size_t y) noexcept {
        return x<tiles.columns() && y<tiles.rows() && tile_t::EMPTY != tiles[x, y];
    }

  private:
    std::vector<pixel::f2::lineseg_t> h_lines;
    std::vector<pixel::f2::lineseg_t> v_lines;


    bool digest_point_line(const std::string& name, point_t& dest, const std::string& line) noexcept {
        float x_pos = 0.0f, y_pos = 0.0f;
        if (2 != sscanf(line.c_str(), "%f %f", &x_pos, &y_pos)) {
            jau::log_printf("maze: error reading 2 floats '%s' from line: '%s'\n", name.c_str(), line.c_str());
            return false;
        }
        dest.set(x_pos, y_pos);
        if( DEBUG_ON ) {
            jau::log_printf("maze: read '%s' position: %s\n", name.c_str(), dest.toString().c_str());
        }
        return true;
    }
    bool digest_2points_line(const std::string& name, point_t& dest1, point_t& dest2, const std::string& line) noexcept {
        float x1_pos = 0.0f, y1_pos = 0.0f;
        float x2_pos = 0.0f, y2_pos = 0.0f;
        if (4 != sscanf(line.c_str(), "%f %f %f %f", &x1_pos, &y1_pos, &x2_pos, &y2_pos)) {
            jau::log_printf("maze: error reading 4 floats '%s' from line: '%s'\n", name.c_str(), line.c_str());
            return false;
        }
        dest1.set(x1_pos, y1_pos);
        dest2.set(x2_pos, y2_pos);
        if( DEBUG_ON ) {
            jau::log_printf("maze: read '%s' position: %s; %s\n", name.c_str(), dest1.toString().c_str(), dest2.toString().c_str());
        }
        return true;
    }

  public:
    bool read(const std::string& fname0) noexcept {
        h_lines.clear();
        v_lines.clear();
        size_t width = 0, height = 0;
        const std::string fname1 = pixel::resolve_asset(fname0);
        if( !fname1.size() ) {
            jau::log_printf("maze_t: Could locate file '%s' in asset dir '%s'\n", fname0.c_str(), pixel::asset_dir().c_str());
            return false;
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
                        if (!digest_point_line("screen dim", screen_dim, line)) {
                            return false;
                        }
                    } else if( 1 == linenum ) {
                        if (!digest_2points_line("field dim", field_offset, field_dim, line)) {
                            return false;
                        }
                        width = size_t(screen_dim.x);
                        height = size_t(screen_dim.y);
                        tiles = DynArray2D<tile_t>(width, height);
                        box1.reset();
                        box1.resize(field_offset);
                        box1.resize(field_offset + field_dim);
                        box2.reset();
                        const vec_t half(0.5f, 0.5f);
                        box2.resize(field_offset - half);
                        box2.resize(field_offset + field_dim + half);
                        tiles.clear(tile_t::EMPTY);
                    } else if( 2 == linenum ) {
                        if (!digest_point_line("fritz pos", fritzchen_start, line)) {
                            return false;
                        }
                    } else if( 3 == linenum ) {
                        if (!digest_point_line("erna pos", erna_start, line)) {
                            return false;
                        }
                    } else if( field_line_iter < height ) {
                        if( DEBUG_ON ) {
                            jau::log_printf("maze: read line y = %zu, len = %zu: %s\n", field_line_iter, line.length(), line.c_str());
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
                                    tiles[x, height-1-y] = tile_t::WALL_VERT;
                                    break;
                                case '+':
                                    tiles[x, height-1-y] = tile_t::WALL_JOINT;
                                    break;
                                case '-':
                                    tiles[x, height-1-y] = tile_t::WALL_HORZ;
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
                return false;
            }
            {
                using pixel::f2::point_t;
                for(size_t y = 0; y < tiles.rows(); ++y) {
                    point_t left;
                    bool has_pre = false;
                    for(size_t x = 0; x < tiles.columns(); ++x) {
                        const tile_t t = tiles[x, y];
                        if(tile_t::WALL_JOINT == t) {
                            if(!has_pre) {
                                left = {(float)x, (float)y};
                                has_pre = true;
                            } else {
                                point_t right = {(float)x, (float)y};
                                h_lines.emplace_back(left, right);
                                has_pre = false;
                            }
                        } else if(tile_t::WALL_HORZ != t) {
                            has_pre = false;
                        }
                    }
                }
                for(size_t x = 0; x < tiles.columns(); ++x) {
                    point_t bottom;
                    bool has_pre = false;
                    for(size_t y = 0; y < tiles.rows(); ++y) {
                        const tile_t t = tiles[x, y];
                        if(tile_t::WALL_JOINT == t) {
                            if(!has_pre) {
                                bottom = {(float)x, (float)y};
                                has_pre = true;
                            } else {
                                point_t top = {(float)x, (float)y};
                                v_lines.emplace_back(bottom, top);
                                has_pre = false;
                            }
                        } else if(tile_t::WALL_VERT != t) {
                            has_pre = false;
                        }
                    }
                }
            }
        }
        gobjects().clear();
        for(const lineseg_t& line0 : h_lines) {
            const lineseg_ref_t& line = std::make_shared<lineseg_t>(line0);
            gobjects().push_back(line);
        }
        for(const lineseg_t& line0 : v_lines) {
            const lineseg_ref_t line = std::make_shared<lineseg_t>(line0);
            gobjects().push_back(line);
        }
        return true;
    }

    void draw() const noexcept {
        for (const auto &l : h_lines) { l.draw(); }
        for (const auto &l : v_lines) { l.draw(); }
    }

    void draw_grid(pixel::f4::vec_t color) {
        pixel::set_pixel_color(color);
        // const float dxy = 0.5f;
        const float dxy = 0;
        for (float x = box1.bl.x; x < box1.tr.x; x+=1.0f) { // NOLINT
            lineseg_t::draw(point_t(x - dxy, box1.bl.y - dxy), point_t(x + dxy, box1.tr.y + dxy));
        }
        for (float y = box1.bl.y; y < box1.tr.y; y+=1.0f) { // NOLINT
            lineseg_t::draw(point_t(box1.bl.x - dxy, y - dxy), point_t(box1.tr.x + dxy, y + dxy));
        }
    }

};
Maze maze;

class Player {
  private:
    constexpr static si_length_f32 height = 1.2f;
    constexpr static si_length_f32 width = 1.8f;
    constexpr static si_length_f32 shoulder_height = height*0.6f;
    constexpr static si_length_f32 arm_width = width*0.20f;
    constexpr static si_length_f32 leg_width = width*0.25f;
    constexpr static float max_ausdauer = 10;
    constexpr static float recovery_time = 10; // [s] of max_ausdauer
    constexpr static float recovery_per_second = max_ausdauer / recovery_time; // [ausdauer/s]
    constexpr static direction_t start_dir = direction_t::LEFT;
    // constexpr static si_length_f32 step_width = height/2;
    constexpr static si_length_f32 nose_length = height/6;
    constexpr static si_velo_f32 def_velocity = 7.0f; // [m/s]
    constexpr static size_t max_balls = 10;
    constexpr static const float ball_radius = 20_cm;
    constexpr static const float ball_velocity_min = 11_m_s; // [m/s]
    constexpr static const float ball_velocity_max = 25_m_s; //
    point_t m_start;
    disk_t m_head;
    rect_t m_shoulders;
    lineseg_t m_nose;
    aabbox_t m_aabbox;
    point_t m_center;
    vec_t m_velocity_v; // [m/s]
    float m_velocity_s; // [m/s]
    direction_t m_dir;
    float m_ausdauer;
    bool m_run;
    float m_last_dt = 1.0f/60.0f;
    float m_ball_velo_s = ball_velocity_min;
    std::vector<physiks::ball_ref_t> m_balls;

    static int round_center(float &f, float &d) noexcept {
        const float r = std::round(f);
        d = r - f;
        f = r;
        return int(r);
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

    void rotate(si_angle_f32 rad) noexcept {
        m_shoulders.rotate(rad);
        m_nose.rotate(rad, m_head.center);
        m_velocity_v.rotate(rad);
        // m_aabbox.rotate(rad);
        m_aabbox.reset();
        m_aabbox.resize(m_head.box());
        m_aabbox.resize(m_shoulders.box());
        m_center = m_aabbox.center();
    }

  public:

    Player(const point_t position)
    : m_start(position), m_head(position, height / 2), m_aabbox(),
      m_velocity_v(), m_velocity_s(0), m_dir(direction_t::UP),
      m_ausdauer(max_ausdauer), m_run(false)
    {
        // build in direction_t::UP
        const point_t tl = point_t(position.x - width/2, position.y + shoulder_height/2);
        m_shoulders = rect_t(tl, width, shoulder_height);
        m_nose = lineseg_t(m_head.center+vec_t(0, m_head.radius), m_head.center+vec_t(0, m_head.radius + nose_length));
        m_aabbox.resize(m_head.box());
        m_aabbox.resize(m_shoulders.box());
        m_center = m_aabbox.center();
        // rotate to start-direction
        rotateTo(dir_to_angle(start_dir));
        m_dir = start_dir;
        // move to start-position
        move(m_start - pos());
        // gobjects().push_back(std::make_shared<disk_t>(m_head));
        // gobjects().push_back(std::make_shared<rect_t>(m_shoulders));
        // gobjects().push_back(std::make_shared<lineseg_t>(m_nose));
    }

    Player() = default;

    const point_t& pos() const noexcept { return m_center; }
    const point_t& nasenpos() const noexcept { return m_nose.p1; }
    direction_t dir() const noexcept { return m_dir; }
    const vec_t &velo_v() const noexcept { return m_velocity_v; }
    const float &velo_s() const noexcept { return m_velocity_s; }
    bool is_running() const noexcept { return m_run; }
    float ausdauer() const noexcept { return m_ausdauer; }

    void reset() noexcept {
        m_velocity_v = vec_t();
        m_velocity_s = 0;
        m_ausdauer = max_ausdauer;
        m_run = false;
        rotateTo(dir_to_angle(start_dir));
        m_dir = start_dir;
        move(m_start - pos());
        m_balls.clear();
    }

    void setVelo(const float speed) noexcept {
        si_angle_f32 angle = dir_to_angle(m_dir);
        m_velocity_v = vec_t::from_length_angle(speed, angle);
        m_velocity_s = speed;
    }

    bool setDir(direction_t dir, const float speed=def_velocity) noexcept {
        if( dir == m_dir && jau::equals(m_velocity_s, speed)) {
            return true;
        }
        if (!test_step(dir, speed)) {
            return false;
        }
        si_angle_f32 angle = dir_to_angle(dir);
        rotateTo(angle);
        m_velocity_v = vec_t::from_length_angle(speed, angle);
        m_velocity_s = speed;
        m_dir = dir;
        return true;
    }

    float ball_velo() const noexcept { return m_ball_velo_s; }
    bool add_ball_velocity(const float pct) {
        m_ball_velo_s += ball_velocity_max * pct;
        if(m_ball_velo_s > ball_velocity_max){
            m_ball_velo_s = ball_velocity_max;
            return false;
        }
        return true;
    }

    bool throwBall() {
        if (m_balls.size() >= max_balls) {
            return false;
        }
        m_balls.push_back( physiks::ball_t::createZeroG("ballFritz",
            nasenpos(),
            ball_radius, // radius
            m_ball_velo_s, // velocity (scalar)
            dir_to_angle(dir()) + jau::adeg_to_rad(45), // angle/direction of velocity: Fritz + 45deg
            m_ball_velo_s, // max velocity (scalar)
            false /* reset */) );
        m_balls.back()->boost_factor = 1;
        m_balls.back()->brake_factor = 1;
        m_ball_velo_s = ball_velocity_min;
        return true;
    }

    bool tick(const float dt) noexcept { // override {
        m_last_dt = dt;

        for(const auto& i : m_balls) {
            i->tick(dt);
        }
        for(auto i = m_balls.begin(); i < m_balls.end();) {
            physiks::ball_ref_t &b = *i;
            if (!b->box().inside(maze.box2)) {
                // delete i
                m_balls.erase(i);
                std::cout << "maze-box: " << maze.box2.toString() << "\n";
                std::cout << "ball-box: " << b->box().toString() << "\n";
            } else if (b->velocity.length() < 3) {
                // delete i
                m_balls.erase(i);
                std::cout << "ball-velo: " << b->velocity.toString() << "\n";
            } else {
                ++i;
            }
        }

        // pixel::f2::vec_t g = sun->gravity_ships(p_center);
        // velocity += g * dt; // [m/s^2] * [s] = [m/s]
        vec_t d = m_velocity_v * dt; // [m/s] * [s] = [m]
        if(m_run) {
            m_ausdauer -= 2.0f*recovery_per_second * dt;
            if( m_ausdauer > 0 ) {
                d = ( float(m_run) + 1.0f ) * m_velocity_v * dt; // [m/s] * [s] = [m]
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
            m_velocity_v = vec_t();
            m_velocity_s = 0;
            return false;
        }
        move(d);
        return true;
    }
    void setRun(bool v) { m_run = v; }

    bool test_step(const direction_t dir, const float speed=def_velocity) const noexcept {
        si_angle_f32 angle = dir_to_angle(dir);
        vec_t velocity = vec_t::from_length_angle(speed, angle);
        vec_t d = velocity * m_last_dt; // [m/s] * [s] = [m]
        if(m_run && m_ausdauer > 0 ) {
            d = ( float(m_run) + 1.0f ) * m_velocity_v * m_last_dt; // [m/s] * [s] = [m]
        }
        return step(dir, d);
    }

    bool step(const direction_t dir, vec_t &d) const noexcept {
        vec_t pos2 = pos() + d;
        float x1 = pos().x;
        float y1 = pos().y;
        float x2 = pos2.x;
        float y2 = pos2.y;
        if (x2 <= maze.box1.bl.x) {
            x2 = maze.box1.tr.x-1;
            d.x = x2 - x1;
        } else if(x2 >= maze.box1.tr.x) {
            x2 = maze.box1.bl.x;
            d.x = x2 - x1;
        }
        if (y2 <= maze.box1.bl.y) {
            y2 = maze.box1.tr.y-1;
            d.y = y2 - y1;
        } else if(y2 >= maze.box1.tr.y) {
            y2 = maze.box1.bl.y;
            d.y = y2 - y1;
        }
        size_t xi2=0, yi2=0; // forward tile position
        bool hit = false;
        switch(dir) {
            case direction_t::DOWN:
                xi2 = round_center(x2, d.x);
                yi2 = (size_t)(y2);      // NOLINT
                if (maze.hasWall(xi2, yi2)) {
                    const float frac = y1 - std::trunc(y1);
                    if( 0.0f < frac && frac < 0.5f) {
                        d.y = -frac;
                    } else {
                        hit = true;
                    }
                }
                break;
            case direction_t::LEFT:
                yi2 = round_center(y2, d.y);
                xi2 = (size_t)(x2);           // NOLINT
                if (maze.hasWall(xi2, yi2)) {
                    const float frac = x1 - std::trunc(x1);
                    if( 0.0 < frac && frac < 0.5f) {
                        d.x = -frac;
                    } else {
                        hit = true;
                    }
                }
                break;
            case direction_t::RIGHT:
                yi2 = round_center(y2, d.y);
                xi2 = (size_t)(x2+1.0f);      // NOLINT
                if (maze.hasWall(xi2, yi2)) {
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
                xi2 = round_center(x2, d.x);
                yi2 = (size_t)(y2+1.0f);      // NOLINT
                if (maze.hasWall(xi2, yi2)) {
                    const float frac = y1 - std::trunc(y1);
                    if( 0.5 < frac && frac < 1.0f) {
                        d.y = 1-frac;
                    } else {
                        hit = true;
                    }
                }
                break;
        }
        if (DEBUG_ON && hit) {
            printf("XXX: %2.2f/%2.2f -> %zu / %zu; pos %s, pos2 %s; aabbox %s, center %s\n",
                x2, y2, xi2, yi2, pos().toString().c_str(), pos2.toString().c_str(),
                m_aabbox.toString().c_str(), m_aabbox.center().toString().c_str());
        }
        return !hit;
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
        for(auto &i : m_balls) {
            i->draw(true);
        }
    }
};
Player fritzchen;
Player erna;
uint64_t t_start = 0; // [ms]
uint64_t frame_count_total = 0;

void erna_ai_tick(float dt) {
    if(!erna.tick(dt)) {
        printf("EEE: collision: dir %c\n", dir_to_char(erna.dir()));
        direction_t new_dir;
        #if 0
        switch(erna.dir()) {
            case direction_t::DOWN: * - ball geschwindigkeit erhoehen zu maximum in 1s (-> canonball)

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
            do {
                new_dir = next_rnd_dir();
            } while (!erna.test_step(new_dir));
        #endif
        printf("EEE: new_dir: dir %c\n", dir_to_char(new_dir));
        erna.setDir(new_dir);
    }
}

void reset_globals() {
    t_start = 0;
    frame_count_total = 0;
}

void mainloop() {
    static uint64_t t_last = getElapsedMillisecond();  // [ms]
    static pixel::input_event_t event;
    static bool animating = true;
    constexpr static float text_height = 1.0f;

    const uint64_t t1 = getElapsedMillisecond();      // [ms]
    const float dt = (float)(t1 - t_last) / 1000.0f;  // [s]
    t_last = t1;

    if( 0 == frame_count_total ) {
        t_start = jau::getElapsedMillisecond();
    }

    while (pixel::handle_one_event(event)) {

        if (event.pressed_and_clr(pixel::input_event_type_t::WINDOW_CLOSE_REQ)) {
            printf("Exit Application\n");
#if defined(__EMSCRIPTEN__)
            emscripten_cancel_main_loop();
#else
            exit(0);
#endif
        } else if (event.pressed_and_clr(pixel::input_event_type_t::WINDOW_RESIZED)) {
            pixel::cart_coord.set_fitting(0.0f, 0.0f, maze.screen_dim.x, maze.screen_dim.y);
        } else if (event.pressed(pixel::input_event_type_t::RESET)) {
            fritzchen.reset();
            erna.reset();
        }
        if (event.released_and_clr(pixel::input_event_type_t::P2_ACTION2)) {
            fritzchen.throwBall();
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
    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);

    static point_t text_tl = {0, maze.screen_dim.y};
    std::vector<pixel::texture_ref> hud_texts = {
        pixel::make_text2(text_tl, 0, {0, 0, 0, 1}, text_height, "%s s, fps %4.2f/%d",
            to_decstring((t1-t_start)/1000, ',', 5).c_str(), pixel::gpu_avg_fps(), pixel::expected_fps()),
        pixel::make_text2(text_tl, 1, {0, 0, 0, 0}, text_height, "run-e %2.2f, throw-e %2.2f, %2.2f/%2.2f",
            fritzchen.ausdauer(), fritzchen.ball_velo(), fritzchen.pos().x, fritzchen.pos().y)
    };

    if (animating) {
        // static/memorized cursor event
        if (event.pressed(pixel::input_event_type_t::P2_ACTION2)) {
            fritzchen.add_ball_velocity(dt);
        }
        if( event.pressed_mask(pixel::p1_cursor_mask)) {
            if (event.pressed(pixel::input_event_type_t::P1_UP)) {
                fritzchen.setDir(direction_t::UP);
            } else if (event.pressed(pixel::input_event_type_t::P1_DOWN)) {
                fritzchen.setDir(direction_t::DOWN);
            } else if (event.pressed(pixel::input_event_type_t::P1_LEFT)) {
                fritzchen.setDir(direction_t::LEFT);
            } else if (event.pressed(pixel::input_event_type_t::P1_RIGHT)) {
                fritzchen.setDir(direction_t::RIGHT);
            }
        } else {
            fritzchen.setVelo(0);
        }
        fritzchen.tick(dt);
        erna_ai_tick(dt);
    }

    // white background
    pixel::clear_pixel_fb(255, 255, 255, 255);

    if (debug_gfx1) {
        maze.draw_grid(pixel::f4::vec_t(127, 127, 127, 255));
    }
    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
    maze.draw();

    pixel::set_pixel_color(    255 /* r */,   0 /* g */, 255 /* b */, 255 /* a */);
    erna.draw();
    pixel::set_pixel_color(    0 /* r */,   0 /* g */, 255 /* b */, 255 /* a */);
    fritzchen.draw();

    pixel::swap_pixel_fb(false);
    for(const pixel::texture_ref& hud_text : hud_texts) {
        if( nullptr != hud_text ) {
            hud_text->draw_fbcoord(pixel::cart_coord.to_fb_dx(maze.screen_dim.x/2) - hud_text->width/2, 0); // NOLINT
        }
    }
    pixel::swap_gpu_buffer();
    ++frame_count_total;
}

int main(int argc, char *argv[])
{
    std::string maze_name = "fritzchen/maze4.txt";
    int window_width = 640, window_height = 720;
    // int window_width = 1920, window_height = 1080;
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
    if (!maze.read(maze_name)) {
        return -1;
    }
    {
        printf("Maze Screen-Dim %s:\n", maze.screen_dim.toString().c_str());
        printf("Maze Field-Pos  %s:\n", maze.field_offset.toString().c_str());
        printf("Maze Field-Dim  %s:\n", maze.field_dim.toString().c_str());
        printf("Maze tiles      %zux%zu:\n", maze.tiles.columns(), maze.tiles.rows());
        printf("Maze box1       %s:\n", maze.box1.toString().c_str());
        printf("Maze box2       %s:\n", maze.box2.toString().c_str());
        for(size_t y=maze.tiles.rows(); y-->0; ) {
            for(size_t x=0; x<maze.tiles.columns(); ++x) {
                printf("%c", getChar(maze.tiles[x,y]));
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

    pixel::cart_coord.set_fitting(0.0f, 0.0f, maze.screen_dim.x, maze.screen_dim.y);
    fritzchen = Player(maze.fritzchen_start);
    erna = Player(maze.erna_start);
    erna.setDir(direction_t::LEFT);

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
 * - bewegung einschraenken (-> pacman)
 * - abbiegen (-> pacman)
 * - ball logik -> Player
 * - ball geschwindigkeit erhoehen zu maximum in 1s (-> canonball)
 *
 * TODO - Erna
 * - weglaufen
 * - jagen
 */
