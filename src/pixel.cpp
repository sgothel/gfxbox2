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
#include "pixel/pixel4f.hpp"

#include <cstdint>
#include <ctime>

int pixel::fb_width=0;
int pixel::fb_height=0;
int pixel::fb_max_x=0;
int pixel::fb_max_y=0;
pixel::pixel_buffer_t pixel::fb_pixels;
int pixel::frames_per_sec=60;
int pixel::font_height = 24;

pixel::cart_coord_t pixel::cart_coord;

uint32_t pixel::draw_color = 0;

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
//
//

void pixel::draw_grid(float raster_sz,
                      uint8_t gr, uint8_t gg, uint8_t gb, uint8_t ga,
                      uint8_t cr, uint8_t cg, uint8_t cb, uint8_t ca){
    float wl = pixel::cart_coord.min_x();
    float hb = pixel::cart_coord.min_y();
    float l = (int)(wl / raster_sz) * raster_sz;
    float b = (int)(hb / raster_sz) * raster_sz;

    pixel::set_pixel_color(gr, gg, gb, ga);
    pixel::f2::point_t bl(pixel::cart_coord.min_x(), pixel::cart_coord.min_y());
    for(float y=b; y<pixel::cart_coord.max_y(); y+=raster_sz) {
        pixel::f2::point_t p0(pixel::cart_coord.min_x(), y);
        pixel::f2::point_t p1(pixel::cart_coord.max_x(), y);
        pixel::f2::lineseg_t::draw(p0, p1);
    }
    for(float x=l; x<pixel::cart_coord.max_x(); x+=raster_sz) {
        pixel::f2::point_t p0(x, pixel::cart_coord.min_y());
        pixel::f2::point_t p1(x, pixel::cart_coord.max_y());
        pixel::f2::lineseg_t::draw(p0, p1);
    }
    pixel::set_pixel_color(cr, cg, cb, ca);
    {
        pixel::f2::point_t p0(-raster_sz, 0);
        pixel::f2::point_t p1(+raster_sz, 0);
        pixel::f2::lineseg_t::draw(p0, p1);
    }
    {
        pixel::f2::point_t p0(0, -raster_sz);
        pixel::f2::point_t p1(0, +raster_sz);
        pixel::f2::lineseg_t::draw(p0, p1);
    }
}

pixel::texture_ref pixel::make_text(const pixel::f2::point_t& tl, const int lineno,
                                    const pixel::f4::vec_t& color, const int font_height_usr,
                                    const std::string& text) noexcept {
    pixel::set_pixel_color4f(color.x, color.y, color.z, color.w);
    pixel::texture_ref tex = pixel::make_text_texture(text.c_str());
    tex->dest_sx = (float)font_height_usr / (float)font_height;
    tex->dest_sy = (float)font_height_usr / (float)font_height;
    tex->dest_x = pixel::cart_coord.to_fb_x(tl.x);
    tex->dest_y = pixel::cart_coord.to_fb_y(tl.y - (int)std::round(lineno * font_height_usr * 1.15f));
    return tex;
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
        const lineseg_t lt(tl, tr);
        const lineseg_t lb(bl, br);
        const float dt = lt.distance( in.p0 );
        const float db = lb.distance( in.p0 );
        if( dt < db ) {
            if( lt.intersection(reflect_out, cross_normal, cross_point, in) ) {
                return true;
            }
            if( lb.intersection(reflect_out, cross_normal, cross_point, in) ) {
                return true;
            }
        } else {
            if( lb.intersection(reflect_out, cross_normal, cross_point, in) ) {
                return true;
            }
            if( lt.intersection(reflect_out, cross_normal, cross_point, in) ) {
                return true;
            }
        }
    }
    {
        const lineseg_t lr(br, tr);
        const lineseg_t ll(bl, tl);
        const float dr = lr.distance( in.p0 );
        const float dl = ll.distance( in.p0 );
        if( dr < dl ) {
            if( lr.intersection(reflect_out, cross_normal, cross_point, in) ) {
                return true;
            }
            if( ll.intersection(reflect_out, cross_normal, cross_point, in) ) {
                return true;
            }
        } else {
            if( ll.intersection(reflect_out, cross_normal, cross_point, in) ) {
                return true;
            }
            if( lr.intersection(reflect_out, cross_normal, cross_point, in) ) {
                return true;
            }
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

std::string pixel::input_event_t::to_string() const noexcept {
    return "event[p1 "+std::to_string(has_any_p1())+
            ", pressed "+std::to_string(m_pressed)+", p1_mask "+std::to_string(p1_mask)+
            ", p2 "+std::to_string(has_any_p2())+
            ", paused "+std::to_string(paused())+
            ", close "+std::to_string(pressed( pixel::input_event_type_t::WINDOW_CLOSE_REQ ))+
            ", last "+std::to_string((int)last)+", key "+std::to_string(last_key_code)+
            ", ptr["+std::to_string(pointer_id)+" "+std::to_string(pointer_x)+"/"+
            std::to_string(pointer_y)+"]]"
            ;
}

