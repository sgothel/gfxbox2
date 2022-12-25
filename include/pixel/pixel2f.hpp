#ifndef PIXEL2F_HPP_
#define PIXEL2F_HPP_

#include "pixel.hpp"

/**
 * 2D computer graphics math based upon two float components.
 */
namespace pixel::f2 {

    struct vec_t {
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

            /**
             * Returns component value, this reinterpreted as `float*` w/o boundary check
             */
            float operator[](size_t i) const noexcept {
                const float* p = reinterpret_cast<const float*>(this);
                return p[i];
            }

            constexpr void add(const float dx, const float dy) noexcept { x+=dx; y+=dy; }

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

            void rotate(const float radians, const vec_t& ctr) noexcept {
                const float cos = std::cos(radians);
                const float sin = std::sin(radians);
                rotate(sin, cos, ctr);
            }
            constexpr void rotate(const float sin, const float cos, const vec_t& ctr) noexcept {
                const float x0 = x - ctr.x;
                const float y0 = y - ctr.y;
                const float tmp = x0 * cos - y0 * sin + ctr.x;
                              y = x0 * sin + y0 * cos + ctr.y;
                x = tmp;
            }

            std::string toString() const noexcept { return std::to_string(x)+" / "+std::to_string(y); }

            constexpr bool is_zero() const noexcept {
                return pixel::is_zero(x) && pixel::is_zero(y);
            }

            /**
             * Return the squared length of this vector, a.k.a the squared <i>norm</i> or squared <i>magnitude</i>
             */
            constexpr float norm_sq() const noexcept {
                return x*x + y*y;
            }

            /**
             * Return the length of this vector, a.k.a the <i>norm</i> or <i>magnitude</i>
             */
            constexpr float norm() const noexcept {
                return std::sqrt(norm_sq());
            }

            /**
             * Return the tangent of this vectors
             */
            constexpr float tan_angle() const noexcept {
                return y / x;
            }

            /**
             * Return the angle of this vectors in radians
             */
            float angle() const noexcept {
                return std::atan( tan_angle() );
            }

