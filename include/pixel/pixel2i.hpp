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
#ifndef PIXEL2I_HPP_
#define PIXEL2I_HPP_

#include "pixel.hpp"

/**
 * 2D computer graphics math based upon two integer components.
 */
namespace pixel::i2 {

    struct point_t {
        int x;
        int y;

        void move(const int d) { x+=d; y+=d; }
        void move(const int dx, const int dy) { x+=dx; y+=dy; }
        void scale(const int s) { x*=s; y*=s; }

        void rotate(const float radians, const point_t& ctr) {
            const float cos = std::cos(radians);
            const float sin = std::sin(radians);
            rotate(sin, cos, ctr);
        }
        void rotate(const float sin, const float cos, const point_t& ctr) {
            const float x0 = (float)(x - ctr.x);
            const float y0 = (float)(y - ctr.y);
            const int tmp = jau::round_to_int( x0 * cos - y0 * sin ) + ctr.x;
                        y = jau::round_to_int( x0 * sin + y0 * cos ) + ctr.y;
            x = tmp;
        }

        std::string toString() const { return std::to_string(x)+"/"+std::to_string(y); }

        bool intersects(const point_t& o) {
            return x == o.x && y == o.y;
        }

        void draw() const {
            pixel::set_pixel(x, y);
        }
    };

    struct lineseg_t {
        typedef std::function<bool(const point_t& p)> point_action_t;

        point_t p0;
        point_t p1;

        void move(const int d) { p0.move(d); p1.move(d); }
        void scale(const int s) { p0.scale(s); p1.scale(s); } // FIXME ????

        std::string toString() const { return "L[" + p0.toString() + ", " + p1.toString() + "]"; }

        static void for_all_points(const point_t& p0, const point_t& p1,
                                   const point_action_t& point_action)
        {
            const int dx = p1.x - p0.x;
            const int dy = p1.y - p0.y;
            if( std::abs(dy) > std::abs(dx) ) {
                const int step_y = ( dy >= 0 ) ? 1 : -1;
                const float step_x = (float)dx / std::abs( (float)dy );
                int sy=0;
                float sx=0.0f;
                for(; sy != dy; sy+=step_y, sx+=step_x) {
                    const point_t p { jau::round_to_int( (float)p0.x + sx ), p0.y + sy };
                    if( !point_action(p) ) {
                        return;
                    }
                }
            } else {
                const int step_x = ( dx >= 0 ) ? 1 : -1;
                const float step_y = (float)dy / std::abs( (float)dx );
                int sx=0;
                float sy=0.0f;
                for(; sx != dx; sx+=step_x, sy+=step_y) {
                    point_t p { p0.x + sx, jau::round_to_int( (float)p0.y + sy ) };
                    if( !point_action(p) ) {
                        return;
                    }
                }
            }
        }
        static void draw(const point_t& p0, const point_t& p1) {
            point_action_t point_draw = [&](const point_t& p) -> bool {
                p.draw();
                return true;
            };
            for_all_points( p0, p1, point_draw );
        }
        void draw() const {
            draw(p0, p1);
        }
    };

    #if 0
    struct aabboxi_t {
        struct pointi_l {
            int x;
            int y;
        };
        /** top left */
        pointi_l tl;
        /** top right */
        pointi_l tr;
        /** bottom left */
        pointi_l bl;
        /** bottom right */
        pointi_l br;


        aabboxi_t(const point2f_t& tl_, const point2f_t& tr_,
                 const point2f_t& bl_, const point2f_t& br_) {
            tl.x = std::floor(tl_.x);
            tl.y = std::ceil(tl_.y);

            tr.x = std::ceil(tr_.x);
            tr.y = std::ceil(tr_.y);

            br.x = std::ceil(br_.x);
            br.y = std::floor(br_.y);

            bl.x = std::floor(bl_.x);
            bl.y = std::floor(bl_.y);
        }

        aabboxi_t(const point2f_t& tl_, const int width, const int height) {
            tl.x = std::floor(tl_.x);
            tl.y = std::ceil(tl_.y);

            // tr.x - tl_.x + 1 = width
            // tr.x = tl_.x - 1 + width
            tr.x = std::ceil(tl_.x + width - 1);
            tr.y = tl.y;

            br.x = tr.x;
            // tl_.y - br.y + 1 = height
            // tl_.y - height + 1 = br.y
            br.y = std::floor(tl_.y - height + 1);

            bl.x = tl.x;
            bl.y = br.y;
        }
        bool on_screen() const {
            const int x0 = cart_to_fb_x( tl.x );
            const int y0 = cart_to_fb_y( tl.y );
            const int x1 = cart_to_fb_x( br.x );
            const int y1 = cart_to_fb_y( br.y );

            // x in [min_x .. max_x] ?
            return 0 <= x0 && x0 <= screen_max_x &&
                    0 <= y0 && y0 <= screen_max_y &&
                    0 <= x1 && x1 <= screen_max_x &&
                    0 <= y1 && y1 <= screen_max_y;
        }

