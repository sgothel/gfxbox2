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
#ifndef PIXEL_HPP_
#define PIXEL_HPP_

#include <memory>
#include <functional> // NOLINT(unused-includes): Used in other header
#include <string>
#include <vector>

#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cctype>

#include <jau/file_util.hpp>
#include <jau/utils.hpp>
#include <pixel/version.hpp>

#if defined(__EMSCRIPTEN__)
    #include <emscripten.h>
#else
    #define EMSCRIPTEN_KEEPALIVE
#endif

namespace pixel::f2 {
    class vec_t; // fwd
    typedef vec_t point_t;
    class aabbox_t;

}
namespace pixel::f4 {
    class vec_t; // fwd
    typedef vec_t point_t;
}

/**
 * Basic computer graphics math and utilities helping with the framebuffer and I/O tooling.
 */
namespace pixel {

    std::string lookup_and_register_asset_dir(const char* exe_path, const char* asset_file="fonts/freefont/FreeSansBold.ttf", const char* asset_install_subdir="gfxbox2") noexcept;
    std::string asset_dir() noexcept;
    std::string resolve_asset(const std::string &asset_file, bool lookup_direct=false) noexcept;

    /** Width of the window, coordinate in window units. */
    extern int win_width;
    /** Height of the window, coordinate in window units. */
    extern int win_height;
    /**
     * Width of the framebuffer coordinate in pixels.
     *
     * Framebuffer origin 0/0 is top-left corner.
     */
    extern int fb_width;
    /** Height of the framebuffer coordinate in pixels. */
    extern int fb_height;
    /** x-axis maximum of the framebuffer coordinate in pixels. */
    extern int fb_max_x;
    /** y-axis maximum of the framebuffer coordinate in pixels. */
    extern int fb_max_y;
    typedef std::vector<uint32_t> pixel_buffer_t; // 32-bit pixel
    extern pixel_buffer_t fb_pixels;
    extern int font_height;

    enum class orientation_t {
        /** Collinear **/
        COL,
        /** Clockwise **/
        CLW,
        /** Counter-Clockwise **/
        CCW
    };

    /**
     * Cartesian coordinate system within the framebuffer space in pixels.
     */
    class cart_coord_t {
        private:
            float m_x1;
            float m_y1;
            float m_x2;
            float m_y2;
            float m_w_to_fbw;
            float m_h_to_fbh;
            float m_sx_win_to_fb;
            float m_sy_win_to_fb;
        public:
            cart_coord_t() noexcept { set_one(0.0f, 0.0f); }
            cart_coord_t(float x1_, float y1_) noexcept { set_one(x1_, y1_); }
            cart_coord_t(float x1_, float y1_, float x2_, float y2_) noexcept { set_free(x1_, y1_, x2_, y2_); }

            std::string toString() const noexcept {
                const int fx1 = to_fb_x(m_x1);
                const int fy1 = to_fb_y(m_y1);
                const int fx2 = to_fb_x(m_x2);
                const int fy2 = to_fb_y(m_y2);
                const int fxO = to_fb_x(0.0f);
                const int fyO = to_fb_y(0.0f);
                return "cart[min "+std::to_string(m_x1)+"/"+std::to_string(m_y1)+", max "+std::to_string(m_x2)+"/"+std::to_string(m_y2)+
                        ", size "+std::to_string(width())+"x"+std::to_string(height())+", scale x "+std::to_string(m_w_to_fbw)+", y "+std::to_string(m_h_to_fbh)
                        +"], fb[min "+std::to_string(fx1)+"/"+std::to_string(fy1)+", max "+std::to_string(fx2)+"/"+std::to_string(fy2)+
                        ", origin "+std::to_string(fxO)+"/"+std::to_string(fyO)+", size "+std::to_string(fb_width)+"x"+std::to_string(fb_height)+
                        ", scale "+std::to_string(m_sx_win_to_fb)+" x "+std::to_string(m_sy_win_to_fb)+" fb/win]";
            }

            /**
             * Setup framebuffer to cartesian pixel-aspect to 1,
             * using given origin factor [0..1] to framebuffer width and height.
             */
            void set_origin(float xo_, float yo_) noexcept {
                const float x1_ = (float)fb_max_x * -1.0f*xo_;
                const float y1_ = (float)fb_max_y * -1.0f*yo_;
                set_one(x1_, y1_);
            }

            /**
             * Setup framebuffer to cartesian pixel-aspect to 1,
             * using given minimum x- and y-axis and framebuffer width and height.
             *
             * The x-axis and y-axis minimum determine the origin.
             */
            void set_one(float x1_, float y1_) noexcept {
                m_x1 = x1_;
                m_y1 = y1_;
                m_w_to_fbw = 1.0f;
                m_h_to_fbh = 1.0f;
                m_x2 = m_x1 + (float)fb_max_x * m_w_to_fbw;
                m_y2 = m_y1 + (float)fb_max_y * m_h_to_fbh;
                m_sx_win_to_fb = 1;
                m_sy_win_to_fb = 1;
                printf("set_one: %s\n", toString().c_str());
            }

