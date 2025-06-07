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

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cinttypes>
#include <ctime>
#include <string>
#include <vector>
#include <sys/types.h>

#include <jau/utils.hpp>
#include "pixel/pixel.hpp"
#include "pixel/pixel2f.hpp"
#include "pixel/pixel4f.hpp"
#include "pixel/Highscore.hpp"
#include <fstream>
using namespace jau;

bool pixel::use_subsys_primitives_val = true;
int pixel::win_width=0;
int pixel::win_height=0;
int pixel::fb_width=0;
int pixel::fb_height=0;
int pixel::fb_max_x=0;
int pixel::fb_max_y=0;
pixel::pixel_buffer_t pixel::fb_pixels;

int pixel::font_height = 24;

pixel::cart_coord_t pixel::cart_coord;

uint32_t pixel::draw_color = 0;

static std::string m_asset_dir;

//
//
//

std::string pixel::lookup_and_register_asset_dir(const char* exe_path, const char* asset_file, const char* asset_install_subdir) noexcept {
    m_asset_dir = jau::fs::lookup_asset_dir(exe_path, asset_file, asset_install_subdir);
    return m_asset_dir;
}
std::string pixel::asset_dir() noexcept { return m_asset_dir; }

std::string pixel::resolve_asset(const std::string &asset_file, bool lookup_direct) noexcept {
    if( lookup_direct && jau::fs::exists(asset_file) ) {
        return asset_file;
    }
    if( m_asset_dir.size() ) {
        std::string fname1 = m_asset_dir+"/"+asset_file;
        if( jau::fs::exists(fname1) ) {
            return fname1;
        }
    }
    return "";
}

//
//
//

void pixel::draw_line(float x1_, float y1_, float x2_, float y2_) noexcept {
    if( use_subsys_primitives_val ) {
        subsys_draw_line(cart_coord.to_fb_x( x1_ ), cart_coord.to_fb_y( y1_ ),
                         cart_coord.to_fb_x( x2_ ), cart_coord.to_fb_y( y2_ ));
    } else {
        pixel::f2::lineseg_t::draw({ x1_, y1_ }, { x2_, y1_ });
    }
}

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
        pixel::draw_line(pixel::cart_coord.min_x(), y, pixel::cart_coord.max_x(), y);
    }
    for(float x=l; x<pixel::cart_coord.max_x(); x+=raster_sz) {
        pixel::draw_line(x, pixel::cart_coord.min_y(), x, pixel::cart_coord.max_y());
    }
    pixel::set_pixel_color(cr, cg, cb, ca);
    pixel::draw_line(-raster_sz, 0, +raster_sz, 0);
    pixel::draw_line(0, -raster_sz, 0, +raster_sz);
}

//
// Bitmap
//
static constexpr const bool DEBUG_TEX = false;

void pixel::bitmap_t::put(const f2::aabbox_t& box, uint32_t abgr) noexcept {
    if(!m_pixels || 0 == width || 0 == height ) {
        return;
    }
    const uint32_t x1 = std::max<uint32_t>(0, floor_to_uint32(box.bl.x));
    const uint32_t y1 = std::max<uint32_t>(0, floor_to_uint32(box.bl.y));
    const uint32_t x2 = std::min<uint32_t>(width, ceil_to_uint32(box.tr.x));
    const uint32_t y2 = std::min<uint32_t>(height, ceil_to_uint32(box.tr.y));
    for(uint32_t y=y1; y<y2; ++y) {
        for(uint32_t x=x1; x<x2; ++x) {
            uint32_t * const target_pixel = std::bit_cast<uint32_t *>(m_pixels + static_cast<size_t>((height - y - 1) * stride) + static_cast<size_t>(x * bpp));
            *target_pixel = abgr;
        }
    }
}

bool pixel::bitmap_t::equals(const f2::aabbox_t& box, uint32_t abgr) noexcept {
    if(!m_pixels || 0 == width || 0 == height ) {
        return false;
    }
    const uint32_t x1 = std::max<uint32_t>(0, floor_to_uint32(box.bl.x));
    const uint32_t y1 = std::max<uint32_t>(0, floor_to_uint32(box.bl.y));
    const uint32_t x2 = std::min<uint32_t>(width, ceil_to_uint32(box.tr.x));
    const uint32_t y2 = std::min<uint32_t>(height, ceil_to_uint32(box.tr.y));
    for(uint32_t y=y1; y<y2; ++y) {
        for(uint32_t x=x1; x<x2; ++x) {
            const uint32_t * const target_pixel = std::bit_cast<uint32_t *>(m_pixels + static_cast<size_t>((height - y - 1) * stride) + static_cast<size_t>(x * bpp));
            if( abgr != *target_pixel ) {
                return false;
            }
        }
    }
    return true;
}

