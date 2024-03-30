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
#include "pixel/pixel.hpp"

#include <thread>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#if !defined(__EMSCRIPTEN__)
    #include <SDL2/SDL_image.h>
#endif

using namespace pixel;

static SDL_Window* sdl_win = nullptr;
static SDL_Renderer* sdl_rend = nullptr;
static float fb_origin_norm[2] = { 0.0f, 0.0f };
static size_t fb_pixels_dim_size = 0;
static size_t fb_pixels_byte_size = 0;
static size_t fb_pixels_byte_width = 0;
static SDL_Texture * fb_texture = nullptr;
static TTF_Font* sdl_font = nullptr;
static float gpu_fps = 0.0f;
static int gpu_frame_count = 0;
static uint64_t gpu_fps_t0 = 0;
static uint64_t gpu_swap_t0 = 0;
static uint64_t gpu_swap_t1 = 0;

uint32_t pixel::rgba_to_uint32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept {
    // SDL_PIXELFORMAT_ARGB8888
    return ( ( (uint32_t)a << 24 ) & 0xff000000U ) |
           ( ( (uint32_t)r << 16 ) & 0x00ff0000U ) |
           ( ( (uint32_t)g <<  8 ) & 0x0000ff00U ) |
           ( ( (uint32_t)b       ) & 0x000000ffU );
}

void pixel::uint32_to_rgba(const uint32_t ui32, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) noexcept {
    // SDL_PIXELFORMAT_ARGB8888
    a = ( ui32 & 0xff000000U ) >> 24;
    r = ( ui32 & 0x00ff0000U ) >> 16;
    g = ( ui32 & 0x0000ff00U ) >>  8;
    b = ( ui32 & 0x000000ffU );
}

static void on_window_resized(const int win_width, const int win_height) noexcept {
    SDL_GetRendererOutputSize(sdl_rend, &fb_width, &fb_height);

    SDL_RendererInfo sdi;
    SDL_GetRendererInfo(sdl_rend, &sdi);
    fb_max_x = fb_width - 1;
    fb_max_y = fb_height - 1;

    cart_coord.set_origin(fb_origin_norm[0], fb_origin_norm[1]);

    {
        SDL_DisplayMode mode;
        const int win_display_idx = SDL_GetWindowDisplayIndex(sdl_win);
        bzero(&mode, sizeof(mode));
        SDL_GetCurrentDisplayMode(win_display_idx, &mode); // SDL_GetWindowDisplayMode(..) fails on some systems (wrong refresh_rate and logical size
        printf("WindowDisplayMode: %d x %d @ %d Hz @ display %d\n", mode.w, mode.h, mode.refresh_rate, win_display_idx);
        frames_per_sec = mode.refresh_rate;
    }
    printf("Renderer %s\n", sdi.name);
    printf("FB Size %d x %d, min 0 / 0, max %d / %d \n",
            fb_width, fb_height, fb_max_x, fb_max_y);
    printf("Win Size %d x %d, FB/Win %f x %f \n",
            win_width, win_height, (float)fb_width/(float)win_width, (float)fb_height/(float)win_height);
    printf("%s\n", cart_coord.toString().c_str());

    fb_pixels_dim_size = (size_t)(fb_width * fb_height);
    fb_pixels_byte_size = fb_pixels_dim_size * 4;
    fb_pixels_byte_width = (size_t)(fb_width * 4);
    if( nullptr != fb_texture ) {
        SDL_DestroyTexture( fb_texture );
        fb_texture = nullptr;
    }
    if( use_subsys_primitives_val ) {
        printf("SDL-Primitives\n");
    } else {
        printf("Soft-Primitives: Tex Size %d x %d x 4 = %zu bytes, width %zu bytes\n", fb_width, fb_height, fb_pixels_byte_size, fb_pixels_byte_width);
        fb_texture = SDL_CreateTexture(sdl_rend, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, fb_width, fb_height);
        fb_pixels.reserve(fb_pixels_dim_size);
        fb_pixels.resize(fb_pixels_dim_size);
    }

    {
        if( nullptr != sdl_font ) {
            TTF_CloseFont(sdl_font);
            sdl_font = nullptr;
        }
        const std::string fontfilename = "fonts/freefont/FreeSansBold.ttf";
        font_height = std::max(24, fb_height / 35);
        sdl_font = TTF_OpenFont(fontfilename.c_str(), font_height);
        if( nullptr == sdl_font ) {
            fprintf(stderr, "font: Null font for '%s': %s\n", fontfilename.c_str(), SDL_GetError());
        } else {
            printf("Using font %s, size %d\n", fontfilename.c_str(), font_height);
        }
    }
}

