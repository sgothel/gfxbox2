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

#include <cinttypes>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>
#include <iostream>

namespace pixel::f2 {
    class vec_t; // fwd
    typedef vec_t point_t;

}
namespace pixel::f4 {
    class vec_t; // fwd
    typedef vec_t point_t;
}

/**
 * Basic computer graphics math and utilities helping with the framebuffer and I/O tooling.
 */
namespace pixel {
    inline constexpr float epsilon() noexcept {
        float a = 1.0f;
        float b;
        do {
            b = a;
            a = a / 2.0f;
        } while(1.0f + a != 1.0f);
        return b;
    }

    /** Returns true of the given float is less than float epsilon. */
    inline constexpr bool is_zero(const float v) noexcept {
        return std::abs(v) < std::numeric_limits<float>::epsilon();
    }
    /** Returns true of the given double  is less than double epsilon. */
    inline constexpr bool is_zero(const double v) noexcept {
        return std::abs(v) < std::numeric_limits<double>::epsilon();
    }

    /**
     * Return true if both values are equal, i.e. their absolute delta is less than float epsilon,
     * false otherwise.
     */
    inline constexpr bool equals(const float a, const float b) noexcept {
        return std::abs(a - b) < std::numeric_limits<float>::epsilon();
    }

    /**
     * Return zero if both values are equal, i.e. their absolute delta is less than float epsilon,
     * -1 if a < b and 1 otherwise.
     */
    inline constexpr int compare(const float a, const float b) noexcept {
        if( std::abs(a - b) < std::numeric_limits<float>::epsilon() ) {
            return 0;
        } else if (a < b) {
            return -1;
        } else {
            return 1;
        }
    }

    /** Returns the rounded float value cast to int. */
    inline constexpr int round_to_int(const float v) noexcept {
        return (int)std::round(v);
    }
    /** Returns the rounded double value cast to int. */
    inline constexpr int round_to_int(const double v) noexcept {
        return (int)std::round(v);
    }

    /** Converts arc-degree to radians */
    inline constexpr float adeg_to_rad(const float arc_degree) noexcept {
        return arc_degree * (float)M_PI / 180.0f;
    }

    /** Converts radians to arc-degree */
    inline constexpr float rad_to_adeg(const float rad) noexcept {
        return rad * 180.0f / (float)M_PI;
    }

    enum class orientation_t {
        /** Collinear **/
        COL,
        /** Clockwise **/
        CLW,
        /** Counter-Clockwise **/
        CCW
    };

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
    extern int frames_per_sec;
    extern int font_height;


    /**
     * Cartesian coordinate system within the framebuffer space in pixels.
     */
    class cart_coord_t {
        private:
            float x1;
            float y1;
            float x2;
            float y2;
            float w_to_fbw;
            float h_to_fbh;
        public:
            cart_coord_t() noexcept { set_one(0.0f, 0.0f); }
            cart_coord_t(float x1_, float y1_) noexcept { set_one(x1_, y1_); }
            cart_coord_t(float x1_, float y1_, float x2_, float y2_) noexcept { set_free(x1_, y1_, x2_, y2_); }

            std::string toString() const noexcept {
                const int fx1 = to_fb_x(x1);
                const int fy1 = to_fb_y(y1);
                const int fx2 = to_fb_x(x2);
                const int fy2 = to_fb_y(y2);
                const int fxO = to_fb_x(0.0f);
                const int fyO = to_fb_y(0.0f);
                return "cart[min "+std::to_string(x1)+"/"+std::to_string(y1)+", max "+std::to_string(x2)+"/"+std::to_string(y2)+
                        ", size "+std::to_string(width())+"x"+std::to_string(height())+", scale x "+std::to_string(w_to_fbw)+", y "+std::to_string(h_to_fbh)
                        +"], fb[min "+std::to_string(fx1)+"/"+std::to_string(fy1)+", max "+std::to_string(fx2)+"/"+std::to_string(fy2)+
                        ", origin "+std::to_string(fxO)+"/"+std::to_string(fyO)+", size "+std::to_string(fb_width)+"x"+std::to_string(fb_height)+"]";
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
                x1 = x1_;
                y1 = y1_;
                w_to_fbw = 1.0f;
                h_to_fbh = 1.0f;
                x2 = x1 + (float)fb_max_x * w_to_fbw;
                y2 = y1 + (float)fb_max_y * h_to_fbh;
                printf("set_one: %s\n", toString().c_str());
            }

            /**
             * Setup cartesian width with given x-axis coordinates
             * and use same aspect ratio for a centered y-axis.
             *
             * The x-axis and y-axis minimum determine the origin.
             */
            void set_width(float x1_, float x2_) noexcept {
                x1 = x1_;
                x2 = x2_;
                w_to_fbw = width() / fb_width;
                h_to_fbh = w_to_fbw;
                const float h = (float)fb_max_y * h_to_fbh;
                y1 = h / -2.0f;
                y2 = y1 + h;
                printf("coord.set_width: %s\n", toString().c_str());
            }

