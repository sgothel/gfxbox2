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
#include "jau/utils.hpp"

#include <cmath>
#include <cstdint>
#include <cinttypes>
#include <ctime>
#include <random>

static std::random_device rng;

static constexpr float rng_to_norm(float v) noexcept {
    // std::random_device::result_type integral integer type
    constexpr std::random_device::result_type a = std::random_device::min();
    constexpr std::random_device::result_type b = std::random_device::max();
    return float(v - a) / float(b - a);
}

#if 0
static constexpr float rng_from_norm(float v) noexcept {
    // std::random_device::result_type integral integer type
    constexpr std::random_device::result_type a = std::random_device::min();
    constexpr std::random_device::result_type b = std::random_device::max();
    return v * float(b - a) + float(a);
}
#endif

float jau::next_rnd() noexcept {
    return rng_to_norm((float)rng());
}

#ifdef CLOCK_MONOTONIC_RAW
    // raw skips NTP adjustments
    #define JAU_CLOCK_MONOTONIC CLOCK_MONOTONIC_RAW
#else
    #define JAU_CLOCK_MONOTONIC CLOCK_MONOTONIC
#endif

/**
 * See <http://man7.org/linux/man-pages/man2/clock_gettime.2.html>
 * <p>
 * Regarding avoiding kernel via VDSO,
 * see <http://man7.org/linux/man-pages/man7/vdso.7.html>,
 * clock_gettime seems to be well supported at least on kernel >= 4.4.
 * Only bfin and sh are missing, while ia64 seems to be complicated.
 */
jau::fraction_timespec jau::getMonotonicTime() noexcept {
    struct timespec t;
    ::clock_gettime(JAU_CLOCK_MONOTONIC, &t);
    return fraction_timespec( (int64_t)t.tv_sec, (int64_t)t.tv_nsec );
}

static const jau::fraction_timespec _exe_start_time0 = jau::getMonotonicTime();
static uint64_t _exe_start_time1 = jau::getCurrentMilliseconds();

jau::fraction_timespec jau::getElapsedMonotonicTime() noexcept {
    return getMonotonicTime() - _exe_start_time0;
}

/**
 * See <http://man7.org/linux/man-pages/man2/clock_gettime.2.html>
 * <p>
 * Regarding avoiding kernel via VDSO,
 * see <http://man7.org/linux/man-pages/man7/vdso.7.html>,
 * clock_gettime seems to be well supported at least on kernel >= 4.4.
 * Only bfin and sh are missing, while ia64 seems to be complicated.
 */
uint64_t jau::getCurrentMilliseconds() noexcept {
    struct timespec t;
    ::clock_gettime(JAU_CLOCK_MONOTONIC, &t);
    return static_cast<uint64_t>( t.tv_sec ) * (uint64_t)MilliPerOne +
           static_cast<uint64_t>( t.tv_nsec ) / (uint64_t)NanoPerMilli;
}

uint64_t jau::getElapsedMillisecond() noexcept {
    return getCurrentMilliseconds() - _exe_start_time1;
}

void jau::milli_sleep(uint64_t td_ms) noexcept {
    const int64_t td_ns_0 = (int64_t)( (td_ms * (uint64_t)NanoPerMilli) % (uint64_t)NanoPerOne );
    struct timespec ts;
    ts.tv_sec = static_cast<decltype(ts.tv_sec)>(td_ms/(uint64_t)MilliPerOne); // signed 32- or 64-bit integer
    ts.tv_nsec = td_ns_0;
    ::nanosleep( &ts, nullptr );
}

void jau::log_printf(const uint64_t elapsed_ms, const char * format, ...) noexcept {
    fprintf(stderr, "[%s] ", to_decstring(elapsed_ms, ',', 9).c_str());
    va_list args;
    va_start (args, format);
    vfprintf(stderr, format, args); // NOLINT(format-nonliteral)
    va_end (args);
}
void jau::log_printf(const char * format, ...) noexcept {
    fprintf(stderr, "[%s] ", to_decstring(getElapsedMillisecond(), ',', 9).c_str());
    va_list args;
    va_start (args, format);
    vfprintf(stderr, format, args); // NOLINT(format-nonliteral)
    va_end (args);
}

//
//
//

using namespace jau::int_literals;

