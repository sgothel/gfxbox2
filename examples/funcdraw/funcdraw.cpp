
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ostream>
#include <thread>

#include "infix_calc.hpp"

#include <pixel/pixel2i.hpp>
#include <pixel/pixel2f.hpp>
#include "pixel/pixel.hpp"

typedef std::vector<infix_calc::rpn_token> rpn_expr;
std::vector<rpn_expr> rpn_funcs;
infix_calc::variable_set variables;
std::atomic_bool rpn_funcs_dirty;

void add_func(std::vector<infix_calc::rpn_token>& expr) {
    variables["x"] = 0.0;

    rpn_expr expr_opt;
    printf("Adding RPN\n\tVanilla: %s\n", infix_calc::to_string(expr).c_str());
    if( !infix_calc::resolved(variables, expr) ) {
        printf("Skipping due to unresolved variables\n");
        return;
    }
    infix_calc::RPNStatus estatus = infix_calc::reduce(expr_opt, expr);
    if( infix_calc::RPNStatus::No_Error != estatus ) {
        printf("Error occurred @ reduce: %s\n", infix_calc::to_string(estatus).c_str());
        return;
    }
    printf("\tReduced: %s\n", infix_calc::to_string(expr_opt).c_str());
    rpn_funcs.push_back(expr_opt);
    rpn_funcs_dirty = true;
}

void draw_funcs() {
    for(rpn_expr e : rpn_funcs ) {
        pixel::f2::vec_t p0;
        bool has_p0 = false;
        for(float x=pixel::cart_min_x; x<=pixel::cart_max_x; x+=1.0f) {
            variables["x"] = x;
            double res = 0.0;
            infix_calc::RPNStatus estatus = infix_calc::eval(res, variables, e);
            if( infix_calc::RPNStatus::No_Error != estatus ) {
                // printf("Error occurred @ eval(%f): %s\n", x, infix_calc::to_string(estatus).c_str());
                continue;
            }
            pixel::f2::vec_t p((float)x, (float)res);
            if( !has_p0 ) {
                p.draw();
                p0 = p;
                has_p0 = true;
            } else {
                pixel::f2::lineseg_t::draw(p0, p);
                p0 = p;
            }
        }
    }
}

void clear_funcs() {
    rpn_funcs.clear();
    rpn_funcs_dirty = true;
}

std::atomic_bool exit_raised;

void exit_app() {
    exit_raised = true;
}

void commandline_proc() {
    infix_calc::compiler cc;
    std::string line;

    std::cout << "> ";
    while( !exit_raised && std::getline(std::cin, line) ) {
        if( line.length() > 0 ) {
            const bool pok = cc.parse (line.c_str(), line.length());
            if( !pok ) {
                std::cerr << "Error occurred @ parsing: " << cc.location() << std::endl;
            }
        }
        std::cout << "> ";
    }
}

void print_usage() {
    printf("Usage:\n");
    printf("\tdraw 200*sin(x/200)\n");
    printf("\t\tunary functions: abs, sin, cos, tan, asin, acos, atan, sqrt, ln, log, exp\n");
    printf("\t\tbinary operations: +, -, *, /, %%, ^\n");
    printf("\t\tbraces: (, )\n");
    printf("\tclear\n");
    printf("\thelp\n");
    printf("\texit\n");
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
        pixel::init_gfx_subsystem("funcdraw", win_width, win_height, origin_norm);
    }

    bool close = false;
    bool resized = false;
    bool set_dir = false;
    pixel::direction_t dir = pixel::direction_t::UP;

    uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]
    pixel::f2::lineseg_t l_x = { { (float)pixel::cart_min_x,  0.0f }, { (float)pixel::cart_max_x, 0.0f } };
    pixel::f2::lineseg_t l_y = { { 0.0f, (float)pixel::cart_min_y }, {  0.0f, (float)pixel::cart_max_y } };
    printf("x-axis: %s\n", l_x.toString().c_str());
    printf("y-axis: %s\n", l_y.toString().c_str());
    print_usage();
    pixel::clear_pixel_fb(255, 255, 255, 255);

    std::thread commandline_thread(&commandline_proc);
    commandline_thread.detach();

    while(!close && !exit_raised) {
        handle_events(close, resized, set_dir, dir);
        if( resized ) {
            l_x = { { (float)pixel::cart_min_x,  0.0f }, { (float)pixel::cart_max_x, 0.0f } };
            l_y = { { 0.0f, (float)pixel::cart_min_y }, {  0.0f, (float)pixel::cart_max_y } };
            printf("x-axis: %s\n", l_x.toString().c_str());
            printf("y-axis: %s\n", l_y.toString().c_str());
        }

        const uint64_t t = pixel::getElapsedMillisecond(); // [ms]
        const float dt = (float)( t - t_last ) / 1000.0f; // [s]
        const float dt_exp = 1.0f / (float)pixel::frames_per_sec; // [s]
        const float dt_diff = (float)( dt_exp - dt ) * 1000.0f; // [ms]
        t_last = t;

        pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
        l_x.draw();
        l_y.draw();

        {
            bool expected = true;
            if( rpn_funcs_dirty.compare_exchange_strong(expected, false) || resized ) {
                pixel::clear_pixel_fb(255, 255, 255, 255);
                draw_funcs();
            }
        }

        if( dt_diff > 1.0f ) {
            pixel::milli_sleep( (uint64_t)dt_diff );
        }
        pixel::swap_pixel_fb();
    }
    exit_raised = true;
    // commandline_thread.join();
    exit(0);
}
