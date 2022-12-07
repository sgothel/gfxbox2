#ifndef PIXEL_HPP_
#define PIXEL_HPP_

#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdint>
#include <cstdarg>
#include <cmath>
#include <limits>
#include <string>
#include <memory>
#include <functional>
#include <vector>

/**
 * Basic computer graphics math and utilities helping with the framebuffer and I/O tooling.
 */
namespace pixel {
    /** Returns true of the given float is less than float epsilon. */
    inline constexpr bool is_zero(const float v) noexcept {
        return std::abs(v) < std::numeric_limits<float>::epsilon();
    }

    /** Returns the rounded float value cast to int. */
    inline constexpr int round_to_int(const float v) noexcept {
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

    /**
     * Cartesian coordinate system origin within the framebuffer space in pixels.
     *
     * Value in range { [0, fb_width-1], [0, fb_height-1] },
     * or { [0, fb_max_x], [0, fb_max_y] }
     * with
     * - 0/0 being the screen-center,
     * - x > 0 denoting the right side
     * - y > 0 denoting the upper side
     */
    extern int cart_origin[2];

    /** x-axis minimum of the cartesian coordinate system within the framebuffer space in pixels. */
    extern int cart_min_x;
    /** y-axis minimum of the cartesian coordinate system within the framebuffer space in pixels. */
    extern int cart_min_y;
    /** x-axis maximum of the cartesian coordinate system within the framebuffer space in pixels. */
    extern int cart_max_x;
    /** y-axis maximum of the cartesian coordinate system within the framebuffer space in pixels. */
    extern int cart_max_y;

    /** The current 32-bit draw color. */
    extern uint32_t draw_color; // 32-bit pixel

    /** Return clipped x-axis framebuffer coordinate. */
    inline unsigned int clip_fb_x(const int x) noexcept { return std::min<int>(fb_max_x, std::max<int>(0, x)); }
    /** Return clipped y-axis framebuffer coordinate. */
    inline unsigned int clip_fb_y(const int y) noexcept { return std::min<int>(fb_max_y, std::max<int>(0, y)); }

    /** Convert cartesian x-axis coordinate in pixels to framebuffer coordinate in pixels. */
    inline int cart_to_fb_x(const int x) noexcept { return cart_origin[0] + x; }
    /** Convert cartesian x-axis coordinate in pixels to framebuffer coordinate in pixels. */
    inline int cart_to_fb_x(const float x) noexcept { return cart_origin[0] + round_to_int(x); }

    /** Convert cartesian y-axis coordinate in pixels to framebuffer coordinate in pixels. */
    inline int cart_to_fb_y(const int y) noexcept { return fb_height - ( cart_origin[1] + y ); }
    /** Convert cartesian y-axis coordinate in pixels to framebuffer coordinate in pixels. */
    inline int cart_to_fb_y(const float y) noexcept { return fb_height - ( cart_origin[1] + round_to_int(y) ); }

    /** Convert framebuffer x-axis coordinate in pixels to cartesian coordinate in pixels. */
    inline int fb_to_cart_x(const int x) noexcept { return x - cart_origin[0]; }
    /** Convert framebuffer y-axis coordinate in pixels to cartesian coordinate in pixels. */
    inline int fb_to_cart_y(const int y) noexcept { return fb_height - y - cart_origin[1]; }

    /**
     * Set a pixel using the given draw_color and given cartesian coordinates in pixels.
     *
     * In case the coordinates are out of bounds, the pixel is discarded, i.e. not set.
     */
    inline void set_pixel(int x_, int y_) noexcept {
        const int x = cart_to_fb_x( x_ );
        const int y = cart_to_fb_y( y_ );
        if( 0 <= x && x < fb_max_x && 0 <= y && y < fb_max_y ) {
            fb_pixels[ y * fb_width + x ] = draw_color;
        }
    }

    /**
     * Set a pixel using the given draw_color and given cartesian coordinates in float pixels.
     *
     * The float coordinate is rounded before converting them to framebuffer coordinates.
     *
     * In case the coordinates are out of bounds, the pixel is discarded, i.e. not set.
     */
    inline void set_pixel(float x_, float y_) noexcept {
        const int x = cart_to_fb_x( round_to_int( x_ ) );
        const int y = cart_to_fb_y( round_to_int( y_ ) );
        if( 0 <= x && x < fb_max_x && 0 <= y && y < fb_max_y ) {
            fb_pixels[ y * fb_width + x ] = draw_color;
        }
    }

    /** Return current milliseconds, since Unix epoch. */
    uint64_t getCurrentMilliseconds() noexcept;
    /** Return current milliseconds, since program launch. */
    uint64_t getElapsedMillisecond() noexcept;
    /** Sleep for the givn milliseconds. */
    void milli_sleep(uint64_t td) noexcept;

    /** Direction enumerator, useful to denote e.g. input cursor keys. */
    enum class direction_t {
        UP,
        DOWN,
        RIGHT,
        LEFT
    };

    /** Convert 32-bit RGBA components to the gfx-toolkit specific RGBA 32-bit integer presentation. */
    uint32_t rgba_to_uint32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept;

    /** Set current pixel draw color */
    inline void set_pixel_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept {
        draw_color = pixel::rgba_to_uint32(r, g, b, a);
    }

    //
    // gfx toolkit dependent API
    //

    /** GFX Toolkit: Initialize a window of given size with a usable framebuffer. */
    void init_gfx_subsystem(const char* title, unsigned int win_width, unsigned int win_height, const float origin_norm[2]);
    /** GFX Toolkit: Clear the framebuffer. */
    void clear_pixel_fb(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept;
    /** GFX Toolkit: Swap back to front framebuffer while maintaining vertical monitor synchronization if possible. */
    void swap_pixel_fb() noexcept;

    /** GFX Toolkit: Handle windowing and keyboard events. */
    void handle_events(bool& close, bool& resized, bool& set_dir, direction_t& dir) noexcept;

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

}

#endif /*  PIXEL_HPP_ */