//
// Texture
//

size_t pixel::add_sub_textures(std::vector<texture_ref>& storage, const std::string& filename, uint32_t w, uint32_t h, uint32_t x_off) noexcept
{
    std::unique_ptr<texture_t> all = std::make_unique<texture_t>(filename);
    all->disown();
    const size_t size_start = storage.size();

    for(uint32_t y=0; y<all->height; y+=h) {
        for(uint32_t x=0; x<all->width; x+=w+x_off) {
            if( storage.size() > size_start ) {
                storage[ storage.size() - 1 ]->disown(); // only last entry is owner of the SDL_Texture
            }
            storage.push_back( std::make_shared<texture_t>(all->handle(), x, y, w, h, all->bpp, all->format, false /* owner*/) );
            if( DEBUG_TEX ) {
                log_printf("add_sub_textures: tex %zd: %d/%d %dx%d of %dx%d: %s of %s\n",
                    storage.size() - 1, x, y, w, h, all->width, all->height,
                    storage[ storage.size() - 1 ]->toString().c_str(),
                    all->toString().c_str());
            }
        }
    }
    return storage.size() - size_start;
}

size_t pixel::add_sub_textures(std::vector<texture_ref>& storage, const texture_ref& parent,
                               uint32_t x_off, uint32_t y_off, uint32_t w, uint32_t h,
                               const std::vector<tex_sub_coord_t>& tex_positions) noexcept
{
    const size_t size_start = storage.size();

    for(tex_sub_coord_t p : tex_positions) {
        const uint32_t x = x_off+p.x;
        const uint32_t y = y_off+p.y;
        if( 0 <= p.x && 0 <= p.x && x+w <= parent->width && y+h <= parent->height ) {
            storage.push_back( std::make_shared<texture_t>(parent->handle(), x, y, w, h, parent->bpp, parent->format, false /* owner*/) );
        } else {
            storage.push_back( std::make_shared<texture_t>() );
        }
        if( DEBUG_TEX ) {
            log_printf("add_sub_textures: tex %zd: %d/%d %dx%d of %dx%d: %s of %s\n",
                storage.size() - 1, x, y, w, h, parent->width, parent->height,
                storage[ storage.size() - 1 ]->toString().c_str(),
                parent->toString().c_str());
        }
    }
    return storage.size() - size_start;
}

pixel::texture_ref pixel::add_sub_texture(const texture_ref& parent, uint32_t x_off, uint32_t y_off, uint32_t w, uint32_t h) noexcept {
    pixel::texture_ref res;
    {
        const uint32_t x = x_off;
        const uint32_t y = y_off;
        if( x+w <= parent->width && y+h <= parent->height ) {
            res = std::make_shared<texture_t>(parent->handle(), x, y, w, h, parent->bpp, parent->format, false /* owner*/);
        } else {
            res = std::make_shared<texture_t>();
        }
        if( DEBUG_TEX ) {
            log_printf("add_sub_texture: tex %d/%d %dx%d of %dx%d: %s of %s\n",
                x, y, w, h, parent->width, parent->height, res->toString().c_str(), parent->toString().c_str());
        }
    }
    return res;
}

//
// animtex_t
//

pixel::animtex_t::animtex_t(std::string name, float sec_per_atex, const std::vector<texture_ref>& textures) noexcept
: m_name( std::move(name) )
{
    for(const texture_ref& t : textures) {
        m_textures.push_back( t->createShared() );
    }
    m_sec_per_atex = sec_per_atex;
    m_atex_sec_left = 0;
    m_animation_index = 0;
    m_paused = false;
}

pixel::animtex_t::animtex_t(std::string name, float sec_per_atex, const std::vector<const char*>& filenames) noexcept
: m_name( std::move(name) )
{
    for(const char* fname : filenames) {
        m_textures.push_back( std::make_shared<texture_t>(fname) );
    }
    m_sec_per_atex = sec_per_atex;
    m_atex_sec_left = 0;
    m_animation_index = 0;
    m_paused = false;
}

pixel::animtex_t::animtex_t(std::string name, float sec_per_atex, const std::string& filename, int w, int h, int x_off) noexcept
: m_name( std::move(name) )
{
    add_sub_textures(m_textures, filename, w, h, x_off);
    m_sec_per_atex = sec_per_atex;
    m_atex_sec_left = 0;
    m_animation_index = 0;
    m_paused = false;
}

