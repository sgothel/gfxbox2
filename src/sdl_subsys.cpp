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
#include "pixel/version.hpp"
#include <jau/file_util.hpp>

#include <atomic>
#include <thread>

#include <SDL2/SDL.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

using namespace pixel;
using namespace jau;

static SDL_Window* sdl_win = nullptr;
static SDL_Renderer* sdl_rend = nullptr;
static float fb_origin_norm[2] = { 0.0f, 0.0f };
static size_t fb_pixels_dim_size = 0;
static size_t fb_pixels_byte_size = 0;
static size_t fb_pixels_byte_width = 0;
static SDL_Texture * fb_texture = nullptr;
static TTF_Font* sdl_font = nullptr;
static int monitor_frames_per_sec=60;
static int gpu_forced_fps_ = -1;
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

static void reset_gpu_fps(float fps) {
    gpu_fps = fps;
    gpu_fps_t0 = getCurrentMilliseconds();
    gpu_swap_t0 = gpu_fps_t0;
    gpu_swap_t1 = gpu_fps_t0;
}

int pixel::monitor_fps() noexcept { return monitor_frames_per_sec; }

int pixel::gpu_forced_fps() noexcept { return gpu_forced_fps_; }

void pixel::set_gpu_forced_fps(int fps) noexcept { gpu_forced_fps_=fps; reset_gpu_fps(float(fps)); }

static void on_window_resized(int wwidth, int wheight) noexcept {
    if( !sdl_rend ) { return; }
    const int old_fb_width = fb_width;
    const int old_fb_height = fb_height;
    SDL_GetRendererOutputSize(sdl_rend, &fb_width, &fb_height);

    if( 0 == wwidth || 0 == wheight ) {
        SDL_GetWindowSize(sdl_win, &wwidth, &wheight);
    }

    SDL_RendererInfo sdi;
    SDL_GetRendererInfo(sdl_rend, &sdi);
    fb_max_x = fb_width - 1;
    fb_max_y = fb_height - 1;

    cart_coord.set_origin(fb_origin_norm[0], fb_origin_norm[1]);
    cart_coord.set_sxy_win_to_fb( (float)fb_width/(float)wwidth, (float)fb_height/(float)wheight );

    printf("Win Size %d x %d -> %d x %d, FB/Win %f x %f \n",
            win_width, win_height, wwidth, wheight, cart_coord.sx_win_to_fb(), cart_coord.sy_win_to_fb());
    printf("FB Size %d x %d -> %d x %d, min 0 / 0, max %d / %d \n",
            old_fb_width, old_fb_height, fb_width, fb_height, fb_max_x, fb_max_y);
    win_width = wwidth;
    win_height = wheight;
    {
        SDL_DisplayMode mode;
        const int win_display_idx = SDL_GetWindowDisplayIndex(sdl_win);
        ::bzero(&mode, sizeof(mode));
        SDL_GetCurrentDisplayMode(win_display_idx, &mode); // SDL_GetWindowDisplayMode(..) fails on some systems (wrong refresh_rate and logical size
        if( mode.refresh_rate > 0 ) {
            monitor_frames_per_sec = mode.refresh_rate;
        }
        printf("WindowDisplayMode: %d x %d @ %d (-> %d) Hz @ display %d\n", mode.w, mode.h, mode.refresh_rate, monitor_frames_per_sec, win_display_idx);
    }
    printf("Renderer %s\n", sdi.name);
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
        const std::string fontfile = "fonts/freefont/FreeSansBold.ttf";
        const std::string fontpath = pixel::resolve_asset(fontfile);
        if( !fontpath.size() ) {
            log_printf("font: No asset path for font-file '%s' in asset dir '%s'\n", fontfile.c_str(), pixel::asset_dir().c_str());
            return;
        }
        font_height = std::max(24, fb_height / 35);
        sdl_font = TTF_OpenFont(fontpath.c_str(), font_height);
        if( nullptr == sdl_font ) {
            fprintf(stderr, "font: Null font for '%s': %s\n", fontpath.c_str(), SDL_GetError());
        } else {
            printf("Using font %s, size %d\n", fontpath.c_str(), font_height);
        }
    }
}

static std::atomic_bool gfx_subsystem_init_called = false;
static std::atomic_bool gfx_subsystem_init = false;

