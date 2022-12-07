#include "pixel/pixel.hpp"

#include <SDL2/SDL.h>

using namespace pixel;

static SDL_Window* sdl_win = nullptr;
static SDL_Renderer* sdl_rend = nullptr;
static float fb_origin_norm[2] = { 0.0f, 0.0f };
static size_t fb_pixels_dim_size = 0;
static size_t fb_pixels_byte_size = 0;
static size_t fb_pixels_byte_width = 0;
static SDL_Texture * fb_texture = nullptr;

uint32_t pixel::rgba_to_uint32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept {
    // SDL_PIXELFORMAT_ARGB8888
    return ( ( (uint32_t)a << 24 ) & 0xff000000U ) |
           ( ( (uint32_t)r << 16 ) & 0x00ff0000U ) |
           ( ( (uint32_t)g <<  8 ) & 0x0000ff00U ) |
           ( ( (uint32_t)b       ) & 0x000000ffU );
}

static void on_window_resized() noexcept {
    SDL_GetRendererOutputSize(sdl_rend, &fb_width, &fb_height);

    SDL_RendererInfo sdi;
    SDL_GetRendererInfo(sdl_rend, &sdi);
    fb_max_x = fb_width - 1;
    fb_max_y = fb_height - 1;

    cart_origin[0] = round_to_int((float)fb_width * fb_origin_norm[0]);
    cart_origin[1] = round_to_int((float)fb_height * fb_origin_norm[1]);

    cart_min_x = -cart_origin[0];
    cart_min_y = -cart_origin[1];
    cart_max_x =  cart_origin[0]-1;
    cart_max_y =  cart_origin[1]-1;

    printf("Renderer %s\n", sdi.name);
    printf("Screen size %d x %d, min 0 / 0, max %d / %d \n",
            fb_width, fb_height, fb_max_x, fb_max_y);

    printf("Math origin %d / %d, min %d / %d, max %d / %d \n",
            cart_origin[0], cart_origin[1], cart_min_x, cart_min_y, cart_max_x, cart_max_y);

    {
        SDL_DisplayMode mode;
        const int win_display_idx = SDL_GetWindowDisplayIndex(sdl_win);
        bzero(&mode, sizeof(mode));
        SDL_GetCurrentDisplayMode(win_display_idx, &mode); // SDL_GetWindowDisplayMode(..) fails on some systems (wrong refresh_rate and logical size
        printf("WindowDisplayMode: %d x %d @ %d Hz @ display %d\n", mode.w, mode.h, mode.refresh_rate, win_display_idx);
        frames_per_sec = mode.refresh_rate;
    }
    fb_pixels_dim_size = (size_t)(fb_width * fb_height);
    fb_pixels_byte_size = fb_pixels_dim_size * 4;
    fb_pixels_byte_width = (size_t)(fb_width * 4);
    if( nullptr != fb_texture ) {
        SDL_DestroyTexture( fb_texture );
        fb_texture = nullptr;
    }
    fb_texture = SDL_CreateTexture(sdl_rend, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, fb_width, fb_height);
    fb_pixels.reserve(fb_pixels_dim_size);
    fb_pixels.resize(fb_pixels_dim_size);
}

void pixel::init_gfx_subsystem(const char* title, unsigned int win_width, unsigned int win_height, const float origin_norm[2]) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL: Error initializing: %s\n", SDL_GetError());
        exit(1);
    }
    fb_origin_norm[0] = origin_norm[0];
    fb_origin_norm[1] = origin_norm[1];

    sdl_win = SDL_CreateWindow(title,
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            (int)win_width,
            (int)win_height,
            SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

    const Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    sdl_rend = SDL_CreateRenderer(sdl_win, -1, render_flags);

    on_window_resized();
}

void pixel::clear_pixel_fb(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept {
    SDL_SetRenderDrawColor(sdl_rend, r, g, b, a);
    SDL_RenderClear(sdl_rend);

    const uint32_t c = rgba_to_uint32(r, g, b, a);
    size_t count = fb_pixels_dim_size;
    uint32_t* p = fb_pixels.data();
    while(count--) { *p++ = c; }
    // std::fill(p, p+count, c);
    // ::memset(fb_pixels.data(), 0, fb_pixels_byte_size);
}

void pixel::swap_pixel_fb() noexcept {
    SDL_UpdateTexture(fb_texture, nullptr, fb_pixels.data(), (int)fb_pixels_byte_width);
    SDL_RenderCopy(sdl_rend, fb_texture, nullptr, nullptr);
    SDL_RenderPresent(sdl_rend);
}

void pixel::handle_events(bool& close, bool& resized, bool& set_dir, direction_t& dir) noexcept {
    static SDL_Scancode scancode = SDL_SCANCODE_STOP;
    close = false;
    resized = false;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                close = true;
                break;

            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_SHOWN:
                        // log_printf("Window Shown\n");
                        break;
                    case SDL_WINDOWEVENT_HIDDEN:
                        // log_printf("Window Hidden\n");
                        break;
                    case SDL_WINDOWEVENT_RESIZED:
                        printf("Window Resized: %d x %d\n", event.window.data1, event.window.data2);
                        resized = true;
                        on_window_resized();
                        break;
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        // printf("Window SizeChanged: %d x %d\n", event.window.data1, event.window.data2);
                        break;
                }
                break;

            case SDL_KEYUP:
                /**
                 * The following key sequence is possible, hence we need to validate whether the KEYUP
                 * matches and releases the current active keyscan/direction:
                 * - KEY DOWN: scancode 81 -> 'D', scancode 81, set_dir 1)
                 * - [    3,131] KEY DOWN: scancode 81 -> 'D', scancode 81, set_dir 1)
                 * - [    3,347] KEY DOWN: scancode 80 -> 'L', scancode 80, set_dir 1)
                 * - [    3,394] KEY UP: scancode 81 (ignored) -> 'L', scancode 80, set_dir 1)
                 * - [    4,061] KEY UP: scancode 80 (release) -> 'L', scancode 80, set_dir 0)
                 */
                if ( event.key.keysym.scancode == scancode ) {
                    set_dir = false;
                }
                break;

            case SDL_KEYDOWN:
                // keyboard API for key pressed
                switch (event.key.keysym.scancode) {
                    case SDL_SCANCODE_Q:
                        [[fallthrough]];
                    case SDL_SCANCODE_ESCAPE:
                        close = true;
                        break;
                    case SDL_SCANCODE_UP:
                        dir = direction_t::UP;
                        set_dir = true;
                        break;
                    case SDL_SCANCODE_LEFT:
                        dir = direction_t::LEFT;
                        set_dir = true;
                        break;
                    case SDL_SCANCODE_DOWN:
                        dir = direction_t::DOWN;
                        set_dir = true;
                        break;
                    case SDL_SCANCODE_RIGHT:
                        dir = direction_t::RIGHT;
                        set_dir = true;
                        break;
                    default:
                        break;
                }
                if( set_dir ) {
                    scancode = event.key.keysym.scancode;
                }
                break;
            default:
                break;
        }
    }
}
