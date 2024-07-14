
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <thread>


#include "rpn_calc.hpp"
#include "infix_calc.hpp"

#include <pixel/pixel4f.hpp>
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
    #if 0
        infix_calc::compiler cc;
        std::string line;

        FILE* c_in = fopen("/dev/stdin", "rb");
        if( nullptr == c_in ) {
            printf("Error opening stdin\n");
            exit_raised = true;
        }
        char* c_line = NULL;
        size_t c_len = 0;
        ssize_t c_nread;

        while( !exit_raised && (c_nread = ::getline(&c_line, &c_len, c_in)) != -1 ) {
            if( c_nread > 0 ) {
                printf("%s", c_line);
                const bool pok = cc.parse (c_line, c_nread);
                if( !pok ) {
                    std::ostringstream ss;
                    ss << cc.location();
                    printf("Error occurred @ parsing: %s\n", ss.str().c_str());
                }
            }
            printf("> ");
        }
    #else
        infix_calc::compiler cc;
        std::string line;

        while( !exit_raised && std::getline(std::cin, line) ) {
            if( line.length() > 0 ) {
                const bool pok = cc.parse (line.c_str(), (int)line.length());
                if( !pok ) {
                    std::cerr << "Error occurred @ parsing: " << cc.location() << std::endl;
                }
            }
            std::cout << "> ";
        }
    #endif
}

void print_usage() {
    printf("Usage:\n");
    printf("  - Semicolon at end of statement is optional.\n");
    printf("  - Expression being any operation of:\n");
    printf("    - binary operations: +, -, *, /, modulo: '%%' or 'mod', pow: '^' or '**'\n");
    printf("    - unary functions: abs, sin, cos, tan, asin, acos, atan, sqrt, ln, log, exp, ceil, floor\n");
    printf("    - misc functions: step(edge, x), mix(x, y, a)\n");
    printf("    - braces: (, )\n");
    printf("  > draw <expression>\n");
    printf("  > draw sin(x)\n");
    printf("  > clear\n");
    printf("  > set_width x1, x2\n");
    printf("  > set_height y1, y2\n");
    printf("  > help\n");
    printf("  > exit\n");
}

