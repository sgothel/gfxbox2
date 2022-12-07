#include "pixel/pixel.hpp"

#include <ctime>

int pixel::fb_width=0;
int pixel::fb_height=0;
int pixel::fb_max_x=0;
int pixel::fb_max_y=0;
pixel::pixel_buffer_t pixel::fb_pixels;
int pixel::frames_per_sec=60;

/**
 * Value in range {[0, screen_width-1], [0, screen_height-1] }
 */
int pixel::cart_origin[2] { 0, 0 };

int pixel::cart_min_x=0;
int pixel::cart_min_y=0;
int pixel::cart_max_x=0;
int pixel::cart_max_y=0;

uint32_t pixel::draw_color = 0;

static constexpr const uint64_t NanoPerMilli = 1000000UL;
static constexpr const uint64_t MilliPerOne = 1000UL;
static constexpr const uint64_t NanoPerOne = NanoPerMilli*MilliPerOne;

/**
 * See <http://man7.org/linux/man-pages/man2/clock_gettime.2.html>
 * <p>
 * Regarding avoiding kernel via VDSO,
 * see <http://man7.org/linux/man-pages/man7/vdso.7.html>,
 * clock_gettime seems to be well supported at least on kernel >= 4.4.
 * Only bfin and sh are missing, while ia64 seems to be complicated.
 */
uint64_t pixel::getCurrentMilliseconds() noexcept {
    struct timespec t;
    ::clock_gettime(CLOCK_MONOTONIC, &t);
    return static_cast<uint64_t>( t.tv_sec ) * MilliPerOne +
           static_cast<uint64_t>( t.tv_nsec ) / NanoPerMilli;
}

static uint64_t _exe_start_time = pixel::getCurrentMilliseconds();

uint64_t pixel::getElapsedMillisecond() noexcept {
    return getCurrentMilliseconds() - _exe_start_time;
}

void pixel::milli_sleep(uint64_t td_ms) noexcept {
    const int64_t td_ns_0 = (int64_t)( (td_ms * NanoPerMilli) % NanoPerOne );
    struct timespec ts { (int64_t)(td_ms/MilliPerOne), td_ns_0 };
    ::nanosleep( &ts, nullptr );
}

namespace pixel {

int from_screenx(const int x) {
    return x - cart_origin[0];
}

}
