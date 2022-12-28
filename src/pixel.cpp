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
#include "pixel/pixel.hpp"
#include "pixel/pixel2f.hpp"

#include <ctime>

int pixel::fb_width=0;
int pixel::fb_height=0;
int pixel::fb_max_x=0;
int pixel::fb_max_y=0;
pixel::pixel_buffer_t pixel::fb_pixels;
int pixel::frames_per_sec=60;

pixel::cart_coord_t pixel::cart_coord;

uint32_t pixel::draw_color = 0;

void pixel::handle_events(bool& close, bool& resized, bool& set_dir, direction_t& dir) noexcept {
    mouse_motion_t mouse_motion = mouse_motion_t();
    pixel::handle_events(close, resized, set_dir, dir, mouse_motion);
}

/**
 * See <http://man7.org/linux/man-pages/man2/clock_gettime.2.html>
 * <p>
 * Regarding avoiding kernel via VDSO,
 * see <http://man7.org/linux/man-pages/man7/vdso.7.html>,
 * clock_gettime seems to be well supported at least on kernel >= 4.4.
 * Only bfin and sh are missing, while ia64 seems to be complicated.
 */
uint64_t pixel::getCurrentMilliseconds() noexcept {
    struct timespec t;
    ::clock_gettime(CLOCK_MONOTONIC, &t);
    return static_cast<uint64_t>( t.tv_sec ) * (uint64_t)MilliPerOne +
           static_cast<uint64_t>( t.tv_nsec ) / (uint64_t)NanoPerMilli;
}

static uint64_t _exe_start_time = pixel::getCurrentMilliseconds();

uint64_t pixel::getElapsedMillisecond() noexcept {
    return getCurrentMilliseconds() - _exe_start_time;
}

void pixel::milli_sleep(uint64_t td_ms) noexcept {
    const int64_t td_ns_0 = (int64_t)( (td_ms * (uint64_t)NanoPerMilli) % (uint64_t)NanoPerOne );
    struct timespec ts { (int64_t)(td_ms/(uint64_t)MilliPerOne), td_ns_0 };
    ::nanosleep( &ts, nullptr );
}

void pixel::log_printf(const uint64_t elapsed_ms, const char * format, ...) noexcept {
    fprintf(stderr, "[%s] ", to_decstring(elapsed_ms, ',', 9).c_str());
    va_list args;
    va_start (args, format);
    vfprintf(stderr, format, args);
    va_end (args);
}
void pixel::log_printf(const char * format, ...) noexcept {
    fprintf(stderr, "[%s] ", to_decstring(getElapsedMillisecond(), ',', 9).c_str());
    va_list args;
    va_start (args, format);
    vfprintf(stderr, format, args);
    va_end (args);
}

//
// pixel::f2
//

pixel::f2::geom_list_t& pixel::f2::gobjects() {
    static pixel::f2::geom_list_t _gobjects;
    return _gobjects;
}

pixel::f2::ageom_list_t& pixel::f2::agobjects() {
    static pixel::f2::ageom_list_t _gobjects;
    return _gobjects;
}

bool pixel::f2::aabbox_t::intersects(const lineseg_t & o) const noexcept {
    return o.intersects(*this);
}

bool pixel::f2::aabbox_t::intersection(vec_t& reflect_out, vec_t& cross_normal, point_t& cross_point, const lineseg_t& in) const noexcept {
    const point_t tl(bl.x, tr.y);
    const point_t br(tr.x, bl.y);
    {
        const lineseg_t l(tl, tr);
        if( l.intersection(reflect_out, cross_normal, cross_point, in) ) {
            return true;
        }
    }
    {
        const lineseg_t l(bl, br);
        if( l.intersection(reflect_out, cross_normal, cross_point, in) ) {
            return true;
        }
    }
    {
        const lineseg_t l(br, tr);
        if( l.intersection(reflect_out, cross_normal, cross_point, in) ) {
            return true;
        }
    }
    {
        const lineseg_t l(bl, tl);
        if( l.intersection(reflect_out, cross_normal, cross_point, in) ) {
            return true;
        }
    }
    return false;
}

void pixel::f2::aabbox_t::draw() const noexcept {
    const point_t tl(bl.x, tr.y);
    const point_t br(tr.x, bl.y);
    lineseg_t::draw(tl, tr);
    lineseg_t::draw(tr, br);
    lineseg_t::draw(br, bl);
    lineseg_t::draw(bl, tl);
}
