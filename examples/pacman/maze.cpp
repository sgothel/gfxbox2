/*
 * Author: Sven Gothel <sgothel@jausoft.com> and Svenson Han Gothel
 * Copyright (c) 2022 Gothel Software e.K.
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
#include "maze.hpp"

#include <cstring>
#include <iostream>
#include <fstream>
#include "game.hpp"
#include <jau/utils.hpp>

#include <strings.h>

//
// tile_t
//
std::string to_string(tile_t tile) noexcept {
    switch(tile) {
        case tile_t::EMPTY: return " ";
        case tile_t::WALL: return "X";
        case tile_t::GATE: return "-";
        case tile_t::PELLET: return ".";
        case tile_t::PELLET_POWER: return "*";
        case tile_t::CHERRY: return "C";
        case tile_t::STRAWBERRY: return "S";
        case tile_t::PEACH: return "P";
        case tile_t::APPLE: return "A";
        case tile_t::MELON: return "M";
        case tile_t::GALAXIAN: return "G";
        case tile_t::BELL: return "B";
        case tile_t::KEY: return "K";
        default: return "?";
    }
}

//
// acoord_t
//

std::string acoord_t::stats_t::toString() const noexcept {
    return "[walked["+std::to_string(fields_walked_f)+", "+std::to_string(fields_walked_i)+"], center "+std::to_string(field_center_count)+", entered "+std::to_string(field_entered_count)+"]";

}

acoord_t::acoord_t() noexcept
: x_pos_i(-1), y_pos_i(-1),
  x_pos_f(-1), y_pos_f(-1),
  last_dir_(direction_t::LEFT),
  last_collided(false)
{}

acoord_t::acoord_t(const int x, const int y) noexcept
: x_pos_i(x), y_pos_i(y),
  x_pos_f((float)x), y_pos_f((float)y),
  last_dir_(direction_t::LEFT),
  last_collided(false)
{}

acoord_t::acoord_t(const float x, const float y) noexcept
: x_pos_i(jau::round_to_int(x)), y_pos_i(jau::round_to_int(y)),
  x_pos_f(x), y_pos_f(y),
  last_dir_(direction_t::LEFT),
  last_collided(false)
{}

void acoord_t::set_pos(const int x, const int y) noexcept {
    x_pos_i = x;
    y_pos_i = y;
    x_pos_f = (float)x;
    y_pos_f = (float)y;
    last_dir_ = direction_t::LEFT;
    last_collided = false;
}

void acoord_t::set_pos(const float x, const float y) noexcept {
    x_pos_f = x;
    y_pos_f = y;
    x_pos_i = jau::round_to_int(x_pos_f);
    y_pos_i = jau::round_to_int(y_pos_f);
    last_dir_ = direction_t::LEFT;
    last_collided = false;
}

void acoord_t::set_pos_clipped(const float x, const float y) noexcept {
    maze_t& maze = *global_maze;
    x_pos_f = maze.clip_pos_x( x );
    y_pos_f = maze.clip_pos_y( y );
    x_pos_i = maze.clip_pos_x( jau::round_to_int(x_pos_f) );
    y_pos_i = maze.clip_pos_y( jau::round_to_int(y_pos_f) );
    last_dir_ = direction_t::LEFT;
    last_collided = false;
}

void acoord_t::set_centered(const keyframei_t& keyframei) noexcept {
    x_pos_f = keyframei.center_value(x_pos_f);
    y_pos_f = keyframei.center_value(y_pos_f);
}

void acoord_t::set_aligned_dir(const direction_t dir, const keyframei_t& keyframei) noexcept {
    switch( dir ) {
        case direction_t::RIGHT:
            [[fallthrough]];
        case direction_t::LEFT:
            x_pos_f = keyframei.align_value( x_pos_f );
            y_pos_f = keyframei.center_value( y_pos_f );
            break;

        case direction_t::DOWN:
            [[fallthrough]];
        case direction_t::UP:
            [[fallthrough]];
        default:
            x_pos_f = keyframei.center_value( x_pos_f );
            y_pos_f = keyframei.align_value( y_pos_f );
            break;
    }
}

bool acoord_t::intersects_f(const acoord_t& other) const noexcept {
    // use machine epsilon delta to avoid matching direct neighbors
#if 1
    // uniform dimension
    return std::abs( x_pos_f - other.x_f() ) < 1.0f - std::numeric_limits<float>::epsilon() &&
           std::abs( y_pos_f - other.y_f() ) < 1.0f - std::numeric_limits<float>::epsilon();
#else
    return !(       x_pos_f + 1.0f - std::numeric_limits<float>::epsilon() < other.x_pos_f ||
              other.x_pos_f + 1.0f - std::numeric_limits<float>::epsilon() <       x_pos_f ||
                    y_pos_f + 1.0f - std::numeric_limits<float>::epsilon() < other.y_pos_f ||
              other.y_pos_f + 1.0f - std::numeric_limits<float>::epsilon() <       y_pos_f );
#endif
}

bool acoord_t::intersects_i(const acoord_t& other) const noexcept {
    return x_pos_i == other.x_i() && y_pos_i == other.y_i();
}

bool acoord_t::intersects_i(const int x, const int y) const noexcept {
    return x_pos_i == x && y_pos_i == y;
}

bool acoord_t::intersects(const acoord_t& other) const noexcept {
    return use_original_pacman_behavior() ? intersects_i(other) : intersects_f(other);
}

bool acoord_t::intersects_f(const box_t& other) const noexcept {
    return !( x_pos_f   + 1.0f               - std::numeric_limits<float>::epsilon() < other.x_f() ||
              other.x_f() + other.width_f()  - std::numeric_limits<float>::epsilon() < x_pos_f ||
              y_pos_f   + 1.0f               - std::numeric_limits<float>::epsilon() < other.y_f() ||
              other.y_f() + other.height_f() - std::numeric_limits<float>::epsilon() < y_pos_f );
}

bool acoord_t::intersects_i(const box_t& other) const noexcept {
    return !( (float)x_pos_i   + 1.0f        - std::numeric_limits<float>::epsilon() < other.x_f() ||
              other.x_f() + other.width_f()  - std::numeric_limits<float>::epsilon() < (float)x_pos_i ||
              (float)y_pos_i   + 1.0f        - std::numeric_limits<float>::epsilon() < other.y_f() ||
              other.y_f() + other.height_f() - std::numeric_limits<float>::epsilon() < (float)y_pos_i );
}

float acoord_t::distance(const float x, const float y) const noexcept {
    const float x_d = x - x_pos_f;
    const float y_d = y - y_pos_f;
    return std::sqrt(x_d * x_d + y_d * y_d);
}

int acoord_t::distance_i(const int x, const int y) const noexcept {
    const int x_d = x - x_pos_i;
    const int y_d = y - y_pos_i;
    return (int)std::round( std::sqrt(x_d * x_d + y_d * y_d) );
}

float acoord_t::sq_distance(const float x, const float y) const noexcept {
    const float x_d = x - x_pos_f;
    const float y_d = y - y_pos_f;
    return x_d * x_d + y_d * y_d;
}

int acoord_t::sq_distance_i(const int x, const int y) const noexcept {
    const int x_d = x - x_pos_i;
    const int y_d = y - y_pos_i;
    return x_d * x_d + y_d * y_d;
}

float acoord_t::distance_manhatten(const float x, const float y) const noexcept {
    const float x_d = std::abs( x - x_pos_f );
    const float y_d = std::abs( y - y_pos_f );
    return x_d + y_d;
}

int acoord_t::distance_manhatten_i(const int x, const int y) const noexcept {
    const int x_d = std::abs( x - x_pos_i );
    const int y_d = std::abs( y - y_pos_i );
    return x_d + y_d;
}

void acoord_t::incr_fwd(const direction_t dir, const keyframei_t& keyframei, const int tile_count) noexcept {
    const float fields_per_frame = (float)tile_count;
    maze_t& maze = *global_maze;

    switch( dir ) {
        case direction_t::DOWN:
            if( jau::round_to_int(y_pos_f + fields_per_frame) < maze.height() ) {
                y_pos_f = keyframei.align_value( y_pos_f + fields_per_frame );
            } else {
                y_pos_f = (float)maze.height() - 1; // clip only, no overflow to other side
            }
            y_pos_i = jau::trunc_to_int(y_pos_f);
            x_pos_f = keyframei.center_value((float)x_pos_i);
            break;

        case direction_t::RIGHT:
            if( jau::round_to_int(x_pos_f + fields_per_frame) < maze.width() ) {
                x_pos_f = keyframei.align_value( x_pos_f + fields_per_frame );
            } else {
                x_pos_f = (float)maze.width() - 1; // clip only, no overflow to other side
            }
            x_pos_i = jau::trunc_to_int(x_pos_f);
            y_pos_f = keyframei.center_value((float)y_pos_i);
            break;

        case direction_t::UP:
            if( jau::round_to_int(y_pos_f - fields_per_frame) >= 0 ) {
                y_pos_f = keyframei.align_value( y_pos_f - fields_per_frame );
            } else {
                y_pos_f = 0; // clip only, no overflow to other side
            }
            y_pos_i = jau::trunc_to_int(y_pos_f);
            x_pos_f = keyframei.center_value((float)x_pos_i);
            break;

        case direction_t::LEFT:
            [[fallthrough]];
        default:
            if( jau::round_to_int(x_pos_f - fields_per_frame) >= 0 ) {
                x_pos_f = keyframei.align_value( x_pos_f - fields_per_frame );
            } else {
                x_pos_f = 0; // clip only, no overflow to other side
            }
            x_pos_i = jau::trunc_to_int(x_pos_f);
            y_pos_f = keyframei.center_value((float)y_pos_i);
            break;
    }
}

bool acoord_t::step_impl(direction_t dir, const bool test_only, const keyframei_t& keyframei, const collisiontest_simple_t& ct0) noexcept {
    const float epsilon = std::numeric_limits<float>::epsilon();
    const float step_width = keyframei.fields_per_frame();
    const float half_step = step_width / 2.0f;
    const float center = keyframei.center();
    maze_t& maze = *global_maze;

    /**
     * The new float position, pixel accurate.
     */
    float new_x_pos_f;
    float new_y_pos_f;

    /**
     * The new int position.
     */
    int new_x_pos_i;
    int new_y_pos_i;

    /**
     * The forward look-ahead int position for wall collision tests,
     * also avoid overstepping center position if wall is following.
     *
     * Depending on the direction, it adds a whole tile position
     * to the pixel accurate position only to test for wall collisions.
     *
     * This way, it avoids 'overstepping' from its path!
     */
    int fwd_x_pos_i;
    int fwd_y_pos_i;

    /**
     * This step walking distance will be accumulated for statistics.
     */
    float fields_stepped_f = 0;

    /**
     * Resulting int position is weighted
     * as the original pacman game.
     *
     * Note: This also allows 'cutting the corner' for pacman to speed up.
     */
    switch( dir ) {
        case direction_t::DOWN:
            if( y_pos_f + step_width < (float)maze.height() + center - half_step - epsilon ) {
                // next
                new_y_pos_f = keyframei.align_value( y_pos_f + step_width );
                fields_stepped_f = step_width;

                if( new_y_pos_f - std::trunc(new_y_pos_f) > center - half_step - epsilon ) {
                    new_y_pos_i = jau::trunc_to_int(new_y_pos_f);
                } else {
                    new_y_pos_i = std::max(0, jau::trunc_to_int(new_y_pos_f) - 1);
                }
                // forward
                if( new_y_pos_f - std::trunc(new_y_pos_f) > center + half_step + epsilon ) {
                    fwd_y_pos_i = std::min(maze.height()-1, jau::trunc_to_int(new_y_pos_f) + 1);
                } else {
                    fwd_y_pos_i = jau::trunc_to_int(new_y_pos_f);
                }
            } else {
                // smooth wrapping bottom to top screen
                new_y_pos_f = 0 - keyframei.center() + step_width; // position above center to let new position hit, i.e. below of center of previous tile
                new_y_pos_f = 0; // is less than center, hence OK
                fwd_y_pos_i = 0;
                new_y_pos_i = 0;
            }
            new_x_pos_f = keyframei.center_value((float)x_pos_i);
            new_x_pos_i = x_pos_i;
            fwd_x_pos_i = x_pos_i;
            break;

        case direction_t::RIGHT:
            if( x_pos_f + step_width < (float)maze.width() + center - half_step - epsilon ) {
                // next
                new_x_pos_f = keyframei.align_value( x_pos_f + step_width );
                fields_stepped_f = step_width;
                if( new_x_pos_f - std::trunc(new_x_pos_f) >= center - half_step - epsilon ) {
                    new_x_pos_i = jau::trunc_to_int(new_x_pos_f);
                } else {
                    new_x_pos_i = std::max(0, jau::trunc_to_int(new_x_pos_f) - 1);
                }
                // forward
                if( new_x_pos_f - std::trunc(new_x_pos_f) > center + half_step + epsilon ) {
                    fwd_x_pos_i = std::min(maze.width()-1, jau::trunc_to_int(new_x_pos_f) + 1);
                } else {
                    fwd_x_pos_i = jau::trunc_to_int(new_x_pos_f);
                }
            } else {
                // smooth wrapping right to left screen
                new_x_pos_f = 0 - keyframei.center() + step_width; // position left of center to let new position hit, i.e. right of center of previous tile
                fwd_x_pos_i = 0;
                new_x_pos_i = 0;
            }
            new_y_pos_f = keyframei.center_value((float)y_pos_i);
            new_y_pos_i = y_pos_i;
            fwd_y_pos_i = y_pos_i;
            break;

        case direction_t::UP:
            if( y_pos_f - step_width > -center + half_step + epsilon ) {
                // next
                new_y_pos_f = keyframei.align_value( y_pos_f - step_width );
                fields_stepped_f = step_width;
                if( new_y_pos_f - std::trunc(new_y_pos_f) > center - half_step - epsilon ) {
                    new_y_pos_i = jau::trunc_to_int(new_y_pos_f);
                } else {
                    new_y_pos_i = std::max(0, jau::trunc_to_int(new_y_pos_f) - 1);
                }
                // forward is same
                fwd_y_pos_i = new_y_pos_i;
            } else {
                // smooth wrapping top to bottom screen
                new_y_pos_f = keyframei.align_value( (float)maze.height() + center - half_step ); // position below center to let new position hit, i.e. above of center of previous tile
                fwd_y_pos_i = maze.height() - 1;
                new_y_pos_i = fwd_y_pos_i;
            }
            new_x_pos_f = keyframei.center_value((float)x_pos_i);
            new_x_pos_i = x_pos_i;
            fwd_x_pos_i = x_pos_i;
            break;

        case direction_t::LEFT:
            [[fallthrough]];
        default:
            if( x_pos_f - step_width > -center + half_step + epsilon ) {
                // next
                new_x_pos_f = keyframei.align_value( x_pos_f - step_width );
                fields_stepped_f = step_width;
                if( new_x_pos_f - std::trunc(new_x_pos_f) > center - half_step - epsilon ) {
                    new_x_pos_i = jau::trunc_to_int(new_x_pos_f);
                } else {
                    new_x_pos_i = std::max(0, jau::trunc_to_int(new_x_pos_f) - 1);
                }
                // forward is same
                fwd_x_pos_i = new_x_pos_i;
            } else {
                // smooth wrapping left to right screen
                new_x_pos_f = keyframei.align_value( (float)maze.width() + center - half_step ); // position right of center to let new position hit, i.e. left of center of previous tile
                fwd_x_pos_i = maze.width() - 1;
                new_x_pos_i = fwd_x_pos_i;
            }
            new_y_pos_f = keyframei.center_value((float)y_pos_i);
            new_y_pos_i = y_pos_i;
            fwd_y_pos_i = y_pos_i;
            break;

    }
    // Collision test with walls
    const tile_t fwd_tile = maze.tile(fwd_x_pos_i, fwd_y_pos_i);
    const bool new_pos_is_center = keyframei.is_center(new_x_pos_f, new_y_pos_f);
    const bool new_pos_entered = entered_tile(keyframei, dir, new_x_pos_f, new_y_pos_f);
    bool collision;
    if( nullptr != ct0 ) {
        collision = ct0(fwd_tile);
    } else {
        collision = false;
    }
    if( !test_only && false ) {
        jau::log_printf("%s: %s -> %s: %9.6f/%9.6f %2.2d/%2.2d c%d e%d -> new %9.6f/%9.6f %2.2d/%2.2d c%d e%d -> fwd %2.2d/%2.2d, tile '%s', collision %d\n",
                test_only ? "test" : "step",
                to_string(last_dir_).c_str(), to_string(dir).c_str(),
                x_pos_f, y_pos_f, x_pos_i, y_pos_i, keyframei.is_center(x_pos_f, y_pos_f), entered_tile(keyframei, dir, x_pos_f, y_pos_f),
                new_x_pos_f, new_y_pos_f, new_x_pos_i, new_y_pos_i, new_pos_is_center, new_pos_entered,
                fwd_x_pos_i, fwd_y_pos_i,
                to_string(fwd_tile).c_str(), collision);
    }
    if( !test_only ) {
        if( !collision ) {
            last_collided = false;
            x_pos_f = new_x_pos_f;
            y_pos_f = new_y_pos_f;
            const int x_pos_i_old = x_pos_i;
            const int y_pos_i_old = y_pos_i;
            x_pos_i = new_x_pos_i;
            y_pos_i = new_y_pos_i;
            last_dir_ = dir;
            stats_.fields_walked_i += std::abs(x_pos_i - x_pos_i_old) + std::abs(y_pos_i - y_pos_i_old);
            stats_.fields_walked_f += fields_stepped_f;
            if( new_pos_is_center ) {
                stats_.field_center_count++;
            }
            if( new_pos_entered ) {
                stats_.field_entered_count++;
            }
        } else {
            last_collided = true;
        }
    }
    return !collision;
}

