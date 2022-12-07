#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"
#include <random>

const float world_height = 2.0f; // [m]
const float earth_accel = 9.81f; // [m/s*s]
float world_to_pixel(const float m) {
    return m * (float)pixel::fb_height / world_height;
}

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
        pixel::init_gfx_subsystem("freefall01a", win_width, win_height, origin_norm);
    }

    const float ball_height = 0.05f; // [m] .. diameter

    const float ball_r = world_to_pixel(ball_height/2.0f); // [pixel]

    uint64_t t0_ball_1 = pixel::getElapsedMillisecond(); // [ms]
    pixel::f2::point_t ball_1_top = { -ball_r-ball_r, (float)pixel::cart_max_y-ball_r };
    pixel::f2::disk_t ball_1( ball_1_top, ball_r);
    ball_1.rotate(pixel::adeg_to_rad(90)); // rotate upwards
    const float ball_1_a = -earth_accel; // gravity down
    float ball_1_v = 0.00; // [m/s]

    uint64_t t0_ball_2 = t0_ball_1; // [ms]
    pixel::f2::point_t ball_2_top = { +ball_r+ball_r, (float)pixel::cart_max_y-ball_r };
    pixel::f2::disk_t ball_2( ball_2_top, ball_r);
    ball_2.rotate(pixel::adeg_to_rad(90)); // rotate upwards
    const float ball_2_a = -earth_accel; // gravity down
    float ball_2_v = 0.00; // [m/s]
    //
    // v = s / t
    //   s = v * t
    //
    // a = v / t
    //   a = s / t^2
    //   s = a * t^2
    //
    // PE = m * a * s;     // [J] = [Kg] * [m/s]^2
    //    = m * v/t * s
    //    = m * ( s^2 ) / ( t^2 )
    //
    // KE = 1/2 * m * v^2; // [J] = [Kg] * [m/s]^2
    //
    // PE == KE (conservation of energy)
    //   m * a * s = 1/2 * m * v^2
    //   a * s = 1/2 * v^2
    //   v^2 = 2 * a * s
    float ball_2_height = world_height;
    float ball_2_vmax = std::sqrt( 2 * std::abs(ball_2_a) * ball_2_height );

    bool close = false;
    bool resized = false;
    bool set_dir = false;
    pixel::direction_t dir = pixel::direction_t::UP;

    uint64_t t_last = t0_ball_1; // [ms]

    while(!close) {
        handle_events(close, resized, set_dir, dir);

        // white background
        pixel::clear_pixel_fb(255, 255, 255, 255);


        const uint64_t t = pixel::getElapsedMillisecond(); // [ms]
        const float dt = (float)( t - t_last ) / 1000.0f; // [s]
        const float dt_exp = 1.0f / (float)pixel::frames_per_sec; // [s]
        const float dt_diff = ( dt_exp - dt ) * 1000.0f; // [ms]
        t_last = t;

        // move ball_1
        if( true ) {
            const float dv = ball_1_a * dt; // [m/s]
            ball_1_v += dv;
            const float ds_m = ball_1_v * dt; // [m/s] * [s] = [m]
            ball_1.move_dir( world_to_pixel(ds_m) );
            //printf("ball_1: dt %f [s], dt_diff %f [ms], dv %f -> %f [m/s], ds %f [m], %s\n",
            //        dt, dt_diff, dv, ball_1_v, ds_m, ball_1.toString().c_str());

            // ball exit screen
            if( !ball_1.on_screen() ) {
                const float tg = (float)(t - t0_ball_1) / 1000.0f; // [s]
                const float vg = world_height / tg;
                printf("Ball 1: tg %f s, vg %f m/s, vl %f m/s\n", tg, vg, ball_1_v);
                // reset ball to start
                t0_ball_1 = t;
                ball_1.set_center( ball_1_top );
                ball_1_v = 0.0;
            }
        }

        // move ball_2
        if( ball_2_vmax > 0.000f ) {
            const float dv = ball_2_a * dt; // [m/s]
            ball_2_v += dv;
            const float ds_m = ( ball_2_v * dt ) + ( 0.5f * ball_2_a * dt*dt); // [m/s] * [s] = [m]
            ball_2.move_dir( world_to_pixel(ds_m) );
            //printf("ball_2: dt %f [s], dt_diff %f [ms], dv %f -> %f / %f [m/s], ds %f [m], %s\n",
            //        dt, dt_diff, dv, ball_2_v, ball_2_vmax, ds_m, ball_2.toString().c_str());
            if( !ball_2.on_screen() ) {
                const float tg = (float)(t - t0_ball_2) / 1000.0f; // [s]
                const float vg = world_height / tg;
                printf("Ball 2: tg %f s, vg %f m/s, vl %f / %f m/s\n", tg, vg, ball_2_v, ball_2_vmax);
                // reset ball to start
                t0_ball_2 = t;
                ball_2_vmax *= 0.75f; // rho
                ball_2_v = ball_2_vmax;
                ball_2.center.y = (float)pixel::cart_min_y+ball_2.radius;
            }
        }

        pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
        ball_1.draw();
        ball_2.draw();

        fflush(nullptr);
        if( dt_diff > 1.0f ) {
            pixel::milli_sleep( (uint64_t)dt_diff );
        }
        pixel::swap_pixel_fb();
    }
    exit(0);
}

