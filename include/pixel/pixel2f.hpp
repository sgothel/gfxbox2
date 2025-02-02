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
#ifndef PIXEL2F_HPP_
#define PIXEL2F_HPP_

#include <string>
#include "pixel.hpp"

/**
 * 2D computer graphics math based upon two float components.
 */
namespace pixel::f2 {


    class vec_t {
    public:
        float x;
        float y;

        static constexpr vec_t from_length_angle(const float magnitude, const float radians) noexcept {
            return vec_t(magnitude * std::cos(radians), magnitude * std::sin(radians));
        }

        constexpr vec_t() noexcept
        : x(0), y(0) {}

        constexpr vec_t(const float x_, const float y_) noexcept
        : x(x_), y(y_) {}

        constexpr vec_t(const vec_t& o) noexcept = default;
        constexpr vec_t(vec_t&& o) noexcept = default;
        constexpr vec_t& operator=(const vec_t&) noexcept = default;
        constexpr vec_t& operator=(vec_t&&) noexcept = default;

        constexpr vec_t copy() noexcept { return vec_t(*this); }

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

        constexpr vec_t& set(const float dx, const float dy) noexcept
        { x=dx; y=dy; return *this; }

        constexpr vec_t& add(const float dx, const float dy) noexcept
        { x+=dx; y+=dy; return *this; }

        constexpr vec_t& operator+=(const vec_t& rhs ) noexcept {
            x+=rhs.x; y+=rhs.y;
            return *this;
        }

        constexpr vec_t& operator-=(const vec_t& rhs ) noexcept {
            x-=rhs.x; y-=rhs.y;
            return *this;
        }

        /**
         * Scale this vector with given scale factor
         * @param s scale factor
         * @return this instance
         */
        constexpr vec_t& operator*=(const float s ) noexcept {
            x*=s; y*=s;
            return *this;
        }

        /**
         * Divide this vector with given scale factor
         * @param s scale factor
         * @return this instance
         */
        constexpr vec_t& operator/=(const float s ) noexcept {
            x/=s; y/=s;
            return *this;
        }

        /** Rotates this vector in place, returns *this */
        vec_t& rotate(const float radians, const vec_t& ctr) noexcept {
            return rotate(std::sin(radians), std::cos(radians), ctr);
        }

        /** Rotates this vector in place, returns *this */
        constexpr vec_t& rotate(const float sin, const float cos, const vec_t& ctr) noexcept {
            const float x0 = x - ctr.x;
            const float y0 = y - ctr.y;
            x = x0 * cos - y0 * sin + ctr.x;
            y = x0 * sin + y0 * cos + ctr.y;
            return *this;
        }

        /** Rotates this vector in place, returns *this */
        vec_t& rotate(const float radians) noexcept {
            return rotate(std::sin(radians), std::cos(radians));
        }

        /** Rotates this vector in place, returns *this */
        constexpr vec_t& rotate(const float sin, const float cos) noexcept {
            const float x0 = x;
            x = x0 * cos - y * sin;
            y = x0 * sin + y * cos;
            return *this;
        }

        std::string toString() const noexcept { return std::to_string(x)+" / "+std::to_string(y); }

        constexpr bool is_zero() const noexcept {
            return jau::is_zero(x) && jau::is_zero(y);
        }

        /**
         * Return the squared length of this vector, a.k.a the squared <i>norm</i> or squared <i>magnitude</i>
         */
        constexpr float length_sq() const noexcept {
            return x*x + y*y;
        }

        /**
         * Return the length of this vector, a.k.a the <i>norm</i> or <i>magnitude</i>
         */
        constexpr float length() const noexcept {
            return std::sqrt(length_sq());
        }

        /**
         * Return the direction angle of this vector in radians
         */
        float angle() const noexcept {
            // Utilize atan2 taking y=sin(a) and x=cos(a), resulting in proper direction angle for all quadrants.
            return std::atan2( y, x );
        }

        /** Normalize this vector in place, returns *this */
        constexpr vec_t& normalize() noexcept {
            const float lengthSq = length_sq();
            if ( jau::is_zero( lengthSq ) ) {
                x = 0.0f;
                y = 0.0f;
            } else {
                const float invSqr = 1.0f / std::sqrt(lengthSq);
                x *= invSqr;
                y *= invSqr;
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
            return dx*dx + dy*dy;
        }

        /**
         * Return the distance between this vector and the given one.
         */
        constexpr float dist(const vec_t& o) const noexcept {
            return std::sqrt(dist_sq(o));
        }

        /**
         * Return the dot product of this vector and the given one
         *
         * u⋅v=∥u∥∥v∥cosθ
         *
         * @return the dot product as float
         */
        constexpr float dot(const vec_t& o) const noexcept {
            return x*o.x + y*o.y;
        }

        /**
         * Returns cross product of this vectors and the given one, i.e. *this x o.
         *
         * The 2D cross product is identical with the 2D perp dot product.
         *
         * @return the resulting scalar
         */
        constexpr float cross(const vec_t& o) const noexcept {
            return x * o.y - y * o.x;
        }

        /**
         * Return the cosines of the angle between two vectors
         */
        constexpr float cos_angle(const vec_t& o) const noexcept {
            return dot(o) / ( length() * o.length() ) ;
        }

        /**
         * Return the angle between two vectors in radians
         */
        float angle(const vec_t& o) const noexcept {
            return std::acos( cos_angle(o) );
        }

        /**
         * Return the counter-clock-wise (CCW) normal of this vector, i.e. perp(endicular) vector
         */
        vec_t normal_ccw() const noexcept {
            return vec_t(-y, x);
        }

        bool intersects(const vec_t& o) const noexcept {
            const float eps = std::numeric_limits<float>::epsilon();
            if( std::abs(x-o.x) >= eps || std::abs(y-o.y) >= eps ) {
                return false;
            }
            return true;
        }

        bool on_screen() const noexcept {
            // x in [min_x .. max_x] ?
            const int x_fb = cart_coord.to_fb_x( x );
            const int y_fb = cart_coord.to_fb_y( y );
            return 0 <= x_fb && x_fb <= pixel::fb_max_x &&
                    0 <= y_fb && y_fb <= pixel::fb_max_y;
        }

        void draw() const noexcept;
    };

    typedef vec_t point_t;

    /** Convert framebuffer coordinates in pixels to cartesian coordinates. */
    inline vec_t fb_to_cart(const int x, const int y) noexcept { return vec_t(cart_coord.from_fb_x(x), cart_coord.from_fb_y(y)); }

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

    constexpr vec_t operator-(const vec_t& lhs) noexcept {
        vec_t r(lhs);
        r *= -1;
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

    constexpr vec_t operator/(const vec_t& lhs, const float s ) noexcept {
        vec_t r(lhs);
        r /= s;
        return r;
    }

    constexpr bool operator==(const vec_t& lhs, const vec_t& rhs ) noexcept {
        return jau::is_zero(lhs.x - rhs.x) && jau::is_zero(lhs.x - rhs.x);
    }
    /** TODO
    constexpr bool operator<=>(const vec_t& lhs, const vec_t& rhs ) noexcept {
        return jau::is_zero(lhs.x - rhs.x) && jau::is_zero(lhs.x - rhs.x);
    } */

    constexpr vec_t min(const vec_t& lhs, const vec_t& rhs) noexcept {
        return vec_t(std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y));
    }
    constexpr vec_t max(const vec_t& lhs, const vec_t& rhs) noexcept {
        return vec_t(std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y));
    }