bool acoord_t::entered_tile(const keyframei_t& keyframei, const direction_t dir, const float x, const float y) noexcept {
    const float epsilon = std::numeric_limits<float>::epsilon();
    const float step_width = keyframei.fields_per_frame();
    const float half_step = step_width / 2.0f;
    const float center = keyframei.center();

    switch( dir ) {
        case direction_t::RIGHT: {
            const float center_p1 = center - half_step - epsilon;
            const float center_p2 = center + half_step - epsilon;
            const float v0_m = x - std::trunc(x);
            return center_p1 < v0_m && v0_m < center_p2;
        }
        case direction_t::LEFT: {
            const float center_p1 = std::max(0.0f, center - step_width - half_step - epsilon);
            const float center_p2 = center - half_step - epsilon;
            const float v0_m = x - std::trunc(x);
            return center_p1 < v0_m && v0_m < center_p2;
        }
        case direction_t::DOWN: {
            const float center_p1 = center - half_step - epsilon;
            const float center_p2 = center + half_step - epsilon;
            const float v0_m = y - std::trunc(y);
            return center_p1 < v0_m && v0_m < center_p2;
        }
        case direction_t::UP:
            [[fallthrough]];
        default: {
            const float center_p1 = std::max(0.0f, center - step_width - half_step - epsilon);
            const float center_p2 = center - half_step - epsilon;
            const float v0_m = y - std::trunc(y);
            return center_p1 < v0_m && v0_m < center_p2;
        }
    }
}

