/*
 * Author: Svenson-Han Göthel <shg@jausoft.com>
 * Copyright (c) 2024 Svenson-Han Göthel
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
#include <pixel/pixel4f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"
#include <random>
#include <cstdio>
#include <cmath>
#include <iostream>

using namespace std;
using namespace pixel::f2;

enum circle_t {
    filled,
    line,
    aabbox
};

void draw_circle_seg(const point_t& pm, float r, const float thickness, float const alpha1, const float alpha2){
    float x;
    float y;
    float i = alpha1;
    for(; i <= alpha2; i += 0.01){
        x = cos(i) * r;
        y = sin(i) * r;
        point_t p0 = point_t(x, y);
        p0 += pm;
        if( 1 >= thickness ) {
            p0.draw();
        } else {
            //const float t = thickness*2.0f/3.0f;
            rect_t(p0, thickness, thickness, true).draw(true);
        }
    }
}

void draw_circle(const point_t& pm, float r, float thickness, circle_t e){
    float y;
    float x;
    float r1 = r + thickness / 2;
    for(y = r + thickness; y >= -r - thickness; --y){
        for(x = -r - thickness; x <= r + thickness; ++x) {
            float l = sqrt(x*x + y*y);
            switch(e){
            case circle_t::line:
                if(l >= r && l <= r + thickness){
                    point_t p(x, y);
                    p += pm;
                    p.draw();
                }
                break;
            case circle_t::filled:
                if(l <= r1){
                    point_t p(x, y);
                    p += pm;
                    p.draw();
                }
                break;
            case circle_t::aabbox:
                if(l > r1){
                    point_t p(x, y);
                    p += pm;
                    p.draw();
                }
                break;
            }
        }
    }
}

static uint8_t unroll_colors[4][4] = {
        {   0,   0, 255, 0 },
        {   0, 255,   0, 0 },
        {   0,   0, 255, 0 },
        { 255,   0,   0, 0 }
};

/**
 * Zeichnen eines Kreissegments,
 * wobei der vom vollen Kreis fehlende Umfang am Boden als Linie dargestellt wird.
 *
 * Aufruf erfolgt innerhalb einer Animationsschleife mit wachsendem off_pct Wert,
 * so das die Animation eines abrollenden Kreises entsteht.
 * Die abgerollten Kreisanteile, d.h. der Umfang, wird als Linie am Boden gezeichnet.
 *
 * Hier wird das Ergebnis U/d farblich nach ganzen Zahlen getrennt dargestellt, d.h. π.
 *
 * @param pm ist der Kreismittelpunkt
 * @param r ist der Radius
 * @param thickness ist die Dicke
 * @param off_pct_in ist der Anteil vom Umfang in Prozent, welcher abgerollt werden soll
 */
void draw_circle_unroll(const point_t& pm, float r, const float thickness, float off_pct_in) {

    const float off_pct = min(1.0f, off_pct_in); // Abzurollender Teil vom Umfang in Prozent <= 1
    const float u_unit = 2.0f * M_PI; // Umfang Einheitskreis
    const float off = (r * u_unit) * off_pct; // Abzurollender Teil vom Kreisumfang

    // Male den Umfang als Linie,
    // wobei pro Durchmesser eine eigene Farbe gewaehlt wird, d.h. U/d visualisiert.
    point_t p0 = point_t(pm.x, pm.y - r);
    int count = 0;
    for(float off_left=off; off_left > 0; off_left -= r * 2, ++count) {
        pixel::set_pixel_color(unroll_colors[count]); // Farbe pro Durchmesser
        float off_done = off - off_left; // Schon gemalter Anteil
        float off_now = min(off_left, r * 2); // Aktuelle Position
        point_t tl = point_t(p0).add(off_done, 0); // top_left für Rechteck
        rect_t(tl, off_now, thickness).draw(true);
    }
    // Male Kreissegment
    const float u1_a = u_unit * ( 1 - off_pct ); // Uebrigbleibender Winkel
    pixel::set_pixel_color(0, 0, 0, 255);
    float w = pixel::adeg_to_rad(270); // Anfangswinkel vom Kreissegment
    // w + u1_a = Endwinkel vom Kreissegment
    draw_circle_seg(point_t(pm.x + off, pm.y), r, thickness, w, w + u1_a);
}

