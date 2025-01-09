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
#ifndef JAU_FILE_UTIL_HPP_
#define JAU_FILE_UTIL_HPP_

#include <unistd.h>
#include <string>

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cctype>

namespace jau::fs {
    /**
     * Return the current working directory or empty on failure.
     */
    std::string get_cwd() noexcept;
    /**
     * Returns the absolute path of given `relpath` if existing,
     * otherwise an empty string.
     * @param relpath a path, might be relative
     */
    std::string absolute(const std::string_view& relpath) noexcept;

    /**
     * Return stripped last component from given path separated by `/`, excluding the trailing separator `/`.
     *
     * If no directory separator `/` is contained, return `.`.
     *
     * If only the root path `/` is given, return `/`.
     *
     * @param path given path
     * @return leading directory name w/o slash or `.`
     */
    std::string dirname(const std::string_view& path) noexcept;

    /**
     * Return stripped leading directory components from given path separated by `/`.
     *
     * If only the root path `/` is given, return `/`.
     *
     * @param path given path
     * @return last non-slash component or `.`
     */
    std::string basename(const std::string_view& path) noexcept;

    /** Returns true if first character is `/` or - in case of Windows - `\\`. */
    bool isAbsolute(const std::string_view& path) noexcept;

    /** Returns true if path exists _and_ is accessible. */
    bool exists(const std::string& path) noexcept;

    std::string lookup_asset_dir(const char* exe_path, const char* asset_file, const char* asset_install_subdir) noexcept;
}

#endif /*  JAU_FILE_UTIL_HPP_ */

