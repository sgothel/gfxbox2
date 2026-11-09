
#include <csignal>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cmath>

#include <cstdlib>
#include <memory>
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
#include <string_view>

#include <iostream>
#include <vector>

using namespace pixel;
using namespace f2;
using namespace jau;
using namespace jau::si_f32_literals;
using pixel::PI;
using pixel::PI_2;

constexpr static int car_id_1 = 1;
constexpr static int car_id_2 = 2;
constexpr static int car_id_3 = 3;
constexpr static int car_id_4 = 4;
constexpr static int car_id_5 = 5;
constexpr static si_velo_f32 start_max_velo = 50_km_h;
constexpr static si_length_f32 text_height = 17_dm;
static uint8_t lifes = 3;;
si_velo_f32 max_velo = start_max_velo;
si_velo_f32 max_train_velo = 100_km_h;
std::vector<geom_ref_t> aus_geom_list;
std::vector<geom_ref_t> loc_geom_list;
std::vector<geom_ref_t> schienen_geom_list;
constexpr static float angle_step = PI;
static bool debug_gfx = false;
static bool debug_prt = false;
static float dollar_per_liter = 0;

float normalize(const float f) {
    return f / abs(f);
}

float other_side(const float angle) {
    if(normalize(angle) == 1) {
        return angle - TWO_PI;
    } else {
        return angle + TWO_PI;
    }
}

template<typename Tp>
float abs_min(const Tp& a, const Tp& b) {
    return std::abs(a) < std::abs(b) ? a : b;
}

template<typename Tp>
float abs_max(const Tp& a, const Tp& b) {
    return std::abs(a) > std::abs(b) ? a : b;
}

void printerr(const char* format, ...) {
    va_list args;
    va_start (args, format);
    std::cerr << jau::to_string(format, args);
    va_end (args);
}

static rect_t tankstelle;
static rect_t werkstatt;

std::vector<rect_t> TunnelsLicet;

class Tunnel {
  private:
    constexpr static uint8_t m_tex_height = 2;

    bool m_d2Setted;
    si_length_f32 m_length;
    uint8_t m_id;

  public:
    disk_t d1;
    disk_t d2;

    Tunnel(const size_t id, point_t p0, const si_length_f32 d)
    : m_d2Setted(false), m_length(0), m_id(id), d1(p0, d/2), d2() { }
    Tunnel()               = default;
    Tunnel(const Tunnel &) = default;
    Tunnel(Tunnel &&)      = default;
    Tunnel& operator=(const Tunnel& o) = default;

    bool addToLength(const si_length_f32 l) { if (!m_d2Setted) { m_length += l; return true; } return false; }
    const si_length_f32& length() const noexcept { return m_length; }
    uint8_t id() const noexcept { return m_id; }

    void setD2(point_t c) {
        d2 = disk_t(c, d1.radius);
        m_d2Setted = true;
        printf("Tunnel[%d]: length %f, from %s to %s\n", m_id, m_length, d1.center.toString().c_str(), d2.center.toString().c_str());
    }

    void draw() const noexcept {
        d1.draw(false);
        if(m_d2Setted) {
            d2.draw(false);
        }
        {
            point_t tl_tex = d1.box().bl;
            pixel::texture_ref tex = make_text2(tl_tex, 0, {0, 0, 0, 255}, text_height, "%d", m_id);
            tex->draw_fbcoord(0, 0);
        }
        if(m_d2Setted) {
            point_t tl_tex = d2.box().bl;
            pixel::texture_ref tex = make_text2(tl_tex, 0, {0, 0, 0, 255}, text_height, "%d", m_id);
            tex->draw_fbcoord(0, 0);
        }
    }

    std::string toString() {
        return to_string("tunnel[%zu]: length %f, from %s to %s",
            m_id, m_length, d1.center.toString().c_str(), d2.center.toString().c_str());
    }
};

std::vector<Tunnel> tunnels;

class Car;

typedef std::shared_ptr<Car> Car_ref;

std::vector<Car_ref> AI_cars;
std::vector<rect_t> houses;

class Car {
  private:
    constexpr static std::string_view m_start_bdv = ":-)";
    constexpr static si_velo_f32 m_start_velocity = 0_km_h; // [m/s]
    constexpr static si_angle_f32 m_start_angle = PI_2; // [rad]
    constexpr static si_time_f32 max_fuse01 = 10_s; // [s]
    constexpr static float max_stability = 100.0f; // [%]
    constexpr static float start_money = 1'000.0f; // [$]
    constexpr static si_volume_f32 max_tank = 100_l; // [m^3]
    constexpr static si_volume_f32 max_fuel_can = 5_l; // [m^3]
    constexpr static float tank_velocity = 10_l / 1_s; // [m^3/s]
    constexpr static float repair_velocity = 10 / 1_s; // [stability/s]
    constexpr static si_time_f32 max_NTVT = 10_s; // No Tank and Velocity Time
    constexpr static si_time_f32 max_break_time = 1_s;
    constexpr static si_time_f32 max_allowed_stand_time = 30_s;

    point_t tl1 = {
        cart_coord.min_x() + 3 * cart_coord.width() / 4, cart_coord.min_y() + 3_m};
    point_t tl2 = {
        cart_coord.min_x() +     cart_coord.width() / 4, cart_coord.min_y() + 3_m};
    point_t tl3 = {0, cart_coord.height() / 2 / 2};

