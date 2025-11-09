#include <algorithm>
#include <bits/types/siginfo_t.h>
#include <c++/12/numbers>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cmath>

#include <cstdlib>
#include <memory>
#include <numeric>
#include <numbers>
#include <ostream>
#include <pixel/pixel4f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include <pixel/pixel.hpp>
#include <physics.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <jau/float_si_types.hpp>
#include <string>
#include <thread>

#include <random>
#include <iostream>
#include <vector>
#include <numbers>

using namespace pixel;
using namespace jau;
using namespace jau::si_f32_literals;

const int car_id_1 = 1;
const int car_id_2 = 2;
const si_velo_f32 start_max_velo = 50_km_h;
si_velo_f32 max_velo = start_max_velo;
si_velo_f32 max_train_velo = 100_km_h;
std::vector<f2::geom_ref_t> aus_geom_list;
std::vector<f2::geom_ref_t> loc_geom_list;
std::vector<f2::geom_ref_t> schienen_geom_list;

/*
class Schienen {
  public:
    si_length_f32 m_length;
    si_length_f32 m_width;
    f2::point_t m_tl;
    
    Schienen(f2::point_t tl, si_length_f32 length)
    : m_length(length), m_width(30_dm), m_tl(tl){}
    
    void draw(){
        // FIXME: Either add track elements to global list only once, e.g. in ctor
        // FIXME: or maintain own f2::geom_list_t instead of using global gobjects (-> ctor)! 
        f2::geom_list_t &list = f2::gobjects();
        {
            f2::rect_ref_t r = std::make_shared<f2::rect_t>(m_tl, 1_dm, m_length);
            r->draw(false);
            list.push_back(r);
        }
        {
            f2::rect_ref_t r = std::make_shared<f2::rect_t>(f2::point_t(m_tl.x + (m_width - 1_dm), 
                                                                        m_tl.y), 
                                                            1_dm, m_length);
            r->draw(false);
            list.push_back(r);
        }
        si_length_f32 l = 0;
        while(l < m_length) {
            f2::rect_ref_t r = std::make_shared<f2::rect_t>(f2::point_t(m_tl.x + 1_dm, m_tl.y - l), 
            m_width - 2_dm, 50_cm);
            r->draw(false);
            list.push_back(r);
            l += 1_m;
        }
    }
};

class Waggon {
  public:
    si_length_f32 length;
    si_length_f32 width;
    f2::rect_ref_t body;
    f2::triangle_ref_t tria1;
    f2::triangle_ref_t tria2;
    f2::lineseg_ref_t door1;
    f2::lineseg_ref_t door2;
    si_velo_f32 velocity;
    si_angle_f32 angle;
    int load = 0;
    Waggon(){
        reset();
    }
    
    void reset(){
        length = 8_m;
        width = 4_m;
        f2::point_t tl_body;
        tl_body = {-width / 2, cart_coord.max_y() - width * 2 - 95_dm};
        body = std::make_shared<f2::rect_t>(tl_body, width, length);
        tria1 = std::make_shared<f2::triangle_t>(
                f2::point_t(tl_body.x + width / 4, tl_body.y), 
                f2::point_t(tl_body.x + width * 0.75f, tl_body.y), 
                f2::point_t(tl_body.x + width * 0.5f, tl_body.y + width / 2));
        tria2 = std::make_shared<f2::triangle_t>(
                f2::point_t(body->m_bl.x + width / 4, body->m_bl.y), 
                f2::point_t(body->m_bl.x + width * 0.75f, body->m_bl.y), 
                f2::point_t(body->m_bl.x + width * 0.5f, body->m_bl.y - width / 2));
        door1 = std::make_shared<f2::lineseg_t>(
                f2::point_t(body->m_tl.x, body->m_tl.y - 1.0f/4 * length), 
                f2::point_t(body->m_bl.x, body->m_bl.y + 1.0f/4 * length));
        door2 = std::make_shared<f2::lineseg_t>(
                f2::point_t(body->m_tr.x, body->m_tr.y - 1.0f/4 * length), 
                f2::point_t(body->m_br.x, body->m_br.y + 1.0f/4 * length));
        velocity = max_train_velo;
        angle = 90_deg;
        aus_geom_list.push_back(door1);
        aus_geom_list.push_back(door2);
        loc_geom_list.push_back(body);
        loc_geom_list.push_back(tria1);
        loc_geom_list.push_back(tria2);
    }
    
    void change_velocity(const si_velo_f32 v){
        velocity = std::min(velocity + v, max_train_velo);
        velocity = std::max(velocity, 0.0f);
    }
    
    void rotate(const float radians){
        angle += radians;
        body->rotate(radians, body->p_center);
        tria1->rotate(radians, body->p_center);
        tria2->rotate(radians, body->p_center);
        door1->rotate(radians, body->p_center);
        door2->rotate(radians, body->p_center);
    }
    
    void move(f2::vec_t vec){
        body->move(vec);
        tria1->move(vec);
        tria2->move(vec);
        door1->p0 += vec;
        door1->p1 += vec;
        door2->p0 += vec;
        door2->p1 += vec;
        if( body->p_center.x < pixel::cart_coord.min_x() ) {
            move({cart_coord.max_x()-body->p_center.x, 0.0f});
        }
        if( body->p_center.x > pixel::cart_coord.max_x() ) {
            move({cart_coord.min_x()-body->p_center.x, 0.0f});
        }
        if( body->p_center.y < pixel::cart_coord.min_y() ) {
            move({0.0f, cart_coord.max_y()-body->p_center.y});
        }
        if( body->p_center.y > pixel::cart_coord.max_y() ) {
            move({0.0f, cart_coord.min_y()-body->p_center.y});
        }
    }
    
    void tick(const float dt){
        f2::vec_t vec = f2::vec_t::from_length_angle(velocity, angle);
        move(vec * dt);
    }

    void draw(){
        body->draw(false);
        tria1->draw(false);
        tria2->draw(false);
        set_pixel_color(255, 0, 0, 255);
        door1->draw();
        door2->draw();
        set_pixel_color(0, 0, 0, 0);
    }
};

// FIXME: Zug or Train is a noun denoting the whole train,
// e.g. Triebwagen + Wagons (Locomotive + Waggons) 
class Automotora {
  public:
    si_length_f32 length;
    si_length_f32 width;
    f2::rect_ref_t body;
    f2::triangle_ref_t tria1;
    f2::triangle_ref_t tria2;
    f2::lineseg_ref_t door1;
    f2::lineseg_ref_t door2;
    si_velo_f32 velocity;
    si_angle_f32 angle;
    Automotora(){
        reset();
    }
    
    void reset(){
        length = 8_m;
        width = 4_m;
        f2::point_t tl_body;
        tl_body = {-width / 2, cart_coord.max_y() - width * 2};
        body = std::make_shared<f2::rect_t>(tl_body, width, length);
        tria1 = std::make_shared<f2::triangle_t>(
                tl_body, 
                f2::point_t(tl_body.x + width, tl_body.y), 
                f2::point_t(tl_body.x + width * 0.5f, tl_body.y + width));
        tria2 = std::make_shared<f2::triangle_t>(
                f2::point_t(body->m_bl.x + width / 4, body->m_bl.y), 
                f2::point_t(body->m_bl.x + width * 0.75f, body->m_bl.y), 
                f2::point_t(body->m_bl.x + width * 0.5f, body->m_bl.y - width / 2));
        door1 = std::make_shared<f2::lineseg_t>(
                f2::point_t(body->m_tl.x, body->m_tl.y - 1.0f/3 * length), 
                f2::point_t(body->m_bl.x, body->m_bl.y + 1.0f/3 * length));
        door2 = std::make_shared<f2::lineseg_t>(
                f2::point_t(body->m_tr.x, body->m_tr.y - 1.0f/3 * length), 
                f2::point_t(body->m_br.x, body->m_br.y + 1.0f/3 * length));
        velocity = max_train_velo;
        angle = 90_deg;
        aus_geom_list.push_back(door1);
        aus_geom_list.push_back(door2);
        loc_geom_list.push_back(body);
        loc_geom_list.push_back(tria1);
        loc_geom_list.push_back(tria2);
    }
    
    void change_velocity(const si_velo_f32 v){
        velocity = std::min(velocity + v, max_train_velo);
        velocity = std::max(velocity, 0.0f);
    }
    
    void rotate(const float radians){
        angle += radians;
        body->rotate(radians, body->p_center);
        tria1->rotate(radians, body->p_center);
        tria2->rotate(radians, body->p_center);
        door1->rotate(radians, body->p_center);
        door2->rotate(radians, body->p_center);
    }
    
    void move(f2::vec_t vec){
        body->move(vec);
        tria1->move(vec);
        tria2->move(vec);
        door1->p0 += vec;
        door1->p1 += vec;
        door2->p0 += vec;
        door2->p1 += vec;
        if( body->p_center.x < pixel::cart_coord.min_x() ) {
            move({cart_coord.max_x()-body->p_center.x, 0.0f});
        }
        if( body->p_center.x > pixel::cart_coord.max_x() ) {
            move({cart_coord.min_x()-body->p_center.x, 0.0f});
        }
        if( body->p_center.y < pixel::cart_coord.min_y() ) {
            move({0.0f, cart_coord.max_y()-body->p_center.y});
        }
        if( body->p_center.y > pixel::cart_coord.max_y() ) {
            move({0.0f, cart_coord.min_y()-body->p_center.y});
        }
    }
    
    void tick(const float dt){
        f2::vec_t vec = f2::vec_t::from_length_angle(velocity, angle);
        move(vec * dt);
    }
    
    void draw(){
        body->draw(false);
        tria1->draw(false);
        tria2->draw(false);
        set_pixel_color(255, 0, 0, 255);
        door1->draw();
        door2->draw();
        set_pixel_color(0, 0, 0, 0);
    }
};

// FIXME: Zug or Train is a noun denoting the whole train,
// e.g. Triebwagen + Wagons (Locomotive + Waggons) 
class Train {
  public:
    Waggon waggon;
    Automotora automotora;
    Train()
    : waggon(), automotora(){}
    
    void reset(){
        waggon.reset();
        automotora.reset();
    }
    
    void change_velocity(si_velo_f32 v){
        velocity = std::min(velocity + v, max_train_velo);
        velocity = std::max(velocity, 0.0f);
    }
    
    si_velo_f32 velocity = automotora.velocity;
    
    void move(f2::vec_t vec){
        waggon.move(vec);
        automotora.move(vec);
    }
    
    si_time_f32 t = 10_s;
    
    void tick(const float dt){
        if(t <= 0_s){
            change_velocity(100_km_h * dt);
        }
        waggon.velocity = velocity;
        automotora.velocity = velocity;
        waggon.tick(dt);
        automotora.tick(dt);
    }
    
    void draw(){
        waggon.draw();
        automotora.draw();
    }
};

Train train;

Schienen gleis = {{-train.waggon.width / 2, cart_coord.max_y()}, 
                       cart_coord.height()};

std::vector<f2::rect_t> bahnhoefe;

void make_bahnhoefe(){
    const si_length_f32 length = 5_m;
    const si_length_f32 width = 5_m;
    {
        bahnhoefe.emplace_back(f2::point_t(-width / 2, cart_coord.max_y()), width, length);
        aus_geom_list.emplace_back(std::make_shared<f2::rect_t>(
                                        f2::point_t(-width / 2, cart_coord.max_y()), 
                                        width, length));
    }
    {
        bahnhoefe.emplace_back(f2::point_t(-width / 2, length / 2), width, length);
        aus_geom_list.emplace_back(std::make_shared<f2::rect_t>(
                                        f2::point_t(-width / 2, length / 2), 
                                        width, length));
    }
    {
        bahnhoefe.emplace_back(f2::point_t(-width / 2, cart_coord.min_y() + length), width, length);
        aus_geom_list.emplace_back(std::make_shared<f2::rect_t>(
                                        f2::point_t(-width / 2, cart_coord.min_y() + length), 
                                        width, length));
    }
}
*/

