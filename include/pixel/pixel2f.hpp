#ifndef PIXEL2F_HPP_
#define PIXEL2F_HPP_

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
             * Return the direction angle of this vector in radians
             */
            float angle() const noexcept {
                // Utilize atan2 taking y=sin(a) and x=cos(a), resulting in proper direction angle for all quadrants.
                return std::atan2( y, x );
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
             * cross product this x o
             * @return the resulting vector
             */
            constexpr float cross(const vec_t& o) const noexcept {
                return x * o.y - y * o.x;
            }

            /**
             * Return the cosines of the angle between two vectors
             */
            constexpr float cos_angle(const vec_t& o) const noexcept {
                return dot(o) / ( norm() * o.norm() ) ;
            }

            /**
             * Return the angle between two vectors in radians
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
     * Return the orientation of the given point triplet p0, p1 and p2.
     */
    constexpr pixel::orientation_t orientation(const point_t& p, const point_t& q, const point_t& r) noexcept {
        const float delta = ( q.y - p.y ) * ( r.x - q.x ) - ( q.x - p.x ) * ( r.y - q.y );
        if ( pixel::is_zero( delta ) ) {
            return pixel::orientation_t::COL;
        }
        return ( delta > 0.0f ) ? pixel::orientation_t::CLW : pixel::orientation_t::CCW;
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
             * and if intersecting, the angle between this object and the given line segment in radians.
             */
            virtual bool intersection(float& angle_res, point_t& cross_res, const lineseg_t& in) const noexcept = 0;

            virtual void draw() const noexcept = 0;
            virtual bool on_screen() const noexcept = 0;

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
            aabbox_t() noexcept {
                reset();
            }

            /**
             * Create an AABBox with given bl (low) and tr (high)
             */
            aabbox_t(const point_t& bl_, const point_t& tr_) noexcept
            : bl( bl_ ), tr( tr_ ) {
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

            aabbox_t box() const noexcept override { return *this; }

            /**
             * Check if the point is bounded/contained by this AABBox
             * @return true if  x belong to (low.x, high.x) and y belong to (low.y, high.y)
             */
            bool contains(const point_t& p) const noexcept override {
                if( p.x<bl.x || p.x>tr.x ) {
                    return false;
                }
                if( p.y<bl.y || p.y>tr.y ) {
                    return false;
                }
                return true;
            }

            bool intersects(const lineseg_t & o) const noexcept override;

            bool intersects(const aabbox_t& o) const noexcept override {
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

            bool intersects(const geom_t& o) const noexcept override {
                return intersects(o.box());
            }

            bool intersection(float& angle_res, point_t& cross_res, const lineseg_t& in) const noexcept override;

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

            void draw() const noexcept;

            std::string toString() const noexcept {
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

            void add(float length) {
                // extend center points p0, p1 with radius in moving direction
                const float a_move = angle();
                vec_t l_move_diff = pixel::f2::vec_t::from_length_angle(length, a_move);
                p0 -= l_move_diff;
                p1 += l_move_diff;
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
                return pixel::is_zero( perpDotProduct );

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

        private:
            /**
             * See [p + t r = q + u s](https://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect/565282#565282)
             * and [its terse C# implementation](https://www.codeproject.com/tips/862988/find-the-intersection-point-of-two-line-segments)
             */
            static bool intersects(point_t& result,
                                   const point_t& p, const point_t& p2,
                                   const point_t& q, const point_t& q2)
            {
                const point_t q_p = q - p;
                const point_t r = p2 - p;
                const point_t s = q2 - q;
                const float rxs = r.cross(s);
                const float qpxr = q_p.cross(r);

                // If r x s = 0 and (q - p) x r = 0, then the two lines are collinear.
                if ( pixel::is_zero(rxs) && pixel::is_zero(qpxr) )
                {
                    // 1. If either  0 <= (q - p) * r <= r * r or 0 <= (p - q) * s <= * s
                    // then the two lines are overlapping,
                    if (false /* considerCollinearOverlapAsIntersect */) {
                        const point_t p_q = p - q;
                        if ( ( 0 <= q_p.dot(r) && q_p.dot(r) <= r.dot(r) ) || ( 0 <= p_q.dot(s) && p_q.dot(s) <= s.dot(s) ) ) {
                            return true;
                        }
                    }

                    // 2. If neither 0 <= (q - p) * r = r * r nor 0 <= (p - q) * s <= s * s
                    // then the two lines are collinear but disjoint.
                    // No need to implement this expression, as it follows from the expression above.
                    return false;
                }

                // 3. If r x s = 0 and (q - p) x r != 0, then the two lines are parallel and non-intersecting.
                if ( pixel::is_zero(rxs) /** && !pixel::is_zero(qpxr) **/ ) {
                    return false;
                }

                // t = (q - p) x s / (r x s)
                const float t = q_p.cross(s) / rxs;

                // u = (q - p) x r / (r x s)
                const float u = qpxr / rxs;

                // 4. If r x s != 0 and 0 <= t <= 1 and 0 <= u <= 1
                // the two line segments meet at the point p + t r = q + u s.
                if (!pixel::is_zero(rxs) && (0 <= t && t <= 1) && (0 <= u && u <= 1))
                {
                    // We can calculate the intersection point using either t or u.
                    result = p + (t * r);
                    return true;
                }

                // 5. Otherwise, the two line segments are not parallel but do not intersect.
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

            bool intersection(float& angle_res, point_t& cross_res, const lineseg_t& in) const noexcept override {
                if( intersects(cross_res, in) ) {
                    angle_res = angle(in);
                    pixel::log_printf("intersection: %s with %s -> angle %f, %s\n",
                            toString().c_str(), in.toString().c_str(),
                            pixel::rad_to_adeg(angle_res), cross_res.toString().c_str());
                    return true;
                }
                return false;
            }
    };

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
            virtual ~ageom_t() = default;

            virtual void rotate(const float rad) noexcept = 0;
            virtual void move_dir(const float d) noexcept = 0;
            virtual void move(const point_t& d) noexcept = 0;
            virtual void move(const float dx, const float dy) noexcept = 0;
    };
    typedef std::shared_ptr<ageom_t> ageom_ref_t;
    typedef std::vector<ageom_ref_t> ageom_list_t;

    ageom_list_t& agobjects();

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
            /** direction angle in radians */
            float dir_angle;

            disk_t(const point_t& c_, const float r_)
            : center( c_), radius(r_), dir_angle(0.0f) {}

            disk_t(float x, float y, const float r_)
            : center(x, y), radius(r_), dir_angle(0.0f) {}

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
                return box().contains(o);
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

            bool intersection(float& angle_res, point_t& cross_res, const lineseg_t& in) const noexcept override {
                if( !in.intersects( box() ) ) {
                    return false;
                }
                angle_res = in.angle();
                cross_res = center; // FIXME
                return true;
            }

            void draw() const noexcept override {
                const float x_ival = pixel::cart_coord.width() / (float)pixel::fb_width;
                const float y_ival = pixel::cart_coord.height() / (float)pixel::fb_height;

                const aabbox_t b = box();
                for(float y=b.bl.y; y<=b.tr.y; y+=y_ival) {
                    for(float x=b.bl.x; x<=b.tr.x; x+=x_ival) {
                        const point_t p { x, y };
                        if( center.dist(p) <= radius ) {
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

            void move_dir(const float d) noexcept override {
                point_t dir { d, 0 };
                dir.rotate(dir_angle, { 0, 0 });
                center += dir;
            }

            void move(const point_t& d) noexcept override {
                center += d;
            }
            void move(const float dx, const float dy) noexcept override {
                center.add(dx, dy);
            }
    };

    class rect_t : public ageom_t {
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
            float dir_angle;

        public:
            rect_t(const point_t& tl_, const float width, const float height, const float radians) noexcept
            {
                p_a = tl_;
                p_b = { p_a.x + width, p_a.y };
                p_c = { p_a.x        , p_a.y - height};
                p_d = { p_a.x + width, p_a.y - height};
                p_center = { p_a.x + width/2  , p_a.y - height/2  };
                dir_angle = 0.0f;
                rect_t::rotate(radians);
            }

            rect_t(const point_t& tl_, const float width, const float height) noexcept 
            : p_a(tl_)
            {
                p_b = { p_a.x + width, p_a.y };
                p_c = { p_a.x        , p_a.y - height};
                p_d = { p_a.x + width, p_a.y - height};
                p_center = { p_a.x + width/2  , p_a.y - height/2  };
                dir_angle = 0.0f;
            }

            rect_t(const point_t& tl_, const point_t& tr_, const point_t& bl_, const point_t& br_) noexcept 
            : p_a(tl_), p_b(tr_), p_c(bl_), p_d(br_)
            {
                p_center = { ( p_a.x + p_b.x ) / 2.0f  , ( p_a.y + p_c.y ) / 2.0f  };
                dir_angle = 0.0f;
            }

            aabbox_t box() const noexcept override {
                return aabbox_t().resize(p_a).resize(p_b).resize(p_c).resize(p_d);
            }

            void move_dir(const float d) noexcept override {
                point_t dir { d, 0 };
                dir.rotate(dir_angle, { 0, 0 });
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
                dir_angle += radians;
            }

            void set_top_left(const point_t& p) {
                // FIXME: Since m_p_a is unknown to be top-left ...
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

            bool intersects(const lineseg_t & o) const noexcept override {
                return o.intersects(box());
            }

            bool intersects(const aabbox_t& o) const noexcept override {
                return box().intersects(o);
            }

            bool intersects(const geom_t& o) const noexcept override {
                return box().intersects(o.box());
            }

            bool intersection(float& angle_res, point_t& cross_res, const lineseg_t& in) const noexcept override {
                {
                    // tl .. tr
                    const lineseg_t l(p_a, p_b);
                    if( l.intersection(angle_res, cross_res, in) ) {
                        return true;
                    }
                }
                {
                    // bl .. br
                    const lineseg_t l(p_c, p_d);
                    if( l.intersection(angle_res, cross_res, in) ) {
                        return true;
                    }
                }
                {
                    // br .. tr
                    const lineseg_t l(p_d, p_b);
                    if( l.intersection(angle_res, cross_res, in) ) {
                        return true;
                    }
                }
                {
                    // bl .. tl
                    const lineseg_t l(p_c, p_a);
                    if( l.intersection(angle_res, cross_res, in) ) {
                        return true;
                    }
                }
                return false;
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