    enum class DAHP { // drive around house possibility
        none = 0,
        first = 1,
        second = 2
    };
  public:
    struct State {
        std::string_view velo_smily = m_start_bdv; // if velocity is under 100% of speed limit: :-)
                                // if velocity is over  100% but under 110% of the speed limit: :-|
                                // else: :-(
        si_time_f32 fuse01 = max_fuse01; // the car is allowed to be only this time over 110% of the speed limit
        float stability = max_stability; // the stability of car
        si_volume_f32 tank = max_tank; // the volume of fuel in the tank
        si_volume_f32 fuel_can = max_fuel_can; // the volume of fuel in the fuel can
        si_time_f32 tunnel_digging_time = 10_min; // how long the car digged the last tunnel
        si_velo_f32 max_car_velo = 240_km_h; // the maximum velocity of car
        float money = start_money; // money
        int life = lifes; // the lifes of the car
        si_time_f32 no_tank_and_velo_time = max_NTVT; // the time, you can have no velocity and tank
        si_time_f32 brake_time = max_break_time; // if you brakes more than this time, you will loss stability
        si_time_f32 allowed_stand_time = max_allowed_stand_time; // you`re not allowed to stand more than this time
        point_t anfang_p = {0, 0};
        Tunnel next_tunnel = {};
        si_length_f32 tunnelLength = 0;
        si_length_f32 tunnelLengthOrig = 0;
        point_t otherTunnelEnd = {0, 0};
        si_time_f32 neverInTunnelTimer = 2_s;
        DAHP last_DAHP_possability = DAHP::none;
        bool repairing : 1; // if true, the car will be repaired
        bool refueling : 1; // if true, the car is refueling
        bool digging_tunnel : 1; // if true, the car is digging a tunnel
        bool last_grab_tunnel : 1;
        bool put_can_in_tank : 1;
        bool inTunnel : 1;
    };

  private:
    int m_id;
    si_velo_f32 m_velocity;
    si_angle_f32 m_angle;
    si_length_f32 m_length;
    si_length_f32 m_width;
    rect_t m_rect_body;
    circle_seg_t circle_seg_body;
    std::vector<rect_t> wheels;
    rect_t m_parking_ground;
    aabbox_t m_aabbox;
    Car* m_target = nullptr;
    float m_punish = 0;
    State m_state;

    void reset_aabbox() {
        m_aabbox.reset();
        m_aabbox.resize(circle_seg_body.box());
        m_aabbox.resize(m_rect_body.box());
        for(rect_t &wheel : wheels){
           m_aabbox.resize(wheel.box());
        }
    }

    bool intersects(const geom_t &o) noexcept {
        reset_aabbox();
        return m_state.life > 0 && !m_state.digging_tunnel && m_aabbox.intersects(o);
    }

    bool intersects(const Car &o) noexcept {
        return intersects(o.m_aabbox);
    }
  public:
    Car(const int id, Car* target = nullptr, const float punish = 0)
    : m_id(id), m_target(target) {
        reset(true);
        m_punish = punish;
        if(debug_prt) {
            printf("Car::ctor %s\n", toString().c_str());
        }
    }

    std::string toString() const {
        return jau::to_string("Car[id %d, this %p, target %p, punish %.2f$]", m_id, this, m_target, m_punish);
    }
    Car(const Car& o) noexcept = default;
    Car& operator=(const Car& o) noexcept = default;

    void reset(bool all){
        m_velocity = m_start_velocity;
        m_angle = m_start_angle;
        m_length = 25_dm;
        m_width = 16_dm;
        if(all) {
            m_state = State();
            m_punish = 0;
        } else {
            const int life = m_state.life;
            const float money = m_state.money;
            m_state = State();
            m_state.life = life;
            m_state.money = money;
        }
        if(m_id == car_id_1){
            m_rect_body = {tl1, m_width, (float)(m_length*0.75)};
        } else if(m_id == car_id_2){
            m_rect_body = {tl2, m_width, (float)(m_length*0.75)};
        } else {
            m_rect_body = {tl3, m_width, (float)(m_length*0.75)};
        }
        circle_seg_body = {
            point_t(m_rect_body.m_tl.x + m_width / 2, m_rect_body.m_tl.y),
            m_width / 2, 0, PI};
        {
            wheels.clear();
            const si_length_f32 wheel_width = (float)(m_width * 0.25);
            const si_length_f32 wheel_length = (float)(wheel_width * 2);
            {
                rect_t wheeltl = {
                    {m_rect_body.m_tl.x - wheel_width, m_rect_body.m_tl.y},
                    wheel_width, wheel_length};
                wheels.push_back(wheeltl);
            }
            {
                rect_t wheeltr = {m_rect_body.m_tr, wheel_width, wheel_length};
                wheels.push_back(wheeltr);
            }
            {
                rect_t wheelbl = {
                    {m_rect_body.m_bl.x - wheel_width, m_rect_body.m_bl.y + wheel_length},
                    wheel_width, wheel_length};
                wheels.push_back(wheelbl);
            }
            {
                rect_t wheelbr = {
                    {m_rect_body.m_br.x, m_rect_body.m_br.y + wheel_length},
                    wheel_width, wheel_length};
                wheels.push_back(wheelbr);
            }
        }
        m_length = m_length*0.75f + m_width / 2;
        m_parking_ground = {{m_rect_body.m_tl.x - 1_m, m_rect_body.m_tl.y + m_width / 2 + 125_cm},
                     m_width + 2_m, m_length + 25_dm};
        m_aabbox.resize(circle_seg_body.box());
        m_aabbox.resize(m_rect_body.box());
        for(rect_t &wheel : wheels){
           m_aabbox.resize(wheel.box());
        }
    }

    bool intersects(const rect_t &o) noexcept {
        reset_aabbox();
        return m_state.life > 0 && !m_state.digging_tunnel && m_aabbox.intersects(o);
    }

    bool intersection(Car &o, const float dt) noexcept {
        if(!intersects(o)) {
            return false;
        }
        const float pre_car1_velo = velocity();
        decrease_stabil((std::abs(velocity()) + std::abs(o.velocity())) * 0.1f);
        move(vec_t::from_length_angle(-velocity()*dt*2, velo_angle()));
        stop();
        o.decrease_stabil((std::abs(pre_car1_velo) + std::abs(o.velocity())) * 0.1f);
        o.move(vec_t::from_length_angle(-o.velocity()*dt*2, o.velo_angle()));
        o.stop();
        return true;
    }