        bool intersects(const aabboxi_t& o) const {
            if( br.x < o.tl.x ||
                br.y > o.tl.y ||
                tl.x > o.br.x ||
                tl.y < o.br.y )
            {
                return false;
            } else {
                return true;
            }
        }

        void draw() const {
            lineseg_t::draw(tl, tl);
            lineseg_t::draw(tr, tr);
            lineseg_t::draw(br, br);
            lineseg_t::draw(bl, bl);
        }

    };
    #endif

    enum class CircleDrawType {
        OUTLINE,
        FILLED,
        BB_INVERTED
    };
    void draw_circle(const int cx, const int cy, const int r, const CircleDrawType mode) {
        const int x1 = cx - r;
        const int y1 = cy - r;
        const int x2 = cx + r;
        const int y2 = cy + r;

        for(int y=y1; y<=y2; ++y) {
            for(int x=x1; x<=x2; ++x) {
                const int dx=x-cx;
                const int dy=y-cy;
                const int rt=jau::round_to_int( std::sqrt( (float)(dx*dx + dy*dy) ) );
                bool draw;
                switch( mode ) {
                    case CircleDrawType::OUTLINE:
                        draw = rt == r;
                        break;
                    case CircleDrawType::FILLED:
                        draw = rt <= r;
                        break;
                    case CircleDrawType::BB_INVERTED:
                        draw = rt >= r;
                        break;
                    default:
                        draw = false;
                        break;
                }
                if( draw) {
                    pixel::set_pixel(x, y);
                }
            }
        }
    }

    static constexpr const bool ROTATE_AT_DRAW = true;

    struct rect_t {
        /** top left */
        point_t tl;
        /** top right */
        point_t tr;
        /** bottom left */
        point_t bl;
        /** bottom right */
        point_t br;
        /** center */
        point_t cx;
        /** direction angle in radians */
        float a;

        rect_t(const point_t& tl_, const int width, const int height, const float radians)
        {
            tl = tl_;
            tr = { tl.x + width - 1, tl.y };
            bl = { tl.x            , tl.y + height - 1};
            br = { tl.x + width - 1, tl.y + height - 1};
            cx = { tl.x + width/2  , tl.y + height/2  };
            if constexpr ( !ROTATE_AT_DRAW ) {
                a = 0.0f;
                rotate(radians);
            } else {
                a = radians;
            }
        }

        rect_t(const point_t& tl_, const int width, const int height) {
            tl = tl_;
            tr = { tl.x + width - 1, tl.y };
            bl = { tl.x            , tl.y + height - 1};
            br = { tl.x + width - 1, tl.y + height - 1};
            cx = { tl.x + width/2  , tl.y + height/2  };
            a = 0.0f;
        }

        void set_top_left(const point_t& p) {
            const int dx = p.x - tl.x;
            const int dy = p.y - tl.y;
            move( dx, dy );
        }

        int width() const {
            const int dx = tr.x - tl.x;
            const int dy = tr.y - tl.y;
            return jau::round_to_int( std::sqrt( (float)(dx*dx + dy*dy) ) );
        }
        int height() const {
            const int dx = bl.x - tl.x;
            const int dy = bl.y - tl.y;
            return jau::round_to_int( std::sqrt( (float)(dx*dx + dy*dy) ) );
        }

        void move_dir(const int d) {
            point_t dir { d, 0 };
            dir.rotate(a, { 0, 0 });
            tl.move(dir.x, dir.y);
            tr.move(dir.x, dir.y);
            bl.move(dir.x, dir.y);
            br.move(dir.x, dir.y);
            cx.move(dir.x, dir.y);
        }

        void move(const int d) {
            tl.move(d);
            tr.move(d);
            bl.move(d);
            br.move(d);
            cx.move(d);
        }

        void move(const int dx, const int dy) {
            tl.move(dx, dy);
            tr.move(dx, dy);
            bl.move(dx, dy);
            br.move(dx, dy);
            cx.move(dx, dy);
        }

        void rotate(const float radians) {
            if constexpr ( !ROTATE_AT_DRAW ) {
                const float cos = std::cos(radians);
                const float sin = std::sin(radians);
                tl.rotate(sin, cos, cx);
                tr.rotate(sin, cos, cx);
                bl.rotate(sin, cos, cx);
                br.rotate(sin, cos, cx);
            }
            a += radians;
        }

        bool on_screen() const {
            const int x0 = pixel::cart_coord.to_fb_x( (float)tl.x );
            const int y0 = pixel::cart_coord.to_fb_y( (float)tl.y );
            const int x1 = pixel::cart_coord.to_fb_x( (float)br.x );
            const int y1 = pixel::cart_coord.to_fb_y( (float)br.y );

            // x in [min_x .. max_x] ?
            return 0 <= x0 && x0 <= pixel::fb_max_x &&
                   0 <= y0 && y0 <= pixel::fb_max_y &&
                   0 <= x1 && x1 <= pixel::fb_max_x &&
                   0 <= y1 && y1 <= pixel::fb_max_y;
        }

