/*
 * Author: Sven Gothel <sgothel@jausoft.com>
 * Copyright (c) 2021-2024 Gothel Software e.K.
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

#ifndef JAU_INT_TYPES_HPP_
#define JAU_INT_TYPES_HPP_

#include <sys/types.h>
#include <cstdint>
#include <cstring>

namespace jau {

    namespace int_literals {
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

    /**@}*/

} // namespace jau

#endif /* JAU_INT_TYPES_HPP_ */
