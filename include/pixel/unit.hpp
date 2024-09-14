/*
 * unit.hpp
 *
 *  Created on: Feb 13, 2024
 *      Author: svenson
 */

#ifndef INCLUDE_PIXEL_UNIT_HPP_
#define INCLUDE_PIXEL_UNIT_HPP_

#include <cstdint>
#include <cstring>
#include <ctime>
#include <numbers>
#include <string>

namespace pixel {

    /** Time in fractions of seconds. */
    typedef float si_time_t;
    /** Length in fractions of meter. */
    typedef float si_length_t;
    /** Mass in fractions of kilograms. */
    typedef float si_mass_t;
    /** Velocity in fractions of meter/seconds. */
    typedef float si_velo_t;
    /** Acceleration in fractions of meter/seconds^2. */
    typedef float si_accel_t;
    /** Angle in fraction of radians. */
    typedef float si_angle_t;

    namespace literals {
        constexpr si_time_t operator ""_year(unsigned long long int __v)   { return (si_time_t)__v*365.25f*24.0f*3600.0f; }
        constexpr si_time_t operator ""_month(unsigned long long int __v)   { return (si_time_t)__v*30.0f*24.0f*3600.0f; }
        constexpr si_time_t operator ""_week(unsigned long long int __v)   { return (si_time_t)__v*7.0f*24.0f*3600.0f; }
        constexpr si_time_t operator ""_day(unsigned long long int __v)   { return (si_time_t)__v*24.0f*3600.0f; }

        constexpr si_time_t operator ""_h(unsigned long long int __v)   { return (si_time_t)__v*3600.0f; }
        constexpr si_time_t operator ""_min(unsigned long long int __v)   { return (si_time_t)__v*60.0f; }
        constexpr si_time_t operator ""_s(unsigned long long int __v)   { return (si_time_t)__v; }
        constexpr si_time_t operator ""_ms(unsigned long long int __v)   { return (si_time_t)__v/1000.0f; }

        constexpr si_length_t operator ""_km(unsigned long long int __v)   { return (si_length_t)__v*1000.0f; }
        constexpr si_length_t operator ""_m(unsigned long long int __v)   { return (si_length_t)__v; }
        constexpr si_length_t operator ""_dm(unsigned long long int __v)   { return (si_length_t)__v/10.0f; }
        constexpr si_length_t operator ""_cm(unsigned long long int __v)   { return (si_length_t)__v/100.0f; }
        constexpr si_length_t operator ""_mm(unsigned long long int __v)   { return (si_length_t)__v/1000.0f; }

        constexpr si_mass_t operator ""_t(unsigned long long int __v)   { return (si_mass_t)__v * 1000.0f; }
        constexpr si_mass_t operator ""_kg(unsigned long long int __v)   { return (si_mass_t)__v; }
        constexpr si_mass_t operator ""_g(unsigned long long int __v)   { return (si_mass_t)__v/1000.0f; }
        constexpr si_mass_t operator ""_mg(unsigned long long int __v)   { return (si_mass_t)__v/1000000.0f; }

        constexpr si_velo_t operator ""_km_s(unsigned long long int __v)   { return (si_velo_t)__v * 1000.0f; }
        constexpr si_velo_t operator ""_m_s(unsigned long long int __v)   { return (si_velo_t)__v; }
        constexpr si_velo_t operator ""_km_h(unsigned long long int __v)   { return (si_velo_t)__v / 3.6f; }
        constexpr si_velo_t operator ""_m_h(unsigned long long int __v)   { return (si_velo_t)__v / 3600.0f; }

        constexpr si_accel_t operator ""_km_s2(unsigned long long int __v)   { return (si_accel_t)__v * 1000.0f; }
        constexpr si_accel_t operator ""_m_s2(unsigned long long int __v)   { return (si_accel_t)__v; }
        constexpr si_accel_t operator ""_dm_s2(unsigned long long int __v)   { return (si_accel_t)__v / 10.0f; }
        constexpr si_accel_t operator ""_cm_s2(unsigned long long int __v)   { return (si_accel_t)__v / 100.0f; }
        constexpr si_accel_t operator ""_mm_s2(unsigned long long int __v)   { return (si_accel_t)__v / 1000.0f; }

        constexpr si_angle_t operator ""_rad(unsigned long long int __v)   { return (si_angle_t)__v; }
        constexpr si_angle_t operator ""_deg(unsigned long long int __v)   { return (si_angle_t)((long double)__v / 180.0 * std::numbers::pi_v<long double>); }

        constexpr si_angle_t operator ""_rad(long double __v)   { return (si_angle_t)__v; }
        constexpr si_angle_t operator ""_deg(long double __v)   { return (si_angle_t)(__v / 180.0 * std::numbers::pi_v<long double>); }

        /** Literal for signed int8_t */
        constexpr int8_t operator ""_i8(unsigned long long int __v)   { return (int8_t)__v; }

        /** Literal for unsigned uint8_t */
        constexpr uint8_t operator ""_u8(unsigned long long int __v)  { return (uint8_t)__v; }

        /** Literal for signed int16_t */
        constexpr int16_t operator ""_i16(unsigned long long int __v)   { return (int16_t)__v; }

        /** Literal for unsigned uint16_t */
        constexpr uint16_t operator ""_u16(unsigned long long int __v)  { return (uint16_t)__v; }

        /** Literal for signed int32_t */
        constexpr int32_t operator ""_i32(unsigned long long int __v)   { return (int32_t)__v; }

        /** Literal for unsigned uint32_t */
        constexpr uint32_t operator ""_u32(unsigned long long int __v)  { return (uint32_t)__v; }

        /** Literal for signed int64_t */
        constexpr int64_t operator ""_i64(unsigned long long int __v)   { return (int64_t)__v; }

        /** Literal for unsigned uint64_t */
        constexpr uint64_t operator ""_u64(unsigned long long int __v)  { return (uint64_t)__v; }

        /** Literal for signed ssize_t */
        constexpr ssize_t operator ""_iz(unsigned long long int __v)  { return (ssize_t)__v; }

        /** Literal for unsigned size_t */
        constexpr size_t operator ""_uz(unsigned long long int __v)  { return (size_t)__v; }
    }

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

#endif /* INCLUDE_PIXEL_UNIT_HPP_ */