class Tunnel {
  private:
    const int tex_height = 30;

  public:
    size_t m_id;
    f2::disk_t d1;
    f2::disk_t d2;
    Tunnel(const size_t id, f2::point_t p0, f2::point_t p1, const si_length_f32 d)
    : m_id(id), d1(p0, d/2), d2(p1, d/2) {}
    Tunnel()               = default;
    Tunnel(const Tunnel &) = default;
    Tunnel(Tunnel &&)      = default;
    Tunnel& operator=(const Tunnel& o) {
        m_id = o.m_id;
        d1 = o.d1;
        d2 = o.d2;
        return *this;
    }
    void draw() {
        d1.draw(false);
        d2.draw(false);
        {
            f2::point_t tl_tex = d1.box().bl;
            pixel::texture_ref tex = make_text(tl_tex, 0, {0, 0, 0, 255}, tex_height, "%d", m_id);
            tex->draw_fbcoord(0, 0);
        }
        {
            f2::point_t tl_tex = d2.box().bl;
            pixel::texture_ref tex = make_text(tl_tex, 0, {0, 0, 0, 255}, tex_height, "%d", m_id);
            tex->draw_fbcoord(0, 0);
        }
    }
};

std::vector<Tunnel> tunnels;

class Car {
  public:
    f2::point_t tl1 = {
        cart_coord.min_x() + 3 * cart_coord.width() / 4, cart_coord.min_y() + 3_m};
    f2::point_t tl2 = {
        cart_coord.min_x() +     cart_coord.width() / 4, cart_coord.min_y() + 3_m};
  private:
    const std::string m_start_bdv = ":-)";
    const si_velo_f32 m_start_velocity = 0_km_h;
    const si_angle_f32 m_start_angle = M_PI_2;
    const si_time_f32 m_start_fuse01 = 10_s;
    const si_time_f32 m_start_fuse02 = 5_s;
    const float m_start_stability = 1000.0f;
    const float m_start_tank = 250.0f;
    const float m_start_money = 1000.0f;
  public:
    const int m_id;
    std::string m_bdv;
    si_velo_f32 m_velocity;
    si_angle_f32 m_angle;
    si_time_f32 m_fuse01;
    si_length_f32 m_length;
    si_length_f32 m_width;
    f2::rect_t rect_body;
    f2::circle_seg_t circle_seg_body;
    std::vector<f2::rect_t> wheels;
    std::vector<f2::geom_t> body;
    float m_stability;
    float m_tank;
    bool tankt;
    bool grab_tunnel;
    si_time_f32 tunnel;
    f2::rect_t parkplatz;
    float max_car_velo;
    float m_money;
    bool reparing;
    int m_life;
    f2::aabbox_t aabbox;

