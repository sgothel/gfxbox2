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
#include <cinttypes>
#include <algorithm>
#include <limits>
#include <memory>
#include <fstream>
#include <vector>

#include <jau/float_si_types.hpp>
#include <jau/utils.hpp>

#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include <pixel/pixel4f.hpp>
#include <pixel/pixel.hpp>
#include "pixel/audio.hpp"
#include <pixel/dynarray2d.hpp>
#include <physics.hpp>

using namespace jau;
using namespace jau::si_f32_literals;
using namespace pixel::f2;
using namespace jau::audio;

enum class player_id_t {
  fritzchen,
  erna
};

enum class score_t {
    ball_hit          =    10,
    erna_home_attract =   100,
    erna_tick_escape  =   400,
    erna_home_random  =  1000,
    erna_home_escape  = 10000
};
constexpr int number(score_t v) noexcept { return static_cast<int>(v); }

enum class strategy_t {
    attract,
    escape,
    random,
    seek_pos
};
static const char *to_string(strategy_t v) noexcept {
    switch(v) {
        case strategy_t::attract:  return "attrac";
        case strategy_t::escape:   return "escape";
        case strategy_t::random:   return "random";
        case strategy_t::seek_pos: return "seekpo";
        default: return "undefined";
    }
}

enum class strategy_duration_t {
    attract = 30,
    escape  = 25,
    random  = 15,
    seekpos = 10
};
constexpr float number(strategy_duration_t v) noexcept { return static_cast<float>(v); }

enum class tile_t : int {
    EMPTY = 0,
    WALL_JOINT = 1,
    WALL_HORZ = 2,
    WALL_VERT = 3
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
constexpr int number(direction_t v) noexcept { return static_cast<int>(v); }

static si_angle_f32 dir_to_angle(direction_t dir) noexcept {
    switch(dir) {
        case direction_t::DOWN: return -90_deg;
        case direction_t::RIGHT: return 0_deg;
        case direction_t::UP: return 90_deg;
        case direction_t::LEFT: return 180_deg;
        default: return 0_deg;
    }
}
static char dir_to_char(direction_t dir) noexcept {
    switch(dir) {
        case direction_t::DOWN: return 'D';
        case direction_t::RIGHT: return 'R';
        case direction_t::UP: return 'U';
        case direction_t::LEFT: return 'L';
        default: return ' ';
    }
}
static direction_t inverse(direction_t dir) noexcept {
    switch(dir) {
        case direction_t::RIGHT: return direction_t::LEFT;
        case direction_t::DOWN: return direction_t::UP;
        case direction_t::LEFT: return direction_t::RIGHT;
        case direction_t::UP: return direction_t::DOWN;
        default: return direction_t::DOWN;
    }
}
static direction_t rot_left(direction_t dir) noexcept {
    switch(dir) {
        case direction_t::RIGHT: return direction_t::UP;
        case direction_t::DOWN: return direction_t::RIGHT;
        case direction_t::LEFT: return direction_t::DOWN;
        case direction_t::UP: return direction_t::LEFT;
        default: return direction_t::LEFT;
    }
}
static direction_t rot_right(direction_t dir) noexcept {
    switch(dir) {
        case direction_t::RIGHT: return direction_t::DOWN;
        case direction_t::DOWN: return direction_t::LEFT;
        case direction_t::LEFT: return direction_t::UP;
        case direction_t::UP: return direction_t::RIGHT;
        default: return direction_t::RIGHT;
    }
}
static direction_t next_rnd_dir() noexcept {
    int r = (int)std::round(next_rnd() * float(4 - 0) + float(0)) % 4;
    return (direction_t)r;
}

///
///

static bool DEBUG_ON = false;
static bool debug_gfx1 = false;
static bool debug_gfx2 = false;
static std::string record_bmpseq_basename;
static uint64_t t_start = 0; // [ms]

static strategy_t erna_mode = strategy_t::attract;
static float erna_mode_to = number(strategy_duration_t::attract);

constexpr int audio_ball_maxvol = MIX_MAX_VOLUME/4;
static audio_sample_ref audio_ball;

static bool load_samples() {
    if( jau::audio::is_audio_subsystem_initialized() ) {
        audio_ball = std::make_shared<jau::audio::audio_sample_t>("fritzchen/ball-1bounce.ogg", false, audio_ball_maxvol);
        return true;
    } else {
        audio_ball = std::make_shared<jau::audio::audio_sample_t>();
        return false;
    }
}

///

struct Connector {
    point_t a, b;
};

class Maze {
  public:
    vec_t screen_dim;
    vec_t field_offset;
    vec_t field_dim;
    pixel::f2::aabbox_t box1; ///< Matching field bounding box
    pixel::f2::aabbox_t box2; ///< Outer field bounding box
    DynArray2D<tile_t> tiles; ///< Orientation: 0/0 bottom-left, Dimension of screen_dim
    point_t fritzchen_start;
    pixel::f2::aabbox_t home_ext; ///< Home exterior box
    pixel::f2::aabbox_t home_int; ///< Home interior box
    point_t erna_start;
    std::vector<Connector> connectors;