std::string acoord_t::toString() const noexcept {
    std::string s("[");
    s.append(std::to_string(x_pos_f)).append("/").append(std::to_string(y_pos_f)).append(" ").append(std::to_string(x_pos_i))
     .append("/").append(std::to_string(y_pos_i)).append(", last[dir ").append(to_string(last_dir_))
     .append(", collided ").append(std::to_string(last_collided)).append("], ").append(stats_.toString()).append("]");
    return s;
}

std::string acoord_t::toShortString() const noexcept {
    std::string s("[");
    s.append("[").append(std::to_string(x_pos_f)).append("/").append(std::to_string(y_pos_f))
     .append(" ").append(std::to_string(x_pos_i)).append("/").append(std::to_string(y_pos_i)).append("]");
    return s;
}

std::string acoord_t::toIntString() const noexcept {
    return std::to_string(x_pos_i)+"/"+std::to_string(y_pos_i);
}

//
// maze_t::field_t
//

maze_t::field_t::field_t() noexcept
: width_(0), height_(0)
{
    std::memset(&count_, 0, sizeof(count_));
}

void maze_t::field_t::clear() noexcept {
    width_ = 0; height_ = 0;
    tiles.clear();
    std::memset(&count_, 0, sizeof(count_));
}

tile_t maze_t::field_t::tile(const int x, const int y) const noexcept {
    if( 0 <= x && x < width_ && 0 <= y && y < height_ ) {
        return tiles[y*width_+x];
    }
    return tile_t::EMPTY;
}