    Car(const int id)
    : m_id(id), m_bdv(m_start_bdv), m_velocity(m_start_velocity), 
      m_angle(m_start_angle), m_fuse01(m_start_fuse01), m_length(25_dm), m_width(16_dm), 
      m_stability(m_start_stability), m_tank(m_start_tank), tankt(false),
      grab_tunnel(false), tunnel(10_min), max_car_velo(500_km_h), m_money(m_start_money), 
      reparing(false), m_life(3) {
        if(id == car_id_1){
            rect_body = {tl1, m_width, m_length*0.75f};
        } else if(id == car_id_2){
            rect_body = {tl2, m_width, m_length*0.75f};
        }
        circle_seg_body = {
            f2::point_t(rect_body.m_tl.x + m_width / 2, rect_body.m_tl.y), 
            m_width / 2, 0, M_PI};
        {
            const si_length_f32 wheel_width = (float)(m_width * 0.25);
            const si_length_f32 wheel_length = (float)(wheel_width * 2);
            {
                f2::rect_t wheeltl = {
                    {rect_body.m_tl.x - wheel_width, rect_body.m_tl.y}, 
                    wheel_width, wheel_length};
                wheels.push_back(wheeltl);
            }
            {
                f2::rect_t wheeltr = {rect_body.m_tr, wheel_width, wheel_length};
                wheels.push_back(wheeltr);
            }
            {
                f2::rect_t wheelbl = {
                    {rect_body.m_bl.x - wheel_width, rect_body.m_bl.y + wheel_length}, 
                    wheel_width, wheel_length};
                wheels.push_back(wheelbl);
            }
            {
                f2::rect_t wheelbr = {
                    {rect_body.m_br.x, rect_body.m_br.y + wheel_length}, 
                    wheel_width, wheel_length};
                wheels.push_back(wheelbr);
            }
        }
        m_length = m_length*0.75f + m_width / 2;
        parkplatz = {{rect_body.m_tl.x - 1_m, rect_body.m_tl.y + m_width / 2 + 125_cm}, 
                     m_width + 2_m, m_length + 25_dm};
        aabbox.resize(circle_seg_body.box());
        aabbox.resize(rect_body.box());
        for(f2::rect_t &wheel : wheels){
           aabbox.resize(wheel.box());
        }
    }
    