            void set_sxy_win_to_fb(float sx, float sy) noexcept {
                m_sx_win_to_fb = sx;
                m_sy_win_to_fb = sy;
                printf("set_sxy: %s\n", toString().c_str());
            }

            /**
             * Setup cartesian width with given x-axis coordinates
             * and use same aspect ratio for a centered y-axis.
             *
             * The x-axis and y-axis minimum determine the origin.
             */
            void set_width(float x1_, float x2_) noexcept {
                m_x1 = x1_;
                m_x2 = x2_;
                m_w_to_fbw = width() / (float)fb_width;
                m_h_to_fbh = m_w_to_fbw;
                const float h = (float)fb_max_y * m_h_to_fbh;
                m_y1 = h / -2.0f;
                m_y2 = m_y1 + h;
                printf("coord.set_width: %s\n", toString().c_str());
            }

            /**
             * Setup cartesian height with given y-axis coordinates
             * and use same aspect ratio for a centered x-axis.
             *
             * The x-axis and y-axis minimum determine the origin.
             */
            void set_height(float y1_, float y2_) noexcept {
                m_y1 = y1_;
                m_y2 = y2_;
                m_h_to_fbh = height() / (float)fb_height;
                m_w_to_fbw = m_h_to_fbh;
                const float w = (float)fb_max_x * m_w_to_fbw;
                m_x1 = w / -2.0f;
                m_x2 = m_x1 + w;
                printf("coord.set_height: %s\n", toString().c_str());
            }

            /**
             * Setup cartesian width free aspect ration for both axis.
             *
             * The x-axis and y-axis minimum determine the origin.
             */
            void set_free(float x1_, float y1_, float x2_, float y2_) noexcept {
                m_x1 = x1_;
                m_y1 = y1_;
                m_x2 = x2_;
                m_y2 = y2_;
                m_w_to_fbw = width() / (float)fb_width;
                m_h_to_fbh = height() / (float)fb_height;
                printf("coord.set_free: %s\n", toString().c_str());
            }

            /** x-axis minimum of the cartesian coordinate system within the framebuffer space. */
            constexpr float min_x() const noexcept { return m_x1; }
            /** y-axis minimum of the cartesian coordinate system within the framebuffer space. */
            constexpr float min_y() const noexcept { return m_y1; }
            /** x-axis maximum of the cartesian coordinate system within the framebuffer space. */
            constexpr float max_x() const noexcept { return m_x2; }
            /** y-axis maximum of the cartesian coordinate system within the framebuffer space. */
            constexpr float max_y() const noexcept { return m_y2; }

            /** scale x-axis window to fb due to high-dpi. */
            constexpr float sx_win_to_fb() const noexcept { return m_sx_win_to_fb; }
            /** scale y-axis window to fb due to high-dpi. */
            constexpr float sy_win_to_fb() const noexcept { return m_sy_win_to_fb; }

            /** scale x-axis window to fb due to high-dpi. */
            constexpr float sx_win_to_fb(int x) const noexcept { return m_sx_win_to_fb * (float)x; }
            /** scale y-axis window to fb due to high-dpi. */
            constexpr float sy_win_to_fb(int y) const noexcept { return m_sy_win_to_fb * (float)y; }

            /** x-axis width of the cartesian coordinate system. */
            constexpr float width() const noexcept { return m_x2 - m_x1; }
            /** y-axis height of the cartesian coordinate system. */
            constexpr float height() const noexcept { return m_y2 - m_y1; }

            /** Convert cartesian x-axis value to framebuffer pixel value. */
            int to_fb_dx(const float dx) const noexcept { return jau::round_to_int( dx / m_w_to_fbw ); }
            /** Convert cartesian y-axis value to framebuffer pixel value. */
            int to_fb_dy(const float dy) const noexcept { return jau::round_to_int( dy / m_h_to_fbh ); }

            /** Convert framebuffer x-axis value in pixels to cartesian pixel value. */
            int from_fb_dx(const int dx) const noexcept { return jau::round_to_int((float)dx * m_w_to_fbw); }
            /** Convert framebuffer y-axis value in pixels to cartesian pixel value. */
            int from_fb_dy(const int dy) const noexcept { return jau::round_to_int((float)dy * m_h_to_fbh); }

            /** Convert cartesian x-axis coordinate to framebuffer coordinate in pixels. */
            int to_fb_x(const float x) const noexcept { return jau::round_to_int( ( x - m_x1 ) / m_w_to_fbw ); }
            /** Convert cartesian y-axis coordinate in pixels to framebuffer coordinate in pixels. */
            int to_fb_y(const float y) const noexcept { return fb_height - jau::round_to_int( ( y - m_y1 ) / m_h_to_fbh ); }

            /** Convert framebuffer x-axis coordinate in pixels to cartesian coordinate. */
            float from_fb_x(const int x) const noexcept { return (float)x * m_w_to_fbw + m_x1; }
            /** Convert framebuffer y-axis coordinate in pixels to cartesian coordinate. */
            float from_fb_y(const int y) const noexcept { return (float)(fb_height-y) * m_h_to_fbh + m_y1; }