            /**
             * Normalize this vector in place
             */
            constexpr void normalize() noexcept {
                const float lengthSq = norm_sq();
                if ( pixel::is_zero( lengthSq ) ) {
                    x = 0.0f;
                    y = 0.0f;
                } else {
                    const float invSqr = 1.0f / std::sqrt(lengthSq);
                    x *= invSqr;
                    x *= invSqr;
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
             * @return the dot product as float
             */
            constexpr float dot(const vec_t& o) const noexcept {
                return x*o.x + y*o.y;
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
            float angle(const vec_t& o) const noexcept {
                return std::acos( cos_angle(o) );
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

            void draw() const noexcept {
                pixel::set_pixel(x, y);
            }
    };

    typedef vec_t point_t;

    /** Convert framebuffer coordinates in pixels to cartesian coordinates. */
    vec_t fb_to_cart(const int x, const int y) noexcept { return vec_t(cart_coord.from_fb_x(x), cart_coord.from_fb_y(y)); }

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

    constexpr vec_t operator/(const vec_t& lhs, const float s ) noexcept {
        vec_t r(lhs);
        r /= s;
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

        std::string toString() const noexcept { return "L[" + p0.toString() + ", " + p1.toString() + "]"; }

        bool on_screen() const noexcept {
            return p0.on_screen() && p1.on_screen();
        }

        static void for_all_points(const point_t& p0, const point_t& p1,
                                   const point_action_t& point_action) noexcept
        {
            const float x_ival = pixel::cart_coord.width() / (float)pixel::fb_width;
            const float y_ival = pixel::cart_coord.height() / (float)pixel::fb_height;
            const float dx = p1.x - p0.x;
            const float dy = p1.y - p0.y;
            if( std::abs(dy) > std::abs(dx) ) {
                const float step_y = ( dy >= 0 ) ? y_ival : -y_ival;
                const float step_x = dx / std::abs( dy ) * y_ival;
                float sy=0.0f;
                float sx=0.0f;
                for(; std::abs(sy - dy) > y_ival/2.0f; sy+=step_y, sx+=step_x) {
                    const point_t p { p0.x + sx, p0.y + sy };
                    if( !point_action(p) ) {
                        return;
                    }
                }
            } else {
                const float step_x = ( dx >= 0 ) ? x_ival : -x_ival;
                const float step_y = dy / std::abs( dx ) * x_ival;
                float sx=0.0f;
                float sy=0.0f;
                for(; std::abs(sx - dx) > x_ival/2.0f; sx+=step_x, sy+=step_y) {
                    point_t p { p0.x + sx, p0.y + sy };
                    if( !point_action(p) ) {
                        return;
                    }
                }
            }
        }
        static void draw(const point_t& p0, const point_t& p1) noexcept {
            point_action_t point_draw = [&](const point_t& p) -> bool {
                p.draw();
                return true;
            };
            for_all_points( p0, p1, point_draw );
        }
        void draw() const noexcept {
            draw(p0, p1);
        }

        bool is_on_line(const point_t& p) const noexcept {
            // Using the perp dot product (PDP),
            // which is the area of the parallelogram of the three points,
            // same as the area of the triangle defined by the three points, multiplied by 2.
            const float perpDotProduct = (p0.x - p.x) * (p1.y - p.y) - (p0.y - p.y) * (p1.x - p.x);
            return std::abs(perpDotProduct) < std::numeric_limits<float>::epsilon();
        }

        /**
         * Test intersection between this line segment and the give point
         * @return true if the line segment contins the point, otherwise false
         */
        bool contains(const point_t& p) const noexcept {
            if ( !( ( p0.x <= p.x && p.x <= p1.x ) || ( p1.x <= p.x && p.x <= p0.x ) ) ) {
                // not in x-range
                return false;
            }
            if ( !( ( p0.y <= p.y && p.y <= p1.y ) || ( p1.y <= p.y && p.y <= p0.y ) ) ) {
                // not in y-range
                return false;
            }
            return is_on_line(p);
        }

        /**
         * Compute intersection between two lines segments
         * @param result storage for the intersection coordinates if the lines intersect, otherwise unchanged
         * @param a vertex 1 of first line
         * @param b vertex 2 of first line
         * @param c vertex 1 of second line
         * @param d vertex 2 of second line
         * @return true if the line segments intersect, otherwise false
         */
        bool intersects(point_t& result, const lineseg_t & o) const noexcept {
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
         * Test intersection between two lines segments
         * @param a vertex 1 of first line
         * @param b vertex 2 of first line
         * @param c vertex 1 of second line
         * @param d vertex 2 of second line
         * @return true if the line segments intersect, otherwise false
         */
        bool intersects(const lineseg_t & o) const noexcept {
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
             * Create an AABBox with given lineseg
             */
            aabbox_t(const lineseg_t& ls) noexcept {
                reset();
                resize(ls.p0).resize(ls.p1);
            }

            aabbox_t(const aabbox_t& o) noexcept = default;
            aabbox_t(aabbox_t&& o) noexcept = default;

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
#if 0
                return intersects( aabbox_t(ls) );
#else
                // separating axis theorem.
                const vec_t d = (ls.p1 - ls.p0) * 0.5f; // half lineseg direction
                const vec_t e = (tr - bl) * 0.5f;
                const vec_t aabb_center = (bl + tr) * 0.5f;
                const vec_t lseg_center = ls.p0 + d;
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
#endif
            }

            bool on_screen() const noexcept {
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

            void draw() const noexcept {
                const point_t tl(bl.x, tr.y);
                const point_t br(tr.x, bl.y);
                lineseg_t::draw(tl, tr);
                lineseg_t::draw(tr, br);
                lineseg_t::draw(br, bl);
                lineseg_t::draw(bl, tl);
            }

            std::string toString() const noexcept {
                return "aabb[bl " + bl.toString() +
                       ", tr " + tr.toString() +
                       "]"; }
    };

    class geom_t {
        public:
            virtual ~geom_t() = default;

            virtual aabbox_t box() const noexcept = 0;
            virtual void draw() const noexcept = 0;
            virtual bool on_screen() const noexcept = 0;
            virtual bool contains(const point_t& o) const noexcept = 0;
            virtual bool intersects(const geom_t& o) const noexcept = 0;
            virtual std::string toString() const noexcept = 0;

            virtual void rotate(const float rad) noexcept = 0;
            virtual void move_dir(const float d) noexcept = 0;
            virtual void move(const point_t& d) noexcept = 0;
            virtual void move(const float dx, const float dy) noexcept = 0;
    };

    std::vector<geom_t*>& gobjects() {
        static std::vector<geom_t*> _gobjects;
        return _gobjects;
    }

    class disk_t : public geom_t {
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
            /** center */
            point_t center;
            float radius;
            /** direction angle in radians */
            float angle;

            disk_t(const point_t& c_, const float r_)
            : center( c_), radius(r_), angle(0.0f) {}

            disk_t(float x, float y, const float r_)
            : center(x, y), radius(r_), angle(0.0f) {}

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

            bool on_screen() const noexcept override {
                return box().on_screen();
            }

            bool contains(const point_t& o) const noexcept override {
                return box().contains(o);
            }

            bool intersects(const geom_t& o) const noexcept override {
                return box().intersects(o.box());
            }

            void draw() const noexcept override {
                const aabbox_t b = box();
                const int b_tr_y = pixel::round_to_int(b.tr.y);
                const int b_tr_x = pixel::round_to_int(b.tr.x);
                for(int y=pixel::round_to_int(b.bl.y); y<=b_tr_y; ++y) {
                    for(int x=pixel::round_to_int(b.bl.x); x<=b_tr_x; ++x) {
                        const point_t p { (float)x, (float)y };
                        if( center.dist(p) <= radius ) {
                            p.draw();
                        }
                    }
                }
            }

            void rotate(const float rad) noexcept override {
                angle += rad;
            }

            void move_dir(const float d) noexcept override {
                point_t dir { d, 0 };
                dir.rotate(angle, { 0, 0 });
                center += dir;
            }

            void move(const point_t& d) noexcept override {
                center += d;
            }
            void move(const float dx, const float dy) noexcept override {
                center.add(dx, dy);
            }
    };

    class rect_t : public geom_t {
        public:
            /**
             * Unrotated:
             *
             *   (a)-----(b)
             *    |       |
             *    |       |
             *    |       |
             *   (c)-----(d)
             */
            /** Unrotated top-left */
            point_t p_a;
            /** Unrotated top-right */
            point_t p_b;
            /** Unrotated bottom-left */
            point_t p_c;
            /** Unrotated bottom_right */
            point_t p_d;
            point_t p_center;
            /** direction angle in radians */
            float angle;

        public:
            rect_t(const point_t& tl_, const float width, const float height, const float radians) noexcept
            {
                p_a = tl_;
                p_b = { p_a.x + width - 1, p_a.y };
                p_c = { p_a.x            , p_a.y + height - 1};
                p_d = { p_a.x + width - 1, p_a.y + height - 1};
                p_center = { p_a.x + width/2  , p_a.y + height/2  };
                angle = 0.0f;
                rect_t::rotate(radians);
            }

            rect_t(const point_t& tl_, const float width, const float height) noexcept 
            : p_a(tl_)
            {
                p_b = { p_a.x + width - 1, p_a.y };
                p_c = { p_a.x            , p_a.y + height - 1};
                p_d = { p_a.x + width - 1, p_a.y + height - 1};
                p_center = { p_a.x + width/2  , p_a.y + height/2  };
                angle = 0.0f;
            }

            rect_t(const point_t& tl_, const point_t& tr_, const point_t& bl_, const point_t& br_) noexcept 
            : p_a(tl_), p_b(tr_), p_c(bl_), p_d(br_)
            {
                p_center = { ( p_a.x + p_b.x ) / 2.0f  , ( p_a.y + p_c.y ) / 2.0f  };
                angle = 0.0f;
            }

            aabbox_t box() const noexcept override {
                return aabbox_t().resize(p_a).resize(p_b).resize(p_c).resize(p_d);
            }

            void move_dir(const float d) noexcept override {
                point_t dir { d, 0 };
                dir.rotate(angle, { 0, 0 });
                p_a += dir;
                p_b += dir;
                p_c += dir;
                p_d += dir;
                p_center += dir;
            }

            void move(const point_t& d) noexcept override {
                p_a += d;
                p_b += d;
                p_c += d;
                p_d += d;
                p_center += d;
            }
            void move(const float dx, const float dy) noexcept override {
                p_a.add(dx, dy);
                p_b.add(dx, dy);
                p_c.add(dx, dy);
                p_d.add(dx, dy);
                p_center.add(dx, dy);
            }

            void rotate(const float radians) noexcept override {
                const float cos = std::cos(radians);
                const float sin = std::sin(radians);
                p_a.rotate(sin, cos, p_center);
                p_b.rotate(sin, cos, p_center);
                p_c.rotate(sin, cos, p_center);
                p_d.rotate(sin, cos, p_center);
                angle += radians;
            }

            void set_top_left(const point_t& p) {
                // FIXME: Since p_a is unknown to be top-left ...
                const float dx = p.x - p_a.x;
                const float dy = p.y - p_a.y;
                move( dx, dy );
            }

            bool on_screen() const noexcept override {
                return box().on_screen();
            }

            bool contains(const point_t& o) const noexcept override {
                return box().contains(o);
            }

            bool intersects(const geom_t& o) const noexcept override {
                return box().intersects(o.box());
            }

            bool intersects(const lineseg_t& l) const {
                return box().intersects(l);
            }

            void draw() const noexcept override {
                lineseg_t::draw(p_a, p_b);
                lineseg_t::draw(p_b, p_d);
                lineseg_t::draw(p_d, p_c);
                lineseg_t::draw(p_c, p_a);
            }

            std::string toString() const noexcept override {
                return "rect[a " + p_a.toString() +
                       ", b " + p_b.toString() +
                       ", c " + p_c.toString() +
                       ", d " + p_d.toString() +
                       "]"; }
        };

} // namespace pixel_2f

#endif /*  PIXEL2F_HPP_ */
