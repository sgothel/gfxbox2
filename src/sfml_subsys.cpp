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

#include <SFML/Graphics.hpp>

using namespace pixel;

static std::shared_ptr<sf::RenderWindow> window;
static float fb_origin_norm[2] = { 0.0f, 0.0f };
static size_t fb_pixels_dim_size = 0;
static size_t fb_pixels_byte_size = 0;
static size_t fb_pixels_byte_width = 0;
static sf::Texture fb_tbuffer;
static std::shared_ptr<sf::Sprite> fb_sbuffer;

static float gpu_fps = 0.0f;
static int gpu_frame_count = 0;
static uint64_t gpu_fps_t0 = 0;
static uint64_t gpu_swap_t0 = 0;
static uint64_t gpu_swap_t1 = 0;

uint32_t pixel::rgba_to_uint32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept {
    // 32-bit ABGR8888
    return ( ( (uint32_t)a << 24 ) & 0xff000000U ) |
            ( ( (uint32_t)b << 16 ) & 0x00ff0000U ) |
            ( ( (uint32_t)g <<  8 ) & 0x0000ff00U ) |
            ( ( (uint32_t)r       ) & 0x000000ffU );
}

void pixel::uint32_to_rgba(const uint32_t ui32, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) noexcept {
    // 32-bit ABGR8888
    a = ( ui32 & 0xff000000U ) >> 24;
    b = ( ui32 & 0x00ff0000U ) >> 16;
    g = ( ui32 & 0x0000ff00U ) >>  8;
    r = ( ui32 & 0x000000ffU );
}

static void on_window_resized(bool set_view, int wwidth, int wheight) noexcept {
    sf::Vector2u size = window->getSize();
    fb_width = (int)size.x;
    fb_height = (int)size.y;
    fb_max_x = fb_width - 1;
    fb_max_y = fb_height - 1;

    if( set_view ) {
        sf::FloatRect visibleArea(0, 0, (float)fb_width, (float)fb_height);
        window->setView(sf::View(visibleArea));
    }

    cart_coord.set_origin(fb_origin_norm[0], fb_origin_norm[1]);

    printf("Screen size %d x %d, min 0 / 0, max %d / %d \n",
            fb_width, fb_height, fb_max_x, fb_max_y);
    printf("%s\n", cart_coord.toString().c_str());
    win_width = wwidth;
    win_height = wheight;

    display_frames_per_sec = 60;
    window->setFramerateLimit(display_frames_per_sec);

    fb_pixels_dim_size = (size_t)(fb_width * fb_height);
    fb_pixels_byte_size = fb_pixels_dim_size * 4;
    fb_pixels_byte_width = (size_t)(fb_width * 4);
    fb_pixels.reserve(fb_pixels_dim_size);
    fb_pixels.resize(fb_pixels_dim_size);

    fb_tbuffer = sf::Texture();
    fb_tbuffer.create(fb_width, fb_height);
    fb_tbuffer.update((uint8_t*)fb_pixels.data());
    fb_sbuffer = std::make_shared<sf::Sprite>(fb_tbuffer);
}

void pixel::init_gfx_subsystem(const char* title, int wwidth, int wheight, const float origin_norm[2],
                               bool enable_vsync, bool use_subsys_primitives) {
    pixel::use_subsys_primitives_val = false; // FIXME use_subsys_primitives
    (void)use_subsys_primitives;
    (void)enable_vsync;

    fb_origin_norm[0] = origin_norm[0];
    fb_origin_norm[1] = origin_norm[1];

    window = std::make_shared<sf::RenderWindow>(sf::VideoMode((unsigned int)wwidth, (unsigned int)wheight), "Freefall01b");
    window->setTitle(std::string(title));
    window->setVerticalSyncEnabled(true);

    gpu_fps = 0.0f;
    gpu_fps_t0 = getCurrentMilliseconds();
    gpu_frame_count = 0;

    on_window_resized(false /* set_view */, wwidth, wheight);
}

