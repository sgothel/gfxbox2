
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ostream>
#include <thread>

#include "rpn_calc.hpp"
#include "infix_calc.hpp"

#include <pixel/pixel2i.hpp>
#include <pixel/pixel2f.hpp>
#include "pixel/pixel.hpp"

std::vector<rpn_calc::rpn_expression_t> rpn_funcs;
rpn_calc::variable_set variables;
std::atomic_bool rpn_funcs_dirty;
std::atomic_bool resized_ext;

void add_func(rpn_calc::rpn_expression_t& expr) {
    variables["x"] = 0.0;

    printf("Adding RPN\n\tVanilla: %s\n", expr.toString().c_str());
    if( !expr.resolved(variables) ) {
        printf("Skipping due to unresolved variables\n");
        return;
    }
    rpn_calc::RPNStatus estatus = expr.reduce();
    if( rpn_calc::RPNStatus::No_Error != estatus ) {
        printf("Error occurred @ reduce: %s\n", rpn_calc::to_string(estatus).c_str());
        return;
    }
    printf("\tReduced: %s\n", expr.toString().c_str());
    rpn_funcs.push_back(expr);
    rpn_funcs_dirty = true;
}

void draw_funcs() {
    const float x_ival = pixel::cart_coord.width() / pixel::fb_width;
    for(rpn_calc::rpn_expression_t& e : rpn_funcs ) {
        pixel::f2::vec_t p0;
        bool has_p0 = false;
        for(float x=pixel::cart_coord.min_x(); x<=pixel::cart_coord.max_x(); x+=x_ival) {
            variables["x"] = x;
            double res = 0.0;
            rpn_calc::RPNStatus estatus = e.eval(res, variables);
            if( rpn_calc::RPNStatus::No_Error != estatus ) {
                // printf("Error occurred @ eval(%f): %s\n", x, rpn_calc::to_string(estatus).c_str());
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

std::function<void()> cart_coord_setup = []() { pixel::cart_coord.set_width(-10.0f, 10.0f); };

void set_width(float x1, float x2) {
    cart_coord_setup = [x1, x2]() { pixel::cart_coord.set_width(x1, x2); };
    resized_ext = true;
    rpn_funcs_dirty = true;
}
void set_height(float y1, float y2) {
    cart_coord_setup = [y1, y2]() { pixel::cart_coord.set_height(y1, y2); };
    resized_ext = true;
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
    printf("\tdraw sin(x);\n");
    printf("\t\tunary functions: abs, sin, cos, tan, asin, acos, atan, sqrt, ln, log, exp\n");
    printf("\t\tbinary operations: +, -, *, /, %%, ^\n");
    printf("\t\tbraces: (, )\n");
    printf("\tclear;\n");
    printf("\tset_width x1, x2;\n");
    printf("\tset_height y1, y2;\n");
    printf("\thelp;\n");
    printf("\texit;\n");
}

int main(int argc, char *argv[])
{
    int win_width = 1920, win_height = 1080;
    std::string commandfile;
    {
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-width", argv[i]) && i+1<argc) {
                win_width = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-height", argv[i]) && i+1<argc) {
                win_height = atoi(argv[i+1]);
                ++i;
            } else {
                commandfile = argv[i];
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
    pixel::mouse_motion_t mouse_motion;
    pixel::texture_ref hud_text;

    cart_coord_setup();

    uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]
    pixel::f2::lineseg_t l_x = { { pixel::cart_coord.min_x(),  0.0f }, { pixel::cart_coord.max_x(), 0.0f } };
    pixel::f2::lineseg_t l_y = { { 0.0f, pixel::cart_coord.min_y() }, {  0.0f, pixel::cart_coord.max_y() } };
    printf("x-axis: %s\n", l_x.toString().c_str());
    printf("y-axis: %s\n", l_y.toString().c_str());
    print_usage();
    pixel::clear_pixel_fb(255, 255, 255, 255);

    if( !commandfile.empty() ) {
        infix_calc::compiler cc;
        std::cout << "Processing command input file: " << commandfile << std::endl;
        const bool pok = cc.parse (commandfile);
        if( !pok ) {
            std::cerr << "Error occurred @ parsing: " << cc.location() << std::endl;
        }
    }

    std::thread commandline_thread(&commandline_proc);
    commandline_thread.detach();

    while(!close && !exit_raised) {
        handle_events(close, resized, set_dir, dir, mouse_motion);
        if( resized || resized_ext ) {
            resized_ext = false;
            cart_coord_setup();
            l_x = { { pixel::cart_coord.min_x(),  0.0f }, { pixel::cart_coord.max_x(), 0.0f } };
            l_y = { { 0.0f, pixel::cart_coord.min_y() }, {  0.0f, pixel::cart_coord.max_y() } };
            printf("x-axis: %s\n", l_x.toString().c_str());
            printf("y-axis: %s\n", l_y.toString().c_str());
        }
        if( 0 <= mouse_motion.id ) {
            const pixel::f2::vec_t p = pixel::f2::fb_to_cart(mouse_motion.x, mouse_motion.y);
            hud_text = pixel::make_text_texture("fps "+std::to_string(pixel::get_gpu_fps())+", "+p.toString());
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
        pixel::swap_pixel_fb(false);
        if( nullptr != hud_text ) {
            hud_text->draw(0, 0);
        }
        pixel::swap_gpu_buffer();
    }
    exit_raised = true;
    // commandline_thread.join();
    exit(0);
}