            /**
             * Setup cartesian height with given y-axis coordinates
             * and use same aspect ratio for a centered x-axis.
             *
             * The x-axis and y-axis minimum determine the origin.
             */
            void set_height(float y1_, float y2_) noexcept {
                y1 = y1_;
                y2 = y2_;
                h_to_fbh = height() / (float)fb_height;
                w_to_fbw = h_to_fbh;
                const float w = (float)fb_max_x * w_to_fbw;
                x1 = w / -2.0f;
                x2 = x1 + w;
                printf("coord.set_height: %s\n", toString().c_str());
            }

            /**
             * Setup cartesian width free aspect ration for both axis.
             *
             * The x-axis and y-axis minimum determine the origin.
             */
            void set_free(float x1_, float y1_, float x2_, float y2_) noexcept {
                x1 = x1_;
                y1 = y1_;
                x2 = x2_;
                y2 = y2_;
                w_to_fbw = width() / fb_width;
                h_to_fbh = height() / fb_height;
                printf("coord.set_free: %s\n", toString().c_str());
            }

            /** x-axis minimum of the cartesian coordinate system within the framebuffer space. */
            constexpr float min_x() const noexcept { return x1; }
            /** y-axis minimum of the cartesian coordinate system within the framebuffer space. */
            constexpr float min_y() const noexcept { return y1; }
            /** x-axis maximum of the cartesian coordinate system within the framebuffer space. */
            constexpr float max_x() const noexcept { return x2; }
            /** y-axis maximum of the cartesian coordinate system within the framebuffer space. */
            constexpr float max_y() const noexcept { return y2; }

            /** x-axis width of the cartesian coordinate system. */
            constexpr float width() const noexcept { return x2 - x1; }
            /** y-axis height of the cartesian coordinate system. */
            constexpr float height() const noexcept { return y2 - y1; }

            /** Convert cartesian x-axis value to framebuffer pixel value. */
            int to_fb_dx(const float dx) const noexcept { return round_to_int( dx / w_to_fbw ); }
            /** Convert cartesian y-axis value to framebuffer pixel value. */
            int to_fb_dy(const float dy) const noexcept { return round_to_int( dy / h_to_fbh ); }

            /** Convert framebuffer x-axis value in pixels to cartesian pixel value. */
            int from_fb_dx(const int dx) const noexcept { return (float)dx * w_to_fbw; }
            /** Convert framebuffer y-axis value in pixels to cartesian pixel value. */
            int from_fb_dy(const int dy) const noexcept { return (float)dy * h_to_fbh; }

            /** Convert cartesian x-axis coordinate to framebuffer coordinate in pixels. */
            int to_fb_x(const float x) const noexcept { return round_to_int( ( x - x1 ) / w_to_fbw ); }
            /** Convert cartesian y-axis coordinate in pixels to framebuffer coordinate in pixels. */
            int to_fb_y(const float y) const noexcept { return fb_height - round_to_int( ( y - y1 ) / h_to_fbh ); }