    void addToVelocity(const si_accel_f32 m_pro_s2, const si_time_f32 dt, bool amBremsen = false){
        if(m_state.life <= 0){
            return;
        }
        if(m_state.tank > 0 || amBremsen){
            m_velocity += m_pro_s2 * dt;
            if(!amBremsen) {
                const float abs_velo = std::abs(m_velocity);
                const float abs_ms2 = std::abs(m_pro_s2);
                if(abs_velo < 130_km_h) {
                    if(abs_velo < 40_km_h) {
                        m_state.tank -= abs_ms2*dt*(1/(abs_velo*10.0f))*1/3000;
                    } else {
                        m_state.tank -= abs_ms2*dt*(1/abs_velo)*1/5000;
                    }
                } else {
                    m_state.tank -= abs_ms2*dt*(1/130_km_h + abs_velo*0.1f)*1/5000;
                }
                m_state.tank = std::max(m_state.tank, 0.0f);
            }
        }
    }

    bool toggle_tankt() noexcept {
        if(intersects(tankstelle)) {
            m_state.refueling = !m_state.refueling;
            return true;
        }
        return false;
    }

    bool toggle_repairing() noexcept {
        if(intersects(werkstatt)) {
            m_state.repairing = !m_state.repairing;
            return true;
        }
        return false;
    }

    const State& state() noexcept { return m_state; }
    float punish() const noexcept { return m_punish; }
    bool is_car_exploded() const noexcept { return m_state.life <= 0; }
    bool toggle_put_can_in_tank() noexcept { return m_state.put_can_in_tank = !m_state.put_can_in_tank; }
    bool toggle_digging_tunnel() noexcept { return m_state.digging_tunnel = !m_state.digging_tunnel; }
    bool set_tankt(const bool new_refueling) noexcept { return m_state.refueling = new_refueling; }
    bool set_repairing(const bool new_repairing) noexcept { return m_state.repairing = new_repairing; }
    void released_brake() noexcept { m_state.brake_time = 0; }
    si_velo_f32 velocity() noexcept { return m_velocity; }
    si_angle_f32 velo_angle() noexcept { return m_angle; }
    const aabbox_t& box() noexcept { return m_aabbox; }
    void decrease_life() noexcept { --m_state.life; }
    void decrease_stabil(const float minuent) noexcept { m_state.stability -= minuent; }
    void stop() noexcept { m_velocity = 0; }
    const rect_t& parking_ground() noexcept { return m_parking_ground; }
    void increase_brake_time(const si_time_f32 dt) noexcept { m_state.brake_time += dt; }

    void rotate(const float rad){
        if(m_state.life <= 0){
            return;
        }
        m_angle += rad;
        m_rect_body.rotate(rad, m_rect_body.p_center);
        for(rect_t &wheel : wheels){
            wheel.rotate(rad, m_rect_body.p_center);
        }
        m_aabbox.rotate(rad, m_rect_body.p_center);
        circle_seg_body.rotate(rad, m_rect_body.p_center);
        if(m_angle > PI*2){
            m_angle = m_angle - PI*2;
        } else if(m_angle < (-PI*2)){
            m_angle = m_angle + PI*2;
        }
    }

    void move(vec_t vec, bool overScreen=true){
        m_rect_body.move(vec);
        for(rect_t &wheel : wheels){
            wheel.move(vec);
        }
        circle_seg_body.move(vec);
        m_aabbox.move(vec);
        if(overScreen) {
            if( m_rect_body.p_center.x < pixel::cart_coord.min_x() ) {
                move({cart_coord.max_x()-m_rect_body.p_center.x, 0.0f}, false);
            }
            if( m_rect_body.p_center.x > pixel::cart_coord.max_x() ) {
                move({cart_coord.min_x()-m_rect_body.p_center.x, 0.0f}, false);
            }
            if( m_rect_body.p_center.y < pixel::cart_coord.min_y() ) {
                move({0.0f, cart_coord.max_y()-m_rect_body.p_center.y}, false);
            }
            if( m_rect_body.p_center.y > pixel::cart_coord.max_y() ) {
                move({0.0f, cart_coord.min_y()-m_rect_body.p_center.y}, false);
            }
        }
    }

    void grabTunnel(const float dt, const bool otherPlayerIsDigging) {
        if(m_state.digging_tunnel){
            if(!m_state.last_grab_tunnel){
                bool b = false;
                for(const rect_t& r : TunnelsLicet) {
                    if(r.contains(m_rect_body.p_center)){
                        b = true;
                        break;
                    }
                }
                m_state.digging_tunnel = b;
                if(!m_state.digging_tunnel) {
                    m_state.last_grab_tunnel = m_state.digging_tunnel;
                    return;
                }
                m_state.anfang_p = m_rect_body.p_center;
                m_state.next_tunnel = Tunnel(otherPlayerIsDigging ? tunnels.size()+1 : tunnels.size(), m_state.anfang_p, m_length);
            }
            m_state.next_tunnel.addToLength(dt*m_velocity);
            m_state.tunnel_digging_time -= dt;
            m_state.money += 100 * dt;
            m_velocity *= 0.99f;
        } else {
            if(m_state.last_grab_tunnel){
                bool b = true;
                for(const rect_t& r : TunnelsLicet) {
                    if(r.contains(m_rect_body.p_center)){
                        b = false;
                        break;
                    }
                }
                m_state.digging_tunnel = b;
                if(!b) {
                    m_state.neverInTunnelTimer = 2_s;
                    m_state.next_tunnel.setD2(m_rect_body.p_center);
                    tunnels.push_back(m_state.next_tunnel);
                    printf("%s\n", m_state.next_tunnel.toString().c_str());
                }
            }
        }
        m_state.last_grab_tunnel = m_state.digging_tunnel;
    }

    void make_police_car(const int id, const float punish, Car* target) {
        AI_cars.push_back(std::make_shared<Car>(id, target, punish));
        if(debug_prt) {
            printf("AI_cars: %zu: %s\n", AI_cars.size(), AI_cars.back()->toString().c_str());
        }
    }