        bool intersects(const rect_t& o) const {
            if( br.x < o.tl.x ||
                tl.x > o.br.x ||
                tl.y < o.br.y ||
                br.y > o.tl.y ) {
                return false;
            }
            return true;
        }

        bool intersects(const lineseg_t& l) const {
            bool res = false;
            lineseg_t::point_action_t point_draw = [&](const point_t& p) -> bool {
                res = intersects(p);
                return !res;
            };
            lineseg_t::for_all_points( l.p0, l.p1, point_draw );
            return res;
        }

        bool intersects(const point_t& o) const {
            if( br.x < o.x ||
                tl.x > o.x ||
                tl.y < o.y ||
                br.y > o.y ) {
                return false;
            }
            return true;
        }

        std::string toString() const { return "Rect[tl " + tl.toString() +
                ", sz " + std::to_string(width()) + " x " + std::to_string(height()) + "]"; }

    #if 0
        static void draw(pixel_buffer_t& rend, const point_t& top_left, const point_t& bottom_right) {
            point_t top_right { bottom_right.x, top_left.y };
            point_t bottom_left { top_left.x, bottom_right.y };

            lineseg_t::draw(rend, top_left, top_right);
            lineseg_t::draw(rend, top_right, bottom_right);
            lineseg_t::draw(rend, bottom_right, bottom_left);
            lineseg_t::draw(rend, bottom_left, top_left);
        }
    #endif
    
        void draw() const {
            if constexpr ( !ROTATE_AT_DRAW ) {
                lineseg_t::draw(tl, tr);
                lineseg_t::draw(tr, br);
                lineseg_t::draw(br, bl);
                lineseg_t::draw(bl, tl);
            } else {
                const float cos = std::cos(a);
                const float sin = std::sin(a);
                point_t tl_ = tl;
                point_t tr_ = tr;
                point_t br_ = br;
                point_t bl_ = bl;
                tl_.rotate(sin, cos, cx);
                tr_.rotate(sin, cos, cx);
                bl_.rotate(sin, cos, cx);
                br_.rotate(sin, cos, cx);
                lineseg_t::draw(tl_, tr_);
                lineseg_t::draw(tr_, br_);
                lineseg_t::draw(br_, bl_);
                lineseg_t::draw(bl_, tl_);
            }
        }

    #if 0
        void fill() const {
            rect_t iter = *this;
            int i=0;
            while( iter.width() >= 1 && iter.height() >= 1) {
                if ( false ) {
                    printf("F.%d: %s\n", ++i, iter.toString().c_str());
                }
                iter.draw();
                iter.tl.x += 1;
                iter.tl.y -= 1;
                iter.br.x -= 1;
                iter.br.y += 1;
            }
        }
    #endif

    };

    struct blob_t {
        point_t position;
        int size;

        void move(const int d) { position.move(d); }
        void move(const int dx, const int dy) { position.move(dx, dy); }

        void scale(const int s) { size *= s; }

        bool on_screen() const {
            const int x = pixel::cart_coord.to_fb_x( (float)position.x );
            const int y = pixel::cart_coord.to_fb_y( (float)position.y );
            // x in [min_x .. max_x] ?
            return 0 <= x && x <= pixel::fb_max_x &&
                   0 <= y && y <= pixel::fb_max_y;
        }

        bool intersects(const blob_t& o) const {
            if( position.x + size/2 < o.position.x - o.size/2 ||
                position.x - size/2 > o.position.x + o.size/2 ||
                position.y + size/2 < o.position.y - o.size/2 ||
                position.y - size/2 > o.position.y + o.size/2 ) {
                return false;
            }
            return true;
        }

        bool intersects(const rect_t& o) const {
            if( position.x + size/2 < o.tl.x      ||
                position.x - size/2 > o.br.x  ||
                position.y + size/2 < o.br.y  ||
                position.y - size/2 > o.tl.y      ) {
                return false;
            }
            return true;
        }

        bool intersects(const lineseg_t& l) const {
            bool res = false;
            lineseg_t::point_action_t point_draw = [&](const point_t& p) -> bool {
                res = intersects(p);
                return !res;
            };
            lineseg_t::for_all_points( l.p0, l.p1, point_draw );
            return res;
        }

        bool intersects(const point_t& o) const {
            if( position.x + size/2 < o.x ||
                position.x - size/2 > o.x ||
                position.y + size/2 < o.y ||
                position.y - size/2 > o.y ) {
                return false;
            }
            return true;
        }

        std::string toString() const { return "Blob[" + position.toString() + " x " + std::to_string(size) + "]"; }

        void draw() const {
            point_t p0 = position;
            p0.move( -size/2 );

            point_t p1 = p0;
            p1.move(0, size);
            lineseg_t::draw(p0, p1);

            point_t p2 = p1;
            p2.move(size, 0);
            lineseg_t::draw(p1, p2);

            point_t p3 = p2;
            p3.move(0, -size);
            lineseg_t::draw(p2, p3);

            point_t p4 = p3;
            p4.move(-size, 0);
            lineseg_t::draw(p3, p4);
        }
    };

} // namespace pixel_2i

#endif /*  PIXEL2I_HPP_ */