bool pixel::is_gfx_subsystem_initialized() noexcept {
    return gfx_subsystem_init;
}

bool pixel::init_gfx_subsystem(const char* exe_path, const char* title, int wwidth, int wheight, const float origin_norm[2],
                        bool enable_vsync, bool use_subsys_primitives) {
    lookup_and_register_asset_dir(exe_path);

    bool exp_init_called = false;
    if( !gfx_subsystem_init_called.compare_exchange_strong(exp_init_called, true) ) {
        return gfx_subsystem_init;
    }
    printf("gfxbox2 version %s\n", pixel::VERSION_LONG);

    pixel::use_subsys_primitives_val = use_subsys_primitives;

    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        printf("SDL: Error initializing mandatory subsys: %s\n", SDL_GetError());
        return false;
    }
    if ( ( IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG ) != IMG_INIT_PNG ) {
        printf("SDL_image: Error initializing: %s\n", SDL_GetError());
        return false;
    }
    if( 0 != TTF_Init() ) {
        printf("SDL_TTF: Error initializing: %s\n", SDL_GetError());
        return false;
    }

    if( enable_vsync ) {
        SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    }
    // SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, std::to_string(pixel_filter_quality).c_str());

    fb_origin_norm[0] = origin_norm[0];
    fb_origin_norm[1] = origin_norm[1];

    const Uint32 win_flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE /* | SDL_WINDOW_OPENGL */;
    const Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;

    if( 0 != win_width && 0 != win_height ) {
        // override using pre-set default, i.e. set_window_size(..)
        wwidth = win_width; wheight = win_height;
    }
    sdl_win = SDL_CreateWindow(title,
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            wwidth, wheight,
            win_flags);

    if (nullptr == sdl_win) {
        printf("SDL: Error initializing window: %s\n", SDL_GetError());
        return false;
    }

    Uint32 sdl_win_id = SDL_GetWindowID(sdl_win);
    if (0 == sdl_win_id) {
        printf("SDL: Error retrieving window ID: %s\n", SDL_GetError());
        SDL_DestroyWindow(sdl_win);
        return false;
    }

    sdl_rend = SDL_CreateRenderer(sdl_win, -1, render_flags);
    if (nullptr == sdl_rend) {
        printf("SDL: Error creating renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(sdl_win);
        return false;
    }

    gpu_frame_count = 0;
    gfx_subsystem_init = true;

    on_window_resized(wwidth, wheight);

    reset_gpu_fps(float(monitor_frames_per_sec));

    return true;
}

extern "C" {
    EMSCRIPTEN_KEEPALIVE void set_forced_fps(int v) noexcept { set_gpu_forced_fps(v); }

    EMSCRIPTEN_KEEPALIVE int get_forced_fps() noexcept { return gpu_forced_fps(); }

    EMSCRIPTEN_KEEPALIVE void set_window_size(int ww, int wh) noexcept {
        static bool warn_once = true;
        if( win_width != ww || win_height != wh ) {
            if( std::abs(win_width - ww) > 1 || std::abs(win_height - wh) > 1 ) {
                if( !is_gfx_subsystem_initialized() || 0 == win_width || 0 == win_height ) {
                    printf("JS Window Initial Size: Win %d x %d -> %d x %d\n", win_width, win_height, ww, wh);
                    win_width = ww;
                    win_height = wh;
                } else {
                    printf("JS Window Resized: Win %d x %d -> %d x %d\n", win_width, win_height, ww, wh);
                    SDL_SetWindowSize( sdl_win, ww, wh );
                    warn_once = true;
                    on_window_resized(ww, wh);
                }
            } else if( warn_once ) {
                warn_once = false;
                printf("JS Window Resize Ignored: Win %d x %d -> %d x %d\n", win_width, win_height, ww, wh);
            }
        }
    }
}