            /** Convert win x-axis coordinate in pixels to cartesian coordinate. */
            float from_win_x(const int x) const noexcept { return sx_win_to_fb(x) * m_w_to_fbw + m_x1; }
            /** Convert win y-axis coordinate in pixels to cartesian coordinate. */
            float from_win_y(const int y) const noexcept { return ((float)fb_height-sy_win_to_fb(y)) * m_h_to_fbh + m_y1; }
    };
    extern cart_coord_t cart_coord;

    //
    // Pixel write
    //
    extern bool use_subsys_primitives_val;

    inline bool use_subsys_primitives() noexcept {
        return use_subsys_primitives_val;
    }
    void subsys_set_pixel_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept;
    void subsys_draw_pixel(int x, int y) noexcept;
    void subsys_draw_line(int x1, int y1, int x2, int y2) noexcept;

    //
    // Pixel color
    //

    /** The current 32-bit draw color. */
    extern uint32_t draw_color; // 32-bit pixel

    /** Return clipped x-axis framebuffer coordinate. */
    inline unsigned int clip_fb_x(const int x) noexcept { return std::min<int>(fb_max_x, std::max<int>(0, x)); }
    /** Return clipped y-axis framebuffer coordinate. */
    inline unsigned int clip_fb_y(const int y) noexcept { return std::min<int>(fb_max_y, std::max<int>(0, y)); }

    /**
     * Set a pixel using the given draw_color and given fb coordinates.
     *
     * In case the coordinates are out of bounds, the pixel is discarded, i.e. not set.
     */
    inline void set_pixel_fbcoord(int x, int y) noexcept {
        if( 0 <= x && x <= fb_max_x && 0 <= y && y <= fb_max_y ) {
            if( use_subsys_primitives_val ) {
                subsys_draw_pixel(x, y);
            } else {
                fb_pixels[ y * fb_width + x ] = draw_color;
            }
        }
    }

    /**
     * Set a pixel using the given draw_color and given cartesian coordinates.
     *
     * In case the coordinates are out of bounds, the pixel is discarded, i.e. not set.
     */
    inline void set_pixel(int x_, int y_) noexcept {
        const int x = cart_coord.to_fb_x( (float)x_ );
        const int y = cart_coord.to_fb_y( (float)y_ );
        set_pixel_fbcoord(x, y);
    }

    /**
     * Set a pixel using the given draw_color and given cartesian coordinates.
     *
     * In case the coordinates are out of bounds, the pixel is discarded, i.e. not set.
     */
    inline void set_pixel(float x_, float y_) noexcept {
        const int x = cart_coord.to_fb_x( x_ );
        const int y = cart_coord.to_fb_y( y_ );
        if( 0 <= x && x <= fb_max_x && 0 <= y && y <= fb_max_y ) {
            fb_pixels[ y * fb_width + x ] = draw_color;
        }
    }

    /** Convert 32-bit RGBA components to the gfx-toolkit specific RGBA 32-bit integer presentation. */
    uint32_t rgba_to_uint32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept;

    /** Convert gfx-toolkit specific RGBA 32-bit integer to 32-bit RGBA components. */
    void uint32_to_rgba(uint32_t ui32, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) noexcept;

    /** Set current pixel draw color */
    inline void set_pixel_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept {
        draw_color = pixel::rgba_to_uint32(r, g, b, a);
        if( use_subsys_primitives_val ) {
            subsys_set_pixel_color(r, g, b, a);
        }
    }
    /** Set current pixel draw color */
    inline void set_pixel_color(const uint8_t rgba[/*4*/]) noexcept {
        draw_color = pixel::rgba_to_uint32(rgba[0], rgba[1], rgba[2], rgba[3]);
        if( use_subsys_primitives_val ) {
            subsys_set_pixel_color(rgba[0], rgba[1], rgba[2], rgba[3]);
        }
    }
    inline uint8_t clip_byte(float v) { return static_cast<uint8_t>(std::max<float>(0.0f, std::min(255.0f, v))); }
    inline void set_pixel_color4f(float r, float g, float b, float a) noexcept {
        set_pixel_color(clip_byte(r*255.0f), clip_byte(g*255.0f), clip_byte(b*255), clip_byte(a*255));
    }
    inline void set_pixel_color(const float rgba[/*4*/]) noexcept {
        set_pixel_color4f(rgba[0], rgba[1], rgba[2], rgba[3]);
    }
    void draw_grid(float raster_sz,
                   uint8_t gr, uint8_t gg, uint8_t gb, uint8_t ga,
                   uint8_t cr, uint8_t cg, uint8_t cb, uint8_t ca);

    //
    // Bitmap
    //
    class bitmap_t {
        private:
            void* m_handle;
            uint8_t *m_pixels;
            void destroy() noexcept;

        public:
            static const char* format_str(uint32_t fmt) noexcept;

            /** bitmap width */
            uint32_t  width;
            /** bitmap width */
            uint32_t height;
            /** bitmap bytes per pixel */
            uint32_t bpp;
            /** bitmap stride (pitch) */
            uint32_t stride;
            /** native format */
            uint32_t format;

