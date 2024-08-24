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

#include <cmath>
#include <cstdint>
#include <ctime>

bool pixel::use_subsys_primitives_val = true;
int pixel::win_width=0;
int pixel::win_height=0;
int pixel::fb_width=0;
int pixel::fb_height=0;
int pixel::fb_max_x=0;
int pixel::fb_max_y=0;
pixel::pixel_buffer_t pixel::fb_pixels;
int pixel::display_frames_per_sec=60;
int pixel::forced_fps = -1;
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
    struct timespec ts;
    ts.tv_sec = static_cast<decltype(ts.tv_sec)>(td_ms/(uint64_t)MilliPerOne); // signed 32- or 64-bit integer
    ts.tv_nsec = td_ns_0;
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
    if( is_zero( raster_sz ) ) {
        return;
    }                    
    float wl = pixel::cart_coord.min_x();
    float hb = pixel::cart_coord.min_y();
    float l = ::floorf(wl / raster_sz) * raster_sz;
    float b = ::floorf(hb / raster_sz) * raster_sz;

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

static std::string to_string_impl(const char* format, va_list args) noexcept {
    size_t nchars;
    std::string str;
    {
        const size_t bsz = 1024; // including EOS
        str.reserve(bsz);  // incl. EOS
        str.resize(bsz-1); // excl. EOS

        nchars = vsnprintf(&str[0], bsz, format, args);
        if( nchars < bsz ) {
            str.resize(nchars);
            return str;
        }
    }
    {
        const size_t bsz = std::min<size_t>(nchars+1, str.max_size()+1); // limit incl. EOS
        str.reserve(bsz);  // incl. EOS
        str.resize(bsz-1); // excl. EOS
        nchars = vsnprintf(&str[0], bsz, format, args);
        str.resize(nchars);
        return str;
    }
}

std::string pixel::to_string(const char* format, ...) noexcept {
    va_list args;
    va_start (args, format);
    std::string str = to_string_impl(format, args);
    va_end (args);
    return str;
}

pixel::texture_ref pixel::make_text_texture(const char* format, ...) noexcept {
    va_list args;
    va_start (args, format);
    std::string s = to_string_impl(format, args);
    va_end (args);
    return make_text_texture(s);
}

pixel::texture_ref pixel::make_text(const pixel::f2::point_t& tl, const int lineno,
                                    const pixel::f4::vec_t& color, const int font_height_usr,
                                    const char* format, ...) noexcept {
    va_list args;
    va_start (args, format);
    std::string s = to_string_impl(format, args);
    va_end (args);
    return make_text(tl, lineno, color, font_height_usr, s);
}

pixel::texture_ref pixel::make_text(const pixel::f2::point_t& tl, const int lineno,
                                    const pixel::f4::vec_t& color, const int font_height_usr,
                                    const std::string& text) noexcept {
    pixel::set_pixel_color4f(color.x, color.y, color.z, color.w);
    pixel::texture_ref tex = pixel::make_text_texture(text.c_str());
    tex->dest_sx = (float)font_height_usr / (float)font_height;
    tex->dest_sy = (float)font_height_usr / (float)font_height;
    tex->dest_x = pixel::cart_coord.to_fb_x(tl.x);
    tex->dest_y = pixel::cart_coord.to_fb_y(
            tl.y - std::round((float)lineno * tex->dest_sy * (float)font_height * 1.15f));
    return tex;
}

//
// pixel::f2::*::draw(..)
//

void pixel::f2::vec_t::draw() const noexcept {
    const int x_ = cart_coord.to_fb_x( x );
    const int y_ = cart_coord.to_fb_y( y );
    pixel::set_pixel_fbcoord(x_, y_);
}

