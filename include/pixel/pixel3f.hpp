#ifndef PIXEL2DF_HPP_
#define PIXEL2DF_HPP_

#include "pixel.hpp"

/**
 * 3D computer graphics math based upon three float components.
 */
namespace pixel::f3 {

    struct vec_t {
        public:
            float x;
            float y;
            float z;

            constexpr vec_t() noexcept
            : x(0), y(0), z(0) {}

            constexpr vec_t(const float x_, const float y_, const float z_) noexcept
            : x(x_), y(y_), z(z_) {}

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

            constexpr void add(const float dx, const float dy, const float dz) noexcept { x+=dx; y+=dy; z+=dz; }

            constexpr vec_t& operator+=(const vec_t& rhs ) noexcept {
                x+=rhs.x; y+=rhs.y; z+=rhs.z;
                return *this;
            }

            constexpr vec_t& operator-=(const vec_t& rhs ) noexcept {
                x-=rhs.x; y-=rhs.y; z-=rhs.z;
                return *this;
            }

            /**
             * Scale this vector with given scale factor
             * @param s scale factor
             * @return this instance
             */
            constexpr vec_t& operator*=(const float s ) noexcept {
                x*=s; y*=s; z*=s;
                return *this;
            }

            /**
             * Divide this vector with given scale factor
             * @param s scale factor
             * @return this instance
             */
            constexpr vec_t& operator/=(const float s ) noexcept {
                x/=s; y/=s; z/=s;
                return *this;
            }

            void rotate(const float radians, const vec_t& ctr) noexcept {
                // FIXME z, consider using a matrix or quaternions
                const float cos = std::cos(radians);
                const float sin = std::sin(radians);
                const float x0 = x - ctr.x;
                const float y0 = y - ctr.y;
                const float tmp = x0 * cos - y0 * sin + ctr.x;
                              y = x0 * sin + y0 * cos + ctr.y;
                x = tmp;
            }
            constexpr void rotate(const float sin, const float cos, const vec_t& ctr) noexcept {
                // FIXME z, consider using a matrix or quaternions
                const float x0 = x - ctr.x;
                const float y0 = y - ctr.y;
                const float tmp = x0 * cos - y0 * sin + ctr.x;
                              y = x0 * sin + y0 * cos + ctr.y;
                x = tmp;
            }

            std::string toString() const noexcept { return std::to_string(x)+"/"+std::to_string(y); }

            constexpr bool is_zero() const noexcept {
                return pixel::is_zero(x) && pixel::is_zero(y) && pixel::is_zero(z);
            }

            /**
             * Return the squared length of a vector, a.k.a the squared <i>norm</i> or squared <i>magnitude</i>
             */
            constexpr float norm_sq() const noexcept {
                return x*x + y*y + z*z;
            }

            /**
             * Return the length of a vector, a.k.a the <i>norm</i> or <i>magnitude</i>
             */
            constexpr float norm() const noexcept {
                return std::sqrt(norm_sq());
            }

            /**
             * Normalize this vector in place
             */
            constexpr void normalize() noexcept {
                const float lengthSq = norm_sq();
                if ( pixel::is_zero( lengthSq ) ) {
                    x = 0.0f;
                    y = 0.0f;
                    z = 0.0f;
                } else {
                    const float invSqr = 1.0f / std::sqrt(lengthSq);
                    x *= invSqr;
                    x *= invSqr;
                    z *= invSqr;
                }
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
                return dx*dx + dy*dy + dz*dz;
            }

            /**
             * Return the distance between this vector and the given one.
             */
            constexpr float dist(const vec_t& o) const noexcept {
                return std::sqrt(dist_sq(o));
            }

            /**
             * Return the dot product of this vector and the given one
             * @return the dot product as float
             */
            constexpr float dot(const vec_t& o) const noexcept {
                return x*o.x + y*o.y + z*o.z;
            }

