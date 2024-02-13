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
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"
#include <random>
#include <cstdio>
#include <cmath>
#include <iostream>

enum circle_t {
    filled02,
    line02,
    aabbox
};

void draw_circle_seg(pixel::f2::point_t pm, float r, float alpha1, float alpha2){
    float x;
    float y;
    float i = alpha1;
    for(; i <= alpha2; i += 0.01){
        x = std::cos(i) * r;
        y = std::sin(i) * r;
        pixel::f2::point_t p0 = pixel::f2::point_t(x, y);
        p0 += pm;
        p0.draw();
    }
}

void draw_circle(pixel::f2::point_t m, float r, float thickness, circle_t e){
    float y;
    float x;
    float r1 = r + thickness / 2;
    for(y = r + thickness; y >= -r - thickness; --y){
        for(x = -r - thickness; x <= r + thickness; ++x) {
            float l = sqrt(x*x + y*y);
            switch(e){
            case circle_t::line02:
                if(l >= r && l <= r + thickness){
                    pixel::f2::point_t p(x, y);
                    p += m;
                    p.draw();
            }
                break;
            case circle_t::filled02:
                if(l <= r1){
                    pixel::f2::point_t p(x, y);
                    p += m;
                    p.draw();
                }
                break;
            case circle_t::aabbox:
                if(l > r1){
                    pixel::f2::point_t p(x, y);
                    p += m;
                    p.draw();
                }
                break;
            }
        }
    }
}

void draw_rechteck_rotate(pixel::f2::point_t p0, float b, float h, float alpha){
    pixel::f2::point_t pm = pixel::f2::point_t(p0).add(b / 2, 0);
    pixel::f2::point_t pr = pixel::f2::point_t(pm).add( b/2, h/2 );
    pixel::f2::point_t p1 = pixel::f2::point_t(pm).add(b, 0);
    pixel::f2::point_t p2 = pixel::f2::point_t(pm).add(b, h);
    pixel::f2::point_t p3 = pixel::f2::point_t(pm).add(0, h);
    pixel::f2::point_t p[] = { pm, p1, p2, p3 };
    for(int i=0; i < 4; ++i) {
        // pixel::f2::point_t& pm = p[i - 1];
        pixel::f2::point_t& pn = p[i];
        pixel::f2::vec_t v1 = pn - pr;
        float v1_l = v1.length();
        float v1_a = v1.angle();
        float A3 = v1_a + alpha;
        pn = pr + pixel::f2::point_t::from_length_angle(v1_l, A3);
    }
    for(int i=1; i <= 4; ++i) {
        pixel::f2::point_t& pa = p[i - 1];
        pixel::f2::point_t& pn = p[i%4];
        pixel::f2::lineseg_t(pa, pn);
    }
}

void draw_circle_unroll(pixel::f2::point_t pm, float r, float off_pct){
    const float u0 = 2.0f * r * M_PI;
    const float u0_a = 2 * M_PI;
    float off = u0 * off_pct;
    float u1 = u0 - off;
    float u1_a = u0_a * u1 / u0;
    // u0a * (u0 - off) / u0 =
    // u0a * (u0 - u0 * off_pct) / u0 =
    // u0a * ( 1 - off_pct )
    pixel::f2::point_t p0 = pixel::f2::point_t(pm.x + off, pm.y - r);
    pixel::f2::point_t p1 = pixel::f2::point_t(p0).add(-off, 0);
    pixel::f2::lineseg_t::draw(p0, p1);
    float w = pixel::adeg_to_rad(270);
    draw_circle_seg(pixel::f2::point_t(pm.x + off, pm.y), r, w, w + u1_a);
}