void pixel::f2::lineseg_t::draw(const point_t& p0, const point_t& p1) noexcept {
    if constexpr ( false ) {
        for_all_points(p0, p1, [](const point_t& p) -> bool { p.draw(); return true; });
    } else if( use_subsys_primitives_val ) {
        const int p0_x = cart_coord.to_fb_x( p0.x );
        const int p0_y = cart_coord.to_fb_y( p0.y );
        const int p1_x = cart_coord.to_fb_x( p1.x );
        const int p1_y = cart_coord.to_fb_y( p1.y );
        subsys_draw_line(p0_x, p0_y, p1_x, p1_y);
    } else {
        const int p0_x = cart_coord.to_fb_x( p0.x );
        const int p0_y = fb_height - cart_coord.to_fb_y( p0.y );
        const int p1_x = cart_coord.to_fb_x( p1.x );
        const int p1_y = fb_height - cart_coord.to_fb_y( p1.y );
        const int dx = p1_x - p0_x;
        const int dy = p1_y - p0_y;
        const int dx_abs = std::abs(dx);
        const int dy_abs = std::abs(dy);
        float p_x = (float)p0_x, p_y = (float)p0_y;
        if( dy_abs > dx_abs ) {
            const float a = (float)dx / (float)dy_abs; // [0..1), dy_abs > 0
            const int step_h = dy / dy_abs; // +1 or -1
            for(int i = 0; i <= dy_abs; ++i) {
                pixel::set_pixel_fbcoord( (int)p_x, fb_height - (int)p_y );
                p_y += (float)step_h; // = p0.y + i * step_h
                p_x += a;
            }
        } else if( dx_abs > 0 ) {
            const float a = (float)dy / (float)dx_abs; // [0..1], dx_abs > 0
            const int step_w = dx / dx_abs; // +1 or -1
            for(int i = 0; i <= dx_abs; ++i) {
                pixel::set_pixel_fbcoord( (int)p_x, fb_height - (int)p_y );
                p_x += (float)step_w; // = p0.x + i * step_w
                p_y += a;
            }
        }
    }
}

void pixel::f2::aabbox_t::draw() const noexcept {
    const point_t tl(bl.x, tr.y);
    const point_t br(tr.x, bl.y);
    lineseg_t::draw(tl, tr);
    lineseg_t::draw(tr, br);
    lineseg_t::draw(br, bl);
    lineseg_t::draw(bl, tl);
}

void pixel::f2::disk_t::draw(const bool filled) const noexcept {
    const float x_ival = pixel::cart_coord.width() / (float)pixel::fb_width;
    const float y_ival = pixel::cart_coord.height() / (float)pixel::fb_height;
    const float ival2 = 1.0f*std::min(x_ival, y_ival);
    const aabbox_t b = box();
    for(float y=b.bl.y; y<=b.tr.y; y+=y_ival) {
        for(float x=b.bl.x; x<=b.tr.x; x+=x_ival) {
            const point_t p { x, y };
            const float cp = center.dist(p);
            if( ( filled && cp <= radius ) ||
                ( !filled && std::abs(cp - radius) <= ival2 ) ) {
                p.draw();
            }
        }
    }
}

void pixel::f2::rect_t::draw(const bool filled) const noexcept {
    if(filled) {
        constexpr bool debug = false;
        vec_t ac = m_bl - m_tl;
        ac.normalize();
        vec_t vunit_per_pixel((float)pixel::cart_coord.from_fb_dx( 1 ), (float)pixel::cart_coord.from_fb_dy( 1 ));
        if( debug ) {
            printf("unit_per_pixel real %s\n", vunit_per_pixel.toString().c_str());
        }
        vunit_per_pixel.normalize();
        float unit_per_pixel = vunit_per_pixel.length();
        /**
         * Problem mit rotation, wo step breite und linien-loop zu luecken fuehrt!
         */
        vec_t step = ac * unit_per_pixel * 1.0f;
        vec_t i(m_tl), j(m_tr);
        if( debug ) {
            printf("%s\n", toString().c_str());
            printf("ac %s\n", ac.toString().c_str());
            printf("unit_per_pixel norm %s, %f\n", vunit_per_pixel.toString().c_str(), unit_per_pixel);
            printf("step %s\n", step.toString().c_str());
            printf("i %s, j %s\n", i.toString().c_str(), j.toString().c_str());
        }
        if( 0.0f == step.length_sq() ){
            lineseg_t::draw(i, j);
            if( debug ) {
                printf("z: i %s, j %s\n", i.toString().c_str(), j.toString().c_str());
            }
        } else if(m_tl.y < m_bl.y){
            for(; /*i.x > p_c.x ||*/ i.y <= m_bl.y; i+=step, j+=step) {
                lineseg_t::draw(i, j);
                if( debug ) {
                    printf("a: i %s, j %s\n", i.toString().c_str(), j.toString().c_str());
                }
            }
        }else{
            for(; /*i.x > p_c.x ||*/ i.y >= m_bl.y; i+=step, j+=step) {
                lineseg_t::draw(i, j);
                if( debug ) {
                    printf("b: i %s, j %s\n", i.toString().c_str(), j.toString().c_str());
                }
            }
        }

    } else {
        lineseg_t::draw(m_tl, m_tr);
        lineseg_t::draw(m_tr, m_br);
        lineseg_t::draw(m_br, m_bl);
        lineseg_t::draw(m_bl, m_tl);
    }
}