    std::string start_bdv(){ return m_start_bdv; }
    si_velo_f32 start_velo(){ return m_start_velocity; }
    si_angle_f32 start_angle(){ return m_start_angle; }
    float start_fuse01(){ return m_start_fuse01; }
    float start_fuse02(){ return m_start_fuse02; }
    float start_stability(){ return m_start_stability; }
    float start_tank(){ return m_start_tank; }
    float start_money(){ return m_start_money; }
    int start_life(){ return 3; }
    bool is_car_explodet(){
        return m_life <= 0;
    }
    si_time_f32 noTankAndVeloTime = 10_s; // you can have 10 sec long no velo and tank
    si_time_f32 bt = 0_s;
    si_time_f32 st = 30_s;
    void reset(){
        m_bdv = m_start_bdv;
        m_velocity = m_start_velocity; 
        m_angle = m_start_angle;
        m_fuse01 = m_start_fuse01;
        m_stability = m_start_stability; 
        m_length = 25_dm;
        m_width = 16_dm;
        m_tank = m_start_tank;
        tankt = false;
        grab_tunnel = false;
        tunnel = 1_min;
        max_car_velo = 500_km_h;
        reparing = false;
        noTankAndVeloTime = 10_s;
        bt = 0;
        st = 30_s;
        if(m_id == car_id_1){
            rect_body = {tl1, m_width, (float)(m_length*0.75)};
        } else if(m_id == car_id_2){
            rect_body = {tl2, m_width, (float)(m_length*0.75)};
        }
        circle_seg_body = {
            f2::point_t(rect_body.m_tl.x + m_width / 2, rect_body.m_tl.y), 
            m_width / 2, 0, M_PI};
        {
            wheels.clear();
            const si_length_f32 wheel_width = (float)(m_width * 0.25);
            const si_length_f32 wheel_length = (float)(wheel_width * 2);
            {
                f2::rect_t wheeltl = {
                    {rect_body.m_tl.x - wheel_width, rect_body.m_tl.y}, 
                    wheel_width, wheel_length};
                wheels.push_back(wheeltl);
            }
            {
                f2::rect_t wheeltr = {rect_body.m_tr, wheel_width, wheel_length};
                wheels.push_back(wheeltr);
            }
            {
                f2::rect_t wheelbl = {
                    {rect_body.m_bl.x - wheel_width, rect_body.m_bl.y + wheel_length}, 
                    wheel_width, wheel_length};
                wheels.push_back(wheelbl);
            }
            {
                f2::rect_t wheelbr = {
                    {rect_body.m_br.x, rect_body.m_br.y + wheel_length}, 
                    wheel_width, wheel_length};
                wheels.push_back(wheelbr);
            }
        }
    }
    
    bool intersects(const f2::rect_t &o) const noexcept {
        if(m_life <= 0){
            return false;
        }
        bool result = false;
        if(grab_tunnel){
            return false;
        }
        result = rect_body.intersects(o);
        for(const f2::rect_t &w1 : wheels){
            result = result || w1.intersects(o);
        }
        result = result || circle_seg_body.intersects(o);
        return result;
    }

    bool intersects(const f2::geom_t &o) const noexcept {
        if(m_life <= 0){
            return false;
        }
        bool result = false;
        if(grab_tunnel){
            return false;
        }
        result = rect_body.intersects(o);
        for(const f2::rect_t &w1 : wheels){
            result = result || w1.intersects(o);
        }
        result = result || circle_seg_body.intersects(o);
        return result;
    }

    bool all_intersects(const f2::rect_t &o) const noexcept {
        if(m_life <= 0){
            return false;
        }
        bool result = false;
        if(grab_tunnel){
            return false;
        }
        result = rect_body.intersects(o);
        for(const f2::rect_t &w1 : wheels){
            result = w1.intersects(o);
        }
        result = circle_seg_body.intersects(o);
        return result;
    }