void mainloop() {
    static infix_calc::compiler cc;
    static const pixel::f4::vec_t text_color(0.4f, 0.4f, 0.4f, 1.0f);
    static const int text_height = 28;

    static uint64_t t_last = pixel::getElapsedMillisecond(); // [ms]
    static pixel::f2::lineseg_t l_x = { { pixel::cart_coord.min_x(),  0.0f }, { pixel::cart_coord.max_x(), 0.0f } };
    static pixel::f2::lineseg_t l_y = { { 0.0f, pixel::cart_coord.min_y() }, {  0.0f, pixel::cart_coord.max_y() } };
    static float grid_gap = std::max(1.0f, std::floor( std::min(pixel::cart_coord.width(), pixel::cart_coord.height()) / 10.0f ));
    static pixel::input_event_t event;
    static std::string input_text;
    {
        while( pixel::handle_one_event(event) ) {
            if( 0 != event.last_key_code && pixel::is_ascii_code(event.last_key_code) ) {
                const int key_code = event.last_key_code;
                event.last_key_code = 0;
                fprintf(stdout, "%c", (char)key_code);
                fflush(stdout);
                input_text = event.text;
                if( 0x08 == key_code ) {
                    fprintf(stdout, "%c%c", ' ', (char)0x08);
                    fflush(stdout);
                } else if( '\n' == key_code ) {
                    input_text.pop_back();
                    const bool pok = cc.parse (event.text.c_str(), (int)event.text.length());
                    if( !pok ) {
                        std::ostringstream ss;
                        ss << cc.location();
                        printf("Error occurred @ parsing: %s: %s\n", ss.str().c_str(), event.text.c_str());
                    }
                    fprintf(stdout, "> ");
                    fflush(stdout);
                }
            }
        }
    }
    if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_CLOSE_REQ ) || exit_raised ) {
        exit_raised = true;
        printf("Exit Application\n");
        #if defined(__EMSCRIPTEN__)
            emscripten_cancel_main_loop();
        #else
            exit(0);
        #endif
    } else if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_RESIZED ) || resized_ext ) {
        resized_ext = false;
        cart_coord_setup();
        l_x = { { pixel::cart_coord.min_x(),  0.0f }, { pixel::cart_coord.max_x(), 0.0f } };
        l_y = { { 0.0f, pixel::cart_coord.min_y() }, {  0.0f, pixel::cart_coord.max_y() } };
        grid_gap = std::max(1.0f, std::floor( std::min(pixel::cart_coord.width(), pixel::cart_coord.height()) / 10.0f ));
        printf("x-axis: %s\n", l_x.toString().c_str());
        printf("y-axis: %s\n", l_y.toString().c_str());
        fprintf(stdout, "> ");
        fflush(stdout);
    }
    // const bool animating = !event.paused();

    const pixel::f2::point_t tl_text(pixel::cart_coord.min_x(), pixel::cart_coord.max_y());
    pixel::texture_ref hud_text = pixel::make_text(tl_text, 0, text_color, text_height,
            "fps %5.2f, %.2f / %.2f: grid %.0f, type > %s", pixel::get_gpu_fps(),
            pixel::cart_coord.from_win_x(event.pointer_x),
            pixel::cart_coord.from_win_y(event.pointer_y),
            grid_gap, input_text.c_str());

    pixel::clear_pixel_fb(255, 255, 255, 255);
    pixel::draw_grid(grid_gap,
            225 /* r */, 225 /* g */, 225 /* b */, 255 /* a */,
            200 /* r */, 200 /* g */, 200 /* b */, 255 /* a */);

    const uint64_t t = pixel::getElapsedMillisecond(); // [ms]
    const float dt = (float)( t - t_last ) / 1000.0f; // [s]
    const float dt_exp = 1.0f / (float)pixel::display_frames_per_sec; // [s]
    const float dt_diff = (float)( dt_exp - dt ) * 1000.0f; // [ms]
    t_last = t;

    pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
    l_x.draw();
    l_y.draw();

    if( false ) {
        bool expected = true;
        if( rpn_funcs_dirty.compare_exchange_strong(expected, false) ||
            event.pressed_and_clr( pixel::input_event_type_t::WINDOW_RESIZED ) ) {
            pixel::clear_pixel_fb(255, 255, 255, 255);
            draw_funcs();
        }
    }
    draw_funcs();

    if( dt_diff > 1.0f ) {
        pixel::milli_sleep( (uint64_t)dt_diff );
    }
    pixel::swap_pixel_fb(false);
    if( nullptr != hud_text ) {
        hud_text->draw(0, 0);
    }
    pixel::swap_gpu_buffer();
}

int main(int argc, char *argv[])
{
    int window_width = 1920, window_height = 1080;
    bool enable_vsync = true;
    std::string commandfile;
    #if defined(__EMSCRIPTEN__)
        window_width = 1024, window_height = 576; // 16:9
    #endif
    {
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-width", argv[i]) && i+1<argc) {
                window_width = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-height", argv[i]) && i+1<argc) {
                window_height = atoi(argv[i+1]);
                ++i;
            } else {
                commandfile = argv[i];
            }
        }
    }

    {
        const float origin_norm[] = { 0.5f, 0.5f };
        if( !pixel::init_gfx_subsystem("funcdraw", window_width, window_height, origin_norm, enable_vsync, true /* subsys primitives */) ) {
            return 1;
        }
    }

    cart_coord_setup();

    print_usage();

    if( !commandfile.empty() ) {
        infix_calc::compiler cc;
        std::cout << "Processing command input file: " << commandfile << std::endl;
        
        size_t lineno = 0;
        std::ifstream fin(commandfile);        
        std::string line;
        while( !exit_raised && std::getline(fin, line) ) {
            ++lineno;
            if( line.length() > 0 ) {
                const bool pok = cc.parse (line.c_str(), (int)line.length());
                if( !pok ) {
                    std::cerr << "Error occurred @ parsing: " << cc.location() << std::endl;
                } else {
                    std::cout << "#" << lineno << ": " << line << std::endl;
                }
            }
            std::cout << std::endl;
        }
        if( false ) {                
        const bool pok = cc.parse (commandfile);
        if( !pok ) {
            std::cerr << "Error occurred @ parsing: " << cc.location() << std::endl;
        }
        }
    }

    printf("> ");

    #if !defined(__EMSCRIPTEN__)
        std::thread commandline_thread(&commandline_proc);
        commandline_thread.detach();
    #endif

    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif

    exit_raised = true;
    // commandline_thread.join();
}