void pixel::f2::triangle_t::draw(const bool filled) const noexcept {
    if(!filled) {
        lineseg_t::draw(p_a, p_b);
        lineseg_t::draw(p_b, p_c);
        lineseg_t::draw(p_c, p_a);
    } else {
        const float x_ival = pixel::cart_coord.width() / (float)pixel::fb_width;
        const float y_ival = pixel::cart_coord.height() / (float)pixel::fb_height;
        const aabbox_t b = box();
        for(float y=b.bl.y; y<=b.tr.y; y+=y_ival) {
            for(float x=b.bl.x; x<=b.tr.x; x+=x_ival) {
                const point_t p { x, y };
                if( contains(p) ){
                    p.draw();
                }
            }
        }
    }
}

void pixel::f2::linestrip_t::draw() const noexcept {
    if( p_list.size() < 2 ) {
        return;
    }
    point_t p0 = p_list[0];
    for(size_t i=1; i<p_list.size(); ++i) {
        const point_t& p1 = p_list[i];
        lineseg_t::draw(p0, p1);
        p0 = p1;
    }
}

//
// pixel::f2::*
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

//
//
//

std::string pixel::input_event_t::to_string() const noexcept {
    return "event[p1 "+std::to_string(has_any_p1())+
            ", p2 "+std::to_string(has_any_p2())+
            ", pressed "+std::to_string(m_pressed)+", lifted "+std::to_string(m_lifted)+
            ", paused "+std::to_string(paused())+
            ", close "+std::to_string(pressed( pixel::input_event_type_t::WINDOW_CLOSE_REQ ))+
            ", last "+std::to_string((int)last)+", key "+std::to_string(last_key_code)+
            ", text "+text+
            ", ptr["+std::to_string(pointer_id)+" "+std::to_string(pointer_x)+"/"+
            std::to_string(pointer_y)+"]]"
            ;
}

///

#if 0
        bool intersects2(const lineseg_t& in) const noexcept {
            /**
             *  Taking
             * E is the starting point of the ray,
             * L is the end point of the ray,
             * C is the center of sphere you're testing against
             * r is the radius of that sphere
             */
            vec_t d = in.p1 - in.p0; // Direction vector of ray, from start to end
            vec_t f = in.p0 - center; // Vector from center sphere to ray start
            float a = d.dot(d);
            float b = 2*f.dot( d ) ;
            float c = f.dot( f ) - radius*radius ;

            float discriminant = b*b - 4*a*c;
            if( discriminant < 0 ){
                return false;
              // no intersection
            } else {
              // ray didn't totally miss sphere,
              // so there is a solution to
              // the equation.

              discriminant = std::sqrt( discriminant );

              // either solution may be on or off the ray so need to test both
              // t1 is always the smaller value, because BOTH discriminant and
              // a are nonnegative.
              float t1 = (-b - discriminant)/(2*a);
              float t2 = (-b + discriminant)/(2*a);

              // 3x HIT cases:
              //          -o->             --|-->  |            |  --|->
              // Impale(t1 hit,t2 hit), Poke(t1 hit,t2>1), ExitWound(t1<0, t2 hit),

              // 3x MISS cases:
              //       ->  o                     o ->              | -> |
              // FallShort (t1>1,t2>1), Past (t1<0,t2<0), CompletelyInside(t1<0, t2>1)

              if( t1 >= 0 && t1 <= 1 )
              {
                // t1 is the intersection, and it's closer than t2
                // (since t1 uses -b - discriminant)
                // Impale, Poke
                return true ;
              }

              // here t1 didn't intersect so we are either started
              // inside the sphere or completely past it
              if( t2 >= 0 && t2 <= 1 )
              {
                // ExitWound
                return true ;
              }

              // no intn: FallShort, Past, CompletelyInside
              return false ;
            }
        }

#endif