    bool intersects(const Car &o) const noexcept {
        if(m_life <= 0){
            return false;
        }
        bool result = false;
        if(grab_tunnel){
            return false;
        }
        if(o.grab_tunnel){
            return false;
        }
        result = rect_body.intersects(o.rect_body);
        for(const f2::rect_t &w1 : wheels){
            for(const f2::rect_t &w2 : o.wheels){
                result = result || w1.intersects(w2);
            }
        }
        result = result || circle_seg_body.intersects(o.circle_seg_body);
        return result;
    }
    
    bool intersection(f2::vec_t &reflect_out, const Car &o) const noexcept {
        if(m_life <= 0){
            return false;
        }
        if(!intersects(o)){
            return false;
        }
        if(rect_body.intersects(o.rect_body)){
            f2::vec_t v;
            rect_body.intersection(reflect_out, v, v, {o.rect_body.m_tl, o.rect_body.m_tr});
            rect_body.intersection(reflect_out, v, v, {o.rect_body.m_tl, o.rect_body.m_bl});
            rect_body.intersection(reflect_out, v, v, {o.rect_body.m_tr, o.rect_body.m_br});
            rect_body.intersection(reflect_out, v, v, {o.rect_body.m_bl, o.rect_body.m_br});
            return true;
        }
        return true;
    }
    
    /*
    bool intersects(const Train &o) const noexcept {
        return intersects(*o.automotora.body) || intersects(*o.waggon.body);
    }
    */
    
    bool inside(const f2::rect_t &o) const noexcept {
        f2::point_t tl = {aabbox.bl.x, aabbox.tr.y};
        f2::point_t tr = aabbox.tr;
        f2::point_t br = {aabbox.tr.x, aabbox.bl.y};
        f2::point_t bl = aabbox.bl;
        return o.m_tl.y >= tl.y &&
               o.m_tl.x <= tl.x &&
               o.m_tr.y >= tr.y &&
               o.m_tr.x >= tr.x &&
               o.m_br.y <= br.y &&
               o.m_br.x >= br.x &&
               o.m_bl.y <= bl.y &&
               o.m_bl.x <= bl.x;
    }
    
    void change_velocity(const float m_pro_s, const int vorzeichen=1){
        if(m_life <= 0){
            return;
        }
        if(m_tank > 0){
            m_velocity += m_pro_s*(float)vorzeichen;
            m_tank -= std::abs(m_pro_s/2);
        }
    }
    void rotate(const float adeg){
        if(m_life <= 0){
            return;
        }
        if(adeg > 45){
            return;
        }
        si_angle_f32 rad = adeg_to_rad(adeg);
        m_angle += rad;
        rect_body.rotate(rad, rect_body.p_center);
        for(f2::rect_t &wheel : wheels){
            wheel.rotate(rad, rect_body.p_center);
        }
        circle_seg_body.rotate(rad, rect_body.p_center);
        if(m_angle > std::numbers::pi_v<float>*2){
            m_angle = m_angle - std::numbers::pi_v<float>*2;
        } else if(m_angle < (-std::numbers::pi_v<float>*2)){
            m_angle = m_angle + std::numbers::pi_v<float>*2;
        }
    }
    
