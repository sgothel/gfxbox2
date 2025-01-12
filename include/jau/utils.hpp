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
#ifndef JAU_UTILS_HPP_
#define JAU_UTILS_HPP_

#include <unistd.h>
#include <cstddef>
#include <limits>
#include <string>
#include <type_traits>

#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cctype>

#include <jau/int_types.hpp>
#include <jau/float_si_types.hpp>
#include <jau/file_util.hpp>
#include <jau/fraction_type.hpp>

/**
 * Basic computer graphics math and utilities helping with the framebuffer and I/O tooling.
 */
namespace jau {
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

    /** Returns the floor float value cast to int. */
    inline constexpr int floor_to_int(const float v) noexcept {
        return (int)std::floor(v);
    }
    /** Returns the floor float value cast to int. */
    inline constexpr uint32_t floor_to_uint32(const float v) noexcept {
        return std::max(0, (int)std::floor(v));
    }

    /** Returns the rounded float value cast to int. */
    inline constexpr int round_to_int(const float v) noexcept {
        return (int)std::round(v);
    }
    /** Returns the rounded double value cast to int. */
    inline constexpr int round_to_int(const double v) noexcept {
        return (int)std::round(v);
    }
    /** Returns the rounded float value cast to int. */
    inline constexpr uint32_t round_to_uint32(const float v) noexcept {
        return std::max(0, (int)std::round(v));
    }

    /** Returns the ceil float value cast to int. */
    inline constexpr int ceil_to_int(const float v) noexcept {
        return (int)std::ceil(v);
    }
    /** Returns the ceil float value cast to int. */
    inline constexpr uint32_t ceil_to_uint32(const float v) noexcept {
        return std::max(0, (int)std::ceil(v));
    }

    /** Returns the floor float value cast to int. */
    inline constexpr int trunc_to_int(const float v) noexcept {
        return (int)std::trunc(v);
    }
    /** Returns the floor float value cast to int. */
    inline constexpr uint32_t trunc_to_uint32(const float v) noexcept {
        return std::max(uint32_t(0), (uint32_t)std::trunc(v));
    }

    /** Converts arc-degree to radians */
    inline constexpr float adeg_to_rad(const float arc_degree) noexcept {
        return arc_degree * std::numbers::pi_v<float> / 180.0f;
    }

    /** Converts radians to arc-degree */
    inline constexpr float rad_to_adeg(const float rad) noexcept {
        return rad * 180.0f / std::numbers::pi_v<float>;
    }

    float next_rnd() noexcept;

    inline float next_rnd(float min, float max) noexcept {
        return next_rnd() * float(max - min) + float(min);
    }
    template <typename T,
              std::enable_if_t< std::is_integral_v<T> && std::is_unsigned_v<T>, bool> = true>
    T next_rnd(T min, T max) noexcept {
        return (T)std::round(next_rnd() * float(max - min) + float(min));
    }

    std::string to_string(const char* format, ...) noexcept;
    std::string to_stringva(const char* format, va_list args) noexcept;

    inline bool is_ascii_code(int c) noexcept {
        return 0 != std::iscntrl(c) || 0 != std::isprint(c);
    }

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

#endif /*  JAU_UTILS_HPP_ */