    lineseg_t make_l_move(const float dt, const float scale=1) const noexcept {
        return lineseg_t(m_rect_body.p_center, m_rect_body.p_center + (vec_t::from_length_angle((m_velocity*dt + m_length/2)*scale, m_angle) ));
    }

    point_t get_target_point() const noexcept {
        switch (m_id) {
            case car_id_3: {
                return m_target->m_rect_body.p_center;
            }
            case car_id_4: {
                return m_target->m_rect_body.p_center + vec_t::from_length_angle( 10_m, m_target->m_angle);
            }
            case car_id_5: {
                return m_target->m_rect_body.p_center + vec_t::from_length_angle(-10_m, m_target->m_angle);
            }
            default: break;
        }
        return {0, 0};
    }

    bool police_going(const float dt, const si_accel_f32 accel = 40_m_s2) {
        if(!m_target) {
            return true;
        }
        const point_t center = m_rect_body.p_center;
        point_t target_point = get_target_point();
        vec_t diff = target_point - center;
        const float diff_length = diff.length();
        if(m_target->m_punish < 1000) {
            addToVelocity(std::min({accel, m_state.max_car_velo - m_velocity, max_velo*0.9f - m_velocity}), dt);
        } else if(m_target->m_punish < 5000) {
            addToVelocity(std::min({accel, m_state.max_car_velo - m_velocity, max_velo*2.0f - m_velocity}), dt);
        } else {
            addToVelocity(std::min(accel, m_state.max_car_velo - m_velocity), dt);
        }
        si_angle_f32 diff_angle = (diff.angle() - m_angle);
        set_pixel_color4f(1, 0, 1, 1);
        if((cart_coord.max_x() - center.x + (target_point - point_t(cart_coord.min_x(), center.y)).length()) < diff_length) {
            diff_angle = 0 - m_angle;
        }
        if((cart_coord.max_y() - center.y + (target_point - point_t(center.x, cart_coord.min_y())).length()) < diff_length) {
            diff_angle = PI_2 - m_angle;
        }
        if((center.x - cart_coord.min_x() + (target_point - point_t(cart_coord.max_x(), center.y)).length()) < diff_length) {
            diff_angle =   PI - m_angle;
        }
        if((center.y - cart_coord.min_y() + (target_point - point_t(center.x, cart_coord.max_y())).length()) < diff_length) {
            diff_angle = -PI_2 - m_angle;
        }
        if(diff_angle > other_side(diff_angle)) {
            diff_angle = other_side(diff_angle);
        }
        const float rotate_angle = std::min(diff_angle, angle_step*normalize(diff_angle)*dt);
        rotate(rotate_angle);
        lineseg_t l_move = make_l_move(dt, 2);
        bool collision = false;
        for(const rect_t& house : houses) {
            if(house.intersects(l_move)) {
                collision = true;
                lineseg_t trtl = lineseg_t(house.m_tr, house.m_tl);
                lineseg_t brbl = lineseg_t(house.m_br, house.m_bl);
                lineseg_t trbr = lineseg_t(house.m_tr, house.m_br);
                lineseg_t bltl = lineseg_t(house.m_bl, house.m_tl);
                lineseg_t line = trtl;
                mark:
                if(l_move.intersects(line)) {
                    l_move.rotate(line.angle() - l_move.angle());
                    lineseg_t l_move2 = l_move;
                    l_move2.rotate(PI);
                    if(debug_gfx) {
                        set_pixel_color(255, 0, 0, 255);
                        l_move.draw();
                        set_pixel_color4f(0, 0, 1, 1);
                        l_move2.draw();
                    }
                    if((target_point - l_move2.p1).length() > (target_point - l_move.p1).length() && (m_state.last_DAHP_possability == DAHP::none ||
                        m_state.last_DAHP_possability == DAHP::first)) {
                        m_state.last_DAHP_possability = DAHP::first;
                        diff_angle = l_move.angle() - m_angle;
                        if(diff_angle > other_side(diff_angle)) {
                            diff_angle = other_side(diff_angle);
                        }
                        rotate(diff_angle); //abs_min(angle_step*normalize(diff_angle), diff_angle));
                    } else {
                        m_state.last_DAHP_possability = DAHP::second;
                        diff_angle = l_move2.angle() - m_angle;
                        if(diff_angle > other_side(diff_angle)) {
                            diff_angle = other_side(diff_angle);
                        }
                        rotate(diff_angle); //abs_min(angle_step*normalize(diff_angle), diff_angle));
                    }
                } else if(l_move.intersects(brbl)) {
                    line = brbl;
                    goto mark;
                } else if(l_move.intersects(trbr)) {
                    line = trbr;
                    goto mark;
                } else if(l_move.intersects(bltl)) {
                    line = bltl;
                    goto mark;
                }
                break;
            }
        }
        if(!collision) {
            m_state.last_DAHP_possability = DAHP::none;
        }
        if(m_state.tank == 0) {
            if(m_state.fuel_can == 0) {
                m_target->m_punish -= m_punish;
                return false;
            }
            m_state.put_can_in_tank = true;
        }
        return true;
    }

