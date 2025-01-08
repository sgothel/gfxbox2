#define TRON_HPP_

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
#include <cctype>

#include <pixel/pixel4f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"

using namespace jau;

namespace Tron {
class Motorrad : public pixel::f2::linestrip_t {
public:
    const pixel::f2::point_t sp;
    float angle;
    float velo;
    pixel::f2::point_t head;
    pixel::f2::point_t last;
    pixel::f2::point_t last_last;
    pixel::f2::disk_t body;

    Motorrad(pixel::f2::point_t sp_)
    : sp(sp_), body(head, 5)
    {
        reset();
    }

    void rotate(float angrad) noexcept override {
        // std::cout << "rot.pre " << toString() << std::endl ;
        angle += angrad;
        if(false){
            if( compare(angle, 2*M_PI) >= 0 ) { // p1_angle >= 2*M_PI
                angle = 0;
            } else if( compare(angle, 0) < 0 ) { // p1_angle < 0
                angle = 1.5f * M_PI;
            }
        }
        p_list.push_back(head);
        last_last = last;
        last = head;
        //std::cout << "rot.post " << toString() << std::endl ;
    }

    /**
     *
     * @param dt in seconds
     */
    bool tick(const float dt) noexcept override {
        pixel::f2::point_t p1_move = pixel::f2::point_t::from_length_angle(velo * dt, angle);
        // std::cout << "tick " << p1_move.toString() << " + " << head.toString();
        head += p1_move;
        // std::cout << " = " << head.toString() << std::endl;
        body.center = head;

        return true;
    }

    void reset() noexcept {
        last = sp;
        head = sp;
        last_last = sp;
        body.center = head;
        p_list.clear();
        p_list.push_back(last_last);
        angle = M_PI_2;
        velo = 2.0f / 0.016f; // 2 pixel pro 16 ms
    }

    void draw() const noexcept override {
        pixel::f2::lineseg_t::draw(last, head);
        pixel::f2::lineseg_t::draw(last_last, last);
        pixel::f2::linestrip_t::draw();
        body.draw(true);
    }

    void changeSpeed(float a){
        velo *= a;
    }

    std::string toString() const noexcept override {
        return "Tron[h "+head.toString()+", a "+std::to_string(angle)+", v "+std::to_string(velo)+
                ", tail "+std::to_string(p_list.size())+"]";
    }

    bool intersects(const Motorrad& o) const {
        const pixel::f2::lineseg_t h(head, last);
        const pixel::f2::lineseg_t o_h(o.head, o.last);
        return my_intersects(h) ||
                o.intersects_lineonly(h) ||
                o_h.intersects(h);
    }
private:
    bool my_intersects(const pixel::f2::lineseg_t & o) const noexcept {
        if( p_list.size() < 3 ) {
            return false;
        }
        pixel::f2::point_t p0 = p_list[0];
        for(size_t i=1; i<p_list.size()-1; ++i) {
            const pixel::f2::point_t& p1 = p_list[i];
            const pixel::f2::lineseg_t l(p0, p1);
            if( l.intersects(o) ) {
                return true;
            }
            p0 = p1;
        }
        return false;
    }
};

class peng_t {
public:
    constexpr static const float diag = 1.0f * 25.0f / 2.0f ;
    pixel::f2::vec_t velo; // [m/s]
    pixel::f2::rect_t peng;

    peng_t(const pixel::f2::point_t& p0, const float v, const float angle)
    : velo( pixel::f2::point_t::from_length_angle(v, angle) ),
      peng(p0 + pixel::f2::point_t(-diag/2, +diag/2), diag, diag, angle)
    { }
    peng_t(const pixel::f2::point_t& p0, const pixel::f2::vec_t& v)
    : velo( v ),
      peng(p0 + pixel::f2::point_t(-diag/2, +diag/2), diag, diag, v.angle())
    { }

    bool tick(const float dt) noexcept {
        velo.add(-0.0001f, -0.0001f);
        peng.move( velo * dt );
        return true;
    }

    void draw() const noexcept {
        peng.draw(false);
    }

    void changeSpeed(float a){
        velo *= a;
    }

    bool on_screen(){
        return peng.on_screen();
    }
    bool intersection(const peng_t& o) const {
        return peng.intersects(o.peng);
    }
};

class Panzer : public pixel::f2::linestrip_t {
public:
    constexpr static const float velo_0 = 10.0f; // 10 pixel pro s
    constexpr static const float velo_max = 200.0f; // 10 pixel pro s
    constexpr static const float peng_velo_0 = 100.0f; // 10 pixel pro s
    constexpr static const int peng_inventory_max = 50; // 45 pengs

    constexpr static const float length = 100.0f;
    constexpr static const float width = length*0.6f;
    constexpr static const float barrel_l = length * 0.8f;
    constexpr static const float barrel_w = barrel_l * 0.25f;
    constexpr static pixel::f2::point_t body_tl(const pixel::f2::point_t& sp) noexcept {
        return sp + pixel::f2::vec_t(-width/2, length/2);
    }
    constexpr static pixel::f2::point_t barrel_tl(const pixel::f2::point_t& sp) noexcept {
        return sp + pixel::f2::vec_t(0, barrel_w / 2);
    }

