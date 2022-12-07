#include <pixel/pixel3f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"
#include <random>

const float world_height = 2.0f; // [m]
const float earth_accel = 9.81f; // [m/s*s]
float meter_to_pixel(const float m) noexcept {
    return m * ( (float)pixel::fb_height / world_height );
}
pixel::f2::vec_t meter_to_pixel(const pixel::f2::vec_t& m) noexcept {
    return m * ( (float)pixel::fb_height / world_height );
}
float pixel_to_meter(const float p) noexcept {
    return p * world_height / (float)pixel::fb_height;
}

/**
 * A falling bouncing ball, i.e. gravity impact only
 */
class ball1_t : public pixel::f2::disk_t {
    public:
        /**
         *
         * v = s / t
         *   s = v * t
         *
         * a = v / t
         *   a = s / t^2
         *   s = a * t^2
         *
         * PE = m * a * s;      * [J] = [Kg] * [m/s]^2
         *    = m * v/t * s
         *    = m * ( s^2 ) / ( t^2 )
         *
         * KE = 1/2 * m * v^2;  * [J] = [Kg] * [m/s]^2
         *
         * PE == KE (conservation of energy)
         *   m * a * s = 1/2 * m * v^2
         *   a * s = 1/2 * v^2
         *   v^2 = 2 * a * s
         *
         *   s = 1/2 * 1/a * v^2
         *   s = 1/2 * t/v * v^2
         *   s = 1/2 * v * t
         *
         *
         */
        std::string id;
        float start_xpos; // [m]
        float start_ypos; // [m]
        float total_fall; // [m]
        float velocity; // [m/s]
        float velocity_max; // [m/s]

        /**
         *
         * @param x_m x position in [m] within coordinate system 0/0 center
         * @param y_m y position in [m] within coordinate system 0/0 center
         * @param r_m
         */
        ball1_t(std::string id_, float x_m, float y_m, const float r_m)
        : pixel::f2::disk_t(meter_to_pixel(x_m), meter_to_pixel(y_m), meter_to_pixel(r_m)),
          id(std::move(id_)), start_xpos(x_m), start_ypos(y_m), total_fall(world_height/2.0f+y_m),
          velocity(0.0f), velocity_max(std::sqrt( 2 * earth_accel * total_fall ))
        {
            printf("ball %s-i %f / %f -> %f / %f [p-coord]\n",
                   id.c_str(), x_m, y_m,
                   meter_to_pixel(x_m), meter_to_pixel(y_m));
            rotate(pixel::adeg_to_rad(90)); // rotate upwards
        }

        std::string toString() const noexcept override {
            return "disk[c " + std::to_string(pixel_to_meter((float)pixel::cart_to_fb_x(center.x))) + "/"
                             + std::to_string(pixel_to_meter((float)pixel::cart_to_fb_y(center.y))) +
                   ", r " + std::to_string(pixel_to_meter(radius)) +
                   "]"; }

        void reset() noexcept {
            velocity = 0.0f;
            velocity_max = std::sqrt( 2 * earth_accel * total_fall );
            center.x = meter_to_pixel(start_xpos);
            center.y = meter_to_pixel(start_ypos)-radius;
        }

        void tick(const float dt) noexcept {
            if( velocity_max > 0.0005f ) {
                velocity += -earth_accel * dt; // [m/s]
                // Textbook static calculus of 's'
                // const float ds_m = ( velocity * dt ) + ( 0.5f * accel * dt*dt); // [m/s] * [s] = [m]
                const float ds_m = ( velocity * dt ); // [m/s] * [s] = [m] -> matches velocity_max roughly in this simulation
                pixel::f2::aabbox_t move_box(this->box());
                move_dir( meter_to_pixel(ds_m) );
                move_box.resize(this->box());

                if( false ) {
                    printf("ball %s-t: dt %f [s], v %f / %f [m/s], ds %f [m], %s\n",
                           id.c_str(), dt, velocity, velocity_max, ds_m, toString().c_str());
                }
                pixel::f2::geom_t* coll = nullptr;
                if( velocity < 0.0f ) {
                    // pixel::f2::aabbox_t b = box();
                    std::vector<pixel::f2::geom_t*>& list = pixel::f2::gobjects();
                    for(pixel::f2::geom_t* g : list) {
                        if( g->box().intersects(move_box) ) {
                            coll = g;
                            break;
                        }
                    }
                }
                if( nullptr != coll ) {
                    printf("Ball %s-e-a: vl %f m/s, vm %f m/s, %s, %s\n",
                            id.c_str(), velocity, velocity_max, toString().c_str(), box().toString().c_str());
                    printf("Ball %s-e-a: move_box %s, coll_box %s\n", id.c_str(), move_box.toString().c_str(), coll->box().toString().c_str());
                    // reset ball to start
                    velocity_max *= 0.75f; // rho
                    velocity = velocity_max;
                    // velocity *= -1.0f * 0.75f; // rho
                    center.y = coll->box().tr.y+radius;
                    printf("Ball %s-e-b: vl %f m/s, vm %f m/s, %s\n", id.c_str(), velocity, velocity_max, toString().c_str());
                } else if( !this->on_screen() ) {
                    printf("Ball %s-off: vl %f m/s, vm %f m/s, %s\n", id.c_str(), velocity, velocity_max, toString().c_str());
                    reset();
                }
            } else {
                reset();
            }
        }
};