void pixel::clear_pixel_fb(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept {
    window->clear(sf::Color(r, g, b, a));

    const uint32_t c = rgba_to_uint32(r, g, b, a);
    size_t count = fb_pixels_dim_size;
    uint32_t* p = fb_pixels.data();
    while(count--) { *p++ = c; }
    // std::fill(p, p+count, c);
    // ::memset(fb_pixels.data(), 0, fb_pixels_byte_size);
}

void pixel::swap_pixel_fb(const bool swap_buffer, int fps) noexcept {
    fb_tbuffer.update((uint8_t*)fb_pixels.data());
    window->draw(*fb_sbuffer);
    if( swap_buffer ) {
        pixel::swap_gpu_buffer(fps);
    }
}
void pixel::swap_gpu_buffer(int fps) noexcept {
    window->display();
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
            struct timespec ts { td_ns/pixel::NanoPerOne, td_ns_0 - fudge_ns };
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
    // TODO
}

void pixel::texture_t::draw(const int x_pos, const int y_pos, const float scale_x, const float scale_y) noexcept {
    (void)x_pos;
    (void)y_pos;
    (void)scale_x;
    (void)scale_y;
    // TODO
}

pixel::texture_ref pixel::make_text_texture(const std::string&) noexcept
{
    return std::make_shared<texture_t>(nullptr, 0, 0, 0, 0);
}

void pixel::subsys_set_pixel_color(uint8_t, uint8_t, uint8_t, uint8_t) noexcept {
    // FIXME
}

void pixel::subsys_draw_pixel(int x, int y) noexcept {
    fb_pixels[ y * fb_width + x ] = draw_color; // FIXME
}

void pixel::subsys_draw_line(int, int, int, int) noexcept {
    // FIXME
}


static input_event_type_t to_event_type(sf::Keyboard::Key scancode) {
    switch ( scancode ) {
        case sf::Keyboard::Key::Escape:
            return input_event_type_t::WINDOW_CLOSE_REQ;
        case sf::Keyboard::Key::P:
            return input_event_type_t::PAUSE;
        case sf::Keyboard::Key::Up:
            return input_event_type_t::P1_UP;
        case sf::Keyboard::Key::Left:
            return input_event_type_t::P1_LEFT;
        case sf::Keyboard::Key::Down:
            return input_event_type_t::P1_DOWN;
        case sf::Keyboard::Key::Right:
            return input_event_type_t::P1_RIGHT;
        case sf::Keyboard::Key::RShift:
            return input_event_type_t::P1_ACTION1;
        case sf::Keyboard::Key::Return:
            return input_event_type_t::P1_ACTION2;
        case sf::Keyboard::Key::RAlt:
            return input_event_type_t::P1_ACTION3;
            /**
        case sf::Keyboard::Key::RGui:
            return input_event_type_t::P1_ACTION4; */
        case sf::Keyboard::Key::W:
            return input_event_type_t::P2_UP;
        case sf::Keyboard::Key::A:
            return input_event_type_t::P2_LEFT;
        case sf::Keyboard::Key::S:
            return input_event_type_t::P2_DOWN;
        case sf::Keyboard::Key::D:
            return input_event_type_t::P2_RIGHT;
        case sf::Keyboard::Key::LShift:
            return input_event_type_t::P2_ACTION1;
        case sf::Keyboard::Key::LControl:
            return input_event_type_t::P2_ACTION2;
        case sf::Keyboard::Key::LAlt:
            return input_event_type_t::P2_ACTION3;
            /**
        case sf::Keyboard::Key::LGui:
            return input_event_type_t::P2_ACTION4; */
        case sf::Keyboard::Key::R:
            return input_event_type_t::RESET;
        default:
            return input_event_type_t::NONE;
    }
}

static uint16_t to_ascii(sf::Keyboard::Key scancode) {
    if(sf::Keyboard::Key::A <= scancode && scancode <= sf::Keyboard::Key::Num9 ) {
        return 'A' + ( scancode - sf::Keyboard::Key::A );
    }
    return 0;
}

bool pixel::handle_one_event(input_event_t& event) noexcept {
    sf::Event sf_event;
    if ( window->pollEvent(sf_event) ) {
        if (sf_event.type == sf::Event::Closed) {
            event.set(input_event_type_t::WINDOW_CLOSE_REQ);
        } else if (sf_event.type == sf::Event::Resized) {
            printf("Window Resized: %u x %u\n", sf_event.size.width, sf_event.size.height);
            event.set(input_event_type_t::WINDOW_RESIZED);
            on_window_resized(true /* set_view */, sf_event.size.width, sf_event.size.height);
        } else if (sf_event.type == sf::Event::MouseMoved) {
            event.pointer_motion((int)1 /* FIXME */,
                                 (int)sf_event.mouseMove.x, (int)sf_event.mouseMove.y);
        } else if (sf_event.type == sf::Event::KeyReleased) {
            const sf::Keyboard::Key scancode = sf_event.key.code;
            event.clear(to_event_type(scancode), to_ascii(scancode));
        } else if (sf_event.type == sf::Event::KeyPressed) {
            const sf::Keyboard::Key scancode = sf_event.key.code;
            event.set(to_event_type(scancode), to_ascii(scancode));
        }
        return true;
    } else {
        return false;
    }
}

void pixel::save_snapshot(const std::string& fname) noexcept {
    // TODO
    (void)fname;
}