            // Create an empty ABGR8888 surface
            bitmap_t(const uint32_t width_, const uint32_t height_) noexcept;

            bitmap_t() noexcept
            : m_handle(nullptr), m_pixels(nullptr), width(0), height(0), bpp(0), stride(0), format(0) {}

            // Creates an ABGR8888 surface from given file
            bitmap_t(const std::string& fname) noexcept;

            /** Returns a clone */
            bitmap_t(const bitmap_t& o, int /*unused*/) noexcept;

            /** Returns a clone */
            std::shared_ptr<bitmap_t> clone() {
                return std::make_shared<bitmap_t>(*this, 0);
            }

            bitmap_t(const bitmap_t&) = delete;
            void operator=(const bitmap_t&) = delete;

            ~bitmap_t() noexcept {
                destroy();
            }

            constexpr void* handle() noexcept { return m_handle; }
            constexpr uint8_t* pixels() noexcept { return m_pixels; }

            void set_pixel(int x, int y, uint32_t abgr) noexcept;

            /** Reads abgr value */
            constexpr uint32_t get(uint32_t x, uint32_t y) const noexcept {
                if(!m_pixels || x >= width || y >= height ) {
                    return 0;
                }
                const uint32_t * const target_pixel = std::bit_cast<uint32_t *>(m_pixels + static_cast<size_t>((height - y - 1) * stride) + static_cast<size_t>(x * bpp));
                return *target_pixel;
            }

            /** Writes given abgr value */
            constexpr void put(uint32_t x, uint32_t y, uint32_t abgr) noexcept {
                if(!m_pixels || x >= width || y >= height ) {
                    return;
                }
                uint32_t * const target_pixel = std::bit_cast<uint32_t *>(m_pixels + static_cast<size_t>((height - y - 1) * stride) + static_cast<size_t>(x * bpp));
                *target_pixel = abgr;
            }

            /** Writes given abgr value into aabbox area */
            void put(const f2::aabbox_t& box, uint32_t abgr) noexcept;

            /** Test if given box equals given abgr value */
            bool equals(const f2::aabbox_t& box, uint32_t abgr) noexcept;

            std::string toString() const noexcept {
                return (m_handle ? " (set) " : " (empty) ") +
                       std::to_string(width)+"x"+std::to_string(height)+"x"+std::to_string(bpp)+
                       ", stride "+std::to_string(stride)+", "+format_str(format)+", pix "+std::to_string((intptr_t)(void*)m_pixels);
            }
    };
    typedef std::shared_ptr<bitmap_t> bitmap_ref;

    //
    // Texture
    //

    class texture_t {
        private:
            void* m_handle;
            bool m_owner;
            void destroy() noexcept;

        public:
            static const char* format_str(uint32_t fmt) noexcept {
                return bitmap_t::format_str(fmt);
            }
            /** texture pos-x */
            uint32_t x;
            /** texture pos-y */
            uint32_t y;
            /** texture width */
            uint32_t width;
            /** texture width */
            uint32_t height;
            /** texture bytes per pixel */
            uint32_t bpp;
            /** texture format */
            uint32_t format;
            /** dest texture pos-x */
            uint32_t dest_x;
            /** dest texture pos-y */
            uint32_t dest_y;
            /** dest texture scale-x */
            float dest_sx;
            /** dest texture scale-y */
            float dest_sy;

            texture_t(void* handle_, const uint32_t x_, const uint32_t y_, const uint32_t width_, const uint32_t height_, const uint32_t bpp_, const uint32_t format_, const bool owner=true) noexcept
            : m_handle(handle_), m_owner(nullptr!=handle_ && owner),
              x(x_), y(y_), width(width_), height(height_), bpp(bpp_), format(format_),
              dest_x(0), dest_y(0), dest_sx(1), dest_sy(1) {}

            /** Create a shared proxy clone w/o ownership, use createShared() */
            texture_t(const texture_t& parent, int /*unused*/) noexcept
            : m_handle(parent.m_handle), m_owner(false),
              x(parent.x), y(parent.y), width(parent.width), height(parent.height), bpp(parent.bpp), format(parent.format),
              dest_x(0), dest_y(0), dest_sx(1), dest_sy(1) {}

            /** Create a shared proxy clone w/o ownership */
            std::shared_ptr<texture_t> createShared() {
                return std::make_shared<texture_t>(*this, 0);
            }

            texture_t() noexcept
            : m_handle(nullptr), m_owner(false), x(0), y(0), width(0), height(0), bpp(0), format(0), dest_x(0), dest_y(0), dest_sx(1), dest_sy(1) {}

            texture_t(const std::string& fname) noexcept;

            texture_t(const bitmap_ref& bmap) noexcept;

            texture_t(const texture_t&) = delete;
            void operator=(const texture_t&) = delete;

            ~texture_t() noexcept {
                destroy();
            }

            constexpr void* handle() noexcept { return m_handle; }
            constexpr bool is_owner() const noexcept { return m_owner; }
            void disown() noexcept { m_owner = false; }
            void set_owner(bool v) noexcept { m_owner = v; }