    bool hasWall(size_t x, size_t y) noexcept {
        return x<tiles.columns() && y<tiles.rows() && tile_t::EMPTY != tiles[x, y];
    }

  private:
    std::vector<pixel::f2::lineseg_t> h_lines;
    std::vector<pixel::f2::lineseg_t> v_lines;


    bool digest_point_line(const std::string& name, point_t& dest, const std::string &line) noexcept {
        return digest_point_line(name, dest, line.c_str());
    }
    bool digest_point_line(const std::string& name, point_t& dest, const char *line) noexcept {
        float x_pos = 0.0f, y_pos = 0.0f;
        if (2 != sscanf(line, "%f %f", &x_pos, &y_pos)) {
            jau::log_printf("maze: error reading 2 floats '%s' from line: '%s'\n", name.c_str(), line);
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
    bool digest_aabbox_line(const std::string& name, aabbox_t& dest, const std::string& line) noexcept {
        float x_pos = 0.0f, y_pos = 0.0f;
        float width = 0.0f, height = 0.0f;
        if (4 != sscanf(line.c_str(), "%f %f %f %f", &x_pos, &y_pos, &width, &height)) {
            jau::log_printf("maze: error reading 4 floats '%s' from line: '%s'\n", name.c_str(), line.c_str());
            return false;
        }
        point_t p = { x_pos, y_pos };
        point_t d = { width, height };
        dest.reset();
        dest.resize(p);
        dest.resize(p + d);
        if( DEBUG_ON ) {
            jau::log_printf("maze: read '%s' aabbox: %s\n", name.c_str(), dest.toString().c_str());
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
                point_t connector_a; bool has_connector_a=false;
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
                        const vec_t half(0.5f, 0.5f);
                        box1.reset();
                        box1.resize(field_offset);
                        box1.resize(field_offset + field_dim);
                        box2.reset();
                        box2.resize(field_offset - half);
                        box2.resize(field_offset + field_dim + half);
                        tiles.clear(tile_t::EMPTY);
                    } else if( 2 == linenum ) {
                        if (!digest_point_line("fritz pos", fritzchen_start, line)) {
                            return false;
                        }
                    } else if( 3 == linenum ) {
                        if (!digest_aabbox_line("home_ext", home_ext, line)) {
                            return false;
                        }
                    } else if( 4 == linenum ) {
                        if (!digest_aabbox_line("home_int", home_int, line)) {
                            return false;
                        }
                    } else if( 5 == linenum ) {
                        if (!digest_point_line("erna pos", erna_start, line)) {
                            return false;
                        }
                    } else if( field_line_iter < height ) {
                        if (0==field_line_iter && line.starts_with('c')) {
                            if (line.starts_with("ca ")) {
                                if (!digest_point_line("conn_a", connector_a, line.c_str()+2)) {
                                    return false;
                                }
                                has_connector_a = true;;
                            } else if (line.starts_with("cb ")) {
                                if (has_connector_a) {
                                    point_t connector_b;
                                    if (!digest_point_line("conn_b", connector_b, line.c_str()+2)) {
                                        return false;
                                    }
                                    connectors.push_back({ connector_a, connector_b});
                                    has_connector_a = false;
                                } else {
                                    jau::log_printf("maze: cb without ca: %s\n", fname0.c_str());
                                    return false;
                                }
                            }
                            continue;
                        }
                        if( DEBUG_ON ) {
                            jau::log_printf("maze: read line y = %zu, len = %zu: %s\n", field_line_iter, line.length(), line.c_str());
                        }
                        const size_t maxlen = std::min(line.length(), width);
                        for(size_t x=0; x<maxlen; ++x) {
                            const char c = line[x];
                            switch( c ) {
                                case '|':
                                    tiles[x, height-1-y] = tile_t::WALL_VERT;
                                    break;
                                case '+':
                                    tiles[x, height-1-y] = tile_t::WALL_JOINT;
                                    break;
                                case '-':
                                    tiles[x, height-1-y] = tile_t::WALL_HORZ;
                                    break;
                                default:
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

    point_t get_random_upper_pos() const noexcept {
        return {jau::next_rnd(box1.bl.x+2, box1.tr.x-2), jau::next_rnd(home_ext.tr.y+2, box1.tr.y-2)};
    }

    void draw() const noexcept {
        for (const auto &l : h_lines) { l.draw(); }
        for (const auto &l : v_lines) { l.draw(); }
    }

    void draw_home() const noexcept {
        home_ext.draw();
        home_int.draw();
    }

    void draw_grid(pixel::f4::vec_t color) {
        pixel::set_pixel_color(color);
        for (float x = box1.bl.x; x < box1.tr.x; x+=1.0f) { // NOLINT
            lineseg_t::draw(point_t(x, box1.bl.y), point_t(x, box1.tr.y));
        }
        for (float y = box1.bl.y; y < box1.tr.y; y+=1.0f) { // NOLINT
            lineseg_t::draw(point_t(box1.bl.x, y), point_t(box1.tr.x, y));
        }
    }

    void print(FILE *out) const noexcept {
        fprintf(out, "Maze Screen-Dim %s:\n", screen_dim.toString().c_str());
        fprintf(out, "Maze Field-Pos  %s:\n", field_offset.toString().c_str());
        fprintf(out, "Maze Field-Dim  %s:\n", field_dim.toString().c_str());
        fprintf(out, "Maze box1       %s:\n", box1.toString().c_str());
        fprintf(out, "Maze box2       %s:\n", box2.toString().c_str());
        fprintf(out, "Maze tiles      %zux%zu:\n", tiles.columns(), tiles.rows());
        fprintf(out, "Maze home-ext   %s:\n", home_ext.toString().c_str());
        fprintf(out, "Maze home-int   %s:\n", home_int.toString().c_str());
        fprintf(out, "Maze connector  %zu\n", connectors.size());
        for(auto &c : connectors) {
            fprintf(out, "- a %s, b %s\n", c.a.toString().c_str(), c.b.toString().c_str());
        }
        for(size_t y=tiles.rows(); y-->0; ) {
            fprintf(out, "%3.3zu: ", y);
            for(size_t x=0; x<tiles.columns(); ++x) {
                fprintf(out, "%c", getChar(tiles[x,y]));
            }
            fprintf(out, "\n");
        }
        fprintf(out, "\n");
    }
};
static Maze maze;

constexpr int round_center(float &f, float &d) noexcept {
    const float r = std::round(f);
    d = r - f;
    f = r;
    return int(r);
}

static bool crossed_center(const point_t &p, const vec_t &d) noexcept {
    bool rx, ry;
    if (d.x < 0) {
        const float frac = std::ceil(p.x) - p.x; // left
        rx = frac <= -d.x;
    } else {
        const float frac = p.x - std::floor(p.x); // right
        rx = frac <= d.x;
    }
    if (d.y < 0) {
        const float frac = std::ceil(p.y) - p.y; // down
        ry = frac <= -d.y;
    } else {
        const float frac = p.y - std::floor(p.y); // up
        ry = frac <= d.y;
    }
    return rx && ry;
}

struct spos_t {
    point_t pos;
    bool is_center = false;
    std::vector<aabbox_t> exclude = {};

    bool one_step(const direction_t dir) noexcept {
        pos.x = std::round(pos.x);
        pos.y = std::round(pos.y);
        si_angle_f32 angle = dir_to_angle(dir);
        vec_t d = vec_t::from_length_angle(1.0f, angle);
        return step(dir, d);
    }

    bool is_legal_pos(size_t x, size_t y) const noexcept {
        if (maze.hasWall(x, y)) {
            return true;
        }
        const point_t p = { float(x), float(y) };
        for(const aabbox_t &b : exclude) {
            if (b.contains(p)) {
                return true;
            }
        }
        return false;
    }

    bool step(const direction_t dir, vec_t &d) noexcept {
        const vec_t pos1 = pos;
        const vec_t pos2 = pos + d;
        float x1 = pos1.x;
        float y1 = pos1.y;
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
                yi2 = (size_t)y2;
                if (is_legal_pos(xi2, yi2)) {
                    const float frac = y1 - std::trunc(y1);
                    if (0.0f < frac && frac < 0.5f) {
                        d.y = -frac; // round-down to center
                    } else {
                        hit = true;
                    }
                }
                break;
            case direction_t::LEFT:
                yi2 = round_center(y2, d.y);
                xi2 = (size_t)x2;
                if (is_legal_pos(xi2, yi2)) {
                    const float frac = x1 - std::trunc(x1);
                    if (0.0f < frac && frac < 0.5f) {
                        d.x = -frac; // round-down to center
                    } else {
                        hit = true;
                    }
                }
                break;
            case direction_t::RIGHT:
                yi2 = round_center(y2, d.y);
                xi2 = (size_t)std::ceil(x2);
                if (is_legal_pos(xi2, yi2)) {
                    const float frac = x1 - std::trunc(x1);
                    if (0.5f < frac) {
                        d.x = 1 - frac; // round-up to center
                    } else {
                        hit = true;
                    }
                }
                break;
            case direction_t::UP:
            default:
                xi2 = round_center(x2, d.x);
                yi2 = (size_t)std::ceil(y2);
                if (is_legal_pos(xi2, yi2)) {
                    const float frac = y1 - std::trunc(y1);
                    if (0.5f < frac) {
                        d.y = 1 - frac; // round-up to center
                    } else {
                        hit = true;
                    }
                }
                break;
        }
        if (!hit) {
            pos += d;
            is_center = crossed_center(pos, d);
        }
        if ( DEBUG_ON && hit ) {
            jau::log_printf("XXX: %2.2f/%2.2f -> %2.2f/%2.2f -> %zu / %zu; pos %s -> %s -> %s, d %s, center %d, hit %d\n",
                x1, y1, x2, y2, xi2, yi2, pos1.toString().c_str(), pos2.toString().c_str(), pos.toString().c_str(), d.toString().c_str(), is_center, hit);
        }
        return !hit;
    }
};

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
    constexpr static si_length_f32 nose_length = height/6;
    constexpr static si_velo_f32 def_velocity = 7.0f; // [m/s]
    constexpr static size_t max_balls = 10;
    constexpr static const float ball_radius = 20_cm;
    constexpr static const float ball_angle = 10_deg;
    constexpr static const float ball_velocity_min = 11_m_s; // [m/s]
    constexpr static const float ball_velocity_max = 25_m_s; //
    player_id_t m_id;
    point_t m_start;
    disk_t m_head;
    rect_t m_shoulders;
    lineseg_t m_nose;
    aabbox_t m_aabbox;
    spos_t m_center;
    vec_t m_velocity_v; // [m/s]
    float m_velocity_s; // [m/s]
    direction_t m_dir;
    float m_ausdauer;
    bool m_run;
    float m_last_dt = 1.0f/60.0f;
    float m_ball_velo_s = ball_velocity_min;
    std::vector<physiks::ball_ref_t> m_balls;
    int m_score = 0;

    void move(const vec_t d) noexcept {
        m_head.move(d);
        m_shoulders.move(d);
        m_nose.move(d);
        m_aabbox.move(d);
        m_center.pos = m_aabbox.center();
    }

    void rotateTo(const si_angle_f32 rad) noexcept {
        rotate(rad - dir_to_angle(m_dir));
    }

    void rotate(si_angle_f32 rad) noexcept {
        m_shoulders.rotate(rad);
        m_nose.rotate(rad, m_head.center);
        m_velocity_v.rotate(rad);
        m_aabbox.reset();
        m_aabbox.resize(m_head.box());
        m_aabbox.resize(m_shoulders.box());
        m_center.pos = m_aabbox.center();
    }

    bool test_ball_erna(physiks::ball_t &ball) const noexcept;

  public:

    Player(player_id_t id, const point_t position) noexcept
    : m_id(id), m_start(position), m_head(position, height / 2), m_aabbox(),
      m_velocity_v(), m_velocity_s(0), m_dir(direction_t::UP),
      m_ausdauer(max_ausdauer), m_run(false)
    {
        // build in direction_t::UP
        const point_t tl = point_t(position.x - width/2, position.y + shoulder_height/2);
        m_shoulders = rect_t(tl, width, shoulder_height);
        m_nose = lineseg_t(m_head.center+vec_t(0, m_head.radius), m_head.center+vec_t(0, m_head.radius + nose_length));
        m_aabbox.resize(m_head.box());
        m_aabbox.resize(m_shoulders.box());
        m_center.pos = m_aabbox.center();
        // rotate to start-direction
        rotateTo(dir_to_angle(start_dir));
        m_dir = start_dir;
        // move to start-position
        move(m_start - pos());
        if (is_fritzchen()) {
            m_center.exclude.push_back(maze.home_ext);
        }
    }

    Player(player_id_t id) noexcept
    : m_id(id) {}

    bool is_center() const noexcept { return m_center.is_center; }
    const point_t& pos() const noexcept { return m_center.pos; }
    const point_t& nasenpos() const noexcept { return m_nose.p1; }
    direction_t dir() const noexcept { return m_dir; }
    const vec_t &velo_v() const noexcept { return m_velocity_v; }
    const float &velo_s() const noexcept { return m_velocity_s; }
    bool is_running() const noexcept { return m_run; }
    float ausdauer() const noexcept { return m_ausdauer; }

    constexpr player_id_t id() const noexcept { return m_id; }
    constexpr bool is_erna() const noexcept { return player_id_t::erna == m_id; }
    constexpr bool is_fritzchen() const noexcept { return player_id_t::fritzchen == m_id; }

    int score() const noexcept { return m_score; }
    int& score() noexcept { return m_score; }

    void reset() noexcept {
        m_velocity_v = vec_t();
        m_velocity_s = 0;
        m_ausdauer = max_ausdauer;
        m_run = false;
        rotateTo(dir_to_angle(start_dir));
        m_dir = start_dir;
        move(m_start - pos());
        m_balls.clear();
        m_score = 0;
    }

    void setVelo(const float speed) noexcept {
        m_velocity_v = vec_t::from_length_angle(speed, dir_to_angle(m_dir));
        m_velocity_s = speed;
    }

    bool setDir(direction_t dir, const float speed=def_velocity) noexcept {
        if( dir == m_dir && jau::equals(m_velocity_s, speed)) {
            return true;
        }
        if (!test_step(dir, speed)) {
            return false;
        }
        const si_angle_f32 angle = dir_to_angle(dir);
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
            nasenpos(), ball_radius, m_ball_velo_s,
            dir_to_angle(dir()) + ball_angle, // angle/direction of velocity: Fritz + ball_angle
            m_ball_velo_s, // max velocity (scalar)
            false /* reset */) );
        m_balls.back()->boost_factor = 1;
        m_balls.back()->brake_factor = 1;
        m_balls.back()->bounce_callback = [](const physiks::ball_t &) { audio_ball->play(); };
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
                if( DEBUG_ON ) {
                    jau::log_printf("maze-box %s\nball-box %s\n", maze.box2.toString().c_str(), b->box().toString().c_str());
                }
            } else if (b->velocity.length() < 3) {
                // delete i
                m_balls.erase(i);
                if( DEBUG_ON ) {
                    jau::log_printf("ball-velo %s\n", b->velocity.toString().c_str());
                }
            } else if(is_fritzchen() && test_ball_erna(*b)) {
                // delete i
                m_balls.erase(i);
                m_score += number(score_t::ball_hit);
                if( DEBUG_ON ) {
                    jau::log_printf("ball-erna %s\n", b->box().toString().c_str());
                }
            } else {
                ++i;
            }
        }
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

    bool test_step(const direction_t dir, float speed=def_velocity) const noexcept {
        if(m_run && m_ausdauer > 0 ) {
            speed *= 2;
        }
        vec_t velocity = vec_t::from_length_angle(speed, dir_to_angle(dir));
        vec_t d = velocity * m_last_dt; // [m/s] * [s] = [m]
        spos_t p = m_center;
        return p.step(dir, d);
    }

    bool step(const direction_t dir, vec_t &d) noexcept {
        return m_center.step(dir, d);
    }

    bool contains(const point_t &p) const noexcept {
        return m_aabbox.contains(p);
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
static Player fritzchen(player_id_t::fritzchen);
static Player erna(player_id_t::erna);

bool Player::test_ball_erna(physiks::ball_t &ball) const noexcept {
    return erna.m_aabbox.intersects(ball.box());
}

static float calc_min_dist(const point_t &src, const point_t &dest) noexcept {
    float l1 = (dest - src).length();
    float l2 = std::numeric_limits<float>::max();
    for(const Connector &c : maze.connectors) {
        float lca = (c.a - dest).length();
        lca += (c.b - src).length();

        float lcb = (c.a - src).length();
        lcb += (c.b - dest).length();
        l2 = std::min({l2, lca, lcb});
    }
    return std::min(l1, l2);
}

static void erna_bot_tick(float dt) {
    static point_t stored_seek_pos;
    point_t target_pos;

    if(erna_mode == strategy_t::seek_pos) {
        target_pos = stored_seek_pos;
        if ((target_pos - erna.pos()).length() < 2.0f) {
            erna_mode = strategy_t::escape;
            erna_mode_to = number(strategy_duration_t::escape);
        }
    } else {
        target_pos = fritzchen.pos();
        if (fritzchen.contains(erna.pos())) {
            if(erna_mode == strategy_t::attract) {
                erna_mode = strategy_t::escape;
                erna_mode_to = number(strategy_duration_t::escape);
            } else {
                if(erna_mode == strategy_t::escape) {
                    fritzchen.score() += number(score_t::erna_tick_escape);
                }
                stored_seek_pos = maze.get_random_upper_pos();
                target_pos = stored_seek_pos;
                erna_mode = strategy_t::seek_pos;
                erna_mode_to = number(strategy_duration_t::seekpos);
            }
        } else if (maze.home_int.contains(erna.pos())) {
            fritzchen.score() += erna_mode == strategy_t::attract ?
                number(score_t::erna_home_attract) :
                (erna_mode == strategy_t::escape ? number(score_t::erna_home_escape) : number(score_t::erna_home_random));
            stored_seek_pos = maze.get_random_upper_pos();
            target_pos = stored_seek_pos;
            erna_mode = strategy_t::seek_pos;
            erna_mode_to = number(strategy_duration_t::seekpos);
        }
    }
    if (erna_mode_to >= dt) {
        erna_mode_to -= dt;
    } else {
        switch (erna_mode) {
            case strategy_t::attract:
                [[fallthrough]];
            case strategy_t::seek_pos:
                erna_mode = strategy_t::escape;
                erna_mode_to = number(strategy_duration_t::escape);
                break;
            case strategy_t::escape:
                erna_mode = strategy_t::random;
                erna_mode_to = number(strategy_duration_t::random);
                break;
            case strategy_t::random:
                [[fallthrough]];
            default:
                erna_mode = strategy_t::attract;
                erna_mode_to = number(strategy_duration_t::attract);
                break;
        }
    }

    if (erna.is_center()) {
        constexpr int R = number(direction_t::RIGHT);
        constexpr int D = number(direction_t::DOWN);
        constexpr int L = number(direction_t::LEFT);
        constexpr int U = number(direction_t::UP);
        constexpr float d_threshold = 0.5f;
        constexpr float d_threshold_bwd = 0.8f;

        const direction_t fwd_dir = erna.dir();
        const direction_t left_dir = rot_left(fwd_dir);
        const direction_t right_dir = rot_right(fwd_dir);
        const direction_t bwd_dir = inverse(fwd_dir);
        const point_t pe = erna.pos();

        spos_t dir_pos[4] = { { pe }, { pe }, { pe }, { pe } }; // R D L U
        direction_t new_dir = fwd_dir;

        bool dir_ok[] {
            dir_pos[R].one_step(direction_t::RIGHT),
            dir_pos[D].one_step(direction_t::DOWN),
            dir_pos[L].one_step(direction_t::LEFT),
            dir_pos[U].one_step(direction_t::UP) };

        if( !dir_ok[ number(left_dir) ] && !dir_ok[ number(right_dir) ] ) {
            // walls left and right
            if( dir_ok[ number(fwd_dir) ] ) {
                // straight ahead and walls left and right
                new_dir = fwd_dir;
            } else {
                // dead-end, can only return .. unusual (not in orig map)
                new_dir = bwd_dir;
            }
        } else if (erna_mode == strategy_t::random) {
            do {
                new_dir = next_rnd_dir();
            } while (!dir_ok[number(new_dir)] || new_dir == bwd_dir);
        } else {
            // decision: find longest path
            const float d_fwd   = calc_min_dist(dir_pos[number(fwd_dir)].pos, target_pos);
            const float d_right = calc_min_dist(dir_pos[number(right_dir)].pos, target_pos);
            const float d_left  = calc_min_dist(dir_pos[number(left_dir)].pos, target_pos);
            const float d_bwd   = calc_min_dist(dir_pos[number(bwd_dir)].pos, target_pos);

            if (erna_mode == strategy_t::escape) {
                float d_max = 0;
                if( dir_ok[number(fwd_dir)] ) {
                    d_max = d_fwd;
                }
                if( dir_ok[number(right_dir)] && d_right - d_max > d_threshold) {
                    d_max = d_right;
                    new_dir = right_dir;
                }
                if( dir_ok[number(left_dir)] && d_left - d_max > d_threshold) {
                    d_max = d_left;
                    new_dir = left_dir;
                }
                if( dir_ok[number(bwd_dir)] && d_bwd - d_max > d_threshold_bwd) {
                    new_dir = bwd_dir;
                }
            } else if (erna_mode == strategy_t::attract || erna_mode == strategy_t::seek_pos) {
                float d_min = std::numeric_limits<float>::max();
                if( dir_ok[number(fwd_dir)] ) {
                    d_min = d_fwd;
                }
                if( dir_ok[number(right_dir)] && d_min - d_right > d_threshold) {
                    d_min = d_right;
                    new_dir = right_dir;
                }
                if( dir_ok[number(left_dir)] && d_min - d_left > d_threshold) {
                    d_min = d_left;
                    new_dir = left_dir;
                }
                if( dir_ok[number(bwd_dir)] && d_min - d_bwd > d_threshold_bwd) {
                    new_dir = bwd_dir;
                }
            }
            if (DEBUG_ON) {
                jau::log_printf("EEE: decision.%d: dist[fwd %f, right %f, left %f, back %f\n", int(erna_mode), d_fwd, d_right, d_left, d_bwd);
            }
        }
        if (DEBUG_ON) {
            jau::log_printf("EEE: center-pos: %s\n", pe.toString().c_str());
            jau::log_printf("EEE: decision.%d: dir_ok[fwd %d, right %d, left %d, back %d\n", int(erna_mode), dir_ok[number(fwd_dir)], dir_ok[number(right_dir)], dir_ok[number(left_dir)], dir_ok[number(bwd_dir)]);
            jau::log_printf("EEE: decision.%d: dir %c -> %c\n", int(erna_mode), dir_to_char(fwd_dir), dir_to_char(new_dir));
        }
        erna.setDir(new_dir);
    }
    if(!erna.tick(dt)) {
        printf("EEE: COLLISION: dir %c\n", dir_to_char(erna.dir())); // should not happen
    }
    if (debug_gfx1) {
        pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
        lineseg_t::draw(erna.pos(), target_pos);
    }
}

///
///

static void reset_all() noexcept {
    t_start = jau::getElapsedMillisecond();
    fritzchen.reset();
    erna.reset();
    erna_mode = strategy_t::attract;
    erna_mode_to = number(strategy_duration_t::attract);
    erna.setDir(direction_t::LEFT);
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
        EMSCRIPTEN_KEEPALIVE void set_debug_gfx(bool v) noexcept { debug_gfx1 = v; }
    }
#endif

void mainloop() {
    static uint64_t frame_count_total = 0;
    static uint64_t snap_count = 0;
    static uint64_t t_last = getElapsedMillisecond();  // [ms]
    static pixel::input_event_t event;
    static bool animating = true;
    constexpr float text_height = 1.0f;

    bool do_snapshot = false;
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
            reset_all();
        } else if( event.released_and_clr(pixel::input_event_type_t::F12) ) {
            do_snapshot = true;
        } else if (event.released_and_clr(pixel::input_event_type_t::P2_ACTION3)) {
            fritzchen.setRun(false);
        } else if (event.released_and_clr(pixel::input_event_type_t::P2_ACTION2)) {
            fritzchen.throwBall();
        }
        if (event.pressed(pixel::input_event_type_t::P2_ACTION3)) {
            fritzchen.setRun(true);
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
        pixel::make_text2(text_tl, 0, {0, 0, 0, 1}, text_height, "%s s, fps %4.2f, score %4.4d",
            to_decstring((t1-t_start)/1000, ',', 5).c_str(), pixel::gpu_avg_fps(), fritzchen.score()),
        pixel::make_text2(text_tl, 1, {0, 0, 0, 0}, text_height, "run %2.2f, ball %2.2f, erna[%s, %.1fs]",
            fritzchen.ausdauer(), fritzchen.ball_velo(), to_string(erna_mode), erna_mode_to)
    };

    // white background
    pixel::clear_pixel_fb(255, 255, 255, 255);

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
        erna_bot_tick(dt);
    }

    if (debug_gfx1) {
        maze.draw_grid(pixel::f4::vec_t(0.8f, 0.8f, 0.8f, 255));
    }
    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
    maze.draw();

    switch (erna_mode) {
        case strategy_t::attract:
            pixel::set_pixel_color(    255 /* r */,   0 /* g */, 0 /* b */, 255 /* a */);
            break;
        case strategy_t::escape:
            pixel::set_pixel_color(    255 /* r */,   0 /* g */, 255 /* b */, 255 /* a */);
            break;
        case strategy_t::random:
        default:
            pixel::set_pixel_color(    0 /* r */,   0 /* g */, 0 /* b */, 255 /* a */);
            break;
    }
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
    if( record_bmpseq_basename.size() > 0 ) {
        std::string snap_fname(128, '\0');
        const int written = std::snprintf(&snap_fname[0], snap_fname.size(), "%s-%7.7" PRIu64 ".bmp", record_bmpseq_basename.c_str(), frame_count_total);
        snap_fname.resize(written);
        pixel::save_snapshot(snap_fname);
    }
    if( do_snapshot ) {
        std::string snap_fname(128, '\0');
        const int written = std::snprintf(&snap_fname[0], snap_fname.size(), "fritzchen-%7.7" PRIu64 ".bmp", snap_count++);
        snap_fname.resize(written);
        pixel::save_snapshot(snap_fname);
        printf("Snapshot written to %s\n", snap_fname.c_str());
    }
    ++frame_count_total;
}

int main(int argc, char *argv[])
{
    std::string maze_name = "fritzchen/maze4.txt";
    int window_width = 640, window_height = 720;
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
            } else if( 0 == strcmp("-fps", argv[i]) && i+1<argc) {
                pixel::set_gpu_forced_fps(atoi(argv[i+1]));
                ++i;
            } else if( 0 == strcmp("-debug_gfx", argv[i]) ) {
                debug_gfx1 = true;
            } else if( 0 == strcmp("-debug_gfx2", argv[i]) ) {
                debug_gfx2 = true;
            } else if( 0 == strcmp("-record", argv[i]) && i+1<argc) {
                record_bmpseq_basename = argv[i+1];
                ++i;
            }
        }
        const uint64_t elapsed_ms = getElapsedMillisecond();
        log_printf(elapsed_ms, "- record %s\n", record_bmpseq_basename.size()==0 ? "disabled" : record_bmpseq_basename.c_str());
        log_printf(elapsed_ms, "- forced_fps %d\n", pixel::gpu_forced_fps());
        log_printf(elapsed_ms, "- debug_gfx1 %d\n", debug_gfx1);
        log_printf(elapsed_ms, "- debug_gfx2 %d\n", debug_gfx2);
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
    maze.print(stdout);

    log_printf(0, "XX %s\n", pixel::cart_coord.toString().c_str());
    {
        float w = pixel::cart_coord.width();
        float h = pixel::cart_coord.height();
        float r01 = h/w;
        float a = w / h;
        printf("-w %f [x]\n-h %f [y]\n-r1 %f [y/x]\n-r2 %f [x/y]\n", w, h, r01, a);
    }

    pixel::cart_coord.set_fitting(0.0f, 0.0f, maze.screen_dim.x, maze.screen_dim.y);
    fritzchen = Player(player_id_t::fritzchen, maze.fritzchen_start);
    erna = Player(player_id_t::erna, maze.erna_start);
    erna.setDir(direction_t::LEFT);

    #if !defined(__EMSCRIPTEN__)
        jau::audio::init_audio_subsystem(MIX_INIT_OGG, MIX_INIT_OGG);
    #endif
    load_samples();
    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
    return 0;
}