void pixel::init_gfx_subsystem(const char* title, unsigned int win_width, unsigned int win_height, const float origin_norm[2],
                               bool enable_vsync, bool use_subsys_primitives) {
    pixel::use_subsys_primitives_val = use_subsys_primitives;

    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) { // SDL_INIT_EVERYTHING
        printf("SDL: Error initializing: %s\n", SDL_GetError());
        exit(1);
    }
    #if !defined(__EMSCRIPTEN__)
        if ( ( IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG ) != IMG_INIT_PNG ) {
            printf("SDL_image: Error initializing: %s\n", SDL_GetError());
            exit(1);
        }
    #endif
    if( 0 != TTF_Init() ) {
        printf("SDL_TTF: Error initializing: %s\n", SDL_GetError());
        exit(1);
    }

    if( enable_vsync ) {
        SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    }
    // SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, std::to_string(pixel_filter_quality).c_str());

    fb_origin_norm[0] = origin_norm[0];
    fb_origin_norm[1] = origin_norm[1];

    const Uint32 win_flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
    const Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;

    sdl_win = SDL_CreateWindow(title,
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            (int)win_width,
            (int)win_height,
            win_flags);

    sdl_rend = SDL_CreateRenderer(sdl_win, -1, render_flags);

    gpu_fps = 0.0f;
    gpu_fps_t0 = getCurrentMilliseconds();
    gpu_swap_t0 = gpu_fps_t0;
    gpu_swap_t1 = gpu_fps_t0;
    gpu_frame_count = 0;

    on_window_resized(win_width, win_height);
}

