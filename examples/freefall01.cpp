#include <pixel/pixel3f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"

const float world_height = 2.0f; // [m]
const float earth_accel = 9.81f; // [m/s*s]

/**
 * A bouncing ball w/ initial velocity in given direction plus gravity exposure (falling)
 */
class ball_t : public pixel::f2::disk_t {
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
        ball_t(std::string id_, float x_m, float y_m, const float r_m, const float velocity_, const float v_angle_rad)
        : pixel::f2::disk_t(x_m, y_m, r_m),
          id(std::move(id_)), start_xpos(x_m), start_ypos(y_m), velocity_start(velocity_), total_fall(world_height),
          velocity_max(std::sqrt( 2 * earth_accel * total_fall )), velocity()
        {
            rotate(v_angle_rad); // direction of velocity
            velocity = pixel::f2::vec_t::from_length_angle(velocity_start, this->angle);
            printf("ball %s-i %f / %f\n", id.c_str(), x_m, y_m);
            printf("Ball %s-i: v %s, |%f| / %f m/s, %s\n",
                    id.c_str(), velocity.toString().c_str(), velocity.norm(), velocity_max, box().toString().c_str());
        }

        void reset() noexcept {
            velocity_max = std::sqrt( 2 * earth_accel * total_fall );
            if( velocity_start > 0.0f ) {
                // shoot back ;-)
                this->angle = M_PI - this->angle; // 180 - adeg
            } else {
                center.x = start_xpos;
                center.y = start_ypos-radius;
            }
            velocity = pixel::f2::vec_t::from_length_angle(velocity_start, this->angle);
        }

        void tick(const float dt) noexcept {
            if( velocity_max > 0.00005f ) {
                // Leave horizontal velocity untouched, will be reduced at bounce
                velocity.y += -earth_accel * dt;
                pixel::f2::vec_t ds_m_dir = velocity * dt; // [m/s] * [s] = [m] -> matches velocity_max roughly in this simulation
                pixel::f2::aabbox_t move_box(this->box());
                this->move( ds_m_dir );
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

    pixel::cart_coord.set_height(0.0f, world_height+ball_radius);

    ball_t ball_1( "one", -4.0f*ball_height, world_height-ball_radius, ball_radius,
                    0.0f /* [m/s] */, pixel::adeg_to_rad(90));
    ball_t ball_2( "two", +2.0f*ball_height, world_height-ball_radius, ball_radius,
                    0.0f /* [m/s] */, pixel::adeg_to_rad(90));
    ball_t ball_3( "can",  pixel::cart_coord.min_x()+2*ball_height, pixel::cart_coord.min_y()+ball_height, ball_radius,
                    6.1f /* [m/s] */, pixel::adeg_to_rad(78));
    {
        std::vector<pixel::f2::geom_t*>& list = pixel::f2::gobjects();
        float thickness = 1.1f * ball_height;
        float small_gap = ball_radius;
        if( false ) {
            pixel::f2::point_t tl = { pixel::cart_coord.min_x()+thickness, pixel::cart_coord.max_y()-thickness };
            pixel::f2::rect_t* r = new pixel::f2::rect_t(tl, (float)pixel::fb_max_x-2*thickness, thickness);
            list.push_back(r);
        }
        {
            pixel::f2::point_t bl = { pixel::cart_coord.min_x()+small_gap, pixel::cart_coord.min_y()+small_gap };
            pixel::f2::rect_t* r = new pixel::f2::rect_t(bl, pixel::cart_coord.width()-2.0f*small_gap, thickness);
            list.push_back(r);
            printf("XX %s\n", pixel::cart_coord.toString().c_str());
            printf("XX %s\n", r->toString().c_str());
        }
    }

    bool close = false;
    bool resized = false;
    bool set_dir = false;
    pixel::direction_t dir = pixel::direction_t::UP;
    pixel::texture_ref hud_text;
    float last_fps = -1.0f;

    uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]

    while(!close) {
        handle_events(close, resized, set_dir, dir);

        {
            float fps = pixel::get_gpu_fps();
            if( fps != last_fps ) {
                hud_text = pixel::make_text_texture("fps "+std::to_string(fps));
                // printf("XX: fps %f -> %f, hud %d\n", last_fps, fps, nullptr!=hud_text);
                last_fps = fps;
            }
        }
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
        pixel::swap_pixel_fb(false);
        if( nullptr != hud_text ) {
            hud_text->draw(0, 0);
        }
        pixel::swap_gpu_buffer();
    }
    exit(0);
}