void maze_t::field_t::add_tile(const tile_t tile) noexcept {
    tiles.push_back(tile);
    ++count_[number(tile)];
}

void maze_t::field_t::set_tile(const int x, const int y, tile_t tile) noexcept {
    if( 0 <= x && x < width_ && 0 <= y && y < height_ ) {
        const tile_t old_tile = tiles[y*width_+x];
        tiles[y*width_+x] = tile;
        --count_[number(old_tile)];
        ++count_[number(tile)];
    }
}

std::string maze_t::field_t::toString() const noexcept {
    return "field["+std::to_string(width_)+"x"+std::to_string(height_)+", pellets["+std::to_string(count(tile_t::PELLET))+", power "+std::to_string(count(tile_t::PELLET_POWER))+"]]";
}

//
// maze_t
//

bool maze_t::digest_iposition_line(const std::string& name, acoord_t& dest, const std::string& line) noexcept {
    if( -1 == dest.x_i() || -1 == dest.y_i() ) {
        int x_pos = 0, y_pos = 0;
        sscanf(line.c_str(), "%d %d", &x_pos, &y_pos);
        dest.set_pos(x_pos, y_pos);
        if( DEBUG_ON ) {
            jau::log_printf("maze: read %s position: %s\n", name.c_str(), dest.toString().c_str());
        }
        return true;
    } else {
        return false;
    }
}

