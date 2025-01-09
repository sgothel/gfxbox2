/*
 * Author: Svenson Han Gothel and Sven Gothel
 * Copyright (c) 2022, 2024, .. Gothel Software e.K.
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

#ifndef JAU_FRACTION_TYPE_HPP_
#define JAU_FRACTION_TYPE_HPP_

#include <cstdint>
#include <cstring>
#include <ctime>
#include <string>

namespace jau {

    /**
     * Timespec structure using int64_t for its components
     * in analogy to `struct timespec_t` on 64-bit platforms.
     *
     * fraction_timespec covers an almost infinite range of time
     * while maintaining high precision like `struct timespec_t` on 64-bit platforms.
     *
     * If used as an absolute time-point, zero is time since Unix Epoch `00:00:00 UTC on 1970-01-01`.
     *
     * Note-1: Counting nanoseconds in int64_t only lasts until `2262-04-12`,
     * since INT64_MAX is 9'223'372'036'854'775'807 for 9'223'372'036 seconds or 292 years.
     *
     * Note-2: Limitations of `struct timespec` on 32-bit platforms
     * - to_timespec() conversion to `struct timespec`
     * - 32-bit signed integer only last for 68 years or until year 2038, starting from 1970 Unix Epoch
     * - [System call conversion for year 2038](https://lwn.net/Articles/643234/)
     * - test/test_fractions_01.cpp `struct timespec type validation Test 03.00`
     */
    struct fraction_timespec {
        /**
         * Seconds component, with its absolute value in range [0..inf[ or [0..inf),
         * sharing same sign with tv_nsec.
         */
        int64_t tv_sec;

        /**
         * Nanoseconds component with its absolute value in range [0..1'000'000'000[ or [0..1'000'000'000),
         * sharing same sign with tv_nsec.
         */
        int64_t tv_nsec;

        /**
         * Constructs a zero fraction_timespec instance
         */
        constexpr fraction_timespec() noexcept
        : tv_sec(0), tv_nsec(0) { }

        /**
         * Constructs a fraction_timespec instance with given components, normalized.
         */
        constexpr fraction_timespec(const int64_t& s, const int64_t& ns) noexcept
        : tv_sec(s), tv_nsec(ns) { normalize(); }

        /**
         * Constructs a fraction_timespec instance with given floating point seconds, normalized.
         */
        explicit constexpr fraction_timespec(const double seconds) noexcept
        : tv_sec( static_cast<int64_t>(seconds) ),
          tv_nsec( static_cast<int64_t>( (seconds - static_cast<double>(tv_sec)) * 1e+9) ) { }

        /**
         * Conversion constructor from broken down values assuming UTC
         * @param year year number, 0 as 0 A.D.
         * @param month month number [1-12]
         * @param day day of the month [1-31]
         * @param hour hours since midnight [0-23]
         * @param minute minutes after the hour [0-59]
         * @param seconds seconds after the minute including one leap second [0-60]
         * @param nano_seconds nanoseconds [0, 1'000'000'000)
        */
        static fraction_timespec from(int year, unsigned month, unsigned day,
                                      unsigned hour=0, unsigned minute=0,
                                      unsigned seconds=0, uint64_t nano_seconds=0) noexcept;

        /**
         * Normalize tv_nsec with its absolute range [0..1'000'000'000[ or [0..1'000'000'000)
         * and having same sign as tv_sec.
         *
         * Used after arithmetic operations.
         *
         * @returns reference to this instance
         */
        constexpr fraction_timespec& normalize() noexcept {
            if( 0 != tv_nsec ) {
                const int64_t ns_per_sec = 1000000000L;
                if( std::abs(tv_nsec) >= ns_per_sec ) {
                    const int64_t c = tv_nsec / ns_per_sec;
                    tv_nsec -= c * ns_per_sec;
                    tv_sec += c;
                }
                if( tv_nsec < 0 && tv_sec >= 1 ) {
                    tv_nsec += ns_per_sec;
                    tv_sec -= 1;
                } else if( tv_nsec > 0 && tv_sec <= -1 ) {
                    tv_nsec -= ns_per_sec;
                    tv_sec += 1;
                }
            }
            return *this;
        }

        constexpr bool isZero() noexcept { return 0 == tv_sec && 0 == tv_nsec; }
        constexpr void clear() noexcept { tv_sec=0; tv_nsec=0; }

        /**
         * Compound assignment (addition)
         *
         * @param rhs the other fraction_timespec
         * @return reference to this instance, normalized
         */
        constexpr fraction_timespec& operator+=(const fraction_timespec& rhs ) noexcept {
            tv_nsec += rhs.tv_nsec; // we allow the 'overflow' over 1'000'000'000, fitting into type and normalize() later
            tv_sec += rhs.tv_sec;
            return normalize();
        }

        /**
         * Negative compound assignment (subtraction)
         *
         * @param rhs the other fraction_timespec
         * @return reference to this instance, normalized
         */
        constexpr fraction_timespec& operator-=(const fraction_timespec& rhs ) noexcept {
            tv_nsec -= rhs.tv_nsec; // we allow the 'overflow' over 1'000'000'000, fitting into type and normalize() later
            tv_sec -= rhs.tv_sec;
            return normalize();
        }

        /**
         * Return conversion to POSIX `struct timespec`,
         * potentially narrowing the components if underlying system is not using 64-bit signed integer.
         *
         * Note-2: Limitations of `struct timespec` on 32-bit platforms
         * - 32-bit signed integer only last for 68 years or until year 2038, starting from 1970 Unix Epoch
         * - [System call conversion for year 2038](https://lwn.net/Articles/643234/)
         * - test/test_fractions_01.cpp `struct timespec type validation Test 03.00`
         */
        constexpr struct timespec to_timespec() const noexcept {
            using ns_type = decltype(timespec::tv_nsec);
            return { static_cast<std::time_t>( tv_sec ), static_cast<ns_type>( tv_nsec ) };
        }

        /**
         * Return simple string representation in seconds and nanoseconds.
         */
        std::string to_string() const noexcept;

        /**
         * Convenience string conversion interpreted since Unix Epoch in UTC
         * to ISO 8601 `YYYY-mm-ddTHH:MM:SS.ssZ`, e.g. '2022-06-23T14:34:10Z` or '2022-06-23T14:34:10.228978909Z`.
         *
         * Implementation uses `strftime()` with format `%Y-%m-%dT%H:%M:%S`
         * and adds 9 digits nanoseconds as fractions of seconds if not zero and the final `Z`.
         *
         * Time will be dropped if hours, minutes, seconds and fractions of a seconds are all zero.
         *
         * @param space_separator if true, use simple space separator instead of `T` and drop `Z`,
         *        otherwise use ISO 8601 as described above. Defaults to `false`.
         *
         * @param muteTime if true, always mute time
         */
        std::string to_iso8601_string(bool space_separator=false, bool muteTime=false) const noexcept;

        int64_t days(){
            return tv_sec / (int64_t)(3600*24);
        }
    };

    inline std::string to_string(const fraction_timespec& v) noexcept { return v.to_string(); }


    constexpr bool operator!=(const fraction_timespec& lhs, const fraction_timespec& rhs ) noexcept {
        return lhs.tv_sec != rhs.tv_sec || lhs.tv_nsec != rhs.tv_nsec;
    }

    constexpr bool operator==(const fraction_timespec& lhs, const fraction_timespec& rhs ) noexcept {
        return !( lhs != rhs );
    }

    constexpr bool operator>(const fraction_timespec& lhs, const fraction_timespec& rhs ) noexcept {
        return ( lhs.tv_sec > rhs.tv_sec ) || ( lhs.tv_sec == rhs.tv_sec && lhs.tv_nsec > rhs.tv_nsec );
    }

    constexpr bool operator>=(const fraction_timespec& lhs, const fraction_timespec& rhs ) noexcept {
        return ( lhs.tv_sec > rhs.tv_sec ) || ( lhs.tv_sec == rhs.tv_sec && lhs.tv_nsec >= rhs.tv_nsec );
    }

    constexpr bool operator<(const fraction_timespec& lhs, const fraction_timespec& rhs ) noexcept {
        return ( lhs.tv_sec < rhs.tv_sec ) || ( lhs.tv_sec == rhs.tv_sec && lhs.tv_nsec < rhs.tv_nsec );
    }

    constexpr bool operator<=(const fraction_timespec& lhs, const fraction_timespec& rhs ) noexcept {
        return ( lhs.tv_sec < rhs.tv_sec ) || ( lhs.tv_sec == rhs.tv_sec && lhs.tv_nsec <= rhs.tv_nsec );
    }

    /** Return the maximum of the two given fraction_timespec */
    constexpr const fraction_timespec& max(const fraction_timespec& lhs, const fraction_timespec& rhs ) noexcept {
        return lhs >= rhs ? lhs : rhs;
    }

    /** Return the minimum of the two given fraction_timespec */
    constexpr const fraction_timespec& min(const fraction_timespec& lhs, const fraction_timespec& rhs ) noexcept {
        return lhs <= rhs ? lhs : rhs;
    }

    /**
     * Returns the absolute fraction_timespec
     */
    inline fraction_timespec abs(const fraction_timespec& rhs) noexcept {
        fraction_timespec copy(rhs); // skip normalize
        copy.tv_sec= std::abs(rhs.tv_sec);
        copy.tv_nsec= std::abs(rhs.tv_nsec);
        return copy;
    }

    /**
     * Returns sum of two fraction_timespec.
     *
     * @param lhs a fraction_timespec
     * @param rhs a fraction_timespec
     * @return resulting new fraction_timespec, each component reduced and both fraction_timespec::normalize() 'ed
     */
    constexpr fraction_timespec operator+(const fraction_timespec& lhs, const fraction_timespec& rhs ) noexcept {
        fraction_timespec r(lhs);
        r += rhs; // implicit normalize
        return r;
    }

    /**
     * Returns difference of two fraction_timespec.
     *
     * See fraction_timespec::to_fraction_i64().
     *
     * @param lhs a fraction_timespec
     * @param rhs a fraction_timespec
     * @return resulting new fraction_timespec, each component reduced and both fraction_timespec::normalize() 'ed
     */
    constexpr fraction_timespec operator-(const fraction_timespec& lhs, const fraction_timespec& rhs ) noexcept {
        fraction_timespec r(lhs);
        r -= rhs; // implicit normalize
        return r;
    }

}

#endif /* JAU_FRACTION_TYPE_HPP_ */