/**
 * A bouncing ball w/ initial velocity in given direction plus gravity exposure (falling)
 */
class ball2_t : public pixel::f2::disk_t {
    public:
        /**
         *
         * v = s / t
         *   s = v * t
         *
         * a = v / t
         *   a = s / t^2
         *   s = a * t^2
         *
         * PE = m * a * s;      * [J] = [Kg] * [m/s]^2
         *    = m * v/t * s
         *    = m * ( s^2 ) / ( t^2 )
         *
         * KE = 1/2 * m * v^2;  * [J] = [Kg] * [m/s]^2
         *
         * PE == KE (conservation of energy)
         *   m * a * s = 1/2 * m * v^2
         *   a * s = 1/2 * v^2
         *   v^2 = 2 * a * s
         *
         *   s = 1/2 * 1/a * v^2
         *   s = 1/2 * t/v * v^2
         *   s = 1/2 * v * t
         *
         *
         */
        std::string id;
        float start_xpos; // [m]
        float start_ypos; // [m]
        float velocity_start; // [m/s] @ angle
        float total_fall; // [m]
        float velocity_max; // [m/s]
        pixel::f2::vec_t velocity; // [m/s]

        /**
         *
         * @param x_m x position in [m] within coordinate system 0/0 center
         * @param y_m y position in [m] within coordinate system 0/0 center
         * @param r_m
         */
        ball2_t(std::string id_, float x_m, float y_m, const float r_m, const float velocity_, const float v_angle_rad)
        : pixel::f2::disk_t(meter_to_pixel(x_m), meter_to_pixel(y_m), meter_to_pixel(r_m)),
          id(std::move(id_)), start_xpos(x_m), start_ypos(y_m), velocity_start(velocity_), total_fall(world_height),
          velocity_max(std::sqrt( 2 * earth_accel * total_fall )), velocity()
        {
            rotate(v_angle_rad); // direction of velocity
            velocity = pixel::f2::vec_t::from_length_angle(velocity_start, this->angle);
            printf("ball %s-i %f / %f -> %f / %f [p-coord]\n",
                   id.c_str(), x_m, y_m, meter_to_pixel(x_m), meter_to_pixel(y_m));
            printf("Ball %s-i: v %s, |%f| / %f m/s, %s\n",
                    id.c_str(), velocity.toString().c_str(), velocity.norm(), velocity_max, box().toString().c_str());
        }

        std::string toString() const noexcept override {
            return "disk[c " + std::to_string(pixel_to_meter((float)pixel::cart_to_fb_x(center.x))) + "/"
                             + std::to_string(pixel_to_meter((float)pixel::cart_to_fb_y(center.y))) +
                   ", r " + std::to_string(pixel_to_meter(radius)) +
                   "]"; }

        void reset() noexcept {
            velocity_max = std::sqrt( 2 * earth_accel * total_fall );
            if( velocity_start > 0.0f ) {
                // shoot back ;-)
                this->angle = M_PI - this->angle; // 180 - adeg
            } else {
                center.x = meter_to_pixel(start_xpos);
                center.y = meter_to_pixel(start_ypos)-radius;
            }
            velocity = pixel::f2::vec_t::from_length_angle(velocity_start, this->angle);
        }