pixel::animtex_t::animtex_t(std::string name, float sec_per_atex, const texture_ref& global_texture,
                            int x_off, int y_off, int w, int h, const std::vector<tex_sub_coord_t>& tex_positions) noexcept
: m_name(std::move(name))
{
    add_sub_textures(m_textures, global_texture, x_off, y_off, w, h, tex_positions);
    m_sec_per_atex = sec_per_atex;
    m_atex_sec_left = 0;
    m_animation_index = 0;
    m_paused = false;
}

void pixel::animtex_t::pause(bool enable) noexcept {
    m_paused = enable;
    if( enable ) {
        m_animation_index = 0;
    }
}

void pixel::animtex_t::reset() noexcept {
    m_animation_index = 0;
    m_atex_sec_left = m_sec_per_atex;
}

void pixel::animtex_t::tick(const float dt) noexcept {
    if( !m_paused ) {
        if( 0 < m_atex_sec_left ) {
            m_atex_sec_left = std::max( 0.0f, m_atex_sec_left - dt );
        }
        if( is_zero(m_atex_sec_left) ) {
            next();
        }
    }
}

void pixel::animtex_t::next() noexcept {
    m_atex_sec_left = m_sec_per_atex;
    if( m_textures.size() > 0 ) {
        m_animation_index = ( m_animation_index + 1 ) % m_textures.size();
    } else {
        m_animation_index = 0;
    }
}

std::string pixel::animtex_t::toString() const noexcept {
    std::shared_ptr<const texture_t> tex = texture();
    std::string tex_s = nullptr != tex ? tex->toString() : "null";
    return m_name+"[anim "+std::to_string(m_atex_sec_left)+"/"+std::to_string(m_sec_per_atex)+
            " s, paused "+std::to_string(m_paused)+", idx "+std::to_string(m_animation_index)+
            "/"+std::to_string(m_textures.size())+
            ", textures["+tex_s+"]]";
}

pixel::texture_ref pixel::make_text(const char* format, ...) noexcept {
    va_list args;
    va_start (args, format);
    std::string s = to_stringva(format, args);
    va_end (args);
    return make_text(s);
}

pixel::texture_ref pixel::make_text(const pixel::f2::point_t& tl, const int lineno,
                                    const pixel::f4::vec_t& color, const int font_height_usr,
                                    const char* format, ...) noexcept {
    va_list args;
    va_start (args, format);
    std::string s = to_stringva(format, args);
    va_end (args);
    return make_text(tl, lineno, color, font_height_usr, s);
}