bool maze_t::digest_fposition_line(const std::string& name, acoord_t& dest, const std::string& line) noexcept {
    if( -1 == dest.x_i() || -1 == dest.y_i() ) {
        float x_pos = 0.0f, y_pos = 0.0f;
        sscanf(line.c_str(), "%f %f", &x_pos, &y_pos);
        dest.set_pos(x_pos, y_pos);
        if( DEBUG_ON ) {
            jau::log_printf("maze: read %s position: %s\n", name.c_str(), dest.toString().c_str());
        }
        return true;
    } else {
        return false;
    }
}

bool maze_t::digest_ibox_line(const std::string& name, box_t& dest, const std::string& line) noexcept {
    if( -1 == dest.x() || -1 == dest.y() ) {
        int x_pos = 0, y_pos = 0, w = 0, h = 0;
        sscanf(line.c_str(), "%d %d %d %d", &x_pos, &y_pos, &w, &h);
        dest.set(x_pos, y_pos, w, h);
        if( DEBUG_ON ) {
            jau::log_printf("maze: read %s box: %s\n", name.c_str(), dest.toString().c_str());
        }
        return true;
    } else {
        return false;
    }
}

maze_t::maze_t(const std::string& fname0) noexcept
: filename(fname0)
{
    const std::string fname1 = pixel::resolve_asset(fname0);
    if( !fname1.size() ) {
        jau::log_printf("maze_t: Could locate file '%s' in asset dir '%s'\n", fname0.c_str(), pixel::asset_dir().c_str());
        return;
    }
    int field_line_iter = 0;
    std::fstream file;
    file.open(fname1, std::ios::in);
    if( file.is_open() ) {
        std::string line;
        while( std::getline(file, line) ) {
            if( 0 == original.width() || 0 == original.height() ) {
                int w=-1, h=-1;
                int visual_width=-1, visual_height=-1;
                sscanf(line.c_str(), "%d %d %d %d", &w, &h, &visual_width, &visual_height);
                set_dim(w, h, visual_width, visual_height);
                if( DEBUG_ON ) {
                    jau::log_printf("maze: read dimension: %s\n", toString().c_str());
                }
            } else if( digest_iposition_line("top_left_scatter", top_left_scatter_, line) ) {
            } else if( digest_iposition_line("bottom_left_scatter", bottom_left_scatter_, line) ) {
            } else if( digest_iposition_line("bottom_right_scatter", bottom_right_scatter_, line) ) {
            } else if( digest_iposition_line("top_right_scatter", top_right_scatter_, line) ) {
            } else if( digest_ibox_line("tunnel1", tunnel1, line) ) {
            } else if( digest_ibox_line("tunnel2", tunnel2, line) ) {
            } else if( digest_ibox_line("red_zone1", red_zone1, line) ) {
            } else if( digest_ibox_line("red_zone2", red_zone2, line) ) {
            } else if( digest_fposition_line("pacman", pacman_start_pos_, line) ) {
            } else if( digest_ibox_line("ghost_home_ext", ghost_home_ext, line) ) {
            } else if( digest_ibox_line("ghost_home_int", ghost_home_int, line) ) {
            } else if( digest_ibox_line("ghost_start", ghost_start, line) ) {
            } else if( 0 == texture_file.length() ) {
                texture_file = line;
            } else if( field_line_iter < original.height() ) {
                if( DEBUG_ON ) {
                    jau::log_printf("maze: read line y = %d, len = %zd: %s\n", field_line_iter, line.length(), line.c_str());
                }
                if( line.length() == (size_t)original.width() ) {
                    for(int x=0; x<original.width(); ++x) {
                        const char c = line[x];
                        switch( c ) {
                            case '_':
                                original.add_tile(tile_t::EMPTY);
                                break;
                            case '|':
                                original.add_tile(tile_t::WALL);
                                break;
                            case '-':
                                original.add_tile(tile_t::GATE);
                                break;
                            case '.':
                                original.add_tile(tile_t::PELLET);
                                break;
                            case '*':
                                original.add_tile(tile_t::PELLET_POWER);
                                break;
                            default:
                                jau::log_printf("maze error: unknown tile @ %d / %d: '%c'\n", x, field_line_iter, c);
                                break;
                        }
                    }
                }
                ++field_line_iter;
            }
        }
        file.close();
        if( original.validate_size() ) {
            reset();

            // center below ghost_home_ext, 1 tile, centered horizontal
            fruit_pos_.set_pos(ghost_home_ext.center_x()-0.5f,  (float)(ghost_home_ext.y()+ghost_home_ext.height()));

            // below ghost_home_ext, centered, whole length, 1 tile height
            message_box_.set( ghost_home_ext.x(), ghost_home_ext.y()+ghost_home_ext.height(), ghost_home_ext.width(), 1);

            return; // OK
        }
    } else {
        jau::log_printf("Could not open maze file: %s\n", filename.c_str());
    }
    original.clear();
    pacman_start_pos_.set_pos(0, 0);
    ghost_home_ext.set(0, 0, 0, 0);
    ghost_home_int.set(0, 0, 0, 0);
    ghost_start.set(0, 0, 0, 0);
    ppt_x_ = 0;
    ppt_y_ = 0;
}