/**
 * Eine Visualisierung von Sinus und Cosinus wird erstellt.
 *
 * Ein Punkt auf dem Umfang wird durch seinen Winkel zur X-Achse uebergeben (alpha).
 * Gemalt werden:
 * - Kreis
 * - Eine Linie vom Punkt zum Mittelpunkt (Radius)
 * - Eine Linie vom Punkt zur X-Achse (Sinus)
 * - Eine Linie vom Kreismittelpunkt zum Schnittpunkt mit o.g. Sinus-Linie
 *
 * Die drei Linien ergeben ein rechtwinkliges Dreieck,
 * wobei die horizontale Linie Cosinus und die wertikale Sinus in einem Einheitskreis wiedergeben.
 *
 * Siehe draw_sin_cos_graph()
 *
 * @param pm Mittelpunkt
 * @param r Radius
 * @param alpha Winkel zwischen X-Achse und Punkt auf dem Umfang.
 * @param thickness Dicke der Linien
 */
void draw_sin_cos(const point_t& pm, const float r, const float alpha, const float thickness){
    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
    draw_circle(pm, r, thickness, circle_t::line);

    const float x1 = cos(alpha) * r; // Cosinus Laenge
    const float y1 = sin(alpha) * r; // Sinus Laenge
    const point_t p0(pm.x + x1, pm.y);
    point_t p1(pm.x + x1, pm.y + y1);
    {
        // Cosinus
        point_t center2 = pm;
        pixel::set_pixel_color(255 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
        // lineseg_t::draw(pm, p0); // cosinus
        rect_t(center2.add(0, thickness / 2), x1, thickness).draw(true);
    }
    {
        // Sinus
        pixel::set_pixel_color(0 /* r */, 0 /* g */, 255 /* b */, 255 /* a */);
        // lineseg_t::draw(p0, p1); // sinus
        rect_t(p1.add(-thickness / 2, 0), thickness, y1).draw(true);
    }
    {
        // Radius
        pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
        lineseg_t::draw(p1, pm); // radius
    }
}

double circumferenceInner(double r, double n){
    const double alpha = 2.0 * M_PI / n; // Den Mittelpunkt verbindet man mit 2 benachbarten Ecken,
    // so bildet sich ein Dreieck und der Winkel aus dem Mittelpunkt ist alpha
    double p0_x = r * cos(2.0 * M_PI - alpha);
    double p0_y = r * sin(2.0 * M_PI - alpha);
    double p1_x = r;
    double p1_y = 0.0;
    double dx = p0_x - p1_x;
    double dy = p0_y - p1_y;
    return n * sqrt( dx*dx + dy*dy );
    // point_t p0 = vec_t::from_length_angle(r, 2 * M_PI - alpha);
    // point_t p1 = vec_t::from_length_angle(r, 0);
    // return n * (p0 - p1).length();
}

/**
 * Zeichnen eines n Ecks das genau in dem Kreis passt wo der Radius r und der Mittelpunkt pm ist.
 * Damit kommt man immer näher an den Kreisumfang und an π.
 * @param pm ist der Mittelpunkt
 * @param r ist der Radius
 * @param n ist die Anzahl der Ecken
 * @return
 */
float draw_circumferenceInner(const point_t& pm, float r, int n){
    const float alpha = 2 * M_PI / n; // Den Mittelpunkt verbindet man mit 2 benachbarten Ecken,
    // so bildet sich ein Dreieck und der Winkel aus dem Mittelpunkt ist alpha
    float res;
    point_t p0 = pm + vec_t::from_length_angle(r, 2 * M_PI - alpha); // 1. Anfangspunkt
    {
        point_t p1 = pm + vec_t::from_length_angle(r, 0); // 2. Anfangsbuch
        res = n * (p0 - p1).length(); // res ist der Umfang von dem Multi-Eck
    }
    for(float i = 0.0f; i < 2 * M_PI; i += alpha){ // i = jetzige Winkel wo wir grade malen
        point_t p1 = pm + vec_t::from_length_angle(r, i);
        lineseg_t::draw(p0, p1); // Eine Linie vom Multi-Eck
        p0 = p1;
    }
    return res;
}

double circumferenceOutter(double r, double n){
    // const float alpha = 2 * M_PI / n;
    // return n * tan(alpha / 2) * 2 * r;
    return n * tan(M_PI / n) * 2.0 * r; // Das ist der Umfang von dem Multi-Eck
}

/**
 * Zeichenen eines n Ecks das genau um dem Kreis passt wo der Radius r und der Mittelpunkt pm ist.
 * Damit kommt man immer näher an den Kreisumfang und π.
 * @param pm ist der Mittelpunkt
 * @param r ist der Radius
 * @param n ist die Anzahl der Ecken
 * @return
 */
float draw_circumferenceOutter(const point_t& pm, float r, int n){
    const float alpha = 2 * M_PI / n; // Den Mittelpunkt verbindet man mit 2 benachbarten Ecken,
    // so bildet sich ein Dreieck und der Winkel aus dem Mittelpunkt ist alpha
    float res, l_2, l;
    {
        l = tan(alpha / 2) * 2 * r; // Seitenlaenge
        l_2 = l / 2; // Halbe Seitenlaenge
        res = n * l; // Umfang von dem Multi-Eck
    }
    point_t p1 = pm +
                 vec_t::from_length_angle(r, 0) +
                 vec_t::from_length_angle(l_2, -M_PI_2);
    for(float i = 0.0f; i < 2 * M_PI; i += alpha){ // i = jetzige Winkel wo wir grade malen
        point_t p2 = p1 + vec_t::from_length_angle(l, i + M_PI_2);
        lineseg_t::draw(p1, p2); // Eine Linie vom Multi-Eck
        p1 = p2;
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
    const double d = 2 * r; // Durchmesser
    long corners = 4; // Ecken
    int matchingDecimals = 0;
    while( true ) {
        const double ci = circumferenceInner(r, corners);
        const double co = circumferenceOutter(r, corners);
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

/**
 * Zusaetzlich fuer draw_sin_cos() wird hier der Sinus und Cosinus
 * Verlauf von 0 - alpha_max in einem Graph dargestellt.
 *
 * @param r Radius
 * @param alpha_max Endwinkel
 * @param angrad_inc Winkelschritt
 * @param plot_inc Schrittweite auf der X-Achse
 */
void draw_sin_cos_graph(const float r, const float alpha_max, const float angrad_inc, const float plot_inc) {
    point_t plotpos(pixel::cart_coord.min_x(), 0);
    point_t pcos, psin; // Sinus und Cosinus Punkte
    const float thickness = r/200; // Dicke von der Linien
    {
        point_t p0(pixel::cart_coord.min_x(), 0);
        point_t p1(pixel::cart_coord.max_x(), 0);
        pixel::set_pixel_color(0, 0, 0, 255);
        lineseg_t::draw(p0, p1); // Mittellinie
    }
    for(float alpha = 0; alpha < alpha_max; alpha += angrad_inc) {
        plotpos.add(plot_inc, 0);
        if( plotpos.x > pixel::cart_coord.width() ) {
            plotpos.set(0, plotpos.y);
        }
        if( get_fract(alpha / (2 * M_PI)) < angrad_inc/2.0f ) { // numeric_limits<float>::epsilon() ){
            pixel::set_pixel_color(255, 0, 255, 255);
            point_t p0(plotpos.x, pixel::cart_coord.max_y());
            point_t p1(plotpos.x, pixel::cart_coord.min_y());
            lineseg_t::draw(p0, p1); // Eine Linie von oben nach unten
        }
        {
            // Cosinus
            float cosval = cos(alpha) * r; // Cosinus Wert von alpha mal radius
            pcos.set(plotpos.x, plotpos.y + cosval);
            pixel::set_pixel_color(255 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
            // pcos.draw();
            disk_t(pcos, thickness).draw();
        }
        {
            // Sinus
            float sinval = sin(alpha) * r; // Sinus Wert von alpha mal radius
            psin.set(plotpos.x, plotpos.y + sinval);
            pixel::set_pixel_color(0 /* r */, 0 /* g */, 255 /* b */, 255 /* a */);
            // psin.draw();
            disk_t(psin, thickness).draw();
        }
    }
    pixel::set_pixel_color(255 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
    disk_t(pcos, thickness*2).draw();
    pixel::set_pixel_color(0 /* r */, 0 /* g */, 255 /* b */, 255 /* a */);
    disk_t(psin, thickness*2).draw();
}

static int forced_fps = 30;

void mainloop() {
    static vector<pixel::texture_ref> texts;
    static const point_t origin(0, 0);
    static const pixel::f4::vec_t text_color(0, 0, 0, 1);

    static const int circles_per_plot = 2;
    const float ticks_per_circle = 12.0f * 60.0f; // one circle in 12s @ 60Hz, [s] * [1/s] = [1]
    const float angrad_inc = ( 2.0f * M_PI ) / ticks_per_circle;
    static float ang_rad = 0.0f, ang_grad = 0.0f;
    static float an = -0.35f;
    static float off_pct = an;
    static int demo_index = 0;
    static const int DEMO_MAX_IDX = 3;
    static const float grid_gap = 50;
    //static float max_radius = pixel::cart_coord.max_y() * 0.9f;
    static int circum_corners = 3;
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

    static bool manual = false;
    static bool m1 = false;
    static bool m2 = false;

    static bool anim1 = true;
    static bool anim2 = true;
    static int a1 = 0;
    static int a2 = 0;
    static pixel::input_event_t event;

    pixel::handle_events(event);
    if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_CLOSE_REQ ) ) {
        printf("Exit Application\n");
        #if defined(__EMSCRIPTEN__)
            emscripten_cancel_main_loop();
        #else
            exit(0);
        #endif
    }
    const bool animating = !event.paused();

    const float plot_inc = pixel::cart_coord.width() / ( ticks_per_circle * circles_per_plot );
    manual = manual && animating;
    m1 = m1 && anim1;
    m2 = m2 && anim2;
    anim1 = animating && demo_index == 1;
    anim2 = animating && demo_index == 2;
    float fps = pixel::get_gpu_fps();
    texts.push_back( pixel::make_text(
            point_t(pixel::cart_coord.min_x(), pixel::cart_coord.max_y()), 0, text_color,
            "fps "+std::to_string(fps)) );

    const float max_radius = pixel::cart_coord.max_y() * 0.9f;

    // white background
    pixel::clear_pixel_fb(255, 255, 255, 255);
    pixel::draw_grid(grid_gap,
            225 /* r */, 225 /* g */, 225 /* b */, 255 /* a */,
            200 /* r */, 200 /* g */, 200 /* b */, 255 /* a */);

    if( event.has_any_p1() ) {
        if( event.pressed_and_clr(pixel::input_event_type_t::P1_UP) ) {
            manual = true;
            if( circum_corners < 128 && demo_index == 3 ) {
                circum_corners += 1;
            } else if( demo_index == 2 ){
                ang_rad += angrad_inc / 2;
                m2 = true;
            } else if(demo_index == 1){
                off_pct += 0.0025;
                m1 = true;
            }
        } else if( event.pressed_and_clr(pixel::input_event_type_t::P1_DOWN) ) {
            manual = true;
            if( circum_corners > 3 && demo_index == 3 ) {
                circum_corners -= 1;
            } else if( demo_index == 2 ){
                ang_rad -= angrad_inc / 2;
                m2 = true;
            } else if(demo_index == 1){
                off_pct -= 0.0025;
                m1 = true;
            }
        } else if( event.released_and_clr(pixel::input_event_type_t::P1_LEFT) ) {
            demo_index -= 1;
            if( demo_index < 0 ) {
                demo_index = DEMO_MAX_IDX;
            }
        } else if( event.released_and_clr(pixel::input_event_type_t::P1_RIGHT) ) {
            demo_index += 1;
            if( demo_index > DEMO_MAX_IDX ) {
                demo_index = 0;
            }
        } else if( !animating ) {
            if( demo_index == 1 ) {
                ++a1;
            } else if( demo_index == 2) {
                ++a2;
            }
        }
    }
    // rect_t(point_t(0, 0), max_radius, 100).draw(true);
    if( false ) {
        rect_t r1(point_t(0, 0), max_radius, max_radius);
        r1.rotate(M_PI/4.0f);
        // r1.rotate(M_PI);
        r1.draw(true);
    } else {
        float enter = pixel::cart_coord.height() / -35;
        point_t text_pos1(pixel::cart_coord.min_x()+pixel::cart_coord.width()*3.0f/4.0f,
                pixel::cart_coord.max_y());
        point_t text_pos2(pixel::cart_coord.min_x() + 50,
                pixel::cart_coord.min_y() + pixel::cart_coord.height() / 4);
        point_t text_pos3(pixel::cart_coord.min_x(), pixel::cart_coord.max_y() + enter * 1.5f);
        const float font_height = max<float>(24, pixel::cart_coord.height() / 35);
        point_t text_pos4(pixel::cart_coord.min_x() + pixel::cart_coord.width() * 3.0f / 4.0f -
                (font_height * 5),
                pixel::cart_coord.max_y());
        switch( demo_index ) {
        case 0: {
            point_t text_pos(-200, pixel::cart_coord.max_y());
            int a = 6;
            texts.push_back( pixel::make_text(text_pos, 0, text_color, "INNHALTSVERZEICHNIS"));
            text_pos.add(-230, enter * a);
            texts.push_back( pixel::make_text(text_pos, 0, text_color, "1. . . . . . . . . . . . . . . . . . . . ."
                    " . . . . . . . . . . . . . . . . . . . . 2*PI*r Ausgerollt"));
            text_pos.add(0, enter * a);
            texts.push_back( pixel::make_text(text_pos, 0, text_color, "2. . . . . . . . . . . . . . . . . . . . ."
                    "  . . . Einheitskreis 2*PI Sinus & Cosinus"));
            text_pos.add(0, enter * a);
            texts.push_back( pixel::make_text(text_pos, 0, text_color, "3. . . . . . . . . . . . . . . . . . . . ."
                    " . . . . . .PI Annaehrung nach Archimedes"));
        }
        break;
        case 1: {
            float radius = max_radius / 2.0f;
            float Umfang = 2 * M_PI * radius;
            float PI = Umfang / (2 * radius);
            draw_circle_unroll(point_t(pixel::cart_coord.min_x() + radius, 0), radius, 6, off_pct);
            point_t tp(pixel::cart_coord.min_x(), pixel::cart_coord.max_y()/2.0f);
            texts.push_back( pixel::make_text(text_pos4, 0, text_color, "2*PI*r Ausgerollt") );
            text_pos4.add(0, enter);
            texts.push_back( pixel::make_text(text_pos4, 0, text_color, "u = umfang, r = radius, d = durchmesser") );
            text_pos4.add(0, enter);
            texts.push_back( pixel::make_text(text_pos4, 0, text_color, "u = 2 * PI * Radius") );
            text_pos4.add(0, enter);
            texts.push_back( pixel::make_text(text_pos4, 0, text_color, "PI = u / 2 * r") );
            text_pos4.add(0, enter);
            texts.push_back( pixel::make_text(text_pos4, 0, text_color, "d = 2 * r") );
            texts.push_back( pixel::make_text(text_pos2, 0, text_color, "PI = "+std::to_string(PI)) );
            text_pos2.add(0, enter);
            texts.push_back( pixel::make_text(text_pos2, 0, text_color, "u = "+std::to_string(Umfang)) );
            text_pos2.add(0, enter);
            texts.push_back( pixel::make_text(text_pos2, 0, text_color, "r = "+std::to_string(radius)) );
            text_pos2.add(0, enter);
            texts.push_back( pixel::make_text(text_pos2, 0, text_color, "d = "+std::to_string(radius * 2)) );
            if( anim1 && !m1/*&& !manual*/ ) {
                if(off_pct < 1.3f){
                    off_pct += 0.005f;
                } else {
                    off_pct = an;
                }
            }
        }
        break;
        case 2: {
            draw_sin_cos_graph(max_radius, ang_rad, angrad_inc, plot_inc);
            draw_sin_cos(origin, max_radius, ang_rad, 5);
            point_t tp(pixel::cart_coord.min_x(), pixel::cart_coord.max_y()/2.0f);
            texts.push_back( pixel::make_text(text_pos4, 0, text_color, "Einheitskreis 2*PI Sinus & Cosinus") );
            float cosval = cos(ang_rad);
            float sinval = sin(ang_rad);
            text_pos4.add(0, enter);
            texts.push_back( pixel::make_text(text_pos4, 0, text_color, "Cosinus = "+std::to_string(cosval)) );
            text_pos4.add(0, enter);
            texts.push_back( pixel::make_text(text_pos4, 0, text_color, "Sinus = "+std::to_string(sinval)) );
            text_pos4.add(0, enter);
            texts.push_back( pixel::make_text(text_pos4, 0, text_color, "Winkel (Grad) = "+std::to_string(ang_grad)) );
            if( anim2 && !m2/*&& !manual*/ ){
                ang_rad += angrad_inc;
            }
        }
        break;
        case 3: {
            pixel::set_pixel_color(0 /* r */, 0 /* g */, 255 /* b */, 255 /* a */);
            draw_circle(origin, max_radius, 1, circle_t::line);
            pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
            draw_circumferenceInner(origin, max_radius, circum_corners);
            draw_circumferenceOutter(origin, max_radius, circum_corners);
            point_t tp(pixel::cart_coord.min_x(), pixel::cart_coord.max_y()/2.0f);
            texts.push_back( pixel::make_text(text_pos3, 0, text_color, "PI Annaehrung nach Archimedes") );
            const double d = 2.0f*max_radius;
            const double ci = circumferenceInner(max_radius, circum_corners);
            const double co = circumferenceOutter(max_radius, circum_corners);
            const double pi_i = ci/d;
            const double pi_o = co/d;
            text_pos3.add(0, enter);
            texts.push_back( pixel::make_text(text_pos3, 0, text_color, "Ecken "+std::to_string(circum_corners)+
                    ", d "+std::to_string(d)) );
            text_pos3.add(0, enter);
            texts.push_back( pixel::make_text(text_pos3, 0, text_color, "Innen : U "+std::to_string(ci)+
                    ", PI "+std::to_string(pi_i)) );
            text_pos3.add(0, enter);
            texts.push_back( pixel::make_text(text_pos3, 0, text_color, "Aussen : U "+std::to_string(co)+
                    ", PI "+std::to_string(pi_o)) );
        }
        [[fallthrough]];
        default:
            break;
        }
    }
    if(ang_rad > circles_per_plot * 2 * M_PI){
        ang_rad = 0.0f;
    }
    ang_grad = pixel::rad_to_adeg(ang_rad);
    if(ang_rad > 2 * M_PI) {
        const int n = (int)( ang_grad / 360.0f );
        ang_grad -= n * 360.0f;
    }

    pixel::swap_pixel_fb(false);
    for(pixel::texture_ref tex : texts) {
        tex->draw(0, 0);
    }
    texts.clear();
    pixel::swap_gpu_buffer( forced_fps );

}

int main(int argc, char *argv[])
{
    unsigned int win_width = 1920, win_height = 1000;
    bool use_subsys_primitives = true;
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
    if( use_subsys_primitives ) {
        forced_fps = -1;
    }
    {
        const float origin_norm[] = { 0.5f, 0.5f };
        pixel::init_gfx_subsystem("piviz", win_width, win_height, origin_norm, true /* enable_vsync */, use_subsys_primitives);
    }

    pixel::log_printf(0, "XX %s\n", pixel::cart_coord.toString().c_str());
    {
        float w = pixel::cart_coord.width();
        float h = pixel::cart_coord.height();
        float r01 = h/w;
        float a = w / h;
        printf("-w %f [x]\n-h %f [y]\n-r1 %f [y/x]\n-r2 %f [x/y]\n", w, h, r01, a);
    }
    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
}