            /** update texture */
            void update(const bitmap_ref& bmap) noexcept;

            /// draw using FB coordinates and dimension, 0/0 is top-left
            void draw_raw(const uint32_t fb_x, const uint32_t fb_y, const uint32_t fb_w, const uint32_t fb_h) const noexcept;
            /// draw using FB coordinates and optional scale, 0/0 is top-left
            void draw_fbcoord(const uint32_t x_pos, const uint32_t y_pos, const float scale_x=1.0f, const float scale_y=1.0f) const noexcept {
                draw_raw(x_pos + dest_x, y_pos + dest_y,
                         jau::round_to_int((float)width*dest_sx*scale_x),
                         jau::round_to_int((float)height*dest_sy*scale_y));
            }
            /// draw using cartesian coordinates and dimension, 0/0 is top-left
            void draw(const float x_pos, const float y_pos, const float w, const float h) const noexcept {
                draw_raw(pixel::cart_coord.to_fb_x( x_pos ), pixel::cart_coord.to_fb_y( y_pos ),
                         pixel::cart_coord.to_fb_dy(w), pixel::cart_coord.to_fb_dy(h));
            }

            /// draw using cartesian coordinates with scaled texture-source-dimension to FB, 0/0 is top-left
            void draw(const float x_pos, const float y_pos) const noexcept {
                draw_raw(pixel::cart_coord.to_fb_x( x_pos ), pixel::cart_coord.to_fb_y( y_pos ),
                         pixel::cart_coord.to_fb_dy(float(width)), pixel::cart_coord.to_fb_dy(float(height)));
            }

            std::string toString() const noexcept {
                return (m_handle ? " (set) " : " (empty) ") +
                       std::to_string(x)+"/"+std::to_string(y) + " " + std::to_string(width)+"x"+std::to_string(height)+"x"+std::to_string(bpp) +
                       ", " + format_str(format) + ", owner " + std::to_string(m_owner);
            }
    };
    typedef std::shared_ptr<texture_t> texture_ref;

    struct tex_sub_coord_t {
        uint32_t x;
        uint32_t y;
    };

    /**
     * Add sub-textures to the storage list of texture_t from file,
     * where the last one is the sole owner of the common SDL_Texture instance.
     * @param storage
     * @param filename
     * @param w
     * @param h
     * @param x_off
     * @return number of added sub-textures, last one is owner of the SDL_Texture instance
     */
    size_t add_sub_textures(std::vector<texture_ref>& storage,
                            const std::string& filename, uint32_t w, uint32_t h, uint32_t x_off) noexcept;

    /**
     * Add sub-textures to the storage list of texture_t from given global texture owner.
     *
     * None of the sub-textures is the owner of the common SDL_Texture instance.
     * @param storage
     * @param parent
     * @param x_off
     * @param y_off
     * @param w
     * @param h
     * @param tex_positions
     * @return number of added sub-textures
     */
    size_t add_sub_textures(std::vector<texture_ref>& storage,
                            const texture_ref& parent, uint32_t x_off, uint32_t y_off, uint32_t w, uint32_t h,
                            const std::vector<tex_sub_coord_t>& tex_positions) noexcept;

    /**
     * Returns a sub-texture from given global texture owner.
     *
     * The sub-texture is not the owner of the common SDL_Texture instance.
     * @param parent
     * @param x_off
     * @param y_off
     * @param w
     * @param h
     * @return sub-texture ref
     */
    texture_ref add_sub_texture(const texture_ref& parent, uint32_t x_off, uint32_t y_off, uint32_t w, uint32_t h) noexcept;

    class animtex_t {
        private:
            std::string m_name;
            std::vector<texture_ref> m_textures;

            float m_sec_per_atex;
            float m_atex_sec_left;
            size_t m_animation_index;
            bool m_paused;

        public:
            animtex_t(std::string name, float sec_per_atex, const std::vector<texture_ref>& textures) noexcept;

            animtex_t(std::string name, float sec_per_atex, const std::vector<const char*>& filenames) noexcept;

            animtex_t(std::string name, float sec_per_atex, const std::string& filename, int w, int h, int x_off) noexcept;

            animtex_t(std::string name, float sec_per_atex, const texture_ref& global_texture,
                     int x_off, int y_off, int w, int h, const std::vector<tex_sub_coord_t>& tex_positions) noexcept;

            animtex_t(const animtex_t& o) noexcept = default;
            animtex_t(animtex_t&& o) noexcept = default;

            animtex_t& operator=(const animtex_t&) = default;
            animtex_t& operator=(animtex_t&&) = default;

            ~animtex_t() noexcept {
                clear();
            }

            void clear() noexcept { m_textures.clear(); m_sec_per_atex=0; m_atex_sec_left=0; m_animation_index=0; m_paused=true; }

            const texture_ref texture(const size_t idx) const noexcept { return idx < m_textures.size() ? m_textures[idx] : nullptr; }
            const texture_ref texture() const noexcept { return m_animation_index < m_textures.size() ? m_textures[m_animation_index] : nullptr; }