    const pixel::f2::point_t sp;
    float velo;
    // pixel::f2::disk_t body;
    pixel::f2::rect_t body;
    pixel::f2::rect_t barrel;
    std::vector<peng_t> pengs;
    int peng_inventory;

    Panzer(pixel::f2::point_t sp_)
    : sp(sp_), velo(velo_0),
      body(body_tl(sp), width, length, 0),
      barrel( barrel_tl( sp ), barrel_l, barrel_w, 0 ),
      peng_inventory(peng_inventory_max)
    {
        rotate_barrel(M_PI_2);
    }

    void reset(const bool e = true) noexcept {
        peng_inventory = peng_inventory_max;
        velo = velo_0;
        body = pixel::f2::rect_t(body_tl(sp), width, length, 0);
        barrel = pixel::f2::rect_t( barrel_tl( sp ), barrel_l, barrel_w, 0);
        if(e){
            pengs.clear();
        }
        rotate_barrel(M_PI_2);
    }

    const pixel::f2::point_t& center() const noexcept {
        return body.p_center;
    }

    void rotate(float angrad) noexcept override {
        // std::cout << "rot.pre " << toString() << std::endl ;
        body.rotate(angrad, center());
        rotate_barrel(angrad);
        //std::cout << "rot.post " << toString() << std::endl ;
    }
    void rotate_barrel(float angrad) noexcept {
        // std::cout << "rot.pre " << toString() << std::endl ;
        barrel.rotate(angrad, center());
        //std::cout << "rot.post " << toString() << std::endl ;
    }

    void peng() noexcept {
        if(peng_inventory > 0){
            pixel::f2::point_t p0 = center() +
                    pixel::f2::point_t::from_length_angle(barrel_l + 0.001f, barrel.dir_angle);
            pengs.push_back( peng_t(p0, velo + peng_velo_0, barrel.dir_angle) );
            --peng_inventory;
        }
    }
    size_t peng_count() const noexcept { return pengs.size(); }

    /**
     *
     * @param dt in seconds
     */
    bool tick(const float dt) noexcept override {
        if( velo > 0.01f ) {
            velo -= 0.01f;
        }
        pixel::f2::point_t p1_move = pixel::f2::point_t::from_length_angle(velo * dt, body.dir_angle + M_PI_2);
        // std::cout << "tick " << p1_move.toString() << " + " << head.toString();
        body.move(p1_move);
        barrel.move(p1_move);
        // std::cout << " = " << head.toString() << std::endl;

        if( !body.on_screen()) {
            pixel::f2::point_t new_center = center();
            if( new_center.x < pixel::cart_coord.min_x() ) {
                new_center.x = pixel::cart_coord.max_x() - width;
            }
            if( new_center.x > pixel::cart_coord.max_x() ) {
                new_center.x = pixel::cart_coord.min_x() + width;
            }
            if( new_center.y < pixel::cart_coord.min_y() ) {
                new_center.y = pixel::cart_coord.max_y() - width;
            }
            if( new_center.y > pixel::cart_coord.max_y() ) {
                new_center.y = pixel::cart_coord.min_y() + width;
            }
            pixel::f2::vec_t m = new_center - center();
            body.move(m);
            barrel.move(m);
        }

        for(auto it = pengs.begin(); it != pengs.end(); ) {
            peng_t& p = *it;
            if(p.on_screen() && p.velo.length_sq() > 0){
                p.tick(dt);
                ++it;
            } else {
                it = pengs.erase(it);
            }
        }
        return true;
    }

    void draw() const noexcept override {
        barrel.draw(false);
        body.draw(false);
        // std::cout << "b " << body.toString() << ", r " << rect.toString() << std::endl;
        // std::vector<peng_t>::iterator it ;-)
        for(auto it = pengs.begin(); it != pengs.end(); ++it) {
            (*it).draw();
        }
    }

    bool on_screen(){ return body.on_screen(); }

    void changeSpeed(float a){
        velo *= a;
        if( a > 1 && velo < velo_0 ) {
            velo = velo_0;
        }
        if( velo > velo_max ) {
            velo = velo_max;
        } else if( velo < -velo_max ) {
            velo = -velo_max;
        }
    }

    bool hit(Panzer o){
        for(auto it = pengs.begin(); it != pengs.end(); ) {
            peng_t& p = *it;
            if( o.body.intersects(p.peng)){
                it = pengs.erase(it);
                return true;
            } else {
                ++it;
            }
        }
        return false;
    }

    std::string toString() const noexcept override {
        return "Panzer[c "+center().toString()+", a "+std::to_string(barrel.dir_angle)+
                ", v "+std::to_string(velo)+
                ", tail "+std::to_string(p_list.size())+"]";
    }
};
}