void pixel::clear_pixel_fb(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept {
    if( !sdl_rend ) { return; }
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

void pixel::swap_pixel_fb(const bool swap_buffer, int fps) noexcept {
    if( sdl_rend && !use_subsys_primitives_val ) {
        SDL_UpdateTexture(fb_texture, nullptr, fb_pixels.data(), (int)fb_pixels_byte_width);
        SDL_RenderCopy(sdl_rend, fb_texture, nullptr, nullptr);
    }
    if( swap_buffer ) {
        pixel::swap_gpu_buffer(fps);
    }
}
void pixel::swap_gpu_buffer(int fps) noexcept {
    if( !sdl_rend ) { return; }
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
        const int64_t fudge_ns = jau::NanoPerMilli / 4;
        const uint64_t ms_per_frame = (uint64_t)std::round(1000.0 / fps);
        const uint64_t ms_this_frame =  gpu_swap_t0 - gpu_swap_t1;
        int64_t td_ns = int64_t( ms_per_frame - ms_this_frame ) * jau::NanoPerMilli;
        if( td_ns > fudge_ns ) {
            const int64_t td_ns_0 = td_ns%jau::NanoPerOne;
            struct timespec ts;
            ts.tv_sec = static_cast<decltype(ts.tv_sec)>(td_ns/jau::NanoPerOne); // signed 32- or 64-bit integer
            ts.tv_nsec = td_ns_0 - fudge_ns;
            nanosleep( &ts, nullptr );
            // pixel::log_printf("soft-sync [exp %zd > has %zd]ms, delay %" PRIi64 "ms (%lds, %ldns)\n",
            //         ms_per_frame, ms_this_frame, td_ns/pixel::NanoPerMilli, ts.tv_sec, ts.tv_nsec);
        }
        gpu_swap_t1 = jau::getCurrentMilliseconds();
    } else {
        gpu_swap_t1 = gpu_swap_t0;
    }
}

float pixel::gpu_avg_fps() noexcept {
    return gpu_fps;
}

//
// Bitmap
//
static constexpr const bool DEBUG_TEX = false;

const char* pixel::bitmap_t::format_str(uint32_t fmt) noexcept {
    const char* s = SDL_GetPixelFormatName(fmt);
    if( 0==strncmp("SDL_PIXELFORMAT_", s, 16) ) {
        return s+16;
    }
    return s;
}
void pixel::bitmap_t::destroy() noexcept {
    if( nullptr != m_handle ) {
        SDL_Surface * s = reinterpret_cast<SDL_Surface*>(m_handle);
        SDL_FreeSurface(s);
        m_handle = nullptr;
        m_pixels = nullptr;
    }
}

pixel::bitmap_t::bitmap_t(const std::string& fname0) noexcept
: m_handle(nullptr), m_pixels(nullptr), width(0), height(0), bpp(0), stride(0), format(0)
{
    const std::string fname1 = pixel::resolve_asset(fname0);
    if( !fname1.size() ) {
        log_printf("bitmap_t: Could locate file '%s' in asset dir '%s'\n", fname0.c_str(), pixel::asset_dir().c_str());
        return;
    }
    SDL_Surface* surface = IMG_Load(fname1.c_str());
    if( nullptr == surface ) {
        log_printf("bitmap_t: Error loading %s: %s\n", fname1.c_str(), SDL_GetError());
        return;
    }
    if( DEBUG_TEX ) {
        log_printf("bitmap_t: Loaded fmt %s, %d x %d pitch %d\n", format_str(surface->format->format), surface->w, surface->h, surface->pitch);
    }
    if( SDL_PIXELFORMAT_ABGR8888 != surface->format->format ) {
        SDL_Surface *old = surface;
        surface = SDL_ConvertSurfaceFormat(old, SDL_PIXELFORMAT_ABGR8888, 0);
        if( DEBUG_TEX ) {
            log_printf("bitmap_t: Converting fmt %s -> %s, target %s\n",
                format_str(old->format->format),
                format_str(surface->format->format),
                format_str(SDL_PIXELFORMAT_ABGR8888));
        }
        SDL_FreeSurface(old);
    }
    m_handle = reinterpret_cast<void*>(surface);
    m_pixels = reinterpret_cast<uint8_t*>(surface->pixels);
    width = surface->w;
    height = surface->h;
    bpp = SDL_BYTESPERPIXEL(surface->format->format);
    stride = surface->pitch;
    format = surface->format->format;
}

pixel::bitmap_t::bitmap_t(const uint32_t width_, const uint32_t height_) noexcept
: m_handle(nullptr), m_pixels(nullptr), width(0), height(0), bpp(0), stride(0), format(0)
{
    SDL_Surface *surface = SDL_CreateRGBSurface(0, (int)width_, (int)height_, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    if( nullptr != surface ) {
        if( DEBUG_TEX ) {
            log_printf("bitmap_t: Created fmt %s, %d x %d pitch %d\n", format_str(surface->format->format), surface->w, surface->h, surface->pitch);
        }
        m_handle = reinterpret_cast<void*>(surface);
        m_pixels = reinterpret_cast<uint8_t*>(surface->pixels);
        width = surface->w;
        height = surface->h;
        bpp = SDL_BYTESPERPIXEL(surface->format->format);
        stride = surface->pitch;
        format = surface->format->format;
    } else {
        log_printf("bitmap_t: Error creating RGBA %dx%d: %s\n", width_, height_, SDL_GetError());
    }
}

pixel::bitmap_t::bitmap_t(const bitmap_t& o, int /*unused*/) noexcept
: m_handle(nullptr), m_pixels(nullptr), width(0), height(0), bpp(0), stride(0), format(0)
{
    if( !o.m_pixels ) {
        log_printf("bitmap_t: Cloning empty surface: %s, %s\n", o.toString().c_str());
        return;
    }
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, (int)o.width, (int)o.height, (int)o.bpp*8, o.format);
    if( surface ) {
        if( DEBUG_TEX ) {
            log_printf("bitmap_t: Created fmt %s, %d x %d pitch %d\n", format_str(surface->format->format), surface->w, surface->h, surface->pitch);
        }
        SDL_Surface * o_s = reinterpret_cast<SDL_Surface*>(o.m_handle);
        SDL_Rect srcrect = { .x=(int)0, .y=(int)0, .w=(int)o.width, .h=(int)o.height};
        SDL_Rect destrect = { .x=(int)0,
                          .y=(int)0,
                          .w=(int)width, .h=(int)height };
        if( SDL_UpperBlit(o_s, &srcrect, surface, &destrect) ) {
            log_printf("bitmap_t: Cloning blit error: %s, %s, %s\n", o.toString().c_str(), toString().c_str(), SDL_GetError());
        }
        m_handle = reinterpret_cast<void*>(surface);
        m_pixels = reinterpret_cast<uint8_t*>(surface->pixels);
        width = surface->w;
        height = surface->h;
        bpp = SDL_BYTESPERPIXEL(surface->format->format);
        stride = surface->pitch;
        format = surface->format->format;
    } else {
        log_printf("bitmap_t: Error cloning surface: %s, %s\n", o.toString().c_str(), SDL_GetError());
    }
}

//
// Texture
//
void pixel::texture_t::destroy() noexcept {
    if( nullptr != m_handle && m_owner ) {
        SDL_Texture* tex = reinterpret_cast<SDL_Texture*>(m_handle);
        SDL_DestroyTexture(tex);
    }
    m_handle = nullptr;
}

void pixel::texture_t::draw_raw(const uint32_t fb_x, const uint32_t fb_y, const uint32_t fb_w, const uint32_t fb_h) const noexcept {
    if( sdl_rend && m_handle ) {
        SDL_Texture* tex = reinterpret_cast<SDL_Texture*>(m_handle);
        SDL_Rect src = { .x=(int)x, .y=(int)y, .w=(int)width, .h=(int)height};
        SDL_Rect dest = { .x=(int)fb_x,
                          .y=(int)fb_y,
                          .w=(int)fb_w, .h=(int)fb_h };
        SDL_RenderCopy(sdl_rend, tex, &src, &dest);
    }
}

pixel::texture_t::texture_t(const bitmap_ref& bmap) noexcept
: m_handle(nullptr), m_owner(false), x(0), y(0), width(0), height(0), bpp(0), format(0), dest_x(0), dest_y(0), dest_sx(1), dest_sy(1)
{
    if( !sdl_rend ) { return; }
    SDL_Surface * surface = reinterpret_cast<SDL_Surface*>(bmap->handle());
    SDL_Texture* tex_;
    if( surface ) {
        if( DEBUG_TEX ) {
            log_printf("texture_t: Given surface fmt %s, %d x %d pitch %d\n", format_str(surface->format->format), surface->w, surface->h, surface->pitch);
        }
        tex_ = SDL_CreateTextureFromSurface(sdl_rend, surface);
        if( nullptr == tex_ ) {
            log_printf("texture_t: Error loading surface: %s\n", SDL_GetError());
        }
    } else {
        tex_ = nullptr;
        log_printf("texture_t: Empty surface\n");
    }
    m_handle = reinterpret_cast<void*>(tex_);
    if( nullptr != tex_ ) {
        Uint32 format_ = 0;
        int w=0, h=0;
        SDL_QueryTexture(tex_, &format_, nullptr, &w, &h);
        width = w;
        height = h;
        bpp = SDL_BYTESPERPIXEL(format_);
        format = format_;
        if( DEBUG_TEX ) {
            log_printf("texture_t: Loaded fmt %s, %d x %d\n", format_str(format), width, height);
        }
    }
    m_owner = true;
}

pixel::texture_t::texture_t(const std::string& fname0) noexcept
: m_handle(nullptr), m_owner(false), x(0), y(0), width(0), height(0), bpp(0), format(0), dest_x(0), dest_y(0), dest_sx(1), dest_sy(1)
{
    if( !sdl_rend ) { return; }
    const std::string fname1 = pixel::resolve_asset(fname0);
    if( !fname1.size() ) {
        log_printf("texture_t: Could locate file '%s' in asset dir '%s'\n", fname0.c_str(), pixel::asset_dir().c_str());
        return;
    }
    SDL_Texture* tex_ = IMG_LoadTexture(sdl_rend, fname1.c_str());
    if( nullptr == tex_ ) {
        log_printf("texture_t: Error loading %s: %s\n", fname1.c_str(), SDL_GetError());
    }
    m_handle = reinterpret_cast<void*>(tex_);
    if( tex_ ) {
        Uint32 format_ = 0;
        int w=0, h=0;
        SDL_QueryTexture(tex_, &format_, nullptr, &w, &h);
        width = w;
        height = h;
        bpp = SDL_BYTESPERPIXEL(format_);
        format = format_;
        if( DEBUG_TEX ) {
            log_printf("texture_t: Loaded fmt %s, %d x %d\n", format_str(format), width, height);
        }
    }
    m_owner = true;
}

void pixel::texture_t::update(const bitmap_ref& bmap) noexcept {
    if( nullptr == m_handle || bmap->format != format || bmap->width > width || bmap->height > height) {
        log_printf("texture_t: Update mismatch: source %s, target %s\n", bmap->toString().c_str(), toString().c_str());
        return;
    }
    SDL_Texture* tex = reinterpret_cast<SDL_Texture*>(m_handle);
    SDL_Rect rect { 0, 0, (int)bmap->width, (int)bmap->height};
    if( SDL_UpdateTexture(tex, &rect, bmap->pixels(), (int)bmap->stride) ) {
        log_printf("texture_t: Update error: %s\n", SDL_GetError());
    }
}

//
// Text Texture
//

static bool _tex_font_warn0_once = true;
static bool _tex_font_warn1_once = true;
pixel::texture_ref pixel::make_text(const std::string& text) noexcept
{
    if( !sdl_rend || !sdl_font ) {
        if( _tex_font_warn0_once ) {
            fprintf(stderr, "make_text_texture: Null texture for '%s': Uninitialized %s\n", text.c_str(),
                !sdl_font ? "font" : "renderer");
            _tex_font_warn0_once = false;
        }
        return std::make_shared<texture_t>();
    }
    uint8_t r, g, b, a;
    pixel::uint32_to_rgba(draw_color, r, g, b, a);
    SDL_Color foregroundColor = { r, g, b, a };

    SDL_Surface* textSurface = TTF_RenderText_Solid(sdl_font, text.c_str(), foregroundColor);
    if( nullptr == textSurface ) {
        if( _tex_font_warn1_once ) {
            fprintf(stderr, "make_text_texture: Null texture for '%s': %s\n", text.c_str(), SDL_GetError());
            _tex_font_warn1_once = false;
        }
        return std::make_shared<texture_t>();
    }
    SDL_Texture* sdl_tex = SDL_CreateTextureFromSurface(sdl_rend, textSurface);
    texture_ref tex = std::make_shared<texture_t>(reinterpret_cast<void*>(sdl_tex), 0, 0, 0, 0, 0, 0);
    {
        Uint32 format_ = 0;
        int w=0, h=0;
        SDL_QueryTexture(sdl_tex, &format_, nullptr, &w, &h);
        tex->width = w;
        tex->height = h;
        tex->bpp = SDL_BYTESPERPIXEL(format_);
        tex->format = format_;
    }
    SDL_FreeSurface(textSurface);
    return tex;
}


//
// Primitives
//

void pixel::subsys_set_pixel_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept {
    if( sdl_rend ) {
        SDL_SetRenderDrawColor(sdl_rend, r, g, b, a);
    }
}

void pixel::subsys_draw_pixel(int x, int y) noexcept {
    if( sdl_rend ) {
        SDL_RenderDrawPoint(sdl_rend, x, y);
    }
}

void pixel::subsys_draw_line(int x1, int y1, int x2, int y2) noexcept {
    if( sdl_rend ) {
       SDL_RenderDrawLine(sdl_rend, x1, y1, x2, y2);
    }
}

void pixel::subsys_draw_box(bool filled, int x, int y, int width, int height) noexcept
{
    if( sdl_rend ) {
       SDL_Rect bounds = { .x=x, .y=y, .w=width, .h=height };
       if( filled ) {
           SDL_RenderFillRect(sdl_rend, &bounds);
       } else {
           SDL_RenderDrawRect(sdl_rend, &bounds);
       }
    }
}

void pixel::subsys_draw_line(int thickness, int x1, int y1, int x2, int y2) noexcept
{
    if( !sdl_rend || 0 >= thickness ) {
        return;
    }
    const int x1_i = x1;
    const int y1_i = y1;
    const int x2_i = x2;
    const int y2_i = y2;
    const int d_extra = thickness - 1;
    if( 0 == d_extra ) {
        SDL_RenderDrawLine(sdl_rend, x1_i, y1_i, x2_i, y2_i);
        return;
    }
    const float d_x = float(std::abs( x1_i - x2_i ));
    const float d_y = float(std::abs( y1_i - y2_i ));

    // Create a polygon of connected dots, aka poly-line
    const int c_l = -1 * ( d_extra / 2 + d_extra % 2 ); // gets the remainder
    const int c_r =        d_extra / 2;
    std::vector<SDL_Point> points;
    bool first_of_two = true;
    for(int i=c_l; i<=c_r; ++i) {
        int ix, iy; // rough and simply thickness delta picking
        if( d_y > d_x ) {
            ix = i;
            iy = 0;
        } else {
            ix = 0;
            iy = i;
        }
        if( first_of_two ) {
            points.push_back( SDL_Point { x1_i+ix, y1_i+iy } );
            points.push_back( SDL_Point { x2_i+ix, y2_i+iy } );
            first_of_two = false;
        } else {
            points.push_back( SDL_Point { x2_i+ix, y2_i+iy } );
            points.push_back( SDL_Point { x1_i+ix, y1_i+iy } );
            first_of_two = true;
        }
    }
    const int err = SDL_RenderDrawLines(sdl_rend, points.data(), int(points.size()));
    if( false ) {
        log_printf("Poly-Line thick_p %d, %d lines, d %.4f/%.4f, err %d, %s\n", thickness, points.size()/2, d_x, d_y, err, SDL_GetError());
    }
}


//
// Events
//

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
        case SDL_SCANCODE_RCTRL:
            return input_event_type_t::P1_ACTION4;
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
        case SDL_SCANCODE_Z:
            return input_event_type_t::P2_ACTION4;
            /**
        case SDL_SCANCODE_LGUI:
            return input_event_type_t::P2_ACTION4; */
        case SDL_SCANCODE_I:
            return input_event_type_t::P3_UP;
        case SDL_SCANCODE_J:
            return input_event_type_t::P3_LEFT;
        case SDL_SCANCODE_K:
            return input_event_type_t::P3_DOWN;
        case SDL_SCANCODE_L:
            return input_event_type_t::P3_RIGHT;
        case SDL_SCANCODE_V:
            return input_event_type_t::P3_ACTION1;
        case SDL_SCANCODE_B:
            return input_event_type_t::P3_ACTION2;
        case SDL_SCANCODE_N:
            return input_event_type_t::P3_ACTION3;
        case SDL_SCANCODE_M:
            return input_event_type_t::P3_ACTION4;
        case SDL_SCANCODE_R:
            return input_event_type_t::RESET;
        case SDL_SCANCODE_F1:
            return input_event_type_t::F1;
        case SDL_SCANCODE_F2:
            return input_event_type_t::F2;
        case SDL_SCANCODE_F3:
            return input_event_type_t::F3;
        case SDL_SCANCODE_F4:
            return input_event_type_t::F4;
        case SDL_SCANCODE_F5:
            return input_event_type_t::F5;
        case SDL_SCANCODE_F6:
            return input_event_type_t::F6;
        case SDL_SCANCODE_F7:
            return input_event_type_t::F7;
        case SDL_SCANCODE_F8:
            return input_event_type_t::F8;
        case SDL_SCANCODE_F9:
            return input_event_type_t::F9;
        case SDL_SCANCODE_F10:
            return input_event_type_t::F10;
        case SDL_SCANCODE_F11:
            return input_event_type_t::F11;
        case SDL_SCANCODE_F12:
            return input_event_type_t::F12;
        default:
            return input_event_type_t::ANY_KEY;
    }
}
static uint16_t to_ascii(SDL_Scancode scancode) {
    if(SDL_SCANCODE_A <= scancode && scancode <= SDL_SCANCODE_Z ) {
        return 'a' + ( scancode - SDL_SCANCODE_A );
    }
    if(SDL_SCANCODE_1 <= scancode && scancode <= SDL_SCANCODE_9 ) {
        return '1' + ( scancode - SDL_SCANCODE_1 );
    }
    if(SDL_SCANCODE_0 == scancode ) {
        return '0' + ( scancode - SDL_SCANCODE_0 );
    }
    switch( scancode ) {
        case SDL_SCANCODE_SEMICOLON: return ';';

        case SDL_SCANCODE_MINUS:
            [[fallthrough]];
        case SDL_SCANCODE_KP_MINUS: return '-';

        case SDL_SCANCODE_KP_PLUS: return '+';

        case SDL_SCANCODE_KP_MULTIPLY: return '*';

        case SDL_SCANCODE_SLASH:
            [[fallthrough]];
        case SDL_SCANCODE_KP_DIVIDE: return '/';

        case SDL_SCANCODE_KP_PERCENT: return '%';

        case SDL_SCANCODE_KP_LEFTPAREN:
            [[fallthrough]];
        case SDL_SCANCODE_KP_LEFTBRACE:
            [[fallthrough]];
        case SDL_SCANCODE_LEFTBRACKET: return '(';

        case SDL_SCANCODE_KP_RIGHTPAREN:
            [[fallthrough]];
        case SDL_SCANCODE_KP_RIGHTBRACE:
            [[fallthrough]];
        case SDL_SCANCODE_RIGHTBRACKET: return ')';

        case SDL_SCANCODE_COMMA: return ',';

        case SDL_SCANCODE_PERIOD: return '.';

        case SDL_SCANCODE_SPACE:
            [[fallthrough]];
        case SDL_SCANCODE_TAB: return ' ';

        case SDL_SCANCODE_RETURN:
            [[fallthrough]];
        case SDL_SCANCODE_KP_ENTER: return '\n';

        case SDL_SCANCODE_BACKSPACE: return 0x08;

        default:
            return 0;
    }
    return 0;
}

static constexpr const bool DEBUG_KEY = false;

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
                        printf("Window SizeChanged: %d x %d\n", sdl_event.window.data1, sdl_event.window.data2);
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
                    if constexpr ( DEBUG_KEY ) {
                        printf("%s: UP  : %s\n",
                            to_decstring(getElapsedMillisecond(), ',', 9).c_str(),
                            to_string(event).c_str());
                    }
                  }
                  break;

                case SDL_KEYDOWN: {
                    const SDL_Scancode scancode = sdl_event.key.keysym.scancode;
                    event.set(to_event_type(scancode), to_ascii(scancode));
                    if constexpr ( DEBUG_KEY ) {
                    printf("%s: DOWN: %s\n",
                            to_decstring(getElapsedMillisecond(), ',', 9).c_str(),
                            to_string(event).c_str());
                 //       printf("%d", scancode);
                    }
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
        SDL_RenderReadPixels(sdl_rend, nullptr, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);
        char * fname2 = strdup(fname.c_str());
        std::thread t(&store_surface, sshot, fname2);
        t.detach();
    }
#endif