std::string jau::to_stringva(const char* format, va_list args) noexcept {
    size_t nchars;
    std::string str;
    {
        const size_t bsz = 1024; // including EOS
        str.reserve(bsz);  // incl. EOS
        str.resize(bsz-1); // excl. EOS

        nchars = vsnprintf(&str[0], bsz, format, args); // NOLINT(format-nonliteral)
        if( nchars < bsz ) {
            str.resize(nchars);
            return str;
        }
    }
    {
        const size_t bsz = std::min<size_t>(nchars+1, str.max_size()+1); // limit incl. EOS
        str.reserve(bsz);  // incl. EOS
        str.resize(bsz-1); // excl. EOS
        nchars = vsnprintf(&str[0], bsz, format, args);
        str.resize(nchars);
        return str;
    }
}

std::string jau::to_string(const char* format, ...) noexcept {
    va_list args;
    va_start (args, format);
    std::string str = to_stringva(format, args);
    va_end (args);
    return str;
}

//
//
//

using jau::fraction_timespec;

fraction_timespec fraction_timespec::from(int year, unsigned month, unsigned day,
                                          unsigned hour, unsigned minute,
                                          unsigned seconds, uint64_t nano_seconds) noexcept {
    fraction_timespec res;
    if( !( 1<=month && month<=12 &&
           1<=day && day<=31 &&
           hour<=23 &&
           minute<=59 && seconds<=60 ) ) {
        return res; // error
    }
    struct std::tm tm_0;
    ::memset(&tm_0, 0, sizeof(tm_0));
    tm_0.tm_year = year - 1900; // years since 1900
    tm_0.tm_mon = static_cast<int>(month) - 1; // months since Janurary [0-11]
    tm_0.tm_mday = static_cast<int>(day);      // day of the month [1-31]
    tm_0.tm_hour = static_cast<int>(hour);     // hours since midnight [0-23]
    tm_0.tm_min = static_cast<int>(minute);    // minutes after the hour [0-59]
    tm_0.tm_sec = static_cast<int>(seconds);   // seconds after the minute [0-60], including one leap second
    std::time_t t1 = ::timegm (&tm_0);
    res.tv_sec = static_cast<int64_t>(t1);
    res.tv_nsec = static_cast<int64_t>(nano_seconds);
    return res;
}

std::string fraction_timespec::to_string() const noexcept {
    return std::to_string(tv_sec) + "s + " + std::to_string(tv_nsec) + "ns";
}

std::string fraction_timespec::to_iso8601_string(bool space_separator, bool muteTime) const noexcept {
    std::time_t t0 = static_cast<std::time_t>(tv_sec);
    struct std::tm tm_0;
    if( nullptr == ::gmtime_r(&t0, &tm_0) ) {
        if( muteTime ) {
            if( space_separator ) {
                return "1970-01-01";
            } else {
                return "1970-01-01Z";
            }
        } else {
            if( space_separator ) {
                return "1970-01-01 00:00:00";
            } else {
                return "1970-01-01T00:00:00Z";
            }
        }
    } else {
        // 2022-05-28T23:23:50Z 20+1
        //
        // 1655994850s + 228978909ns
        // 2022-06-23T14:34:10.228978909Z 30+1
        char b[30 + 1];
        size_t p;
        if( muteTime || ( 0 == tm_0.tm_hour && 0 == tm_0.tm_min && 0 == tm_0.tm_sec && 0 == tv_nsec ) ) {
            p = ::strftime(b, sizeof(b), "%Y-%m-%d", &tm_0);
        } else {
            if( space_separator ) {
                p = ::strftime(b, sizeof(b), "%Y-%m-%d %H:%M:%S", &tm_0);
            } else {
                p = ::strftime(b, sizeof(b), "%Y-%m-%dT%H:%M:%S", &tm_0);
            }
        }
        if( 0 < p && p < sizeof(b) - 1 ) {
            size_t q = 0;
            const size_t remaining = sizeof(b) - p;
            if( !muteTime && 0 < tv_nsec ) {
                q = ::snprintf(b + p, remaining, ".%09" PRIi64, tv_nsec);
            }
            if( !space_separator ) {
                ::snprintf(b + p + q, remaining-q, "Z");
            }
        }
        return std::string(b);
    }
}