void maze_t::draw(const std::function<void(const float x, const float y, tile_t tile)>& draw_pixel) noexcept {
    for(int y=0; y<height(); ++y) {
        for(int x=0; x<width(); ++x) {
            if( fruit_pos_.intersects_i(x, y) ) {
                draw_pixel(fruit_pos_.x_f(), fruit_pos_.y_f(), active.tile_nc(x, y));
            } else {
                draw_pixel((float)x, (float)y, active.tile_nc(x, y));
            }
        }
    }
}

void maze_t::reset() noexcept {
    active = original;
}

std::string maze_t::toString() const noexcept {
    std::string errstr = is_ok() ? "ok" : "error";
    return filename+"["+errstr+", "+active.toString()+
                    ", pacman "+pacman_start_pos_.toShortString()+
                    ", ghost[ext "+ghost_home_ext.toString()+", int "+ghost_home_int.toString()+
                    ", start "+ghost_start.toString()+
                    "], fruit "+fruit_pos_.toShortString()+
                    ", pellets[ normal "+std::to_string(count(tile_t::PELLET))+"/"+std::to_string(max(tile_t::PELLET))+
                    ", power "+std::to_string(count(tile_t::PELLET_POWER))+"/"+std::to_string(max(tile_t::PELLET_POWER))+
                    "], tex "+texture_file+
                    ", pix["+std::to_string(pwidth_)+"x"+std::to_string(pheight_)+"], ppt["+std::to_string(ppt_x_)+"x"+std::to_string(ppt_y_)+
                    "]";
}