    inline std::ostream& operator<<(std::ostream& out, const vec_t& v) {
        return out << v.toString();
    }

    /**
     * Computes oriented double area of a triangle,
     * i.e. the 2x2 determinant with b-a and c-a per column.
     * <pre>
     *       | bx-ax, cx-ax |
     * det = | by-ay, cy-ay |
     * </pre>
     * @param a first vertex
     * @param b second vertex
     * @param c third vertex
     * @return area > 0 CCW, ..
     */
    constexpr double tri_area(const point_t& a, const point_t& b, const point_t& c){
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    /**
     * Return the orientation of the given point triplet a, b and c using triArea()
     */
    constexpr pixel::orientation_t orientation(const point_t& a, const point_t& b, const point_t& c) noexcept {
        const double area = tri_area(a, b, c);
        if ( jau::is_zero( area ) ) {
            return pixel::orientation_t::COL;
        }
        return ( area > 0.0f ) ? pixel::orientation_t::CCW : pixel::orientation_t::CLW;
    }

    class aabbox_t; // fwd
    class lineseg_t; // fwd

    /**
     * Geometric object
     */
    class geom_t {
    public:
        virtual ~geom_t() = default;

        virtual aabbox_t box() const noexcept = 0;
        virtual bool contains(const point_t& o) const noexcept = 0;
        virtual bool intersects(const lineseg_t & o) const noexcept = 0;
        virtual bool intersects(const aabbox_t& box) const noexcept = 0;
        virtual bool intersects(const geom_t& o) const noexcept = 0;

        /**
         * Return whether this object intersects with the given line segment
         * and if intersecting, the crossing point (intersection), the normalized normal of the crossing surface and the reflection out vector.
         */
        virtual bool intersection(vec_t& reflect_out, vec_t& cross_normal, point_t& cross_point, const lineseg_t& in) const noexcept = 0;

        virtual void draw() const noexcept = 0;
        virtual bool on_screen() const noexcept = 0;
        /** Returns whether this object is fully inside the given aabbox_t. */
        virtual bool inside(const aabbox_t& o) const noexcept = 0;

        virtual std::string toString() const noexcept = 0;
    };
    typedef std::shared_ptr<geom_t> geom_ref_t;
    typedef std::vector<geom_ref_t> geom_list_t;

    geom_list_t& gobjects();

    /**
     * Axis Aligned Bounding Box. Defined by two 3D coordinates (low and high)
     * The low being the the lower left corner of the box, and the high being the upper
     * right corner of the box.
     *
     * A few references for collision detection, intersections:
     * - http://www.realtimerendering.com/intersections.html
     * - http://www.codercorner.com/RayAABB.cpp
     * - http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtinter0.htm
     * - http://realtimecollisiondetection.net/files/levine_swept_sat.txt
     */
    class aabbox_t : public geom_t {
    public:
        /** bottom left (low) */
        point_t bl;
        /** top right (high) */
        point_t tr;

        /**
         * Create an Axis Aligned bounding box (AABBox)
         * where the low and and high MAX float Values.
         */
        constexpr aabbox_t() noexcept {
            reset();
        }

        /**
         * Create an AABBox with given bl (low) and tr (high)
         */
        constexpr aabbox_t(const point_t& bl_, const point_t& tr_) noexcept
        : bl( bl_ ), tr( tr_ ) {
        }

        constexpr ~aabbox_t() noexcept override = default; // gcc bug 93413 (constexpr, solved in gcc 13)

        constexpr aabbox_t(const aabbox_t& o) noexcept = default;
        constexpr aabbox_t(aabbox_t&& o) noexcept = default;
        constexpr aabbox_t& operator=(const aabbox_t&) noexcept = default;
        constexpr aabbox_t& operator=(aabbox_t&&) noexcept = default;

        /**
         * Reset this box to the inverse low/high, allowing the next {@link #resize(float, float, float)} command to hit.
         * @return this AABBox for chaining
         */
        constexpr aabbox_t& reset() noexcept {
            bl.x = std::numeric_limits<float>::max();
            bl.y = std::numeric_limits<float>::max();
            tr.x = -std::numeric_limits<float>::max();
            tr.y = -std::numeric_limits<float>::max();
            return *this;
        }

        constexpr point_t center() const noexcept { return (bl + tr)/2; }

        /**
         * Resize the AABBox to encapsulate another AABox
         * @param newBox AABBox to be encapsulated in
         * @return this AABBox for chaining
         */
        constexpr aabbox_t& resize(const aabbox_t& o) noexcept {
            /** test bl (low) */
            if (o.bl.x < bl.x) {
                bl.x = o.bl.x;
            }
            if (o.bl.y < bl.y) {
                bl.y = o.bl.y;
            }

            /** test tr (high) */
            if (o.tr.x > tr.x) {
                tr.x = o.tr.x;
            }
            if (o.tr.y > tr.y) {
                tr.y = o.tr.y;
            }
            return *this;
        }

        /**
         * Resize the AABBox to encapsulate the passed coordinates.
         * @param x x-axis coordinate value
         * @param y y-axis coordinate value
         * @return this AABBox for chaining
         */
        constexpr aabbox_t& resize(float x, float y) noexcept {
            /** test bl (low) */
            if (x < bl.x) {
                bl.x = x;
            }
            if (y < bl.y) {
                bl.y = y;
            }

            /** test tr (high) */
            if (x > tr.x) {
                tr.x = x;
            }
            if (y > tr.y) {
                tr.y = y;
            }
            return *this;
        }

        /**
         * Resize the AABBox to encapsulate the passed point.
         * @param x x-axis coordinate value
         * @param y y-axis coordinate value
         * @return this AABBox for chaining
         */
        constexpr aabbox_t& resize(const point_t& p) noexcept {
            return resize(p.x, p.y);
        }

        constexpr aabbox_t box() const noexcept override { return *this; }

        constexpr float width() const noexcept { return tr.x - bl.x; }
        constexpr float height() const noexcept { return tr.y - bl.y; }

        /**
         * Check if the point is bounded/contained by this AABBox
         * @return true if  x belong to (low.x, high.x) and y belong to (low.y, high.y)
         */
        bool contains(const point_t& p) const noexcept override {
            return !( p.x<bl.x || p.x>tr.x ||
                      p.y<bl.y || p.y>tr.y );
        }

        /** Returns whether this object is fully inside the given aabbox_t. */
        bool inside(const aabbox_t& o) const noexcept override {
            return o.tr.x >= tr.x &&
                   o.tr.y >= tr.y &&
                   o.bl.x <= bl.x &&
                   o.bl.y <= bl.y;
        }

        bool intersects(const lineseg_t & o) const noexcept override;

        bool intersects(const aabbox_t& o) const noexcept override {
            /**
             * Traditional boolean equation leads to multiple branches,
             * using max/min approach allowing for branch-less optimizations.
             *
                return !( tr.x < o.bl.x ||
                          tr.y < o.bl.y ||
                          bl.x > o.tr.x ||
                          bl.y > o.tr.y );
             */
            const point_t lo = max(bl, o.bl);
            const point_t hi = min(tr, o.tr);
            return lo.x <= hi.x && lo.y <= hi.y;
        }

#if 0
        bool intersection(aabbox_t a, aabbox_t b){
            if(aabbox_t::intersects(a)){
                return false;
            }

        }
#endif

        bool intersects(const geom_t& o) const noexcept override {
            return intersects(o.box());
        }

        /// Returns intersecting aabbox, maybe of width() and height() zero if not intersecting
        constexpr aabbox_t intersection(const aabbox_t& o) const noexcept {
            aabbox_t res( /*bl=*/ max(bl, o.bl), /*tr=*/ min(tr, o.tr) );
            if (res.bl.x > res.tr.x || res.bl.y > res.tr.y) {
                res.bl = { 0, 0 };
                res.tr = { 0, 0 };
            }
            return res;;
        }

        bool intersection(vec_t& reflect_out, vec_t& cross_normal, point_t& cross_point, const lineseg_t& in) const noexcept override;

        bool on_screen() const noexcept override {
            const int x0 = pixel::cart_coord.to_fb_x( bl.x );
            const int y0 = pixel::cart_coord.to_fb_y( tr.y );
            const int x1 = pixel::cart_coord.to_fb_x( tr.x );
            const int y1 = pixel::cart_coord.to_fb_y( bl.y );

            // x in [min_x .. max_x] ?
            return 0 <= x0 && x0 <= pixel::fb_max_x &&
                   0 <= y0 && y0 <= pixel::fb_max_y &&
                   0 <= x1 && x1 <= pixel::fb_max_x &&
                   0 <= y1 && y1 <= pixel::fb_max_y;
        }

        void draw() const noexcept override;

        std::string toString() const noexcept override {
            return "aabb[bl " + bl.toString() +
                    ", tr " + tr.toString() +
                    "]"; }
    };

    class lineseg_t : public geom_t {
    public:
        typedef std::function<bool(const point_t& p)> point_action_t;

        point_t p0;
        point_t p1;

        constexpr lineseg_t() noexcept
        : p0(), p1() {}

        constexpr lineseg_t(const point_t& p0_, const point_t& p1_) noexcept
        : p0( p0_ ), p1( p1_ ) {}

        /**
         * Scale this line segment with given scale factor
         * @param s scale factor
         * @return this instance
         */
        constexpr lineseg_t& operator*=(const float s ) noexcept {
            p0 *= s;
            p1 *= s;
            return *this;
        }

        /**
         * Return the length of this line segment, i.e. distance between both points.
         */
        constexpr float length() const noexcept {
            return p1.dist(p0);
        }

        /**
         * Return the angle of this line segment in radians
         */
        float angle() const noexcept {
            return (p1 - p0).angle();
        }

        /**
         * Return the angle between two line segments in radians
         */
        float angle(const lineseg_t & o) const noexcept {
            const vec_t a = p1 - p0;
            const vec_t b = o.p1 - o.p0;
            return a.angle(b);
        }

        void rotate(const float radians) noexcept {
            p1.rotate(radians);
        }

        void rotate(const float radians, point_t p) noexcept {
            p1.rotate(radians);
            p1.rotate(radians, p);
            p0.rotate(radians, p);
        }

        void add(float length) {
            // extend center points p0, p1 with radius in moving direction
            const float a_move = angle();
            vec_t l_move_diff = pixel::f2::vec_t::from_length_angle(length, a_move);
            p0 -= l_move_diff;
            p1 += l_move_diff;
        }

        std::string toString() const noexcept override { return "L[" + p0.toString() + ", " + p1.toString() + "]"; }

        bool on_screen() const noexcept override {
            return p0.on_screen() && p1.on_screen();
        }

        static void for_all_points(const point_t& p0, const point_t& p1,
                const point_action_t& point_action) noexcept
        {
            const float dx = p1.x - p0.x;
            const float dy = p1.y - p0.y;
            const float dx_abs = std::abs(dx);
            const float dy_abs = std::abs(dy);
            if( dy_abs > dx_abs ) {
                const float y_ival = pixel::cart_coord.height() / (float)pixel::fb_height;
                const float step_y = ( dy >= 0 ) ? y_ival : -y_ival;
                const float step_x = dx / dy_abs * y_ival;
                float sy=0.0f;
                float sx=0.0f;
                for(; std::abs(dy - sy) > y_ival/2.0f; sy+=step_y, sx+=step_x) {
                    const point_t p { p0.x + sx, p0.y + sy };
                    if( !point_action(p) ) {
                        return;
                    }
                }
            } else if( !jau::is_zero(dx_abs) ) {
                const float x_ival = pixel::cart_coord.width() / (float)pixel::fb_width;
                const float step_x = ( dx >= 0 ) ? x_ival : -x_ival;
                const float step_y = dy / dx_abs * x_ival;
                float sx=0.0f;
                float sy=0.0f;
                for(; std::abs(dx - sx) > x_ival/2.0f; sx+=step_x, sy+=step_y) {
                    point_t p { p0.x + sx, p0.y + sy };
                    if( !point_action(p) ) {
                        return;
                    }
                }
            }
        }
        static void draw(const point_t& p0, const point_t& p1) noexcept;

        void draw() const noexcept override {
            draw(p0, p1);
        }

        /**
         * Create an AABBox with given lineseg
         */
        aabbox_t box() const noexcept override {
            return aabbox_t().resize(p0).resize(p1);
        }

    private:
        bool is_on_line(const point_t& p2) const noexcept {
            // Using the perp dot product (PDP),
            // which is the area of the parallelogram of the three points,
            // same as the area of the triangle defined by the three points, multiplied by 2.
            const float perpDotProduct = (p0.x - p2.x) * (p1.y - p2.y) - (p0.y - p2.y) * (p1.x - p2.x);
            return jau::is_zero( perpDotProduct );

        }
        bool is_on_line2(const point_t& p2) const noexcept {
            if ( p2.x <= std::max(p0.x, p1.x) && p2.x >= std::min (p0.x, p1.x) &&
                    p2.y <= std::max(p0.y, p2.y) && p2.y >= std::min (p0.y, p1.y) )
            {
                return true;
            }
            return false;
        }

    public:
        /**
         * Test intersection between this line segment and the give point
         * @return true if the line segment contains the point, otherwise false
         */
        bool contains(const point_t& p2) const noexcept override {
            if ( !( ( p0.x <= p2.x && p2.x <= p1.x ) || ( p1.x <= p2.x && p2.x <= p0.x ) ) ) {
                // not in x-range
                return false;
            }
            if ( !( ( p0.y <= p2.y && p2.y <= p1.y ) || ( p1.y <= p2.y && p2.y <= p0.y ) ) ) {
                // not in y-range
                return false;
            }
            return is_on_line(p2);
        }

        /** Returns whether this object is fully inside the given aabbox_t. */
        bool inside(const aabbox_t& o) const noexcept override {
            return box().inside(o);
        }

    private:
        /**
         * See [p + t r = q + u s](https://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect/565282#565282)
         * and [its terse C# implementation](https://www.codeproject.com/tips/862988/find-the-intersection-point-of-two-line-segments)
         */
        static bool intersects(point_t& result,
                const point_t& p, const point_t& p2,
                const point_t& q, const point_t& q2, const bool do_collinear=false)
        {
            // Operations: 11+, 8*, 2 branches without collinear case
            constexpr const float eps = std::numeric_limits<float>::epsilon();
            const vec_t r = p2 - p;
            const vec_t s = q2 - q;
            const float rxs = r.cross(s);

            if ( jau::is_zero(rxs) ) {
                if ( do_collinear ) {
                    const vec_t q_p = q - p;
                    const float qpxr = q_p.cross(r);
                    if ( jau::is_zero(qpxr) ) // disabled collinear case
                    {
                        // 1) r x s = 0 and (q - p) x r = 0, the two lines are collinear.

                        const point_t p_q = p - q;
                        const float qp_dot_r = q_p.dot(r);
                        const float pq_dot_s = p_q.dot(s);
                        // if ( ( 0 <= qp_dot_r && qp_dot_r <= r.dot(r) ) ||
                        //      ( 0 <= pq_dot_s && pq_dot_s <= s.dot(s) ) )
                        if ( ( eps <= qp_dot_r && qp_dot_r - r.dot(r) <= eps ) ||
                             ( eps <= pq_dot_s && pq_dot_s - s.dot(s) <= eps ) )
                        {
                            // 1.1) 0 <= (q - p) · r <= r · r or 0 <= (p - q) · s <= s · s, the two lines are overlapping
                            // FIXME: result set to q2 endpoint, OK?
                            result = q2;
                            return true;
                        }

                        // 1.2 the two lines are collinear but disjoint.
                        return false;
                    } else {
                        // 2) r × s = 0 and (q − p) × r ≠ 0, the two lines are parallel and non-intersecting.
                        return false;
                    }
                } else {
                    // Not considering collinear case as an intersection
                    return false;
                }
            } else {
                // r x s != 0
                const vec_t q_p = q - p;
                const float qpxr = q_p.cross(r);

                // p + t r = q + u s
                // (p + t r) × s = (q + u s) × s
                // t (r × s) = (q − p) × s, with s x s = 0
                // t = (q - p) x s / (r x s)
                const float t = q_p.cross(s) / rxs;

                // u = (p − q) × r / (s × r) = (q - p) x r / (r x s), with s × r = − r × s
                const float u = qpxr / rxs;

                // if ( (0 <= t && t <= 1) && (0 <= u && u <= 1) )
                if ( (eps <= t && t - 1 <= eps) && (eps <= u && u - 1 <= eps) )
                {
                    // 3) r × s ≠ 0 and 0 ≤ t ≤ 1 and 0 ≤ u ≤ 1, the two line segments meet at the point p + t * r = q + u * s.
                    result = p + (t * r); // == q + (u * s)
                    return true;
                }
            }

            return false;
        }

    public:

        /**
         * Compute intersection between two lines segments
         * @param result storage for the intersection coordinates if the lines intersect, otherwise unchanged
         * @param o the other line segment.
         * @return true if the line segments intersect, otherwise false
         */
        bool intersects(point_t& result, const lineseg_t & o) const noexcept {
            return intersects(result, p0, p1, o.p0, o.p1);
        }

        /**
         * Return true if this line segment intersect with the given line segment
         * @param o the other line segment.
         * @return true if both intersect, otherwise false
         */
        bool intersects(const lineseg_t & o) const noexcept override {
            point_t result;
            return intersects(result, p0, p1, o.p0, o.p1);
#if 0
            const pixel::orientation_t or_1 = orientation (p0, p1, o.p0);
            const pixel::orientation_t or_2 = orientation (p0, p1, o.p1);
            const pixel::orientation_t or_3 = orientation(o.p0, o.p1, p0);
            const pixel::orientation_t or_4 = orientation(o.p0, o.p1, p1);

            // General case : Points are non-collinear.
            if (or_1 != or_2 && or_3 != or_4) {
                pixel::log_printf("i-g-0: %s with %s\n", toString().c_str(), o.toString().c_str());
                return true;
            }

#if 0
            // Special case : Points are collinear.

            // If points p0, p1 and o.p0 are collinear, check if point o.p0 lies on this segment p0-p1
            if (or_1 == pixel::orientation_t::COL && is_on_line (o.p0)) {
                pixel::log_printf("i-s-1: %s with %s\n", toString().c_str(), o.toString().c_str());
                return true;
            }

            // If points p0, p1 and o.p1 are collinear, check if point o.p1 lies on this segment p0-p1
            if (or_2 == pixel::orientation_t::COL && is_on_line (o.p1)) {
                pixel::log_printf("i-s-2: %s with %s\n", toString().c_str(), o.toString().c_str());
                return true;
            }

            // If points o.p0, o.p1 and p0 are collinear, check if point p0 lies on given segment o.p0-o.p1
            if (or_3 == pixel::orientation_t::COL && o.is_on_line (p0)) {
                pixel::log_printf("i-s-3: %s with %s\n", toString().c_str(), o.toString().c_str());
                return true;
            }

            // If points o.p0, o.p1 and p1 are collinear, check if point p1 lies on given segment o.p0-o.p1
            if (or_4 == pixel::orientation_t::COL && o.is_on_line (p1)) {
                pixel::log_printf("i-s-4: %s with %s\n", toString().c_str(), o.toString().c_str());
                return true;
            }
#endif

            return false;
#endif
        }

        /**
         * Returns minimum distance between this line segment and given point p
         * <p>
         * See [Shortest distance between a point and a line segment](https://stackoverflow.com/a/1501725)
         * </p>
         * <p>
         * Slightly more expensive than intersects().
         * </p>
         */
        float distance(point_t p) const noexcept {
            // Operations: 15+, 9*, 1-sqrt, 3 branches
            const float l2 = p1.dist_sq(p0); // i.e. |p1-p0|^2 -  avoid a sqrt
            if( l2 < std::numeric_limits<float>::epsilon() ) {
                return p.dist(p1);   // p1 == p0 case
            }
            // Consider the line extending the segment, parameterized as p0 + t (p1 - p0).
            // We find projection of point p onto the line.
            // It falls where t = [(p-p0) . (p1-p0)] / |p1-p0|^2
            // We clamp t from [0,1] to handle points outside the line segment.
            vec_t pv = p - p0;
            vec_t wv = p1 - p0;
            const float t = std::max(0.0f, std::min(1.0f, pv.dot(wv) / l2));
            const vec_t projection = p0 + t * (p1 - p0);  // Projection falls on the segment
            return p.dist(projection);
        }

        bool intersects(const aabbox_t& box) const noexcept override {
            // separating axis theorem.
            const vec_t d = (p1 - p0) * 0.5f; // half lineseg direction
            const vec_t e = (box.tr - box.bl) * 0.5f;
            const vec_t aabb_center = (box.bl + box.tr) * 0.5f;
            const vec_t lseg_center = p0 + d;
            const vec_t c = lseg_center - aabb_center;
            const vec_t ad(std::abs(d.x), std::abs(d.y));
            if (std::abs(c.x) > e.x + ad.x) {
                return false;
            }
            if (std::abs(c.y) > e.y + ad.y) {
                return false;
            }
            /**
                if (std::abs(d.y * c.z - d.z * c.y) > e.y * ad.z + e.z * ad.y + std::numeric_limits<float>::epsilon()) {
                    return false;
                }
                if (std::abs(d.z * c.x - d.x * c.z) > e.z * ad.x + e.x * ad.z + std::numeric_limits<float>::epsilon()) {
                    return false;
                }
             */
            if (std::abs(d.x * c.y - d.y * c.x) > e.x * ad.y + e.y * ad.x + std::numeric_limits<float>::epsilon()) {
                return false;
            }
            return true;
        }

        bool intersects(const geom_t& o) const noexcept override {
            return intersects(o.box());
        }

        bool intersection(vec_t& reflect_out, vec_t& cross_normal, point_t& cross_point, const lineseg_t& in) const noexcept override {
            if( intersects(cross_point, in) ) {
                cross_normal = (p1 - p0).normal_ccw().normalize();
                const vec_t v_in = cross_point - in.p0;
                reflect_out = v_in - ( 2.0f * v_in.dot(cross_normal) * cross_normal );
                return true;
            }
            return false;
        }
    };
    typedef std::shared_ptr<lineseg_t> lineseg_ref_t;

    /**
     * Simple compound denoting a ray.
     * <p>
     * A ray, also known as a half line, consists out of it's <i>origin</i>
     * and <i>direction</i>. Hence it is bound to only the <i>origin</i> side,
     * where the other end is +infinitive.
     * <pre>
     * R(t) = R0 + Rd * t with R0 origin, Rd direction and t > 0.0
     * </pre>
     * </p>
     */
    class ray_t /** TODO : public geom_t **/ {
    public:
        /** Origin of Ray. */
        point_t orig;

        /** Normalized direction vector of ray. */
        vec_t dir;

        std::string toString() const noexcept { return "Ray[orig "+orig.toString()+", dir "+dir.toString() +"]"; }
    };

    /**
     * Animated geometric object
     */
    class ageom_t : public geom_t {
    public:
        ~ageom_t() override = default;

        virtual void rotate(const float rad) noexcept = 0;
        virtual void move_dir(const float d) noexcept = 0;
        virtual void move(const point_t& d) noexcept = 0;
        virtual void move(const float dx, const float dy) noexcept = 0;
        virtual bool tick(const float dt) noexcept { (void)dt; return true; }
    };
    typedef std::shared_ptr<ageom_t> ageom_ref_t;
    typedef std::vector<ageom_ref_t> ageom_list_t;

    ageom_list_t& agobjects();

    class triangle_t : public ageom_t {
    public:
        /**
         * Unrotated, clockwise (CW):
         *
         *       (a)
         *       / \
         *      /   \
         *     /     \
         *   (b)-----(c)
         */
        /** Unrotated top */
        point_t p_a;
        /** Unrotated bottom-left */
        point_t p_b;
        /** Unrotated bottom-right */
        point_t p_c;
        point_t p_center;
        /** direction angle in radians */
        float dir_angle;

    public:
        triangle_t() noexcept
        : p_a(), p_b(), p_c(), p_center(){}

        triangle_t(const point_t& a_, const point_t& b_, const point_t& c_) noexcept
        : p_a(a_), p_b(b_), p_c(c_)
        {
            // FIXME ???
            p_center = (p_a + p_b + p_c) / 3.0f;//{ (p_c.x + p_b.x) / 2.0f, (p_a.y + p_b.y) / 2.0f };
            dir_angle = 0.0f;
        }

        aabbox_t box() const noexcept override {
            return aabbox_t().resize(p_a).resize(p_b).resize(p_c);
        }

        void move_dir(const float d) noexcept override {
            point_t dir { d, 0 };
            dir.rotate(dir_angle);
            p_a += dir;
            p_b += dir;
            p_c += dir;
            p_center += dir;
        }

        void move(const point_t& d) noexcept override {
            p_a += d;
            p_b += d;
            p_c += d;
            p_center += d;
        }
        void move(const float dx, const float dy) noexcept override {
            p_a.add(dx, dy);
            p_b.add(dx, dy);
            p_c.add(dx, dy);
            p_center.add(dx, dy);
        }

        void rotate(const float radians) noexcept override {
            const float cos = std::cos(radians);
            const float sin = std::sin(radians);
            p_a.rotate(sin, cos, p_center);
            p_b.rotate(sin, cos, p_center);
            p_c.rotate(sin, cos, p_center);
            p_center = (p_a + p_b + p_c) / 3.0f;
            dir_angle += radians;
        }

        void rotate(const float radians, const point_t p) noexcept {
            const float cos = std::cos(radians);
            const float sin = std::sin(radians);
            p_a.rotate(sin, cos, p);
            p_b.rotate(sin, cos, p);
            p_c.rotate(sin, cos, p);
            p_center = (p_a + p_b + p_c) / 3.0f;
            dir_angle += radians;
        }

        bool on_screen() const noexcept override {
            return box().on_screen();
        }

        float area() const noexcept { return area(p_a, p_b, p_c); }

        static float area(const point_t &a, const point_t &b, const point_t &c) noexcept {
            if( false ) {
                vec_t v_ba = b - a;
                vec_t v_ca = c - a;
                return std::abs( v_ba.cross(v_ca) ) * 0.5f;
            } else {
                return std::abs( 0.5f * ( (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x) ) );
            }
        }

        bool contains(const point_t& o) const noexcept override {
            if( true ) {
                return std::numeric_limits<float>::epsilon() >
                std::abs(area(p_a, p_b, p_c) -
                        ( area(p_a, p_b, o) +
                                area(o,   p_b, p_c) +
                                area(p_a, o,   p_c)
                        ) );
            } else {
                return 0 == jau::compare(area(p_a, p_b, p_c),
                        area(p_a, p_b, o) +
                        area(o,   p_b, p_c) +
                        area(p_a, o,   p_c));
            }
        }

        /** Returns whether this object is fully inside the given aabbox_t. */
        bool inside(const aabbox_t& o) const noexcept override {
            return box().inside(o);
        }

        bool intersects(const lineseg_t & o) const noexcept override {
            return o.intersects(box());
        }

        bool intersects(const aabbox_t& o) const noexcept override {
            return box().intersects(o);
        }

        bool intersects(const geom_t& o) const noexcept override {
            return box().intersects(o.box());
        }

        bool intersection(vec_t& reflect_out, vec_t& cross_normal, point_t& cross_point, const lineseg_t& in) const noexcept override {
            {
                // tl .. tr
                const lineseg_t l(p_a, p_b);
                if( l.intersection(reflect_out, cross_normal, cross_point, in) ) {
                    return true;
                }
            }
            {
                // bl .. br
                const lineseg_t l(p_b, p_c);
                if( l.intersection(reflect_out, cross_normal, cross_point, in) ) {
                    return true;
                }
            }
            {
                // br .. tr
                const lineseg_t l(p_c, p_a);
                if( l.intersection(reflect_out, cross_normal, cross_point, in) ) {
                    return true;
                }
            }
            return false;
        }

        bool intersection(vec_t& reflect_out, vec_t& cross_normal, point_t& cross_point, const lineseg_t& in,
                          const float in_radius) const noexcept {
            {
                // tl .. tr
                lineseg_t l(p_a, p_b);
                const vec_t added_size = (l.p1 - l.p0).normal_ccw().normalize() * in_radius;
                l.p0 += added_size;
                l.p1 += added_size;
                if( l.intersection(reflect_out, cross_normal, cross_point, in) ) {
                    return true;
                }
            }
            {
                // bl .. br
                lineseg_t l(p_b, p_c);
                const vec_t added_size = (l.p1 - l.p0).normal_ccw().normalize() * in_radius;
                l.p0 += added_size;
                l.p1 += added_size;
                if( l.intersection(reflect_out, cross_normal, cross_point, in) ) {
                    return true;
                }
            }
            {
                // br .. tr
                lineseg_t l(p_c, p_a);
                const vec_t added_size = (l.p1 - l.p0).normal_ccw().normalize() * in_radius;
                l.p0 += added_size;
                l.p1 += added_size;
                if( l.intersection(reflect_out, cross_normal, cross_point, in) ) {
                    return true;
                }
            }
            return false;
        }

        void draw() const noexcept override {
            draw(false);
        }

        void draw(const bool filled) const noexcept;

        std::string toString() const noexcept override {
            return "tri[a " + p_a.toString() +
                    ", b " + p_b.toString() +
                    ", c " + p_c.toString() +
                    "]"; }
    };

    typedef std::shared_ptr<triangle_t> triangle_ref_t;


    class disk_t : public ageom_t {
    public:
        /**
         * Imagine a circle ;-)
         *
         *     ---------
         *    |    |r   |
         *    |    |    |
         *    |    c    |
         *    |         |
         *     ---------
         */
        /** m_center */
        point_t center;
        float radius;
        float thickness = 0;
        /** direction angle in radians */
        float dir_angle;

        disk_t()
        : center(), radius(), dir_angle() {}

        disk_t(const point_t& c_, const float r_)
        : center( c_), radius(r_), dir_angle(0.0f) {}

        disk_t(const point_t& c_, const float r_, const float thickness_)
        : center( c_), radius(r_), thickness(thickness_), dir_angle(0.0f) {}

        disk_t(const point_t& c_, const point_t& p_)
        : center( c_), radius((c_ - p_).length()), dir_angle(0.0f) {}

        disk_t(float x, float y, const float r_)
        : center(x, y), radius(r_), dir_angle(0.0f) {}

        ~disk_t() override = default;

        std::string toString() const noexcept override {
            return "disk[c " + center.toString() +
                    ", r " + std::to_string(radius) +
                    "]"; }

        void set_center(const point_t& p) {
            center = p;
        }

        aabbox_t box() const noexcept override {
            point_t bl = { center.x - radius, center.y - radius};
            point_t tr = { center.x + radius, center.y + radius};
            return aabbox_t(bl, tr);
        }

        bool contains(const point_t& o) const noexcept override {
            return center.dist(o) <= radius;
            // return box().contains(o);
        }

        /** Returns whether this object is fully inside the given aabbox_t. */
        bool inside(const aabbox_t& o) const noexcept override {
            return box().inside(o);
        }

        bool intersects(const lineseg_t& line ) const noexcept override {
            /** Buggy
            const float min_distance = triangle_t::area(center, line.p0, line.p1) /
                                       (line.p1 - line.p0).length();
            return min_distance <= radius;
            */
            return line.intersects( box() );
        }

        bool intersects(const aabbox_t& o) const noexcept override {
            return box().intersects(o);
        }

        bool intersects(const geom_t& o) const noexcept override {
            return box().intersects(o.box());
        }

        bool intersection(vec_t& reflect_out, vec_t& cross_normal, point_t& cross_point, const lineseg_t& in) const noexcept override {
            if( !intersects(in) ) {
                return false;
            }
            cross_point = center; // use center
            const vec_t v_in = in.p1 - in.p0;
            cross_normal = v_in.normal_ccw().normalize();
            // reflect_out = v_in - ( 2.0f * v_in.dot(cross_normal) * cross_normal );
            // TODO: check if cross_normal is OK for this case
            reflect_out = -1.0f * v_in;
            return true;
        }

        void draw() const noexcept override {
            draw(true);
        }
        void draw(const bool filled) const noexcept;

        void draw_with_thickness(){
            const float r1 = radius;
            const float r2 = radius - thickness;
            const float x_ival = pixel::cart_coord.width() / (float)pixel::fb_width;
            const float y_ival = pixel::cart_coord.height() / (float)pixel::fb_height;
            // const float ival2 = 1.0f*std::min(x_ival, y_ival);
            const aabbox_t b = box();
            for(float y=b.bl.y; y<=b.tr.y; y+=y_ival) {
                for(float x=b.bl.x; x<=b.tr.x; x+=x_ival) {
                    const point_t p { x, y };
                    const float cp = center.dist(p);
                    if( cp >= r2 && cp <= r1 ) {
                        p.draw();
                    }
                }
            }
        }
        bool on_screen() const noexcept override {
            return box().on_screen();
        }

        void rotate(const float rad) noexcept override {
            dir_angle += rad;
        }

        void rotate(const float radians, const point_t& p) noexcept {
            const float cos = std::cos(radians);
            const float sin = std::sin(radians);
            center.rotate(sin, cos, p);
            dir_angle += radians;
        }

        void move_dir(const float d) noexcept override {
            point_t dir { d, 0 };
            dir.rotate(dir_angle);
            center += dir;
        }

        void move(const point_t& d) noexcept override {
            center += d;
        }
        void move(const float dx, const float dy) noexcept override {
            center.add(dx, dy);
        }
    };
    typedef std::shared_ptr<disk_t> disk_ref_t;

    class rect_t : public ageom_t {
    public:
        /**
         * Unrotated, clockwise (CW):
         *
         *   (a)-----(b)
         *    |       |
         *    |       |
         *    |       |
         *   (c)-----(d)
         */
        /** Unrotated top-left */
        point_t m_tl;
        /** Unrotated top-right */
        point_t m_tr;
        /** Unrotated bottom-left */
        point_t m_bl;
        /** Unrotated bottom-right */
        point_t m_br;
        /** Unrotated center */
        point_t p_center;
        /** direction angle in radians */
        float dir_angle;

    public:
        rect_t()
        : m_tl(), m_tr(), m_bl(), m_br(){}

        rect_t(const point_t& tl_, const float width, const float height, const float radians) noexcept
        {
            m_tl = tl_;
            m_tr = { m_tl.x + width, m_tl.y };
            m_bl = { m_tl.x        , m_tl.y - height};
            m_br = { m_tl.x + width, m_tl.y - height};
            p_center = { m_tl.x + width/2  , m_tl.y - height/2  };
            dir_angle = 0.0f;
            rotate(radians);
        }

        rect_t(const point_t& tl_, const float width, const float height) noexcept{
            m_tl = tl_;
            m_tr = { m_tl.x + width, m_tl.y };
            m_bl = { m_tl.x        , m_tl.y - height};
            m_br = { m_tl.x + width, m_tl.y - height};
            p_center = { m_tl.x + width/2  , m_tl.y - height/2  };
            dir_angle = 0.0f;
        }


        rect_t(const point_t& tl_, const point_t& tr_, const point_t& bl_, const point_t& br_) noexcept
        : m_tl(tl_), m_tr(tr_), m_bl(bl_), m_br(br_)
        {
            p_center = { ( m_tl.x + m_tr.x ) / 2.0f  , ( m_tl.y + m_bl.y ) / 2.0f  };
            dir_angle = 0.0f;
        }

        bool operator==(const rect_t &o) const {
            return m_tl == o.m_tl &&
                   m_tr == o.m_tr &&
                   m_bl == o.m_bl &&
                   m_br == o.m_br;
        }

        aabbox_t box() const noexcept override {
            return aabbox_t().resize(m_tl).resize(m_tr).resize(m_bl).resize(m_br);
        }

        void move_dir(const float d) noexcept override {
            point_t dir { d, 0 };
            dir.rotate(dir_angle);
            m_tl += dir;
            m_tr += dir;
            m_bl += dir;
            m_br += dir;
            p_center += dir;
        }

        void move(const point_t& d) noexcept override {
            m_tl += d;
            m_tr += d;
            m_bl += d;
            m_br += d;
            p_center += d;
        }
        void move(const float dx, const float dy) noexcept override {
            m_tl.add(dx, dy);
            m_tr.add(dx, dy);
            m_bl.add(dx, dy);
            m_br.add(dx, dy);
            p_center.add(dx, dy);
        }

        void rotate(const float radians) noexcept override {
            rotate(radians, p_center);
        }
        void rotate(const float radians, const point_t& p) noexcept {
            const float cos = std::cos(radians);
            const float sin = std::sin(radians);
            m_tl.rotate(sin, cos, p);
            m_tr.rotate(sin, cos, p);
            m_bl.rotate(sin, cos, p);
            m_br.rotate(sin, cos, p);
            dir_angle += radians;
        }

        void set_top_left(const point_t& p) {
            // FIXME: Since m_p_a is unknown to be top-left ...
            const float dx = p.x - m_tl.x;
            const float dy = p.y - m_tl.y;
            move( dx, dy );
        }

        bool on_screen() const noexcept override {
            return box().on_screen();
        }

        bool contains(const point_t& o) const noexcept override {
            return box().contains(o);
        }

        /** Returns whether this object is fully inside the given aabbox_t. */
        bool inside(const aabbox_t& o) const noexcept override {
            return box().inside(o);
        }

        bool intersects(const lineseg_t & o) const noexcept override {
            return o.intersects(box());
        }

        bool intersects(const aabbox_t& o) const noexcept override {
            return box().intersects(o);
        }

        bool intersects(const geom_t& o) const noexcept override {
            return box().intersects(o.box());
        }

        bool intersection(vec_t& reflect_out, vec_t& cross_normal, point_t& cross_point, const lineseg_t& in) const noexcept override {
            {
                const lineseg_t lt(m_tl, m_tr);
                const lineseg_t lb(m_bl, m_br);
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
                const lineseg_t lr(m_br, m_tr);
                const lineseg_t ll(m_bl, m_tl);
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

        bool intersection(vec_t& reflect_out, vec_t& cross_normal, point_t& cross_point, const lineseg_t& in, const float in_radius) const noexcept {
            {
                // tl .. tr
                lineseg_t l(m_tl, m_tr);
                const vec_t added_size = (l.p1 - l.p0).normal_ccw().normalize() * in_radius;
                l.p0 += added_size;
                l.p1 += added_size;
                if( l.intersection(reflect_out, cross_normal, cross_point, in) ) {
                    return true;
                }
            }
            {
                // bl .. br
                lineseg_t l(m_bl, m_br);
                const vec_t added_size = (l.p1 - l.p0).normal_ccw().normalize() * in_radius;
                l.p0 += added_size;
                l.p1 += added_size;
                if( l.intersection(reflect_out, cross_normal, cross_point, in) ) {
                    return true;
                }
            }
            {
                // br .. tr
                lineseg_t l(m_br, m_tr);
                const vec_t added_size = (l.p1 - l.p0).normal_ccw().normalize() * in_radius;
                l.p0 += added_size;
                l.p1 += added_size;
                if( l.intersection(reflect_out, cross_normal, cross_point, in) ) {
                    return true;
                }
            }
            {
                // bl .. tl
                lineseg_t l(m_bl, m_tl);
                const vec_t added_size = (l.p1 - l.p0).normal_ccw().normalize() * in_radius;
                l.p0 += added_size;
                l.p1 += added_size;
                if( l.intersection(reflect_out, cross_normal, cross_point, in) ) {
                    return true;
                }
            }
            return false;
        }

        void draw() const noexcept override {
            draw(false);
        }
        void draw(const bool filled) const noexcept;

        std::string toString() const noexcept override {
            return "rect[a " + m_tl.toString() +
                    ", b " + m_tr.toString() +
                    ", c " + m_bl.toString() +
                    ", d " + m_br.toString() +
                    "]";
        }
    };
    typedef std::shared_ptr<rect_t> rect_ref_t;

    /**
     * A clockwise (CW) polyline
     */
    class linestrip_t : public ageom_t {
    public:
        std::vector<point_t> p_list;
        point_t p_center;
        /** direction angle in radians */
        float dir_angle;

    public:
        linestrip_t() noexcept
        : p_list(), p_center(), dir_angle(0.0f) {
        }

        linestrip_t(const point_t& center, const float angle) noexcept
        : p_list(), p_center(center), dir_angle(angle) {
        }

        void normalize_center() noexcept {
            point_t c;
            int n = 0;
            for(size_t i=0; i<p_list.size()-1; ++i) {
                c += p_list[i];
                n++;
            }
            // skip first == last case
            if( p_list[p_list.size()-1] != p_list[0] ) {
                c += p_list[p_list.size()-1];
                n++;
            }
            this->p_center = c / (float)n;
        }

        aabbox_t box() const noexcept override {
            aabbox_t box;
            for(const point_t& p : p_list) {
                box.resize(p);
            }
            return box;
        }

        void move_dir(const float d) noexcept override {
            point_t dir { d, 0 };
            dir.rotate(dir_angle);
            for(point_t& p : p_list) {
                p += dir;
            }
            p_center += dir;
        }

        void move(const point_t& d) noexcept override {
            for(point_t& p : p_list) {
                p += d;
            }
            p_center += d;
        }
        void move(const float dx, const float dy) noexcept override {
            for(point_t& p : p_list) {
                p.add(dx, dy);
            }
            p_center.add(dx, dy);
        }

        void rotate(const float radians) noexcept override {
            const float cos = std::cos(radians);
            const float sin = std::sin(radians);
#if 0
            // pre-ranged loop
            for(size_t i=0; i<p_list.size(); ++i) {
                point_t& p = p_list[i];
                p.rotate(sin, cos, p_center);
            }
#endif
            for(point_t& p : p_list) {
                p.rotate(sin, cos, p_center);
            }
            dir_angle += radians;
        }

        void set_center(const point_t& p) {
            const float dx = p.x - p_center.x;
            const float dy = p.y - p_center.y;
            move( dx, dy );
        }

        bool on_screen() const noexcept override {
            return box().on_screen();
        }

        bool contains(const point_t& o) const noexcept override {
            return box().contains(o);
        }

        /** Returns whether this object is fully inside the given aabbox_t. */
        bool inside(const aabbox_t& o) const noexcept override {
            return box().inside(o);
        }

        bool intersects(const lineseg_t & o) const noexcept override {
            return o.intersects(box());
        }

        bool intersects(const aabbox_t& o) const noexcept override {
            return box().intersects(o);
        }

        bool intersects(const geom_t& o) const noexcept override {
            return box().intersects(o.box());
        }

        bool intersects_lineonly(const lineseg_t & o) const noexcept {
            if( p_list.size() < 2 ) {
                return false;
            }
            point_t p0 = p_list[0];
            for(size_t i=1; i<p_list.size(); ++i) {
                const point_t& p1 = p_list[i];
                const lineseg_t l(p0, p1);
                if( l.intersects(o) ) {
                    return true;
                }
                p0 = p1;
            }
            return false;
        }

        bool intersection(vec_t& reflect_out, vec_t& cross_normal, point_t& cross_point,
                const lineseg_t& in) const noexcept override {
            if( p_list.size() < 2 ) {
                return false;
            }
            point_t p0 = p_list[0];
            for(size_t i=1; i<p_list.size(); ++i) {
                const point_t& p1 = p_list[i];
                const lineseg_t l(p0, p1);
                if( l.intersection(reflect_out, cross_normal, cross_point, in) ) {
                    return true;
                }
                p0 = p1;
            }
            return false;
        }

        void draw() const noexcept override;

        std::string toString() const noexcept override {
            return "linestrip[center " + p_center.toString() +
                    ", points " + std::to_string(p_list.size())+"]"; }
    };
    typedef std::shared_ptr<linestrip_t> linestrip_ref_t;

    /**
     * Unrotated:
     *
     *       (a1)
     *      /   \
     *     /     \
     *    /       \
     *   (c)------(b)
     */
    inline linestrip_ref_t make_triangle( const point_t& m, const float h) noexcept
    {
        linestrip_ref_t lf = std::make_shared<linestrip_t>(m, 0.0f);

        // a1
        point_t p = m;
        p.y += h/2.0f;
        lf->p_list.push_back(p);

        // b
        p.y -= h;
        const float width = 4.0f/5.0f * h;;
        p.x += width/2.0f;
        lf->p_list.push_back(p);

        // c
        p.x -= width;
        lf->p_list.push_back(p);

        return lf;
    }

    class circle_seg_t : public ageom_t {
      private:
        point_t m_center;
        float m_radius;
        float m_start_angle;
        float m_end_angle;
        float dir_angle = 0;
      public:
        circle_seg_t()
        : m_center(), m_radius(), m_start_angle(), m_end_angle() {}

        circle_seg_t(const point_t center, const float radius,
                     const float start_angle, const float end_angle) noexcept
        : m_center(center), m_radius(radius), m_start_angle(start_angle), m_end_angle(end_angle) {}

        circle_seg_t(const point_t center, const float radius,
                     const float an_angle, const float winkel, const bool b) noexcept
        : m_center(center), m_radius(radius)
        {
            if(b){
                m_start_angle = an_angle;
                m_end_angle = m_start_angle + winkel;
            } else {
                m_end_angle = an_angle;
                m_start_angle = m_end_angle - winkel;
            }
        }

        std::string toString() const noexcept override {
            return "center " + m_center.toString() + ", radius " + std::to_string(m_radius) +
                   ", start_angle " + std::to_string(m_start_angle) + ", end_angle " + std::to_string(m_end_angle);
        }

        void set_center(const point_t &new_center) {
            m_center = new_center;
        }

        aabbox_t box() const noexcept override {
            point_t bl = { m_center.x - m_radius, m_center.y - m_radius};
            point_t tr = { m_center.x + m_radius, m_center.y + m_radius};
            return aabbox_t(bl, tr);
        }

        bool contains(const point_t& o) const noexcept override {
            return box().contains(o);
            // return box().contains(o);
        }

        /** Returns whether this object is fully inside the given aabbox_t. */
        bool inside(const aabbox_t& o) const noexcept override {
            return box().inside(o);
        }

        bool intersects(const lineseg_t &o) const noexcept override {
            return box().intersects(o);
        }

        bool intersects(const aabbox_t &o) const noexcept override {
            return box().intersects(o);
        }

        bool intersects(const geom_t &o) const noexcept override {
            return box().intersects(o);
        }

        bool intersection(vec_t& reflect_out, vec_t& cross_normal, point_t& cross_point,
                          const lineseg_t& in) const noexcept override {
            if( !in.intersects( box() ) ) {
                return false;
            }
            cross_point = m_center; // use center
            const vec_t v_in = in.p1 - in.p0;
            cross_normal = v_in.normal_ccw().normalize();
            // reflect_out = v_in - ( 2.0f * v_in.dot(cross_normal) * cross_normal );
            // TODO: check if cross_normal is OK for this case
            reflect_out = -1.0f * v_in;
            return true;
        }

        void draw(pixel::f2::point_t pm, float r, float alpha1, float alpha2) const noexcept {
            float x;
            float y;
            float i = alpha1;
            for(; i <= alpha2; i += 0.01){
                x = std::cos(i) * r;
                y = std::sin(i) * r;
                pixel::f2::point_t p0 = pixel::f2::point_t(x, y);
                p0 += pm;
                p0.draw();
            }
        }

        void draw() const noexcept override {
            float x;
            float y;
            float i = m_start_angle;
            for(; i <= m_end_angle; i += 0.01){
                x = std::cos(i) * m_radius;
                y = std::sin(i) * m_radius;
                pixel::f2::point_t p0 = pixel::f2::point_t(x, y);
                p0 += m_center;
                p0.draw();
            }
        }

        bool on_screen() const noexcept override {
            return box().on_screen();
        }

        void rotate(const float radians) noexcept override {
            m_start_angle += radians;
            m_end_angle += radians;
        }

        void rotate(const float radians, point_t p) noexcept {
            m_start_angle += radians;
            m_end_angle += radians;
            m_center.rotate(radians, p);
        }

        void move_dir(const float d) noexcept override {
            point_t dir { d, 0 };
            dir.rotate(dir_angle);
            m_center += dir;
        }

        void move(const point_t& d) noexcept override {
            m_center += d;
        }
        void move(const float dx, const float dy) noexcept override {
            m_center.add(dx, dy);
        }
    };
    typedef std::shared_ptr<circle_seg_t> circle_seg_ref_t;

    class dashed_lineseg_t : public lineseg_t {
      public:
        float distance_length;
        float quantity;
        dashed_lineseg_t()
        : lineseg_t(), distance_length(), quantity() {}

        dashed_lineseg_t(dashed_lineseg_t& bl) noexcept;
        dashed_lineseg_t(const point_t p0_, const point_t p1_, const float distance_length_,
                         const float quantity_)
            : lineseg_t(p0_, p1_), distance_length(distance_length_), quantity(quantity_) {}

        dashed_lineseg_t(const lineseg_t &l, const float distance_length_, const float quantity_)
            : lineseg_t(l), distance_length(distance_length_), quantity(quantity_) {}

        dashed_lineseg_t& operator=(const dashed_lineseg_t& bl) noexcept = default;

        bool error() const {
            return quantity * distance_length >= length();
        }

        void draw() const noexcept override {
            const float a = (length() - quantity * distance_length) / (quantity + 1);
            point_t pa = p0;
            point_t pb = pa;
            const vec_t v1 = vec_t::from_length_angle(a, angle());
            const vec_t v2 = vec_t::from_length_angle(distance_length, angle());
            pb += v1;
            lineseg_t::draw(pa, pb);
            pa = pb + v2;

            for(float i = 0; i < quantity; ++i){
                pb += (v1 + v2);
                lineseg_t::draw(pa, pb);
                pa = pb + v2;
            }
        }
        std::string toString() const noexcept override {
            return "DL[" + p0.toString() + ", " + p1.toString() + "]";
        }
    };
}  // namespace pixel::f2

#endif /*  PIXEL2F_HPP_ */