    bool police_strategy(const float dt, const si_accel_f32 accel) {
        if(m_target && intersects(*m_target) && (m_target->m_state.inTunnel == m_state.inTunnel && m_state.tunnelLength == m_target->m_state.tunnelLength) && !m_target->m_state.digging_tunnel) {
            m_target->m_state.money -= m_punish;
            m_target->m_punish -= m_punish;
            return false;
        }
        reset_aabbox();
        point_t target_point = get_target_point();
        if(!police_going(dt, accel)) {
            return false;
        }
        m_punish += dt;
        m_target->m_punish += dt;
        const int next_id = m_id < car_id_5 ? m_id + 1 : car_id_3;
        if(m_punish > 100) {
            m_punish /= 3;
            make_police_car(next_id, m_punish*2, m_target);
        }
        if(m_id > car_id_3) {
            if(m_aabbox.contains(target_point) && !m_target->m_state.digging_tunnel && !m_target->m_state.inTunnel) {
                if(debug_prt) {
                    printf("Car %d -> %d!\n", m_id, next_id);
                }
                m_id = next_id;
                return true;
            }
            bool go_one_id_over = true;
            for(const Car_ref& AI_car : AI_cars) {
                if(AI_car && m_target == AI_car->m_target && ((AI_car->m_id == next_id) || std::abs((AI_car->m_rect_body.p_center - m_target->m_rect_body.p_center).length()) * 3 <
                    std::abs((m_rect_body.p_center - m_target->m_rect_body.p_center).length()))) {
                    go_one_id_over = false;
                    break;
                }
            }
            if(go_one_id_over) {
                m_id = next_id;
            }
        }
        return true;
    }

    bool tick(const float dt, const bool otherPlayerIsDigging, const si_accel_f32 accel = 40_m_s2){
        if(m_target) {
            if(!police_strategy(dt, accel)){
                return false;
            }
        }
        if(m_state.tunnelLength > 0) {
            m_state.tunnelLength -= dt*m_velocity;
        }
        if(m_state.neverInTunnelTimer <= 0 && !m_state.inTunnel && !m_state.digging_tunnel) {
            for(const Tunnel& t : tunnels) {
                if(t.d1.contains(m_rect_body.p_center) || t.d2.contains(m_rect_body.p_center)){
                    m_state.tunnelLength = (m_state.tunnelLengthOrig = t.length());
                    m_state.inTunnel = true;
                    if(t.d1.contains(m_rect_body.p_center)){
                        m_state.otherTunnelEnd = t.d2.center;
                    }
                    if(t.d2.contains(m_rect_body.p_center)){
                        m_state.otherTunnelEnd = t.d1.center;
                    }
                }
            }
        }
        if(m_state.tunnelLength <= 0 && m_state.inTunnel) {
            m_state.inTunnel = false;
            move(-m_rect_body.p_center + m_state.otherTunnelEnd);
            m_state.neverInTunnelTimer = 2_s;
            m_state.tunnelLength = 0;
        } else if( m_state.tunnelLength > m_state.tunnelLengthOrig && m_state.inTunnel) {
            m_state.inTunnel = false;
            m_state.neverInTunnelTimer = 2_s;
            m_state.tunnelLength = 0;
        }
        if(m_state.neverInTunnelTimer > 0) {
            m_state.neverInTunnelTimer -= dt;
        }
        if(m_state.life <= 0){
            return false;
        }
        lineseg_t l_move;
        l_move.p0 = m_rect_body.p_center;
        if(!m_state.inTunnel) {
            vec_t m_velo_vec = vec_t::from_length_angle(m_velocity, m_angle);
            move(m_velo_vec * dt);
        }
        l_move.p1 = m_rect_body.p_center + vec_t::from_length_angle(m_length/2, m_angle);
        if(!m_target && !m_state.inTunnel && !m_state.digging_tunnel) {
            #if 1
                using namespace pixel::f2;
                for(const rect_t& house : houses) {
                    point_t coll_point;
                    vec_t coll_normal;
                    vec_t coll_out;
                    if (intersects(house) && house.box().intersection(coll_out, coll_normal, coll_point, l_move)) {
                        m_state.stability -= m_velocity;
                        m_velocity *= 0.5f;
                        rotate(coll_out.angle() - m_angle);
                        const point_t new_pos = coll_point + coll_out;
                        move(new_pos - m_rect_body.p_center);
                        if(!m_target) {
                            const float punish = std::abs(m_velocity * 15);
                            m_punish += punish;
                            printf("Car %d: collision with house!!! Reflection angle: %.2f° Punishment: %.2f$ Total: %.2f$\n",
                                m_id, rad_to_adeg(coll_out.angle()), m_velocity*15, m_punish);
                            make_police_car(car_id_3, punish/3, this);
                            make_police_car(car_id_4, punish/3, this);
                            make_police_car(car_id_5, punish/3, this);
                        }
                        if(debug_gfx) {
                            set_pixel_color(255,   0, 255, 255);
                            lineseg_t::draw(coll_point, coll_point + coll_normal);
                            set_pixel_color(255,   0,   0, 255);
                            const point_t p0 = coll_point + vec_t( 3_m,  2_m);
                            const point_t p1 = coll_point + vec_t( 3_m, -2_m);
                            const point_t p2 = coll_point + vec_t(-3_m,  2_m);
                            const point_t p3 = coll_point + vec_t(-3_m, -2_m);
                            lineseg_t::draw(p0, p3);
                            lineseg_t::draw(p1, p2);
                        }
                        break;
                    }
                }
            #else
                for(const rect_t& house : houses) {
                    if(intersects(house)) {
                        m_velocity *= 0.1f;
                        rotate(PI);
                        AI_cars.push_back(std::make_shared<Car>(3, this));
                        break;
                    }
                }
            #endif
        }
        if(m_state.fuse01 < 0){
            const float punish = (std::abs(m_velocity) - max_velo)*10;
            make_police_car(car_id_3, punish, this);
            m_state.fuse01 = 10;
            m_punish += punish;
        }
        if(m_state.fuse01 > 0) {
            if(std::abs(m_velocity) <= max_velo){
                m_state.velo_smily = ":-)";
            } else if(std::abs(m_velocity) <= max_velo*1.10){
                m_state.velo_smily = ":-|";
            } else if(std::abs(m_velocity) > max_velo*1.10){
                m_state.velo_smily = ":-(";
                if(!m_state.digging_tunnel) {
                    m_state.fuse01 -= dt;
                }
                if(m_state.fuse01 == 0) {
                    m_state.fuse01 = -1;
                }
            }
        }
        if(m_state.stability <= 0){
            --m_state.life;
            reset(false);
        }
        if(std::abs(m_velocity) > m_state.max_car_velo){
            m_state.stability -= (std::abs(m_velocity) - m_state.max_car_velo) / 100 * dt;
        }

        if((m_state.tank <= 0 && m_velocity <= 0) && m_state.no_tank_and_velo_time > 0){
            m_state.no_tank_and_velo_time -= dt;
        } else if(m_state.no_tank_and_velo_time <= 0){
            --m_state.life;
            reset(false);
        }
        if(m_state.refueling && m_state.money > 0){
            if(m_state.fuel_can < max_fuel_can) {
                const si_volume_f32 moved_volume = std::min(tank_velocity * dt, max_fuel_can - m_state.fuel_can);
                m_state.fuel_can += moved_volume;
                m_state.money -= moved_volume*dollar_per_liter;
            } else {
                const si_volume_f32 moved_volume = std::min(tank_velocity * dt, max_tank - m_state.tank);
                m_state.tank += moved_volume;
                m_state.money -= moved_volume * dollar_per_liter;
            }
        }
        if(m_state.put_can_in_tank) {
            if(m_state.fuel_can > 0 && m_state.tank < max_tank) {
                const si_volume_f32 moved_volume = std::min(tank_velocity * dt, m_state.fuel_can);
                m_state.fuel_can -= moved_volume;
                m_state.tank += moved_volume;
            } else if(m_state.fuel_can <= 0) {
                m_state.put_can_in_tank = false;
            }
        }
        if(m_state.repairing && m_state.money > 0){
            const float repaired = std::min(repair_velocity * dt, max_stability - m_state.stability);
            m_state.stability += repaired;
            m_state.money -= repaired * 0.2f * dt;
        }
        if(m_state.stability > max_stability){
            ++m_state.life;
            m_state.stability = m_state.stability - max_stability;
        }
        if(m_state.brake_time > 2_s){
            m_state.stability -= dt/10;
        }
        if(m_velocity == 0){
            m_state.allowed_stand_time -= dt;
        }
        if(m_velocity != 0){
            m_state.allowed_stand_time = 1_min;
        }
        if(m_state.allowed_stand_time <= 0){
            --m_state.life;
            m_state.allowed_stand_time = 1_min;
        }
        grabTunnel(dt, otherPlayerIsDigging);

        m_state.max_car_velo = std::min((m_state.stability / max_stability) * 500_km_h, 240_km_h);
        m_velocity += -((m_velocity == 0 ? 1 : m_velocity / abs(m_velocity)) * std::min(3_m_s2 * dt, abs(0 - m_velocity)));
        return true;
    }
    void draw() const noexcept {
        if(is_car_exploded()){
            return;
        }

        set_pixel_color4f(0, 0, 0, 0);
        if(!m_target) {
            m_parking_ground.draw(false);
        }

        if(m_state.digging_tunnel || m_state.inTunnel){
            m_state.next_tunnel.draw();
            set_pixel_color4f(0.5f, 0.5f, 0.5f, 0.5f);
        }
        if(m_target) {
            if(debug_gfx) {
                set_pixel_color4f(1, 0, 0, 1);
                lineseg_t::draw(m_rect_body.p_center, get_target_point());
                set_pixel_color4f(0, 1, 0, 1);
                make_l_move(1).draw();
            }
            set_pixel_color4f(0, 0, 1, 1);
        }
        m_rect_body.draw(false);
        circle_seg_body.draw();
        for(const rect_t &r : wheels){
            r.draw(false);
        }
        set_pixel_color4f(0, 0, 0, 0);
    }

