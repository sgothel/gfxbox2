/*
 * Author: Sven Gothel <sgothel@jausoft.com>
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
#include "utils.hpp"

#include <pixel/pixel.hpp>

#include <string>
#include <cstdio>
#include <cmath>

#include <ctime>

//
// direction_t
//
std::string to_string(direction_t dir) noexcept {
    switch(dir) {
        case direction_t::RIGHT: return "R";
        case direction_t::DOWN: return "D";
        case direction_t::LEFT: return "L";
        case direction_t::UP: return "U";
        default: return "?";
    }
}

direction_t inverse(direction_t dir) noexcept {
    switch(dir) {
        case direction_t::RIGHT: return direction_t::LEFT;
        case direction_t::DOWN: return direction_t::UP;
        case direction_t::LEFT: return direction_t::RIGHT;
        case direction_t::UP: return direction_t::DOWN;
        default: return direction_t::DOWN;
    }
}

direction_t rot_left(direction_t dir) noexcept {
    switch(dir) {
        case direction_t::RIGHT: return direction_t::UP;
        case direction_t::DOWN: return direction_t::RIGHT;
        case direction_t::LEFT: return direction_t::DOWN;
        case direction_t::UP: return direction_t::LEFT;
        default: return direction_t::LEFT;
    }
}

direction_t rot_right(direction_t dir) noexcept {
    switch(dir) {
        case direction_t::RIGHT: return direction_t::DOWN;
        case direction_t::DOWN: return direction_t::LEFT;
        case direction_t::LEFT: return direction_t::UP;
        case direction_t::UP: return direction_t::RIGHT;
        default: return direction_t::RIGHT;
    }
}

//
// keyframei_t
//
int keyframei_t::calc_odd_frames_per_field(const float frames_per_second, const float fields_per_second) noexcept {
    const float v0 = frames_per_second / fields_per_second;
    const int v0_floor = jau::floor_to_int( v0 );
    if( 0 != v0_floor % 2 ) {
        // Use faster floor, i.e. resulting in higher fields per second.
        // Difference is 'only' < 1
        return v0_floor;
    } else {
        // Avoid slower ceiling, i.e. would result in lower fields per second.
        return v0_floor - 1; // faster
    }
}

int keyframei_t::calc_nearest_frames_per_field(const float frames_per_second, const float fields_per_second) noexcept {
    const float v0 = frames_per_second / fields_per_second;
    const int v0_floor = jau::floor_to_int( v0 );
    return v0_floor;
}

int keyframei_t::sync_frame_count() const noexcept {
    const float fps_d = frames_per_second_diff();
    if( fps_d > std::numeric_limits<float>::epsilon() ) {
        return jau::round_to_int( frames_per_second() / fps_d );
    } else {
        return -1;
    }
}

float keyframei_t::sync_delay() const noexcept {
    return ( 1000.0f / ( frames_per_second() - frames_per_second_diff() ) ) - ( 1000.0f / frames_per_second() ) ;
}

bool keyframei_t::intersects_center(const float x, const float y) const noexcept {
    // use epsilon delta to avoid matching direct neighbors
    const float fields_per_frame_ = fields_per_frame();
    const float cx = std::trunc(x) + center_;
    const float cy = std::trunc(y) + center_;
#if 1
    // uniform dimension
    return std::abs( cx - x ) < fields_per_frame_ - std::numeric_limits<float>::epsilon() &&
           std::abs( cy - y ) < fields_per_frame_ - std::numeric_limits<float>::epsilon();
#else
    return !( cx + fields_per_frame_ - std::numeric_limits<float>::epsilon() < x || x + fields_per_frame - std::numeric_limits<float>::epsilon() < cx ||
              cy + fields_per_frame_ - std::numeric_limits<float>::epsilon() < y || y + fields_per_frame - std::numeric_limits<float>::epsilon() < cy );
#endif
}

bool keyframei_t::is_center(const float x, const float y) const noexcept {
    // use epsilon delta to have tolerance
    const float cx = std::trunc(x) + center_;
    const float cy = std::trunc(y) + center_;

    return std::abs( cx - x ) <= std::numeric_limits<float>::epsilon() &&
           std::abs( cy - y ) <= std::numeric_limits<float>::epsilon();
}

bool keyframei_t::is_center(const float v) const noexcept {
    const float cv = std::trunc(v) + center_;
    return std::abs( cv - v ) <= std::numeric_limits<float>::epsilon();
}

float keyframei_t::align_value(const float v) const noexcept {
    const float fields_per_frame_ = fields_per_frame();
    if( std::abs( fields_per_frame_ - 1.0f ) <= std::numeric_limits<float>::epsilon() ) {
        return v;
    }
    const float v0_trunc = std::trunc(v);
    const float v0_m = v - v0_trunc;
    const int n = jau::round_to_int(v0_m / fields_per_frame_);
    return v0_trunc + (float)n*fields_per_frame_;
}

float keyframei_t::center_value(const float v) const noexcept {
    return std::trunc(v) + center_;
}

std::string keyframei_t::toString() const noexcept {
    std::string s("[fps ");
    s.append(std::to_string( frames_per_second() ))
     .append(", frames/field ").append(std::to_string( frames_per_field() ))
     .append(", fields/s ").append(std::to_string( fields_per_second() )).append("/").append(std::to_string( fields_per_second_requested() ))
     .append(" (diff ").append(std::to_string( fields_per_second_diff() ))
     .append(", ").append(std::to_string( frames_per_second_diff() ))
     .append("f/s, ").append(std::to_string( sync_delay() )).append(" ms, sync ").append(std::to_string( sync_frame_count() ))
     .append("/f), center ").append(std::to_string( center() )).append("]");
    return s;
}

//
// box_t
//
std::string box_t::toString() const noexcept {
    std::string s("[");
    s.append(std::to_string(x_)).append("/").append(std::to_string(y_))
     .append(" ").append(std::to_string(w_)).append("x").append(std::to_string(h_)).append("]");
    return s;
}

//
// countdown_t
//
bool countdown_t::count_down() noexcept {
    if( 0 == counter_ ) {
        return false;
    }
    const bool r = 0 == --counter_;
    if( r ) {
        ++events_;
        if( 0 < reload_value_ ) {
            counter_ = reload_value_;
        }
    }
    return r;
}

std::string countdown_t::toString() const noexcept {
    std::string s("[");
    s.append(std::to_string(counter_)).append("/").append(std::to_string(reload_value_))
     .append(", events ").append(std::to_string(events_)).append("]");
    return s;
}

//
//
//

#include "maze.hpp"

pixel::texture_ref draw_text(const std::string& text, uint8_t r, uint8_t g, uint8_t b,
                             const std::function<void(const pixel::texture_t& tex_, float &x_, float&y_, float tw, float th)>& set_coord) noexcept

{
    pixel::set_pixel_color(r, g, b, 255);
    pixel::texture_ref ttex = pixel::make_text(text);
    if( nullptr != ttex ) {
        float x_pos=0, y_pos=0;
        set_coord(*ttex, x_pos, y_pos, pixel::cart_coord.from_fb_dx(ttex->width)-3, pixel::cart_coord.from_fb_dy(ttex->height)-2); // FIXME: text-dim adjustment
        ttex->dest_sx = (float)global_maze->ppt_x() / (float)pixel::font_height;
        ttex->dest_sy = (float)global_maze->ppt_y() / (float)pixel::font_height;
        ttex->draw(x_pos, y_pos);
        return ttex;
    } else {
        return nullptr;
    }
}