    void move(f2::vec_t vec){
        rect_body.move(vec);
        for(f2::rect_t &wheel : wheels){
            wheel.move(vec);
        }
        circle_seg_body.move(vec);
        if( rect_body.p_center.x < pixel::cart_coord.min_x() ) {
            move({cart_coord.max_x()-rect_body.p_center.x, 0.0f});
        }
        if( rect_body.p_center.x > pixel::cart_coord.max_x() ) {
            move({cart_coord.min_x()-rect_body.p_center.x, 0.0f});
        }
        if( rect_body.p_center.y < pixel::cart_coord.min_y() ) {
            move({0.0f, cart_coord.max_y()-rect_body.p_center.y});
        }
        if( rect_body.p_center.y > pixel::cart_coord.max_y() ) {
            move({0.0f, cart_coord.min_y()-rect_body.p_center.y});
        }
    }
    void tick(const float dt){
        if(m_life <= 0){
            return;
        }
        f2::vec_t m_velo_vec = f2::vec_t::from_length_angle(m_velocity, m_angle);
        move(m_velo_vec * dt);
        if(m_fuse01 <= 0 && m_money <= 100){
            --m_life;
            reset();
        } else if(m_fuse01 <= 0 && m_money >= 100){
            m_money -= 100;
            reset();
        } else if(std::abs(m_velocity) <= max_velo){
            m_bdv = ":-)";
        } else if(std::abs(m_velocity) <= max_velo*1.10){
            m_bdv = ":-|";
        } else if(std::abs(m_velocity) > max_velo*1.10){
            m_bdv = ":-(";
            m_fuse01 -= 1_s * dt;
        }
        if(m_stability <= 0){
            --m_life;
            reset();
        }
        if(std::abs(m_velocity) > max_car_velo){
            m_stability -= (std::abs(m_velocity) - max_car_velo) / 10 * dt;
            max_car_velo -= (std::abs(m_velocity) - max_car_velo) / 10 * dt;
        }
        if(m_velocity-1_km_h * dt >= 0){
            m_velocity -= 1_km_h * dt;
        } else if(m_velocity + 1_km_h * dt <= 0){
            m_velocity += 1_km_h * dt;
        } else {
            m_velocity = 0;
        }
        if(noTankAndVeloTime <= 0){
            --m_life;
            reset();
        }
        if(((m_tank <= 0 && m_velocity <= 0) || m_money < 0) && noTankAndVeloTime > 0){
            noTankAndVeloTime -= dt;
        }
        if(tankt && m_money > 0){
            m_tank += 50 * dt;
            m_money = m_money - 50 * dt;
        }
        const float max_tank = m_start_tank;
        if(m_tank > max_tank){
            m_tank = max_tank;
            tankt = false;
        }
        if(reparing && m_money > 0){
            m_stability += 5 * dt;
            max_car_velo += 5 * dt;
            m_money = m_money - 10 * dt;
        }
        if(m_stability > m_start_stability){
            ++m_life;
            m_stability = m_stability - m_start_stability;
        }
        static bool last_grab_tunnel = false;
        static f2::point_t anfang_p;
        static Tunnel next_tunnel;
        if(grab_tunnel){
            if(!last_grab_tunnel){
                anfang_p = rect_body.p_center;
            }
            next_tunnel = Tunnel(tunnels.size(), anfang_p, rect_body.p_center, m_length);
            tunnel -= dt;
            m_money += 100 * dt;
            m_velocity *= 0.99f;
        } else {
            if(last_grab_tunnel){
                tunnels.push_back(next_tunnel);
            }
        }
        last_grab_tunnel = grab_tunnel;
        if(bt > 1_s){
            m_stability -= dt;
        }
        if(m_velocity == 0){
            st -= dt;
        }
        if(m_velocity != 0){
            st = 30_s;
        }
        if(st <= 0){
            --m_life;
            st = 30_s;
        }
        /*
        for(const f2::geom_ref_t &g : loc_geom_list){
            if(intersects(*g)){
                for(const f2::geom_ref_t &aug : aus_geom_list){
                    if(!intersects(*aug)){
                        for(f2::rect_t &bahnhof : bahnhoefe){
                            if(!inside(bahnhof) || !inside(*train.automotora.body) || 
                            !inside(*train.waggon.body)){
                                m_stability -= m_velocity * dt;
                            }
                        }
                    }
                }
            }
        }
        
        if((inside(*train.automotora.body) || inside(*train.waggon.body))){
            m_velocity = train.velocity;
        }
        */
    }
    void draw(){
        parkplatz.draw(false);
        if(is_car_explodet()){
            return;
        }
        if(grab_tunnel){
            return;
        }
        rect_body.draw(false);
        circle_seg_body.draw();
        for(f2::rect_t &r : wheels){
            r.draw(false);
        }
    }
    
    f2::point_t make_point(f2::point_t p, const float radians, const float length) {
        const float x = length * cos(radians);
        const float y = length * sin(radians);
        f2::point_t p1 = {x, y};
        return p1 + p;
    }
};

const float space_height = 100_m;