    point_t make_point(point_t p, const float radians, const float length) {
        const float x = length * cos(radians);
        const float y = length * sin(radians);
        point_t p1 = {x, y};
        return p1 + p;
    }
};

// static Car car1(car_id_1);
// static Car car2(car_id_2);

const float space_height = 100_m;

struct d_times_t {
    si_time_f32 dtime_max;
    si_time_f32 dtime;
};

static int fps = 60;
extern "C" {
    EMSCRIPTEN_KEEPALIVE void set_forces_fps(int new_fps) noexcept { fps = new_fps; }
}

void mainloop() {
    static Car car1(car_id_1);
    static Car car2(car_id_2);
    static const f4::vec_t text_color = {0, 0, 0, 0};
    static uint64_t t_last = getElapsedMillisecond(); // [ms]
    static input_event_t event;
    static std::vector<texture_ref> text_list;
    static si_accel_f32 velo_step = 40_m_s2;
    static d_times_t velo_limit = {1_min, 0};
    static d_times_t fuel_price = {51_s, 0};
    static std::vector<si_velo_f32> velo_limits = { 30_km_h, 50_km_h, 70_km_h, 90_km_h,
                                            100_km_h, 120_km_h, 130_km_h, 150_km_h, 200_km_h,
                                            500_km_h, 600_km_h };
    static std::vector<si_velo_f32> fuel_prices = { 0.25f / 1_l, 0.5f / 1_l, 0.75f / 1_l, 1 / 1_l, 1.2f / 1_l,
                                            1.4f / 1_l, 1.5f / 1_l, 1.6 / 1_l, 1.7 / 1_l, 1.8f / 1_l, 2.0f / 1_l,
                                            2.1f / 1_l, 2.4f / 1_l, 2.5f / 1_l, 3 / 1_l, };
    static bool parken = false;


    const point_t tl_text(pixel::cart_coord.min_x(), pixel::cart_coord.max_y());
    const uint64_t t1 = getElapsedMillisecond(); // [ms]
    const float dt = (float)( t1 - t_last ) / 1000.0f; // [s]
    t_last = t1;

    bool animating = !event.paused();
    while(handle_one_event(event)){
        if( event.pressed_and_clr( input_event_type_t::WINDOW_CLOSE_REQ ) ) {
            printf("Exit Application, td %" PRIu64 "\n", t1 / 1000);
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
            car1.reset(true);
            car2.reset(true);
            max_velo = start_max_velo;
            tunnels.clear();
            AI_cars.clear();
            loc_geom_list.clear();
            schienen_geom_list.clear();
            // train.reset();
        }
    }
    if(animating) {
        if(event.has_any_p1()){
            if (event.pressed(input_event_type_t::P1_UP) && !car1.is_car_exploded()) {
                car1.addToVelocity(velo_step, dt);
            } else if (event.pressed(input_event_type_t::P1_DOWN) && !car1.is_car_exploded()) {
                car1.addToVelocity(-velo_step, dt);
            } else if (event.pressed(input_event_type_t::P1_LEFT)) {
                car1.rotate( angle_step*dt);
            } else if (event.pressed(input_event_type_t::P1_RIGHT)) {
                car1.rotate(-angle_step*dt);
            } else if (event.pressed(input_event_type_t::P1_ACTION1)) {
                if(car1.velocity() > 0){
                    car1.addToVelocity(-std::min(2*velo_step,  car1.velocity()), dt, true);
                } else if(car1.velocity() < 0) {
                    car1.addToVelocity( std::min(2*velo_step, -car1.velocity()), dt, true);
                }
                if(car1.velocity() != 0){
                    car1.increase_brake_time(dt);
                }
            }
        }
        if(event.released_and_clr(input_event_type_t::P1_ACTION1)){
            car1.released_brake();
        } else if(event.released_and_clr(input_event_type_t::P1_ACTION4)){
            car1.toggle_put_can_in_tank();
        } else if(event.released_and_clr(input_event_type_t::P1_ACTION2)) {
            car1.toggle_digging_tunnel();
        } else if(event.released_and_clr(input_event_type_t::P1_ACTION3)){
            car1.toggle_tankt();
            car1.toggle_repairing();
        }
        if(event.released_and_clr(input_event_type_t::P2_ACTION4)){
            car2.toggle_put_can_in_tank();
        } else if(event.released_and_clr(input_event_type_t::P2_ACTION2)) {
            car2.toggle_digging_tunnel();
        } else if(event.released_and_clr(input_event_type_t::P2_ACTION1)){
            car2.released_brake();
        } else if(event.released_and_clr(input_event_type_t::P2_ACTION3)){
            car2.toggle_tankt();
            car2.toggle_repairing();
        }
        if(event.has_any_p2()){
            if (event.pressed(input_event_type_t::P2_UP) && !car2.is_car_exploded()) {
                car2.addToVelocity( velo_step, dt);
            } else if (event.pressed(input_event_type_t::P2_DOWN) && !car2.is_car_exploded()) {
                car2.addToVelocity(-velo_step, dt);
            } else if (event.pressed(input_event_type_t::P2_LEFT)) {
                car2.rotate( angle_step*dt);
            } else if (event.pressed(input_event_type_t::P2_RIGHT)) {
                car2.rotate(-angle_step*dt);
            } else if (event.pressed(input_event_type_t::P2_ACTION1)) {
                if(car2.velocity() > 0){
                    car2.addToVelocity(-std::min(velo_step*2,  car2.velocity()), dt, true);
                } else if(car2.velocity() < 0) {
                    car2.addToVelocity( std::min(velo_step*2, -car2.velocity()), dt, true);
                }
                if(car2.velocity() != 0){
                    car2.increase_brake_time(dt);
                }
            }
        }
    }
    {
        text_list.push_back(pixel::make_text2(tl_text, 0, text_color, text_height,
            "td %s, fps %2.2f, d_time %2d, car1(velo %3.2f km/h, %s, "
            "fuse %.2f s, stability %.2f pct, tank %.2fl, tunnel %.2f, max velo %.2fkm/h, money %.2f$, "
            "life %d, total punish %.2f$), ",
                to_decstring(t1/1000, ',', 9).c_str(), gpu_avg_fps(), (int)velo_limit.dtime, car1.velocity() * 3.6f,
                car1.state().velo_smily.data(), // NOLINT(bugprone-suspicious-stringview-data-usage)
                car1.state().fuse01,
                car1.state().stability, car1.state().tank*1000, car1.state().tunnel_digging_time,
                car1.state().max_car_velo * 3.6f, car1.state().money, car1.state().life, car1.punish()));
        text_list.push_back(pixel::make_text2(tl_text, 1, text_color, text_height,
            "car2(velo %.2f km/h, %s, fuse %.2f s, stability %.2f pct, tank %.2fl, "
            "tunnel %.2f, max velo %.2fkm/h, money %.2f$, life %d, total punish %.2f$), dtime %d, fuel price %.3f$/l",
                car2.velocity() * 3.6f, car2.state().velo_smily.data(), // NOLINT(bugprone-suspicious-stringview-data-usage)
                car2.state().fuse01, car2.state().stability, car2.state().tank * 1000, car2.state().tunnel_digging_time,
                car2.state().max_car_velo * 3.6f, car2.state().money, car2.state().life, car2.punish(), (int)fuel_price.dtime, dollar_per_liter * 1_l));
        if(parken){
            text_list.push_back(make_text2(tl_text, 2, text_color, text_height,
                "park, %.2f km/h", max_velo * 3.6f));
        } else {
            text_list.push_back(make_text2(tl_text, 2, text_color, text_height,
                "%.2f km/h", max_velo * 3.6f));
        }
        text_list.push_back(pixel::make_text2(tankstelle.m_tl, 0, text_color, text_height * 2, "F"));
        text_list.push_back(pixel::make_text2(werkstatt.m_tl, 0, text_color, text_height * 2, "R"));
    }
    // white background
    clear_pixel_fb(255, 255, 255, 255);
    set_pixel_color(0, 0, 0, 255);
    if(animating) {
        fuel_price.dtime -= dt;
        if(fuel_price.dtime <= 0) {
            fuel_price.dtime = fuel_price.dtime_max;
            const float old_fuel_price = dollar_per_liter;
            do {
                size_t idx = (size_t)((float)(fuel_prices.size()) * next_rnd()) % fuel_prices.size();
                dollar_per_liter = fuel_prices[idx] - (0.001f / 1_l);
                std::cout << "rnd: idx " << idx <<
                             ", fuel price " << old_fuel_price * 1_l << " -> " << dollar_per_liter * 1_l << " $/l" << std::endl;
            } while( equals(old_fuel_price, dollar_per_liter) );
        }
        if(parken && car1.intersects(car1.parking_ground()) && car2.intersects(car2.parking_ground())){
            velo_limit.dtime = 0;
        }
        velo_limit.dtime -= dt;
        if(velo_limit.dtime <= 0){
            float max_velo_old = max_velo;
            do {
                size_t a = (size_t)((float)(velo_limits.size()) * next_rnd()) % velo_limits.size();
                max_velo = velo_limits[a];
                std::cout << "rnd: idx " << a <<
                             ", max_velo " << max_velo_old * 3.6f << " -> " << max_velo * 3.6f << " km/h" << std::endl;
            } while( equals(max_velo_old, max_velo) );
            if(parken){
                if(!car1.intersects(car1.parking_ground())){
                    car1.decrease_life();
                    car1.reset(false);
                }
                if(!car2.intersects(car2.parking_ground())){
                    car2.decrease_life();
                    car2.reset(false);
                }
                parken = false;
            } else if(max_velo > 500_km_h){
                max_velo = 50_km_h;
                parken = true;
            }
            velo_limit.dtime = velo_limit.dtime_max;
        }
        car1.intersection(car2, dt);
        if(!car1.intersects(tankstelle)){
            car1.set_tankt(false);
        }
        if(!car2.intersects(tankstelle)){
            car2.set_tankt(false);
        }
        if(!car1.intersects(werkstatt)){
            car1.set_repairing(false);
        }
        if(!car2.intersects(werkstatt)){
            car2.set_repairing(false);
        }
        if(!car1.is_car_exploded()){
            car1.tick(dt, car2.state().digging_tunnel);
        } else {
            printerr("GAME OVER: car 1 is exploded!\n");
            exit(1);
        }
        if(!car2.is_car_exploded()){
            car2.tick(dt, car1.state().digging_tunnel);
        } else {
            printerr("GAME OVER: car 2 is exploded!\n");
            exit(2);
        }
        const size_t pre_ai_sz=AI_cars.size();
        for(size_t i = AI_cars.size(); i-->0;) {
            auto &AI_car = AI_cars[i];
            if(!AI_car->tick(dt, false)){
                AI_cars.erase(AI_cars.begin()+ssize_t(i));
            }
        }
        if(debug_prt && AI_cars.size() != pre_ai_sz) {
            printf("XXX.X: AI_cars %zu -> %zu\n", pre_ai_sz, AI_cars.size());
        }
    }
    tankstelle.draw(false);
    werkstatt.draw(false);
    car1.draw();
    car2.draw();
    for(const rect_t& tLicet : TunnelsLicet) {
        set_pixel_color(0, 255, 0, 255);
        tLicet.draw(false);
    }
    set_pixel_color(0, 0, 0, 0);
    for(const Tunnel& t : tunnels) {
        t.draw();
    }
    for(const rect_t& house : houses) {
        house.draw();
    }
    for(const auto& AI_car : AI_cars) {
         AI_car->draw();
    }
    swap_pixel_fb(false);
    for(pixel::texture_ref &tex : text_list) {
        tex->draw_fbcoord(0, 0);
    }
    text_list.clear();
    swap_gpu_buffer(fps);
}

