/*
 * unit.hpp
 *
 *  Created on: Feb 13, 2024
 *      Author: svenson
 */

#ifndef INCLUDE_PIXEL_UNIT_HPP_
#define INCLUDE_PIXEL_UNIT_HPP_

#include <numbers>

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
        constexpr si_velo_t operator ""_m_h(unsigned long long int __v)   { return (si_velo_t)__v / 3600.0; }
    
        constexpr si_accel_t operator ""_km_s2(unsigned long long int __v)   { return (si_accel_t)__v * 1000.0f; }
        constexpr si_accel_t operator ""_m_s2(unsigned long long int __v)   { return (si_accel_t)__v; }
        constexpr si_accel_t operator ""_dm_s2(unsigned long long int __v)   { return (si_accel_t)__v / 10.0f; }
        constexpr si_accel_t operator ""_cm_s2(unsigned long long int __v)   { return (si_accel_t)__v / 100.0f; }
        constexpr si_accel_t operator ""_mm_s2(unsigned long long int __v)   { return (si_accel_t)__v / 1000.0f; }
    
        constexpr si_angle_t operator ""_rad(unsigned long long int __v)   { return (si_angle_t)__v; }
        constexpr si_angle_t operator ""_deg(unsigned long long int __v)   { return (si_angle_t)((long double)__v / 180.0 * std::numbers::pi_v<long double>); }
        
        constexpr si_angle_t operator ""_rad(long double __v)   { return (si_angle_t)__v; }
        constexpr si_angle_t operator ""_deg(long double __v)   { return (si_angle_t)(__v / 180.0 * std::numbers::pi_v<long double>); }
    }
}

#endif /* INCLUDE_PIXEL_UNIT_HPP_ */