void mainloop() {
    static Car car1(car_id_1);
    static Car car2(car_id_2);
    static const f4::vec_t text_color = {0, 0, 0, 0};
    static const int text_height = 30;
    static uint64_t t_last = getElapsedMillisecond(); // [ms]
    static input_event_t event;
    static std::vector<texture_ref> text_list;
    static float angle_step = 5;
    static float velo_step = 0.5f;
    static si_time_f32 start_d_time = 1_min;
    static si_time_f32 dtime = start_d_time;
    static std::vector<float> max_velos = {  10_km_h, 30_km_h, 50_km_h, 70_km_h, 90_km_h, 
                                            100_km_h, 120_km_h, 240_km_h, 500_km_h, 600_km_h, 
                                            700_km_h, 800_km_h, 850_km_h, 900_km_h, 950_km_h, 
                                            1000_km_h };
    static f2::rect_t tankstelle = {{cart_coord.min_x(), 0}, 4_m, 5_m};
    static f2::rect_t werkstatt = {{cart_coord.max_x() - 4_m, 0}, 4_m, 5_m};
    static bool parken;
    static int m = 0;
    //static bool b = false;
    static f2::rect_t ba;

    const pixel::f2::point_t tl_text(pixel::cart_coord.min_x(), pixel::cart_coord.max_y());
    const uint64_t t1 = getElapsedMillisecond(); // [ms]
    const float dt = (float)( t1 - t_last ) / 1000.0f; // [s]
    t_last = t1;

    bool animating = !event.paused();
    if(getElapsedMillisecond() - (long double)10_min*1000*m >= 10_min*1000){
        ++m;
        car1.m_money += car1.start_money();
        car2.m_money += car2.start_money();
    }
    while(handle_one_event(event)){
        if( event.pressed_and_clr( input_event_type_t::WINDOW_CLOSE_REQ ) ) {
            printf("Exit Application\n");
            #if defined(__EMSCRIPTEN__)
                emscripten_cancel_main_loop();
            #else
                exit(0);
            #endif
        } else if( event.pressed_and_clr( input_event_type_t::WINDOW_RESIZED ) ) {
            pixel::cart_coord.set_height(-space_height/2.0f, space_height/2.0f);
            tankstelle = {{cart_coord.min_x(), 0}, 4_m, 5_m};
            werkstatt = {{cart_coord.max_x() - 4_m, 0}, 4_m, 5_m};
        }
        animating = !event.paused();
        if(event.released_and_clr(input_event_type_t::RESET)){
            aus_geom_list.clear();
            car1.reset();
            car2.reset();
            car1.m_tank = car1.start_tank();
            car2.m_tank = car2.start_tank();
            car1.m_money = car1.start_money();
            car2.m_money = car2.start_money();
            car1.m_life = car1.start_life();
            car2.m_life = car2.start_life();
            max_velo = start_max_velo;
            loc_geom_list.clear();
            schienen_geom_list.clear();
            // train.reset();
        }
        if(event.pressed(input_event_type_t::P1_ACTION1) && 
           car1.all_intersects(werkstatt)){
            if(event.released_and_clr(input_event_type_t::P1_ACTION3)){
               car1.reparing = !car1.reparing;
            }
        } else if(event.pressed(input_event_type_t::P2_ACTION1) && 
           car2.all_intersects(werkstatt)){
            if(event.released_and_clr(input_event_type_t::P2_ACTION3)){
               car2.reparing = !car2.reparing;
            }
        } else if(event.released_and_clr(input_event_type_t::P1_ACTION3) && 
                  car1.all_intersects(tankstelle)){
            car1.tankt = !car1.tankt;
        } else if(event.released_and_clr(input_event_type_t::P2_ACTION3) && 
                  car2.all_intersects(tankstelle)){
            car2.tankt = !car2.tankt;
        } else if(event.released_and_clr(input_event_type_t::P1_ACTION1)){
            car1.bt = 0;
        } else if(event.released_and_clr(input_event_type_t::P2_ACTION1)){
            car2.bt = 0;
        }
    }
    {
        text_list.push_back(pixel::make_text(tl_text, 0, text_color, text_height,
        "td %s, fps %2.2f, dt %.2f ms, %s, d_time %2d, car1(velo %.2f km/h, angle %.2f grad, %s, "
        "fuse %.2f s, stability %.2f, tank %.2f, tunnel %.2f, max velo %.2f, money %.2f, "
        "life %d), ",
                to_decstring(t1/1000, ',', 9).c_str(), gpu_avg_fps(), (dt*1000),
                animating?"animating":"paused", car1.m_velocity * 3.6f, 
                rad_to_adeg(car1.m_angle), (int)dtime, car1.m_bdv.c_str(), car1.m_fuse01, 
                car1.m_stability, car1.m_tank, car1.tunnel, car1.max_car_velo * 3.6f, car1.m_money, 
                car1.m_life));
        text_list.push_back(pixel::make_text(tl_text, 1, text_color, text_height,
        "car2(velo %.2f km/h, angle %.2f grad, %s, fuse %.2f s, stability %.2f, tank %.2f, "
        "tunnel %.2f, max velo %.2f, money %.2f, life %d)",
                car2.m_velocity * 3.6f, rad_to_adeg(car2.m_angle), car2.m_bdv.c_str(),
                car2.m_fuse01, car2.m_stability, car2.m_tank, car2.tunnel, car2.max_car_velo * 3.6f,
                car2.m_money, car2.m_life));
        if(parken){
            text_list.push_back(make_text(tl_text, 2, text_color, text_height,
            "park, %.2f km/h", max_velo * 3.6f));
        } else {
            text_list.push_back(make_text(tl_text, 2, text_color, text_height,
            "%.2f km/h", max_velo * 3.6f));
        }
        text_list.push_back(pixel::make_text(tankstelle.m_tl, 0, text_color, text_height * 2, "P"));
        text_list.push_back(pixel::make_text(werkstatt.m_tl, 0, text_color, text_height * 2, "R"));
    }
    // white background
    clear_pixel_fb(255, 255, 255, 255);
    set_pixel_color(0, 0, 0, 255);
    if( animating ) {
        if(parken && car1.all_intersects(car1.parkplatz) && car2.all_intersects(car2.parkplatz)){
            dtime = 0;
        }
        car1.grab_tunnel = event.pressed(input_event_type_t::P1_ACTION2);
        printf("Car1 grab: %d\n", car1.grab_tunnel);
        car2.grab_tunnel = event.pressed(input_event_type_t::P2_ACTION2);
        printf("Car2 grab: %d\n\n", car2.grab_tunnel);
        dtime -= dt;
        if(dtime <= 0){
            float max_velo_old = max_velo;
            do {
                size_t a = (size_t)((float)(max_velos.size()-1) * next_rnd());
                max_velo = max_velos[a];
                std::cout << "rnd: idx " << a << 
                             ", max_velo " << max_velo_old * 3.6f << " -> " << max_velo * 3.6f << " km/h" << std::endl; 
            } while( equals(max_velo_old, max_velo) );
            if(parken){
                if(!car1.all_intersects(car1.parkplatz)){
                    --car1.m_life;
                    car1.reset();
                }
                if(!car2.all_intersects(car1.parkplatz)){
                    --car1.m_life;
                    car2.reset();
                }
                parken = false;
            } else if(max_velo > 800_km_h){
                max_velo = 50_km_h;
                parken = true;
            }
            dtime = start_d_time;
        }
        {
            f2::vec_t rfo;
            if(car1.intersection(rfo, car2)){
                car1.move(rfo);
                car1.m_stability -= (car1.m_velocity + car2.m_velocity) * dt;
            }
        }
        {
            f2::vec_t rfo;
            if(car2.intersection(rfo, car1)){
                car2.move(rfo * dt);
                car2.m_stability -= (car2.m_velocity + car1.m_velocity) * dt;
            }
        }
        if(!car1.all_intersects(tankstelle)){
            car1.tankt = false;
        }
        if(!car2.all_intersects(tankstelle)){
            car2.tankt = false;
        }
        if(!car1.all_intersects(werkstatt)){
            car1.reparing = false;
        }
        if(!car2.all_intersects(werkstatt)){
            car2.reparing = false;
        }
        if(event.has_any_p1()){
            if (event.pressed(input_event_type_t::P1_UP) && !car1.is_car_explodet()) {
                car1.change_velocity(velo_step,  1);
            } else if (event.pressed(input_event_type_t::P1_DOWN) && !car1.is_car_explodet()) {
                car1.change_velocity(velo_step, -1);
            } else if (event.pressed(input_event_type_t::P1_LEFT)) {
                car1.rotate( angle_step);
            } else if (event.pressed(input_event_type_t::P1_RIGHT)) {
                car1.rotate(-angle_step);
            } else if (event.pressed(input_event_type_t::P1_ACTION1)) {
                if(car1.m_velocity > 0){
                    car1.change_velocity(-1);
                } else if(car1.m_velocity < 0) {
                    car1.change_velocity( 1);
                }
                if(car1.m_velocity != 0){
                    car1.bt += dt;
                }
            }
        }
        if(event.has_any_p2()){
            if (event.pressed(input_event_type_t::P2_UP) && !car2.is_car_explodet()) {
                car2.change_velocity( velo_step);
            } else if (event.pressed(input_event_type_t::P2_DOWN) && !car2.is_car_explodet()) {
                car2.change_velocity(-velo_step);
            } else if (event.pressed(input_event_type_t::P2_LEFT)) {
                car2.rotate( angle_step);
            } else if (event.pressed(input_event_type_t::P2_RIGHT)) {
                car2.rotate(-angle_step);
            } else if (event.pressed(input_event_type_t::P2_ACTION1)) {
                if(car2.m_velocity > 0){
                    car2.change_velocity(-1);
                } else if(car2.m_velocity < 0) {
                    car2.change_velocity( 1);
                }
                if(car2.m_velocity != 0){
                    car2.bt += dt;
                }
            }
        }
        if(!car1.is_car_explodet()){
            car1.tick(dt);
        }
        if(!car2.is_car_explodet()){
            car2.tick(dt);
        }
    }
    /*
    train.t -= dt;
    for(f2::rect_t &bahnhof : bahnhoefe){
        if(train.waggon.body->intersects(bahnhof) && train.t > 0 ){//&& !b){
            train.change_velocity(-200_km_h * dt);
            ba = bahnhof;
        }
    }
    if(!train.waggon.body->intersects(ba)){
        train.t = 10_s;
    }
    train.tick(dt);

    if(zug.wagon.body.intersects(ba)){
        b = true;
    }
    */
    /*
    if(((!car1.all_intersects(*train.waggon.body) && 
          car1.all_intersects(*train.waggon.body)) || 
        (!car1.all_intersects(*train.automotora.body) && 
          car1.all_intersects(*train.automotora.body))) &&
        train.velocity > 0){
        car1.m_stability -= 100.0f * dt;
    }
    
    if(((!car2.all_intersects(*train.waggon.body) && 
          car2.all_intersects(*train.waggon.body)) || 
        (!car2.all_intersects(*train.automotora.body) && 
          car2.all_intersects(*train.automotora.body))) &&
        train.velocity > 0){
        car2.m_stability -= 100.0f * dt;
    }
    */
    car1.draw();
    car2.draw();
    // gleis.draw();
    tankstelle.draw(false);
    werkstatt.draw(false);
    car1.draw();
    car2.draw();
    for(Tunnel t : tunnels) {
        t.draw();
    }
    /*
    train.draw();
    set_pixel_color(0, 0, 0, 255);
    for(f2::rect_t &bahnhof : bahnhoefe){
        bahnhof.draw(false);
    }
    */
    swap_pixel_fb(false);
    for(pixel::texture_ref &tex : text_list) {
        tex->draw_fbcoord(0, 0);
    }
    text_list.clear();
    swap_gpu_buffer();
}

int main(int argc, char *argv[])
{
    unsigned int win_width = 1920, win_height = 1000;
    #if defined(__EMSCRIPTEN__)
        win_width = 1024, win_height = 576; // 16:9
    #endif
    {
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-width", argv[i]) && i+1<argc) {
                win_width = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-height", argv[i]) && i+1<argc) {
                win_height = atoi(argv[i+1]);
                ++i;
            }
        }
    }
    {
        const float origin_norm[] = { 0.5f, 0.5f };
        init_gfx_subsystem(argv[0], "car", (int)win_width, (int)win_height, origin_norm, true);
    }
    pixel::cart_coord.set_height(-space_height/2.0f, space_height/2.0f);
    log_printf(0, "XX %s\n", cart_coord.toString().c_str());
    {
        float w = cart_coord.width();
        float h = cart_coord.height();
        float r01 = h/w;
        float a = w / h;
        printf("-w %f [x]\n-h %f [y]\n-r1 %f [y/x]\n-r2 %f [x/y]\n", w, h, r01, a);
    }
    //make_bahnhoefe();
    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
}
