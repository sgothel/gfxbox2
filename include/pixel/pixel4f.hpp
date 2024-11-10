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
#ifndef PIXEL4F_HPP_
#define PIXEL4F_HPP_

#include "pixel.hpp"

/**
 * 3D computer graphics math based upon four float components.
 */
namespace pixel {
namespace f4 {
    class vec_t {
        public:
            float x;
            float y;
            float z;
            float w;

            constexpr vec_t() noexcept
            : x(0), y(0), z(0), w(0) {}

            constexpr vec_t(const float x_, const float y_, const float z_, const float w_) noexcept
            : x(x_), y(y_), z(z_), w(w_) {}

            constexpr vec_t(const vec_t& o) noexcept = default;
            constexpr vec_t(vec_t&& o) noexcept = default;
            constexpr vec_t& operator=(const vec_t&) noexcept = default;
            constexpr vec_t& operator=(vec_t&&) noexcept = default;

            /**
             * Returns component value, this reinterpreted as `float*` w/o boundary check
             */
            float operator[](size_t i) const noexcept {
                const float* p = reinterpret_cast<const float*>(this);
                return p[i];
            }
            const float* data() const noexcept {
                return reinterpret_cast<const float*>(this);
            }
            float* data() noexcept {
                return reinterpret_cast<float*>(this);
            }

            constexpr void add(const float dx, const float dy, const float dz, const float dw) noexcept {
                x+=dx; y+=dy; z+=dz; w+=dw;
            }

            constexpr vec_t& operator+=(const vec_t& rhs ) noexcept {
                x+=rhs.x; y+=rhs.y; z+=rhs.z; w+=rhs.w;
                return *this;
            }

            constexpr vec_t& operator-=(const vec_t& rhs ) noexcept {
                x-=rhs.x; y-=rhs.y; z-=rhs.z; w-=rhs.w;
                return *this;
            }

            /**
             * Scale this vector with given scale factor
             * @param s scale factor
             * @return this instance
             */
            constexpr vec_t& operator*=(const float s ) noexcept {
                x*=s; y*=s; z*=s; w*=s;
                return *this;
            }

            /**
             * Divide this vector with given scale factor
             * @param s scale factor
             * @return this instance
             */
            constexpr vec_t& operator/=(const float s ) noexcept {
                x/=s; y/=s; z/=s; w/=s;
                return *this;
            }

            std::string toString() const noexcept { return std::to_string(x)+"/"+std::to_string(y)+"/"+std::to_string(z)+"/"+std::to_string(w); }

            constexpr bool is_zero() const noexcept {
                return pixel::is_zero(x) && pixel::is_zero(y) && pixel::is_zero(z) && pixel::is_zero(w);
            }

            /**
             * Return the squared length of a vector, a.k.a the squared <i>norm</i> or squared <i>magnitude</i>
             */
            constexpr float length_sq() const noexcept {
                return x*x + y*y + z*z + w*w;
            }

            /**
             * Return the length of a vector, a.k.a the <i>norm</i> or <i>magnitude</i>
             */
            constexpr float length() const noexcept {
                return std::sqrt(length_sq());
            }

            /**
             * Normalize this vector in place
             */
            constexpr vec_t& normalize() noexcept {
                const float lengthSq = length_sq();
                if ( pixel::is_zero( lengthSq ) ) {
                    x = 0.0f;
                    y = 0.0f;
                    z = 0.0f;
                    w = 0.0f;
                } else {
                    const float invSqr = 1.0f / std::sqrt(lengthSq);
                    x *= invSqr;
                    y *= invSqr;
                    z *= invSqr;
                    w *= invSqr;
                }
                return *this;
            }

            /**
             * Return the squared distance between this vector and the given one.
             * <p>
             * When comparing the relative distance between two points it is usually sufficient to compare the squared
             * distances, thus avoiding an expensive square root operation.
             * </p>
             */
            constexpr float dist_sq(const vec_t& o) const noexcept {
                const float dx = x - o.x;
                const float dy = y - o.y;
                const float dz = z - o.z;
                const float dw = w - o.w;
                return dx*dx + dy*dy + dz*dz + dw*dw;
            }

            /**
             * Return the distance between this vector and the given one.
             */
            constexpr float dist(const vec_t& o) const noexcept {
                return std::sqrt(dist_sq(o));
            }

            bool intersects(const vec_t& o) const noexcept {
                const float eps = std::numeric_limits<float>::epsilon();
                if( std::abs(x-o.x) >= eps || std::abs(y-o.y) >= eps ||
                    std::abs(z-o.z) >= eps || std::abs(w-o.w) >= eps ) {
                    return false;
                }
                return true;
            }
    };

    typedef vec_t point_t;

    constexpr vec_t operator+(const vec_t& lhs, const vec_t& rhs ) noexcept {
        vec_t r(lhs);
        r += rhs;
        return r;
    }

    constexpr vec_t operator-(const vec_t& lhs, const vec_t& rhs ) noexcept {
        vec_t r(lhs);
        r -= rhs;
        return r;
    }

    constexpr vec_t operator*(const vec_t& lhs, const float s ) noexcept {
        vec_t r(lhs);
        r *= s;
        return r;
    }

    constexpr vec_t operator*(const float s, const vec_t& rhs) noexcept {
        vec_t r(rhs);
        r *= s;
        return r;
    }
} // namespace pixel_4f

    inline void set_pixel_color(const f4::vec_t& rgba) noexcept {
        set_pixel_color4f(rgba[0], rgba[1], rgba[2], rgba[3]);
    }

} // namespace pixel

#endif /*  PIXEL3F_HPP_ */
