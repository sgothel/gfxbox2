/*
 * Author: Hermann Kai Gothel <hkg@jausoft.com>
 * Copyright (c) 2026 Gothel Software e.K.
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

#ifndef DYNARRAY2D_HPP_
#define DYNARRAY2D_HPP_

#include <cstdlib>
#include <type_traits>

template<typename T>
requires std::is_trivially_copyable_v<T>
class DynArray2D {
  private:
    T *m_mem;
    size_t m_columns;
    size_t m_rows;

  public:
    constexpr DynArray2D() noexcept : m_mem(nullptr), m_columns(0), m_rows(0) {}
    ~DynArray2D() noexcept {
        ::free(m_mem);
        m_mem = nullptr;
    }
    constexpr DynArray2D(DynArray2D && o) noexcept
    : m_mem(o.m_mem), m_columns(o.m_columns), m_rows(o.m_rows)
    {
        o.m_mem = nullptr;
        o.m_columns = 0;
        o.m_rows = 0;
    }
    DynArray2D& operator=(DynArray2D && o) noexcept
    {
        ::free(m_mem);
        m_mem = o.m_mem;
        m_columns = o.m_columns;
        m_rows = o.m_rows;

        o.m_mem = nullptr;
        o.m_columns = 0;
        o.m_rows = 0;
        return *this;
    }

    constexpr DynArray2D(size_t columns, size_t rows)
    : m_mem((T*)::malloc(columns*rows*sizeof(T))), m_columns(columns), m_rows(rows) {}

    void clear(const T &v) noexcept {
        for(size_t y = 0; y < m_rows; ++y) {
            for(size_t x = 0; x < m_columns; ++x) {
                operator[](x, y) = v;
            }
        }
    }
    constexpr size_t columns() noexcept { return m_columns; };
    constexpr size_t rows() noexcept { return m_rows; };
    constexpr const T operator[](size_t x, size_t y) const noexcept {
        return m_mem[x + y * m_columns];
    };
    constexpr T& operator[](size_t x, size_t y) noexcept {
        return m_mem[x + y * m_columns];
    };
};

#endif // #define DYNARRAY2D_HPP_