void pixel::clear_pixel_fb(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept {
    SDL_SetRenderDrawColor(sdl_rend, r, g, b, a);
    SDL_RenderClear(sdl_rend);

    if( !use_subsys_primitives_val ) {
        const uint32_t c = rgba_to_uint32(r, g, b, a);
        size_t count = fb_pixels_dim_size;
        uint32_t* p = fb_pixels.data();
        while(count--) { *p++ = c; }
        // std::fill(p, p+count, c);
        // ::memset(fb_pixels.data(), 0, fb_pixels_byte_size);
    }
}

void pixel::swap_pixel_fb(const bool swap_buffer) noexcept {
    if( !use_subsys_primitives_val ) {
        SDL_UpdateTexture(fb_texture, nullptr, fb_pixels.data(), (int)fb_pixels_byte_width);
        SDL_RenderCopy(sdl_rend, fb_texture, nullptr, nullptr);
    }
    if( swap_buffer ) {
        pixel::swap_gpu_buffer();
    }
}
void pixel::swap_gpu_buffer(int fps) noexcept {
    SDL_RenderPresent(sdl_rend);
    gpu_swap_t0 = getCurrentMilliseconds();
    ++gpu_frame_count;
    const uint64_t td = gpu_swap_t0 - gpu_fps_t0;
    if( td >= 5000 ) {
        gpu_fps = (float)gpu_frame_count / ( (float)td / 1000.0f );
        gpu_fps_t0 = gpu_swap_t0;
        gpu_frame_count = 0;
    }
    if( 0 < fps ) {
        const int64_t fudge_ns = pixel::NanoPerMilli / 4;
        const uint64_t ms_per_frame = (uint64_t)std::round(1000.0 / fps);
        const uint64_t ms_this_frame =  gpu_swap_t0 - gpu_swap_t1;
        int64_t td_ns = int64_t( ms_per_frame - ms_this_frame ) * pixel::NanoPerMilli;
        if( td_ns > fudge_ns ) {
            const int64_t td_ns_0 = td_ns%pixel::NanoPerOne;
            struct timespec ts;
            ts.tv_sec = static_cast<decltype(ts.tv_sec)>(td_ns/pixel::NanoPerOne); // signed 32- or 64-bit integer
            ts.tv_nsec = td_ns_0 - fudge_ns;
            nanosleep( &ts, NULL );
            // pixel::log_printf("soft-sync [exp %zd > has %zd]ms, delay %" PRIi64 "ms (%lds, %ldns)\n",
            //         ms_per_frame, ms_this_frame, td_ns/pixel::NanoPerMilli, ts.tv_sec, ts.tv_nsec);
        }
        gpu_swap_t1 = pixel::getCurrentMilliseconds();
    } else {
        gpu_swap_t1 = gpu_swap_t0;
    }
}

float pixel::get_gpu_fps() noexcept {
    return gpu_fps;
}

void pixel::texture_t::destroy() noexcept {
    SDL_Texture* tex = reinterpret_cast<SDL_Texture*>(data);
    if( nullptr != tex ) {
        SDL_DestroyTexture(tex);
    }
    data = nullptr;
}

void pixel::texture_t::draw(const int x_pos, const int y_pos, const float scale_x, const float scale_y) noexcept {
    SDL_Texture* tex = reinterpret_cast<SDL_Texture*>(data);
    if( nullptr != tex ) {
        SDL_Rect src = { .x=x, .y=y, .w=width, .h=height};
        SDL_Rect dest = { .x=x_pos + dest_x,
                          .y=y_pos + dest_y,
                          .w=round_to_int(width*dest_sx*scale_x), .h=round_to_int(height*dest_sy*scale_y) };
        SDL_RenderCopy(sdl_rend, tex, &src, &dest);
    }
}

pixel::texture_ref pixel::make_text_texture(const std::string& text) noexcept
{
    if( nullptr == sdl_rend || nullptr == sdl_font ) {
        return nullptr;
    }
    uint8_t r, g, b, a;
    pixel::uint32_to_rgba(draw_color, r, g, b, a);
    SDL_Color foregroundColor = { r, g, b, a };

    SDL_Surface* textSurface = TTF_RenderText_Solid(sdl_font, text.c_str(), foregroundColor);
    if( nullptr == textSurface ) {
        fprintf(stderr, "make_text_texture: Null texture for '%s': %s\n", text.c_str(), SDL_GetError());
        return nullptr;
    }
    SDL_Texture* sdl_tex = SDL_CreateTextureFromSurface(sdl_rend, textSurface);
    texture_ref tex = std::make_shared<texture_t>(reinterpret_cast<void*>(sdl_tex), 0, 0, 0, 0);
    SDL_QueryTexture(sdl_tex, NULL, NULL, &tex->width, &tex->height);
    SDL_FreeSurface(textSurface);
    return tex;
}

void pixel::subsys_set_pixel_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept {
    SDL_SetRenderDrawColor(sdl_rend, r, g, b, a);
}

void pixel::subsys_draw_pixel(int x, int y) noexcept {
    SDL_RenderDrawPoint(sdl_rend, x, y);
}

void pixel::subsys_draw_line(int x1, int y1, int x2, int y2) noexcept {
    SDL_RenderDrawLine(sdl_rend, x1, y1, x2, y2);
}

static input_event_type_t to_event_type(SDL_Scancode scancode) {
    switch ( scancode ) {
        case SDL_SCANCODE_ESCAPE:
            return input_event_type_t::WINDOW_CLOSE_REQ;
        case SDL_SCANCODE_P:
            return input_event_type_t::PAUSE;
        case SDL_SCANCODE_UP:
            return input_event_type_t::P1_UP;
        case SDL_SCANCODE_LEFT:
            return input_event_type_t::P1_LEFT;
        case SDL_SCANCODE_DOWN:
            return input_event_type_t::P1_DOWN;
        case SDL_SCANCODE_RIGHT:
            return input_event_type_t::P1_RIGHT;
        case SDL_SCANCODE_RSHIFT:
            return input_event_type_t::P1_ACTION1;
        case SDL_SCANCODE_RETURN:
            return input_event_type_t::P1_ACTION2;
        case SDL_SCANCODE_RALT:
            return input_event_type_t::P1_ACTION3;
            /**
        case SDL_SCANCODE_RGUI:
            return input_event_type_t::P1_ACTION4; */
        case SDL_SCANCODE_W:
            return input_event_type_t::P2_UP;
        case SDL_SCANCODE_A:
            return input_event_type_t::P2_LEFT;
        case SDL_SCANCODE_S:
            return input_event_type_t::P2_DOWN;
        case SDL_SCANCODE_D:
            return input_event_type_t::P2_RIGHT;
        case SDL_SCANCODE_LSHIFT:
            return input_event_type_t::P2_ACTION1;
        case SDL_SCANCODE_LCTRL:
            return input_event_type_t::P2_ACTION2;
        case SDL_SCANCODE_LALT:
            return input_event_type_t::P2_ACTION3;
            /**
        case SDL_SCANCODE_LGUI:
            return input_event_type_t::P2_ACTION4; */
        case SDL_SCANCODE_R:
            return input_event_type_t::RESET;
        default:
            return input_event_type_t::NONE;
    }
}
static uint16_t to_ascii(SDL_Scancode scancode) {
    if(SDL_SCANCODE_A <= scancode && scancode <= SDL_SCANCODE_Z ) {
        return 'A' + ( scancode - SDL_SCANCODE_A );
    }
    if(SDL_SCANCODE_1 <= scancode && scancode <= SDL_SCANCODE_9 ) {
        return '1' + ( scancode - SDL_SCANCODE_1 );
    }
    if(SDL_SCANCODE_0 == scancode ) {
        return '0' + ( scancode - SDL_SCANCODE_0 );
    }
    return 0;
}

bool pixel::handle_one_event(input_event_t& event) noexcept {
    SDL_Event sdl_event;
    if( SDL_PollEvent(&sdl_event) ) {
        switch (sdl_event.type) {
            case SDL_QUIT:
                event.set( input_event_type_t::WINDOW_CLOSE_REQ );
                printf("Window Close Requested\n");
                break;

            case SDL_WINDOWEVENT:
                switch (sdl_event.window.event) {
                    case SDL_WINDOWEVENT_SHOWN:
                        // log_printf("Window Shown\n");
                        break;
                    case SDL_WINDOWEVENT_HIDDEN:
                        // log_printf("Window Hidden\n");
                        break;
                    case SDL_WINDOWEVENT_RESIZED:
                        printf("Window Resized: %d x %d\n", sdl_event.window.data1, sdl_event.window.data2);
                        event.set(input_event_type_t::WINDOW_RESIZED);
                        on_window_resized(sdl_event.window.data1, sdl_event.window.data2);
                        break;
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        // printf("Window SizeChanged: %d x %d\n", event.window.data1, event.window.data2);
                        break;
                }
                break;

                case SDL_MOUSEMOTION:
                    event.pointer_motion((int)sdl_event.motion.which,
                                         (int)sdl_event.motion.x, (int)sdl_event.motion.y);
                    break;
                case SDL_KEYUP: {
                    const SDL_Scancode scancode = sdl_event.key.keysym.scancode;
                    event.clear(to_event_type(scancode), to_ascii(scancode));
                  }
                  break;

                case SDL_KEYDOWN: {
                    const SDL_Scancode scancode = sdl_event.key.keysym.scancode;
                    event.set(to_event_type(scancode), to_ascii(scancode));
             //       printf("%d", scancode);
                  }
                  break;
        }
        return true;
    } else {
        return false;
    }
}

#if defined(__EMSCRIPTEN__)
    void pixel::save_snapshot(const std::string&) noexcept {
    }
#else
    static std::atomic<int> active_threads = 0;

    static void store_surface(SDL_Surface *sshot, char* fname) noexcept {
        active_threads++;
        if( false ) {
            fprintf(stderr, "XXX: %d: %s\n", active_threads.load(), fname);
        }
        SDL_SaveBMP(sshot, fname);
        free(fname);
        SDL_UnlockSurface(sshot);
        SDL_FreeSurface(sshot);
        active_threads--;
    }

    void pixel::save_snapshot(const std::string& fname) noexcept {
        SDL_Surface *sshot = SDL_CreateRGBSurface(0, fb_width, fb_height, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
        SDL_LockSurface(sshot);
        SDL_RenderReadPixels(sdl_rend, NULL, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);
        char * fname2 = strdup(fname.c_str());
        std::thread t(&store_surface, sshot, fname2);
        t.detach();
    }
#endif