pixel::texture_ref pixel::make_text(const pixel::f2::point_t& tl, const int lineno,
                                    const pixel::f4::vec_t& color, const int font_height_usr,
                                    const std::string& text) noexcept {
    pixel::set_pixel_color4f(color.x, color.y, color.z, color.w);
    pixel::texture_ref tex = pixel::make_text(text.c_str());
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
        subsys_draw_line(cart_coord.to_fb_x( p0.x ), cart_coord.to_fb_y( p0.y ),
                         cart_coord.to_fb_x( p1.x ), cart_coord.to_fb_y( p1.y ));
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

//
// Highscore
//
std::vector<pixel::texture_ref> pixel::HighScore::textEntries;

void pixel::HighScore::readFile(const std::string& fname){
    std::ifstream in_file(fname, std::ios::in);
    if( !in_file.is_open() ) {
        if(debug_on){
            printf("HS: failed to open %s\n", fname.c_str());
        }
    } else {
        if(debug_on){
            printf("HS: opened %s\n", fname.c_str());
        }
        size_t i = 0;
        std::string line;
        std::getline(in_file, line);
        while( !line.empty() && i < table.size() ) {
            if(debug_on){
                printf("%s\n", line.c_str());
            }
            Entry e;
            char sbuf[3+1];
            std::sscanf(line.c_str(), "%3s %" PRIu32 "", &sbuf[0], &e.score);
            e.name = std::string(sbuf);
            table[i++] = e;
            std::getline(in_file, line);
        }
    }
}

void pixel::HighScore::write_file(const std::string& fname){
    std::ofstream out_file = std::ofstream(fname, std::ios::out | std::ios::trunc);
    for(const Entry& e : table){
        out_file << e.name << " " << std::to_string(e.score) << std::endl;
    }
}

bool pixel::HighScore::addScore(const Entry& p){
    //Entry& e = table[i];
    if(!goodEnough(p)){
        return false;
    }
    table.erase(table.end()-1);
    for(auto it = table.begin(); it != table.end(); ++it){
        if(p.score >= it->score){
            table.insert(it, p);
            return true;
        }
    }
    assert(0);
    return false;
}

size_t pixel::HighScore::find_idx(const Entry& p) const {
    //Entry& e = table[i];
    if(!goodEnough(p)){
        return table.size();
    }
    for(size_t i = 0; i < table.size(); ++i){
        if(p.score >= table[i].score){
            return i;
        }
    }
    assert(0);
    return 0;
}

std::string zeichen(int a){
    switch(a % 10){
      case 1:
        return "st";
        break;
      case 2:
        return "nd";
        break;
      case 3:
        return "rd";
        break;
      default:
        return "th";
    }
}

int pixel::HighScore::resetTextEntries(int lineno, int edit) {
    f2::point_t tl_text = topLeft();
    int pos = 0;
    textEntries.clear();
    for(size_t i = 0; i < table.size(); ++i){
        //printf("AJKSKDJFDKJAJSKDFJJDKAJSDKFJOHJO1234567890\n");
        pixel::HighScore::Entry e = table[i];
        if(size_t(edit) == i){
            textEntries.push_back(pixel::make_text(tl_text, lineno+int(i+1), m_text_color, 30, 
            "%2.2d%s    : %d", i+1, zeichen(int(i)+1).c_str(), e.score));
            pos = lineno+int(i+1);
        } else {
            textEntries.push_back(pixel::make_text(tl_text, lineno+int(i+1), m_text_color, 30, 
            "%2.2d%s %3.3s: %d", i+1, zeichen(int(i)+1).c_str(), e.name.c_str(), e.score));       
        }
        //printf("ALSALALALALALALAJSJDFJDJJDJDJDJDJJDDJDJ %s\n", e.name.c_str()); 
    }
    return pos;
}
bool pixel::HighScore::enterEntry(Entry& p, pixel::input_event_t& event){
    f2::point_t tl_text = topLeft();
    //printf("ABC\n");
    static char s = 'A';
    static int b = 0;
    static bool a = true;
    static size_t idx = find_idx(p);
    static std::string word;
    textEntries.push_back(make_text(tl_text, 0, m_text_color, 40, "Insert Name"));
    static int t = 40/30;
    //printf("HUHU\n");
    if(a){
        b = 0;
        //printf("JUHUHUHUHUHU\n");
        if(!goodEnough(p)){
            return true;
        }
        a = false;
        //printf("TOLL\n");
        word = "";
    }
    //printf("HEY, HEY DU\n");
    std::string letter;
    letter = s;
    //printf("Willst du dieses unsichtbare Eis\n");
    /*
    for(size_t i = 0; i < table.size(); ++i){
        //printf("AJKSKDJFDKJAJSKDFJJDKAJSDKFJOHJO1234567890\n");
        pixel::HighScore::Entry e = table[i];
        textEntries.push_back(pixel::make_text(tl_text, int(i+1)+t, m_text_color, 30, 
        "%d%s %s: %dP", i+1, zeichen(int(i)+1).c_str(), e.name.c_str(), e.score));       
        //printf("ALSALALALALALALAJSJDFJDJJDJDJDJDJJDDJDJ %s\n", e.name.c_str()); 
    }
    */
    resetTextEntries(t);
    textEntries.push_back(pixel::make_text(tl_text, t, f4::vec_t(255, 0, 0, 255), 30, 
    "%d%s %s: %dP <-", idx+1, zeichen(int(idx)+1).c_str(), p.name.c_str(), p.score));
    //addScore(p);
    //textEntries[idx]-
    if( event.pressed_and_clr( pixel::input_event_type_t::P1_RIGHT ) ) {
        if(s == 'Z'){
            s = 'A';
        } else {
            ++s;
        }
    } else if( event.pressed_and_clr( pixel::input_event_type_t::P1_LEFT ) ) {
        if(s == 'A'){
            s = 'Z';
        } else {
            --s;
        }
    } else if( event.pressed_and_clr( pixel::input_event_type_t::P2_ACTION2 ) ) {
        ++b;
        p.name = word;
        p.name += letter;
        for(int i = 0; i < 3-b; ++i){
            p.name += "A";
        }
        word += letter;
        s = 'A';
        if(b >= 3){
            addScore(p);
            a = true;
            return true;
        }
    }
    return false;
}

void pixel::HighScore::showScores() const {
    f2::point_t tl_text = pixel::f2::point_t(pixel::cart_coord.min_x(), pixel::cart_coord.max_y());
    std::string s;
    textEntries.push_back(make_text(tl_text, 0, m_text_color, 40, "Highscore"));
    static int t = 40/30;
    (void)t; (void)s;
}