int main(int argc, char *argv[])
{
    unsigned int window_width = 1920, window_height = 1000;
    #if defined(__EMSCRIPTEN__)
        win_width = 1024, win_height = 576; // 16:9
    #endif
    {
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-width", argv[i]) && i+1<argc) {
                window_width = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-height", argv[i]) && i+1<argc) {
                window_height = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-lifes", argv[i]) && i+1<argc) {
                lifes = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-debug", argv[i]) ) {
                debug_gfx = true;
                debug_prt = true;
            } else if( 0 == strcmp("-debug_gfx", argv[i]) ) {
                debug_gfx = true;
            } else if( 0 == strcmp("-debug_prt", argv[i]) ) {
                debug_prt = true;
            } else if( 0 == strcmp("--help", argv[i]) ) {
                printf("-debug: set debug_gfx and debug_prt on\n");
                printf("-debug_gfx: set debug_gfx on\n");
                printf("-debug_prt: set debug_prt on\n");
                printf("-lifes [uint8_t]: set number of lifes\n\n");
            }
        }
    }
    {
        const float origin_norm[] = { 0.5f, 0.5f };
        init_gfx_subsystem(argv[0], "car", (int)window_width, (int)window_height, origin_norm, true);
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
    TunnelsLicet.emplace_back(point_t(0, 0), 20_m, 15_m, 1_rad);
    TunnelsLicet.emplace_back(point_t(cart_coord.max_x()-40_m, 20_m), 30_m, 50_m);
    TunnelsLicet.emplace_back(point_t(-50, -10), 20_m, 30_m, 30_deg);
    TunnelsLicet.emplace_back(point_t(-80, 40), 30_m, 50_m);

    houses.emplace_back(point_t(-10_m, cart_coord.height()/4-3_m), 30_m, 10_m);
    houses.emplace_back(point_t(cart_coord.min_x(), cart_coord.max_y()), 6_m, 7_m);
    houses.emplace_back(point_t(-20_m, cart_coord.min_y()+25_m), 40_m, 20_m);
    houses.emplace_back(point_t(houses[0].m_tr.x + 5_m, houses[0].m_tr.y), 10_m, 40_m);

    tankstelle = {{cart_coord.min_x(), 0}, 4_m, 5_m};
    werkstatt = {{cart_coord.max_x() - 4_m, 0}, 4_m, 5_m};

    //make_bahnhoefe();
    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
}