            /**
             * cross product this x o
             * @return the resulting vector
             */
            constexpr vec_t cross(const vec_t& o) const noexcept {
                return vec_t( y * o.z - z * o.y,
                              z * o.x - x * o.z,
                              x * o.y - y * o.y);
            }

            /**
             * Return the cosines of the angle between to vectors
             * @param vec1 vector 1
             * @param vec2 vector 2
             */
            constexpr float cos_angle(const vec_t& o) const noexcept {
                return dot(o) / ( norm() * o.norm() ) ;
            }

            /**
             * Return the angle between to vectors in radians
             * @param vec1 vector 1
             * @param vec2 vector 2
             */
            float angle(const vec_t& o) noexcept {
                return std::acos( cos_angle(o) );
            }

            bool intersects(const vec_t& o) const noexcept {
                const float eps = std::numeric_limits<float>::epsilon();
                if( std::abs(x-o.x) >= eps || std::abs(y-o.y) >= eps || std::abs(z-o.z) >= eps ) {
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
    struct ray_t {
        /** Origin of Ray. */
        point_t orig;

        /** Normalized direction vector of ray. */
        vec_t dir;

        std::string toString() const noexcept { return "Ray[orig "+orig.toString()+", dir "+dir.toString() +"]"; }
    };

    struct lineseg_t {
        typedef std::function<bool(const point_t& p)> point_action_t;

        point_t p0;
        point_t p1;
        point_t p2;

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

        std::string toString() const noexcept { return "L[" + p0.toString() + ", " + p1.toString() + "]"; }

        /**
         * Compute intersection between two lines segments
         * @param result storage for the intersection coordinates if the lines intersect, otherwise unchanged
         * @param a vertex 1 of first line
         * @param b vertex 2 of first line
         * @param c vertex 1 of second line
         * @param d vertex 2 of second line
         * @return true if the line segments intersect, otherwise false
         */
        bool intersects(point_t& result, const lineseg_t & o) {
            const float determinant = ( p0.x - p1.x ) * ( o.p0.y - o.p1.y) - ( p0.y - p1.y ) * ( o.p0.x - o.p1.x );
            if( 0 == determinant ) {
                return false;
            }
            const float alpha = p0.x * p1.y - p0.y * p1.x;
            const float beta = o.p0.x*o.p1.y-o.p0.y*o.p1.y;
            const float xi = ((o.p0.x-o.p1.x)*alpha-(p0.x-p1.x)*beta)/determinant;

            const float gamma0 = (xi - p0.x) / (p1.x - p0.x);
            const float gamma1 = (xi - o.p0.x) / (o.p1.x - o.p0.x);
            if(gamma0 <= 0 || gamma0 >= 1 || gamma1 <= 0 || gamma1 >= 1) {
                return false;
            }
            const float yi = ((o.p0.y-o.p1.y)*alpha-(p0.y-p1.y)*beta)/determinant;
            result.x = xi;
            result.y = yi;
            return true;
        }

        /**
         * Compute intersection between two lines segments
         * @param a vertex 1 of first line
         * @param b vertex 2 of first line
         * @param c vertex 1 of second line
         * @param d vertex 2 of second line
         * @return true if the line segments intersect, otherwise false
         */
        bool intersects(const lineseg_t & o) {
            const float determinant = ( p0.x - p1.x ) * ( o.p0.y - o.p1.y) - ( p0.y - p1.y ) * ( o.p0.x - o.p1.x );
            if( 0 == determinant ) {
                return false;
            }
            const float alpha = p0.x * p1.y - p0.y * p1.x;
            const float beta = o.p0.x*o.p1.y-o.p0.y*o.p1.y;
            const float xi = ((o.p0.x-o.p1.x)*alpha-(p0.x-p1.x)*beta)/determinant;

            const float gamma0 = (xi - p0.x) / (p1.x - p0.x);
            const float gamma1 = (xi - o.p0.x) / (o.p1.x - o.p0.x);
            if(gamma0 <= 0 || gamma0 >= 1 || gamma1 <= 0 || gamma1 >= 1) {
                return false;
            }
            return true;
        }
    };

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
    class aabbox_t {
        public:
            /** bottom left (low) */
            point_t bl;
            /** top right (high) */
            point_t tr;

            /**
             * Create an Axis Aligned bounding box (AABBox)
             * where the low and and high MAX float Values.
             */
            aabbox_t() noexcept {
                reset();
            }

            /**
             * Create an AABBox with given bl (low) and tr (high)
             */
            aabbox_t(const point_t& bl_, const point_t& tr_) noexcept
            : bl( bl_ ), tr( tr_ ) {
            }

            /**
             * Reset this box to the inverse low/high, allowing the next {@link #resize(float, float, float)} command to hit.
             * @return this AABBox for chaining
             */
            aabbox_t& reset() noexcept {
                bl.x = std::numeric_limits<float>::max();
                bl.y = std::numeric_limits<float>::max();
                tr.x = -std::numeric_limits<float>::max();
                tr.y = -std::numeric_limits<float>::max();
                return *this;
            }

            /**
             * Resize the AABBox to encapsulate another AABox
             * @param newBox AABBox to be encapsulated in
             * @return this AABBox for chaining
             */
            aabbox_t& resize(const aabbox_t& o) noexcept {
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
            aabbox_t& resize(float x, float y) noexcept {
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
            aabbox_t& resize(const point_t& p) noexcept {
                return resize(p.x, p.y);
            }

            /**
             * Check if the coordinates are bounded/contained by this AABBox
             * @param x  x-axis coordinate value
             * @param y  y-axis coordinate value
             * @return true if  x belong to (low.x, high.x) and y belong to (low.y, high.y)
             */
            bool contains(float x, float y) const noexcept {
                if( x<bl.x || x>tr.x ) {
                    return false;
                }
                if( y<bl.y || y>tr.y ) {
                    return false;
                }
                return true;
            }

            /**
             * Check if the point is bounded/contained by this AABBox
             * @return true if  x belong to (low.x, high.x) and y belong to (low.y, high.y)
             */
            bool contains(const point_t& p) const noexcept {
                return contains(p.x, p.y);
            }

            bool intersects(const aabbox_t& o) const noexcept {
                if( tr.x < o.bl.x ||
                    tr.y < o.bl.y ||
                    bl.x > o.tr.x ||
                    bl.y > o.tr.y )
                {
                    return false;
                } else {
                    return true;
                }
            }

            bool intersects(const lineseg_t& ls) const noexcept {
                // separating axis theorem.
                const vec_t d = (ls.p1 - ls.p0) * 0.5f; // half lineseg direction
                const vec_t e = (tr - bl) * 0.5f;
                const vec_t aabb_center = (bl + tr) * 0.5f;
                const vec_t lseg_center = ls.p0 + d;
                const vec_t c = lseg_center - aabb_center;
                 vec_t ad(std::abs(d.x), std::abs(d.y), std::abs(d.z));
                 if (std::abs(c.x) > e.x + ad.x)
                     return false;
                 if (std::abs(c.y) > e.y + ad.y)
                     return false;
                 if (std::abs(c.z) > e.z + ad.z)
                     return false;
                 if (std::abs(d.y * c.z - d.z * c.y) > e.y * ad.z + e.z * ad.y + std::numeric_limits<float>::epsilon()) {
                     return false;
                 }
                 if (std::abs(d.z * c.x - d.x * c.z) > e.z * ad.x + e.x * ad.z + std::numeric_limits<float>::epsilon()) {
                     return false;
                 }
                 if (std::abs(d.x * c.y - d.y * c.x) > e.x * ad.y + e.y * ad.x + std::numeric_limits<float>::epsilon()) {
                     return false;
                 }
                 return true;
            }

            std::string toString() const noexcept {
                return "aabb[bl " + bl.toString() +
                       ", tr " + tr.toString() +
                       "]"; }
    };

} // namespace pixel_3f

#endif /*  PIXEL2DI_HPP_ */