            /** Convert framebuffer x-axis coordinate in pixels to cartesian coordinate. */
            float from_fb_x(const int x) const noexcept { return (float)x * w_to_fbw + x1; }
            /** Convert framebuffer y-axis coordinate in pixels to cartesian coordinate. */
            float from_fb_y(const int y) const noexcept { return (float)(fb_height-y) * h_to_fbh + y1; }
    };
    extern cart_coord_t cart_coord;

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
            fb_pixels[ y * fb_width + x ] = draw_color;
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
        if( 0 <= x && x <= fb_max_x && 0 <= y && y <= fb_max_y ) {
            fb_pixels[ y * fb_width + x ] = draw_color;
        }
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
    }
    /** Set current pixel draw color */
    inline void set_pixel_color(const uint8_t rgba[/*4*/]) noexcept {
        draw_color = pixel::rgba_to_uint32(rgba[0], rgba[1], rgba[2], rgba[3]);
    }
    inline float clip_byte(float v) { return std::max<float>(0.0f, std::min(255.0f, v)); }
    inline void set_pixel_color4f(float r, float g, float b, float a) noexcept {
        set_pixel_color(clip_byte(r*255.0f), clip_byte(g*255.0f), clip_byte(b*255), clip_byte(a*255));
    }

    void draw_grid(float raster_sz,
                   uint8_t gr, uint8_t gg, uint8_t gb, uint8_t ga,
                   uint8_t cr, uint8_t cg, uint8_t cb, uint8_t ca);

    //
    // Texture
    //

    class texture_t {
        private:
            void* data;
        public:
            /** source texture pos-x */
            int x;
            /** source texture pos-y */
            int y;
            /** source texture width */
            int width;
            /** source texture width */
            int height;
            /** dest texture pos-x */
            int dest_x;
            /** dest texture pos-y */
            int dest_y;
            /** dest texture scale-x */
            float dest_sx;
            /** dest texture scale-y */
            float dest_sy;

            texture_t(void* data_, const int x_, const int y_, const int width_, const int height_) noexcept
            : data(data_), x(x_), y(y_), width(width_), height(height_), dest_x(0), dest_y(0), dest_sx(1), dest_sy(1) {}

            texture_t() noexcept
            : data(nullptr), x(0), y(0), width(0), height(0), dest_x(0), dest_y(0), dest_sx(1), dest_sy(1) {}

            texture_t(const texture_t&) = delete;
            void operator=(const texture_t&) = delete;

            ~texture_t() noexcept {
                destroy();
            }

            void destroy() noexcept;

            void draw(const int x_pos, const int y_pos, const float scale_x=1.0f, const float scale_y=1.0f) noexcept;
    };
    typedef std::shared_ptr<texture_t> texture_ref;

    //
    // gfx toolkit dependent API
    //

    /** GFX Toolkit: Initialize a window of given size with a usable framebuffer. */
    void init_gfx_subsystem(const char* title, unsigned int win_width, unsigned int win_height, const float origin_norm[2], bool enable_vsync=true);
    /** GFX Toolkit: Clear the soft-framebuffer. */
    void clear_pixel_fb(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept;
    /** GFX Toolkit: Copy the soft-framebuffer to the GPU back-buffer, if swap_buffer is true (default) also swap_gpu_buffer(). */
    void swap_pixel_fb(const bool swap_buffer=true) noexcept;
    /** GFX Toolkit: Swap GPU back to front framebuffer while maintaining vertical monitor synchronization if possible. */
    void swap_gpu_buffer(int fps=0) noexcept;
    float get_gpu_fps() noexcept;

    texture_ref make_text_texture(const std::string& text) noexcept;


    texture_ref make_text(const pixel::f2::point_t& tl, const int lineno,
                          const pixel::f4::vec_t& color, const int font_height_usr,
                          const std::string& text) noexcept;

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
        ANY_KEY_UP,
        ANY_KEY_DOWN,
        P1_UP, // 5
        P1_DOWN,
        P1_RIGHT,
        P1_LEFT,
        P1_ACTION1,
        P1_ACTION2,
        PAUSE, // 11
        P2_UP,
        P2_DOWN,
        P2_RIGHT,
        P2_LEFT,
        P2_ACTION1,
        P2_ACTION2,
        RESET, // 18
        /** Request to close window, which then should be closed by caller */
        WINDOW_CLOSE_REQ,
        WINDOW_RESIZED, // 20
    };
    constexpr int bitno(const input_event_type_t e) noexcept {
        return static_cast<int>(e) - static_cast<int>(input_event_type_t::P1_UP);
    }
    constexpr uint32_t bitmask(const input_event_type_t e) noexcept {
        return 1U << bitno(e);
    }
    constexpr uint32_t bitmask(const int bit) noexcept {
        return 1U << bit;
    }

    class input_event_t {
        private:
            constexpr static const uint32_t p1_mask =
                    bitmask(input_event_type_t::P1_UP) |
                    bitmask(input_event_type_t::P1_DOWN) |
                    bitmask(input_event_type_t::P1_RIGHT) |
                    bitmask(input_event_type_t::P1_LEFT) |
                    bitmask(input_event_type_t::P1_ACTION1) |
                    bitmask(input_event_type_t::P1_ACTION2);
            constexpr static const uint32_t p2_mask =
                    bitmask(input_event_type_t::P2_UP) |
                    bitmask(input_event_type_t::P2_DOWN) |
                    bitmask(input_event_type_t::P2_RIGHT) |
                    bitmask(input_event_type_t::P2_LEFT) |
                    bitmask(input_event_type_t::P2_ACTION1) |
                    bitmask(input_event_type_t::P2_ACTION2);
            uint32_t m_pressed; // [P1_UP..RESET]
            uint32_t m_lifted; // [P1_UP..RESET]
            bool m_paused;
        public:
            input_event_type_t last;
            /** ASCII code, ANY_KEY_UP, ANY_KEY_DOWN key code */
            uint16_t last_key_code;
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
                if( 0 <= bit && bit <= 31 ) {
                    const uint32_t m = bitmask(bit);
                    m_lifted &= ~m;
                    m_pressed |= m;
                }
                this->last = e;
                this->last_key_code = key_code;
            }
            void clear(input_event_type_t e, uint16_t key_code=0) noexcept {
                (void)key_code;
                const int bit = bitno(e);
                if( 0 <= bit && bit <= 31 ) {
                    const uint32_t m = bitmask(bit);
                    m_lifted |= m_pressed & m;
                    m_pressed &= ~m;
                }
                if( input_event_type_t::PAUSE == e ) {
                    m_paused = !m_paused;
                }
            }
            bool paused() const noexcept { return m_paused; }
            bool pressed(input_event_type_t e) const noexcept {
                const int bit = bitno(e);
                if( 0 <= bit && bit <= 31 ) {
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
                if( 0 <= bit && bit <= 31 ) {
                    const uint32_t m = bitmask(bit);
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
    void save_snapshot(const std::string& fname) noexcept;

    //
    // data packing macros
    //

    /** packed__: lead in macro, requires __packed lead out as well. Consider using __pack(...). */
    #ifndef packed__
        #ifdef _MSC_VER
            #define packed__ __pragma( pack(push, 1) )
        #else
            #define packed__
        #endif
    #endif

    /** __packed: lead out macro, requires packed__ lead in as well. Consider using __pack(...). */
    #ifndef __packed
        #ifdef _MSC_VER
            #define __packed __pragma( pack(pop))
        #else
            #define __packed __attribute__ ((packed))
        #endif
    #endif

    /** __pack(...): Produces MSVC, clang and gcc compatible lead-in and -out macros. */
    #ifndef __pack
        #ifdef _MSC_VER
            #define __pack(...) __pragma( pack(push, 1) ) __VA_ARGS__ __pragma( pack(pop))
        #else
            #define __pack(...) __VA_ARGS__ __attribute__ ((packed))
        #endif
    #endif

    //
    // Misc
    //

    inline constexpr const int64_t NanoPerMilli = 1000000L;
    inline constexpr const int64_t MilliPerOne = 1000L;
    inline constexpr const int64_t NanoPerOne = NanoPerMilli*MilliPerOne;

    /** Return current milliseconds, since Unix epoch. */
    uint64_t getCurrentMilliseconds() noexcept;
    /** Return current milliseconds, since program launch. */
    uint64_t getElapsedMillisecond() noexcept;
    /** Sleep for the givn milliseconds. */
    void milli_sleep(uint64_t td) noexcept;

    void log_printf(const uint64_t elapsed_ms, const char * format, ...) noexcept;
    void log_printf(const char * format, ...) noexcept;

    //
    // Cut from jaulib
    //

    template <typename T>
    constexpr ssize_t sign(const T x) noexcept
    {
        return (T(0) < x) - (x < T(0));
    }

    template <typename T>
    constexpr T invert_sign(const T x) noexcept
    {
        return std::numeric_limits<T>::min() == x ? std::numeric_limits<T>::max() : -x;
    }

    template<typename T>
    constexpr size_t digits10(const T x, const ssize_t x_sign, const bool sign_is_digit=true) noexcept
    {
        if( x_sign == 0 ) {
            return 1;
        }
        if( x_sign < 0 ) {
            return 1 + static_cast<size_t>( std::log10<T>( invert_sign<T>( x ) ) ) + ( sign_is_digit ? 1 : 0 );
        } else {
            return 1 + static_cast<size_t>( std::log10<T>(                 x   ) );
        }
    }

    template< class value_type,
              std::enable_if_t< std::is_integral_v<value_type>,
                                bool> = true>
    std::string to_decstring(const value_type& v, const char separator=',', const size_t width=0) noexcept {
        const ssize_t v_sign = sign<value_type>(v);
        const size_t digit10_count1 = digits10<value_type>(v, v_sign, true /* sign_is_digit */);
        const size_t digit10_count2 = v_sign < 0 ? digit10_count1 - 1 : digit10_count1; // less sign

        const size_t comma_count = 0 == separator ? 0 : ( digit10_count1 - 1 ) / 3;
        const size_t net_chars = digit10_count1 + comma_count;
        const size_t total_chars = std::max<size_t>(width, net_chars);
        std::string res(total_chars, ' ');

        value_type n = v;
        size_t char_iter = 0;

        for(size_t digit10_iter = 0; digit10_iter < digit10_count2 /* && char_iter < total_chars */; digit10_iter++ ) {
            const int digit = v_sign < 0 ? invert_sign( n % 10 ) : n % 10;
            n /= 10;
            if( 0 < digit10_iter && 0 == digit10_iter % 3 ) {
                res[total_chars-1-(char_iter++)] = separator;
            }
            res[total_chars-1-(char_iter++)] = '0' + digit;
        }
        if( v_sign < 0 /* && char_iter < total_chars */ ) {
            res[total_chars-1-(char_iter++)] = '-';
        }
        return res;
    }
}

#endif /*  PIXEL_HPP_ */

