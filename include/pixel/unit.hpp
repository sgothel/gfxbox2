/*
 * unit.hpp
 *
 *  Created on: Feb 13, 2024
 *      Author: svenson
 */

#ifndef INCLUDE_PIXEL_UNIT_HPP_
#define INCLUDE_PIXEL_UNIT_HPP_

namespace pixel {

    /** Time in fractions of seconds. */
    typedef float si_time_t;
    /** Length in fractions of meter. */
    typedef float si_length_t;
    /** Mass in fractions of kilograms. */
    typedef float si_mass_t;
    /** Speed in fractions of meter/seconds. */
    typedef float si_speed_t;

    constexpr si_time_t operator ""_h(unsigned long long int __v)   { return (si_time_t)__v*3600.0; }
    constexpr si_time_t operator ""_min(unsigned long long int __v)   { return (si_time_t)__v*60.0; }
    constexpr si_time_t operator ""_s(unsigned long long int __v)   { return (si_time_t)__v; }
    constexpr si_time_t operator ""_ms(unsigned long long int __v)   { return (si_time_t)__v/1000.0; }

    constexpr si_length_t operator ""_km(unsigned long long int __v)   { return (si_length_t)__v*1000.0; }
    constexpr si_length_t operator ""_m(unsigned long long int __v)   { return (si_length_t)__v; }
    constexpr si_length_t operator ""_cm(unsigned long long int __v)   { return (si_length_t)__v/100.0; }
    constexpr si_length_t operator ""_mm(unsigned long long int __v)   { return (si_length_t)__v/1000.0; }

    constexpr si_mass_t operator ""_kg(unsigned long long int __v)   { return (si_mass_t)__v; }
    constexpr si_mass_t operator ""_g(unsigned long long int __v)   { return (si_mass_t)__v/1000.0; }

    constexpr si_mass_t operator ""_m_s(unsigned long long int __v)   { return (si_mass_t)__v; }
    constexpr si_mass_t operator ""_km_h(unsigned long long int __v)   { return (si_mass_t)__v / 3.6; }

}

#endif /* INCLUDE_PIXEL_UNIT_HPP_ */