            uint32_t width() noexcept { texture_ref tex = texture(); return nullptr!=tex ? tex->width : 0; }
            uint32_t height() noexcept { texture_ref tex = texture(); return nullptr!=tex ? tex->height : 0; }

            void reset() noexcept;
            void pause(bool enable) noexcept;
            void tick(const float dt) noexcept;
            void next() noexcept;

            /// draw using cartesian coordinates and dimension, 0/0 is top-left
            void draw(const float x_pos, const float y_pos, const float w, const float h) const noexcept {
                texture_ref tex = texture();
                if( nullptr != tex ) {
                    tex->draw(x_pos, y_pos, w, h);
                }
            }
            /// draw using cartesian coordinates with scaled texture-source-dimension to FB, 0/0 is top-left
            void draw(const float x_pos, const float y_pos) const noexcept {
                texture_ref tex = texture();
                if( nullptr != tex ) {
                    tex->draw(x_pos, y_pos);
                }
            }

            std::string toString() const noexcept;
    };

    //
    // gfx toolkit dependent API
    //

    /** Monitor frames per seconds */
    int monitor_fps() noexcept;

    /**
     * Returns optional forced frames per seconds or -1 if unset, set via set_gpu_forced_fps().
     * Passed to swap_gpu_buffer() by default.
     */
    int gpu_forced_fps() noexcept;

    /** Optional forced frames per seconds, pass to swap_gpu_buffer() by default. */
    void set_gpu_forced_fps(int fps) noexcept;

    /** Returns expected fps, either gpu_forced_fps() if set, otherwise monitor_fps(). */
    inline int expected_fps() noexcept { int v=gpu_forced_fps(); return v>0?v:monitor_fps(); }
    /** Returns the measured gpu frame duration in [s] */
    inline float expected_framedur() noexcept {
        return 1.0f/float(expected_fps());
    }