        void tick(const float dt) noexcept {
            if( velocity_max > 0.00005f ) {
                // Leave horizontal velocity untouched, will be reduced at bounce
                velocity.y += -earth_accel * dt;
                pixel::f2::vec_t ds_m_dir = velocity * dt; // [m/s] * [s] = [m] -> matches velocity_max roughly in this simulation
                pixel::f2::aabbox_t move_box(this->box());
                this->move( meter_to_pixel(ds_m_dir) );
                move_box.resize(this->box());

                if( false ) {
                    printf("ball %s-t: dt %f [s], v %s, |%f| / %f [m/s], ds %s [m/s]\n",
                           id.c_str(), dt, velocity.toString().c_str(), velocity.norm(), velocity_max, ds_m_dir.toString().c_str());
                    // printf("ball %s-t: move_box %s\n", id.c_str(), move_box.toString().c_str());
                    // printf("ball %s-t: this_box %s\n", id.c_str(), box().toString().c_str());
                }
                pixel::f2::geom_t* coll = nullptr;
                if( velocity.y < 0.0f ) {
                    // pixel::f2::aabbox_t b = box();
                    std::vector<pixel::f2::geom_t*>& list = pixel::f2::gobjects();
                    for(pixel::f2::geom_t* g : list) {
                        if( g->box().intersects(move_box) ) {
                            coll = g;
                            break;
                        }
                    }
                }
                if( nullptr != coll ) {
                    printf("Ball %s-e-a: v %s, |%f| / %f m/s, %s, %s\n",
                            id.c_str(), velocity.toString().c_str(), velocity.norm(), velocity_max, toString().c_str(), box().toString().c_str());
                    printf("Ball %s-e-a: move_box  %s, coll_box %s\n", id.c_str(), move_box.toString().c_str(), coll->box().toString().c_str());
                    // reset ball to start
                    velocity_max *= 0.75f; // rho
                    // velocity.y = velocity_max;
                    velocity.x *=         0.75f; // rho
                    velocity.y *= -1.0f * 0.75f; // rho
                    center.y = coll->box().tr.y+radius;
                    printf("Ball %s-e-b: vl %s m/s, vm %f m/s, %s, %s\n", id.c_str(), velocity.toString().c_str(), velocity_max, toString().c_str(), box().toString().c_str());
                } else if( !this->on_screen() ) {
                    printf("Ball %s-off: v %s, |%f| / %f m/s, %s, %s\n",
                            id.c_str(), velocity.toString().c_str(), velocity.norm(), velocity_max, toString().c_str(), box().toString().c_str());
                    reset();
                }
            } else {
                reset();
            }
        }
};

int main(int argc, char *argv[])
{
    int win_width = 1920, win_height = 1080;
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
        pixel::init_gfx_subsystem("freefall01b", win_width, win_height, origin_norm);
    }

    const float ball_height = 0.05f; // [m] .. diameter
    const float ball_radius = ball_height/2.0f; // [m]

    ball1_t ball_1( "one", -4.0f*ball_height, world_height/2.0f-ball_radius, ball_radius );
    ball2_t ball_2( "two", +2.0f*ball_height, world_height/2.0f-ball_radius, ball_radius,
                    0.0f /* [m/s] */, pixel::adeg_to_rad(90));
    ball2_t ball_3( "can",  pixel_to_meter((float)pixel::cart_min_x)+2*ball_height, pixel_to_meter((float)pixel::cart_min_y)+ball_height, ball_radius,
                    6.2f /* [m/s] */, pixel::adeg_to_rad(78));
    {
        std::vector<pixel::f2::geom_t*>& list = pixel::f2::gobjects();
        float thickness = 2; // meter_to_pixel(ball_height/8.0f);
        if( false ) {
            pixel::f2::point_t tl = { (float)pixel::cart_min_x+thickness, (float)pixel::cart_max_y-thickness };
            pixel::f2::rect_t* r = new pixel::f2::rect_t(tl, (float)pixel::fb_max_x-2*thickness, thickness);
            list.push_back(r);
        }
        {
            pixel::f2::point_t bl = { (float)pixel::cart_min_x+10, (float)pixel::cart_min_y+10 };
            pixel::f2::rect_t* r = new pixel::f2::rect_t(bl, (float)pixel::fb_max_x-2*10, thickness);
            list.push_back(r);
        }
    }

    bool close = false;
    bool resized = false;
    bool set_dir = false;
    pixel::direction_t dir = pixel::direction_t::UP;

    uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]

    while(!close) {
        handle_events(close, resized, set_dir, dir);

        // white background
        pixel::clear_pixel_fb(255, 255, 255, 255);

        const uint64_t t = pixel::getElapsedMillisecond(); // [ms]
        const float dt = (float)( t - t_last ) / 1000.0f; // [s]
        const float dt_exp = 1.0f / (float)pixel::frames_per_sec; // [s]
        const float dt_diff = (float)( dt_exp - dt ) * 1000.0f; // [ms]
        t_last = t;

        // move ball_1
        ball_1.tick(dt);

        // move ball_2
        ball_2.tick(dt);

        // move ball_3
        ball_3.tick(dt);

        pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
        {
            std::vector<pixel::f2::geom_t*>& list = pixel::f2::gobjects();
            for(pixel::f2::geom_t* g : list) {
                g->draw();
            }
        }
        ball_1.draw();
        ball_2.draw();
        ball_3.draw();

        fflush(nullptr);
        if( dt_diff > 1.0f ) {
            pixel::milli_sleep( (uint64_t)dt_diff );
        }
        pixel::swap_pixel_fb();
    }
    exit(0);
}