void draw_sin_cos(const pixel::f2::point_t& center, const float r, const float alpha, const float d){
    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
    draw_circle(center, r, d, circle_t::line02);
    const float x1 = cos(alpha) * r;
    const float y1 = sin(alpha) * r;
    const pixel::f2::point_t p0(center.x + x1, center.y);
    const pixel::f2::point_t p1(center.x + x1, center.y + y1);

    pixel::set_pixel_color(255 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
    pixel::f2::lineseg_t::draw(center, p0); // cosine
    draw_rechteck_rotate(center, r, d, 0);

    pixel::set_pixel_color(0 /* r */, 0 /* g */, 255 /* b */, 255 /* a */);
    pixel::f2::lineseg_t::draw(p0, p1); // sine
    pixel::f2::point_t p2(p0);
    pixel::f2::point_t p3(p1);
    float l = (std::abs((p3 - p2).length()));
    draw_rechteck_rotate(p0, l, d, M_PI / 2);

    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
    pixel::f2::lineseg_t::draw(p1, center); // radius
    draw_rechteck_rotate(center, r, d, alpha);
}

double circumferenceInner(double r, double n){
    const double alpha = 2.0 * M_PI / n;
    double p0_x = r * std::cos(2.0 * M_PI - alpha);
    double p0_y = r * std::sin(2.0 * M_PI - alpha);
    double p1_x = r;
    double p1_y = 0.0;
    double dx = p0_x - p1_x;
    double dy = p0_y - p1_y;
    return n * sqrt( dx*dx + dy*dy );
    // pixel::f2::point_t p0 = pixel::f2::vec_t::from_length_angle(r, 2 * M_PI - alpha);
    // pixel::f2::point_t p1 = pixel::f2::vec_t::from_length_angle(r, 0);
    // return n * (p0 - p1).length();
}
float draw_circumferenceInner(pixel::f2::point_t center, float r, int n){
    const float alpha = 2 * M_PI / n;
    float res;
    pixel::f2::point_t p0 = center + pixel::f2::vec_t::from_length_angle(r, 2 * M_PI - alpha);
    {
        pixel::f2::point_t p1 = center + pixel::f2::vec_t::from_length_angle(r, 0);
        res = n * (p0 - p1).length();
    }
    for(float i = 0.0f; i < 2 * M_PI; i += alpha){
        pixel::f2::point_t p1 = center + pixel::f2::vec_t::from_length_angle(r, i);
        pixel::f2::lineseg_t::draw(p0, p1);
        p0 = p1;
    }
    return res;
}

double circumferenceOutter(double r, double n){
    // const float alpha = 2 * M_PI / n;
    // return n * std::tan(alpha / 2) * 2 * r;
    return n * std::tan(M_PI / n) * 2.0 * r;
}
float draw_circumferenceOutter(pixel::f2::point_t center, float r, int n){
    const float alpha = 2 * M_PI / n;
    float res, l_2, l;
    {
        l = std::tan(alpha / 2) * 2 * r;
        l_2 = l / 2;
        res = n * l;
    }
    for(float i = 0.0f; i < 2 * M_PI; i += alpha){
        pixel::f2::point_t p1 = center + pixel::f2::point_t::from_length_angle(r, i);
        pixel::f2::point_t p1a = p1 + pixel::f2::vec_t::from_length_angle(l_2, i + M_PI_2);
        pixel::f2::point_t p1b = p1 + pixel::f2::vec_t::from_length_angle(l_2, i - M_PI_2);
        pixel::f2::lineseg_t::draw(p1a, p1b);
    }
    return res;
}

int equalDecDigits(double a, double b, int maxDecDigits) {
    if( (int)a != (int)b ) {
        return -1;
    }
    a = a - (int)a;
    b = b - (int)b;
    int i = 0;
    for(; i < maxDecDigits; ++i) {
        a *= 10.0;
        b *= 10.0;
        if( (int)a != (int)b ) {
            break;
        }
        a = a - (int)a;
        b = b - (int)b;
    }
    return i;
}
/**
 * Returns an approximation of PI with given accuracy in decimal digits
 * @param decimalAccuracy decimal digits (after the dot) which must match PI
 * @param cornerCount number of corners (return value)
 */
double approxPi(const int decimalAccuracy, long& cornerCount) {
    const double r = 1; // unit circle
    const double d = 2 * r;
    long corners = 4;
    int matchingDecimals = 0;
    while( true ) {
        const double ci = circumferenceInner(1.0, corners);
        const double co = circumferenceOutter(1.0, corners);
        const double pi_i = ci/d;
        const double pi_o = co/d;
        matchingDecimals = equalDecDigits(pi_i, pi_o, decimalAccuracy+1);
        printf("%ld: accuracy %d / %d\n", corners, matchingDecimals, decimalAccuracy+1);
        printf("%ld: ci[%lf, ~pi %20.20lf]\n", corners, ci, pi_i);
        printf("%ld: co[%lf, ~pi %20.20lf]\n", corners, co, pi_o);
        if( matchingDecimals == decimalAccuracy+1 ) {
            cornerCount = corners;
            return pi_i;
        }
        if( corners * 2l < 0 ) {
            printf("Overflow Error\n");
            cornerCount = corners;
            return pi_i;
        }
        corners *= 2l;
    }
    return 0; // never ever
}

float get_fract(float f) {
    return f - (int)f;
}

void draw_sin_cos_graph(const float r, const float alpha_max, const float angrad_inc, const float plot_inc) {
    pixel::f2::point_t plotpos(pixel::cart_coord.min_x(), 0);
    pixel::f2::point_t pcos, psin;
    const float thickness = r/200;
    {
        pixel::f2::point_t p0(pixel::cart_coord.min_x(), 0);
        pixel::f2::point_t p1(pixel::cart_coord.max_x(), 0);
        pixel::set_pixel_color(0, 0, 0, 255);
        pixel::f2::lineseg_t::draw(p0, p1);
    }
    for(float alpha = 0; alpha < alpha_max; alpha += angrad_inc) {
        plotpos.add(plot_inc, 0);
        if( plotpos.x > pixel::cart_coord.width() ) {
            plotpos.set(0, plotpos.y);
        }
        if( get_fract(alpha / (2 * M_PI)) < angrad_inc/2.0f ) { // std::numeric_limits<float>::epsilon() ){
            pixel::set_pixel_color(255, 0, 255, 255);
            pixel::f2::point_t p0(plotpos.x, pixel::cart_coord.max_y());
            pixel::f2::point_t p1(plotpos.x, pixel::cart_coord.min_y());
            pixel::f2::lineseg_t::draw(p0, p1);
        }
        float cosval = cos(alpha) * r;
        pcos.set(plotpos.x, plotpos.y + cosval);
        pixel::set_pixel_color(255 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
        // pcos.draw();
        pixel::f2::disk_t(pcos, thickness).draw();

        float sinval = sin(alpha) * r;
        psin.set(plotpos.x, plotpos.y + sinval);
        pixel::set_pixel_color(0 /* r */, 0 /* g */, 255 /* b */, 255 /* a */);
        // psin.draw();
        pixel::f2::disk_t(psin, thickness).draw();
    }
    pixel::set_pixel_color(255 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
    pixel::f2::disk_t(pcos, thickness*2).draw();
    pixel::set_pixel_color(0 /* r */, 0 /* g */, 255 /* b */, 255 /* a */);
    pixel::f2::disk_t(psin, thickness*2).draw();
}

int main(int argc, char *argv[])
{
    unsigned int win_width = 1920, win_height = 1000;
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
        pixel::init_gfx_subsystem("gfxbox example01", win_width, win_height, origin_norm);
    }

    bool close = false;
    bool resized = false;
    bool set_dir  = false;
    bool set_dir2 = false;
    pixel::direction_t dir = pixel::direction_t::UP;
    pixel::texture_ref hud_text;
    float last_fps = -1.0f;

    pixel::log_printf(0, "XX %s\n", pixel::cart_coord.toString().c_str());
    {
        float w = pixel::cart_coord.width();
        float h = pixel::cart_coord.height();
        float r01 = h/w;
        float a = w / h;
        printf("-w %f [x]\n-h %f [y]\n-r1 %f [y/x]\n-r2 %f [x/y]", w, h, r01, a);
    }
    printf("Pre-Loop\n");

    const int circles_per_plot = 2;
    const float ticks_per_circle = 12.0f * 60.0f; // one circle in 12s @ 60Hz, [s] * [1/s] = [1]
    const float plot_inc = pixel::cart_coord.width() / ( ticks_per_circle * circles_per_plot );
    const float angrad_inc = ( 2.0f * M_PI ) / ticks_per_circle;
    float angrad = 0.0f;
    const float grid_gap = 50;
    //float max_radius = pixel::cart_coord.max_y() * 0.9f;
    int circum_corners = 3;
/*
    {
        const int decimalAccuracy = 10;
        long cornerCount;
        double api = approxPi(decimalAccuracy, cornerCount);
        printf("DecimalAccuracy: %d, corners %ld\n", decimalAccuracy, cornerCount);
        printf("Our-PI-Approx: %20.20lf\n", api);
        printf("Std-PI-Approx: %20.20lf\n", M_PI);
    }
*/
    pixel::f2::point_t center(0, 0);
    bool manual = false;
    while( !close ) {
        handle_events2(close, resized, set_dir, set_dir2, dir);
        const bool animating = pixel::direction_t::PAUSE != dir;
        manual = manual && animating;

        float fps = pixel::get_gpu_fps();
        if( true ) { // fps != last_fps ) {
            pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
            // hud_text = pixel::make_text_texture("fps "+std::to_string(fps)+", corners "+std::to_string(circum_corners));
            hud_text = pixel::make_text_texture("fps "+std::to_string(fps)+", "+(animating?"animating":"paused")+
                                                ", "+(manual?"manual":"auto"));
            // printf("XX: fps %f -> %f, hud %d\n", last_fps, fps, nullptr!=hud_text);
            last_fps = fps;
            (void)last_fps;
        }
        const float max_radius = pixel::cart_coord.max_y() * 0.9f;

        // white background
        pixel::clear_pixel_fb(255, 255, 255, 255);
        pixel::draw_grid(grid_gap,
                225 /* r */, 225 /* g */, 225 /* b */, 255 /* a */,
                200 /* r */, 200 /* g */, 200 /* b */, 255 /* a */);

        if( set_dir ) {
            set_dir = false;
                if( pixel::direction_t::UP == dir ) {
                    manual = true;
                    if( circum_corners < 128 ) {
                        circum_corners += 1;
                    }
                    angrad += 2*angrad_inc;
                } else if( pixel::direction_t::DOWN == dir ) {
                    manual = true;
                    if( circum_corners >= 4 ) {
                     circum_corners -= 1;
                    }
                    angrad -= 2*angrad_inc;
                }
        }
        draw_sin_cos_graph(max_radius, angrad, angrad_inc, plot_inc);
        draw_sin_cos(center, max_radius, angrad, 5);

        //pixel::set_pixel_color(0 /* r */, 0 /* g */, 255 /* b */, 255 /* a */);
        //draw_circle(center, max_radius, 1, circle_t::line02);
        //pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
        //draw_circumferenceInner(center, max_radius, circum_corners);
        //draw_circumferenceOutter(center, max_radius, circum_corners);

        if( animating && !manual ){
            angrad += angrad_inc;
        }
        if(angrad > circles_per_plot * 2 * M_PI){
            angrad = 0.0f;
        }

        pixel::swap_pixel_fb(false);
        if( nullptr != hud_text ) {
            hud_text->draw(0, 0);
        }
        pixel::swap_gpu_buffer();
    }
    printf("Exit\n");
    exit(0);
}