    bool is_gfx_subsystem_initialized() noexcept;
    /** GFX Toolkit: Initialize a window of given size with a usable framebuffer. */
    bool init_gfx_subsystem(const char* exe_path, const char* title, int window_width, int window_height, const float origin_norm[2],
                            bool enable_vsync=true, bool use_subsys_primitives=true);
    /** GFX Toolkit: Clear the soft-framebuffer. */
    void clear_pixel_fb(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept;
    /** GFX Toolkit: Copy the soft-framebuffer to the GPU back-buffer, if swap_buffer is true (default) also swap_gpu_buffer(). */
    void swap_pixel_fb(const bool swap_buffer=true, int fps=gpu_forced_fps()) noexcept;
    /** GFX Toolkit: Swap GPU back to front framebuffer while maintaining vertical monitor synchronization if possible. */
    void swap_gpu_buffer(int fps=gpu_forced_fps()) noexcept;
    /** Returns the measured gpu fps each 5s, starting with monitor_fps() */
    float gpu_avg_fps() noexcept;
    /** Returns the measured gpu frame duration in [s] each 5s, starting with 1/monitor_fps() */
    inline float gpu_avg_framedur() noexcept {
        const float fps = gpu_avg_fps();
        return fps > 0 ? 1.0f/fps : 0;
    }

    texture_ref make_text_texture(const std::string& text) noexcept;

    texture_ref make_text_texture(const char* format, ...) noexcept;


    texture_ref make_text(const pixel::f2::point_t& tl, const int lineno,
                          const pixel::f4::vec_t& color, const int font_height_usr,
                          const std::string& text) noexcept;

    texture_ref make_text(const pixel::f2::point_t& tl, const int lineno,
                          const pixel::f4::vec_t& color, const int font_height_usr,
                          const char* format, ...) noexcept;

    inline texture_ref make_text(const pixel::f2::point_t& tl, const int lineno,
                          const pixel::f4::vec_t& color,
                          const std::string& text) noexcept {
        return make_text(tl, lineno, color, font_height, text);
    }

    //
    // input
    //

    /** Fixed input action enumerator, useful to denote typical game actions e.g. cursor keys. */
    enum class input_event_type_t : int {
        NONE,
        POINTER_BUTTON,
        POINTER_MOTION,
        ANY_KEY, // 3
        P1_UP,      ///< UP, 4
        P1_DOWN,    ///< DOWN
        P1_RIGHT,   ///< RIGHT
        P1_LEFT,    ///< LEFT
        P1_ACTION1, ///< RSHIFT
        P1_ACTION2, ///< RETURN
        P1_ACTION3, ///< RALT
        P1_ACTION4, ///< RCTRL
        PAUSE,      ///< P, 11
        P2_UP,      ///< W
        P2_DOWN,    ///< A
        P2_RIGHT,   ///< S
        P2_LEFT,    ///< D
        P2_ACTION1, ///< LSHIFT
        P2_ACTION2, ///< LCTRL
        P2_ACTION3, ///< LALT
        P2_ACTION4, ///< Z
        P3_UP,      ///< I
        P3_DOWN,    ///< J
        P3_RIGHT,   ///< K
        P3_LEFT,    ///< L
        P3_ACTION1, ///< V
        P3_ACTION2, ///< B
        P3_ACTION3, ///< N
        P3_ACTION4, ///< M
        RESET,      ///< R, 26
        F1,         ///< F1
        F2,         ///< F2
        F3,         ///< F3
        F4,         ///< F4
        F5,         ///< F5
        F6,         ///< F6, 32
        F7,         ///< F7
        F8,         ///< F8
        F9,         ///< F9
        F10,         ///< F10
        F11,         ///< F11
        F12,         ///< F12, 38
        /** Request to close window, which then should be closed by caller */
        WINDOW_CLOSE_REQ,
        WINDOW_RESIZED, // 40
    };
    constexpr int bitno(const input_event_type_t e) noexcept {
        return static_cast<int>(e) - static_cast<int>(input_event_type_t::ANY_KEY);
    }
    constexpr uint64_t bitmask(const input_event_type_t e) noexcept {
        return (uint64_t)1 << bitno(e);
    }
    constexpr uint64_t bitmask(const int bit) noexcept {
        return (uint64_t)1 << bit;
    }

    enum class player_event_type_t : int {
        NONE,
        UP,
        DOWN,
        RIGHT,
        LEFT,
        ACTION1,
        ACTION2,
        ACTION3,
        ACTION4,
    };
    constexpr input_event_type_t to_input_event(int player, player_event_type_t pe) noexcept {
        if(1 == player) {
            switch(pe) {
                case player_event_type_t::UP: return input_event_type_t::P1_UP;
                case player_event_type_t::DOWN: return input_event_type_t::P1_DOWN;
                case player_event_type_t::RIGHT: return input_event_type_t::P1_RIGHT;
                case player_event_type_t::LEFT: return input_event_type_t::P1_LEFT;
                case player_event_type_t::ACTION1: return input_event_type_t::P1_ACTION1;
                case player_event_type_t::ACTION2: return input_event_type_t::P1_ACTION2;
                case player_event_type_t::ACTION3: return input_event_type_t::P1_ACTION3;
                case player_event_type_t::ACTION4: return input_event_type_t::P1_ACTION4;
                default: return input_event_type_t::NONE;
            }
        } else if(2 == player) {
            switch(pe) {
                case player_event_type_t::UP: return input_event_type_t::P2_UP;
                case player_event_type_t::DOWN: return input_event_type_t::P2_DOWN;
                case player_event_type_t::RIGHT: return input_event_type_t::P2_RIGHT;
                case player_event_type_t::LEFT: return input_event_type_t::P2_LEFT;
                case player_event_type_t::ACTION1: return input_event_type_t::P2_ACTION1;
                case player_event_type_t::ACTION2: return input_event_type_t::P2_ACTION2;
                case player_event_type_t::ACTION3: return input_event_type_t::P2_ACTION3;
                case player_event_type_t::ACTION4: return input_event_type_t::P2_ACTION4;
                default: return input_event_type_t::NONE;
            }
        } else if(3 == player) {
            switch(pe) {
                case player_event_type_t::UP: return input_event_type_t::P3_UP;
                case player_event_type_t::DOWN: return input_event_type_t::P3_DOWN;
                case player_event_type_t::RIGHT: return input_event_type_t::P3_RIGHT;
                case player_event_type_t::LEFT: return input_event_type_t::P3_LEFT;
                case player_event_type_t::ACTION1: return input_event_type_t::P3_ACTION1;
                case player_event_type_t::ACTION2: return input_event_type_t::P3_ACTION2;
                case player_event_type_t::ACTION3: return input_event_type_t::P3_ACTION3;
                case player_event_type_t::ACTION4: return input_event_type_t::P3_ACTION4;
                default: return input_event_type_t::NONE;
            }
        }
        return input_event_type_t::NONE;
    }

    class input_event_t {
        private:
            constexpr static const uint64_t p1_mask =
                    bitmask(input_event_type_t::P1_UP) |
                    bitmask(input_event_type_t::P1_DOWN) |
                    bitmask(input_event_type_t::P1_RIGHT) |
                    bitmask(input_event_type_t::P1_LEFT) |
                    bitmask(input_event_type_t::P1_ACTION1) |
                    bitmask(input_event_type_t::P1_ACTION2) |
                    bitmask(input_event_type_t::P1_ACTION3) |
                    bitmask(input_event_type_t::P1_ACTION4);

            constexpr static const uint64_t p2_mask =
                    bitmask(input_event_type_t::P2_UP) |
                    bitmask(input_event_type_t::P2_DOWN) |
                    bitmask(input_event_type_t::P2_RIGHT) |
                    bitmask(input_event_type_t::P2_LEFT) |
                    bitmask(input_event_type_t::P2_ACTION1) |
                    bitmask(input_event_type_t::P2_ACTION2) |
                    bitmask(input_event_type_t::P2_ACTION3) |
                    bitmask(input_event_type_t::P2_ACTION4);

            constexpr static const uint64_t p3_mask =
                    bitmask(input_event_type_t::P3_UP) |
                    bitmask(input_event_type_t::P3_DOWN) |
                    bitmask(input_event_type_t::P3_RIGHT) |
                    bitmask(input_event_type_t::P3_LEFT) |
                    bitmask(input_event_type_t::P3_ACTION1) |
                    bitmask(input_event_type_t::P3_ACTION2) |
                    bitmask(input_event_type_t::P3_ACTION3) |
                    bitmask(input_event_type_t::P3_ACTION4);
            uint64_t m_pressed; // [P1_UP..F12]
            uint64_t m_lifted; // [P1_UP..F12]
            bool m_paused;
        public:
            input_event_type_t last;
            /** ASCII code, ANY_KEY_UP, ANY_KEY_DOWN key code */
            uint16_t last_key_code;
            std::string text;
            int pointer_id;
            int pointer_x;
            int pointer_y;

            input_event_t() noexcept { clear(); }
            void clear() noexcept {
                m_pressed = 0;
                m_lifted = 0;
                m_paused = false;
                last = input_event_type_t::NONE;
                pointer_id = -1;
                pointer_x = -1;
                pointer_y = -1;
            }
            void pointer_motion(int id, int x, int y) noexcept {
                set(input_event_type_t::POINTER_MOTION);
                pointer_id = id;
                pointer_x = x;
                pointer_y = y;
            }
            void set(input_event_type_t e, uint16_t key_code=0) noexcept {
                const int bit = bitno(e);
                if( 0 <= bit && bit < 64 ) {
                    const uint64_t m = bitmask(bit);
                    m_lifted &= ~m;
                    m_pressed |= m;
                }
                this->last = e;
                this->last_key_code = key_code;
                if( this->text.length() > 0 && '\n' == this->text[this->text.length()-1] ) {
                    this->text.clear();
                }
                if( 0 != key_code && jau::is_ascii_code(key_code) ) {
                    if( 0x08 == key_code ) {
                        if( this->text.length() > 0 ) {
                            this->text.pop_back();
                        }
                    } else {
                        this->text.push_back( (char)key_code );
                    }
                }
            }
            void clear(input_event_type_t e, uint16_t key_code=0) noexcept {
                (void)key_code;
                const int bit = bitno(e);
                if( 0 <= bit && bit < 64 ) {
                    const uint64_t m = bitmask(bit);
                    m_lifted |= m_pressed & m;
                    m_pressed &= ~m;
                }
                this->last = e;
                this->last_key_code = key_code;
                if( input_event_type_t::PAUSE == e ) {
                    m_paused = !m_paused;
                }
            }
            void set_paused(bool v) noexcept { m_paused = v; }
            bool paused() const noexcept { return m_paused; }
            bool pressed(input_event_type_t e) const noexcept {
                const int bit = bitno(e);
                if( 0 <= bit && bit < 64 ) {
                    return 0 != ( m_pressed & bitmask(bit) );
                } else {
                    return false;
                }
            }
            bool pressed_and_clr(input_event_type_t e) noexcept {
                if( pressed(e) ) {
                    clear(e);
                    return true;
                } else {
                    return false;
                }
            }
            bool released_and_clr(input_event_type_t e) noexcept {
                const int bit = bitno(e);
                if( 0 <= bit && bit < 64 ) {
                    const uint64_t m = bitmask(bit);
                    if( 0 != ( m_lifted & m ) ) {
                        m_lifted &= ~m;
                        return true;
                    }
                }
                return false;
            }
            bool has_any_p1() const noexcept {
                return 0 != ( ( m_pressed | m_lifted ) & p1_mask );
            }
            bool has_any_p2() const noexcept {
                return 0 != ( ( m_pressed | m_lifted ) & p2_mask );
            }
            bool has_any_p3() const noexcept {
                return 0 != ( ( m_pressed | m_lifted ) & p3_mask );
            }
            bool has_any_pn(int player) const noexcept {
                switch (player) {
                    case 1: return has_any_p1();
                    case 2: return has_any_p2();
                    case 3: return has_any_p3();
                    default: return false;
                }
            }
            std::string to_string() const noexcept;
    };
    inline std::string to_string(const input_event_t& e) noexcept { return e.to_string(); }

    /**
     * GFX Toolkit: Handle windowing and keyboard events.
     *
     * Should be called until function returns false
     * to process all buffered events.
     *
     * @param event
     * @return true if event received, false otherwise
     */
    bool handle_one_event(input_event_t& event) noexcept;

    inline bool handle_events(input_event_t& event) noexcept {
        bool one = false;
        while( pixel::handle_one_event(event) ) {
            one = true;
            // std::cout << "Input " << to_string(event) << std::endl;
        }
        return one;
    }

    /**
     * Event callback function pointer.
     *
     * @return true to continue event polling, otherwise end event polling to return immediately
     */
    typedef bool (*event_callback)(input_event_t& event) noexcept;

    inline bool handle_events(input_event_t& event, event_callback cb) noexcept {
        bool cont = true;
        bool one = false;
        while( cont && pixel::handle_one_event(event) ) {
            one = true;
            cont = cb(event);
            // std::cout << "Input " << to_string(event) << std::endl;
        }
        return one;
    }

    void save_snapshot(const std::string& fname) noexcept;

}

#endif /*  PIXEL_HPP_ */

