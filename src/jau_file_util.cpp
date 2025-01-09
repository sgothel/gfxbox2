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
#include "jau/file_util.hpp"
#include <strings.h>
#include <cerrno>
#include <cstring>

extern "C" {
    #include <unistd.h>
    #include <dirent.h>
    #include <fcntl.h>
    #include <sys/stat.h>
    // #include <sys/types.h>
    // #include <sys/wait.h>
    // #include <sys/mount.h>
}

std::string jau::fs::get_cwd() noexcept {
    const size_t bsz = PATH_MAX; // including EOS
    std::string str;
    str.reserve(bsz);  // incl. EOS
    str.resize(bsz-1); // excl. EOS

    char* res = ::getcwd(&str[0], bsz);
    if( res == &str[0] ) {
        str.resize(::strnlen(res, bsz));
        str.shrink_to_fit();
        return str;
    } else {
        return std::string();
    }
}

std::string jau::fs::absolute(const std::string_view& relpath) noexcept {
    const size_t bsz = PATH_MAX; // including EOS
    std::string str;
    str.reserve(bsz);  // incl. EOS
    str.resize(bsz-1); // excl. EOS

    char *res = ::realpath(&relpath[0], &str[0]);
    if( res == &str[0] ) {
        str.resize(::strnlen(res, bsz));
        str.shrink_to_fit();
        return str;
    } else {
        return std::string();
    }
}

static const char c_slash('/');
// static const char c_backslash('\\');
// static const std::string s_slash("/");
// static const std::string s_slash_dot_slash("/./");
// static const std::string s_slash_dot("/.");
// static const std::string s_dot_slash("./");
static const std::string s_dot(".");
// static const std::string s_slash_dotdot_slash("/../");
// static const std::string s_slash_dotdot("/..");
// static const std::string s_dotdot("..");

std::string jau::fs::dirname(const std::string_view& path) noexcept {
    if( 0 == path.size() ) {
        return s_dot;
    }
    size_t end_pos;
    if( c_slash == path[path.size()-1] ) {
        if( 1 == path.size() ) { // maintain a single '/'
            return std::string(path);
        }
        end_pos = path.size()-2;
    } else {
        end_pos = path.size()-1;
    }
    size_t idx = path.find_last_of(c_slash, end_pos);
    if( idx == std::string_view::npos ) {
        return s_dot;
    } else {
        // ensure `/lala` -> '/', i.e. don't cut off single '/'
        return std::string( path.substr(0, std::max<size_t>(1, idx)) );
    }
}

std::string jau::fs::basename(const std::string_view& path) noexcept {
    if( 0 == path.size() ) {
        return s_dot;
    }
    size_t end_pos;
    if( c_slash == path[path.size()-1] ) {
        if( 1 == path.size() ) { // maintain a single '/'
            return std::string(path);
        }
        end_pos = path.size()-2;
    } else {
        end_pos = path.size()-1;
    }
    size_t idx = path.find_last_of(c_slash, end_pos);
    if( idx == std::string_view::npos ) {
        return std::string( path.substr(0, end_pos+1));
    } else {
        return std::string( path.substr(idx+1, end_pos-idx) );
    }
}

bool jau::fs::isAbsolute(const std::string_view& path) noexcept {
    return path.size() > 0 && c_slash == path[0];
}

#if defined(__FreeBSD__)
    typedef struct ::stat struct_stat64;
    typedef off_t off64_t;
    #define __posix_fstatat64 ::fstatat
    #define __posix_openat64  ::openat
#else
    typedef struct ::stat64 struct_stat64;
    #define __posix_fstatat64 ::fstatat64
    #define __posix_openat64  ::openat64
#endif

bool jau::fs::exists(const std::string& path) noexcept {
    struct_stat64 s;
    ::memset(&s, 0, sizeof(s));
    errno = 0;
    int stat_res = __posix_fstatat64(AT_FDCWD, path.c_str(), &s, 0); // lstat64 compatible
    if( 0 != stat_res ) {
        fprintf(stderr, "exists '%s': %d: %d %s\n", path.c_str(), stat_res, errno, strerror(errno));
    }
    return 0 == stat_res; // errno EACCES=no access, ENOENT=not existing
}

std::string jau::fs::lookup_asset_dir(const char* exe_path, const char* asset_file, const char* asset_install_subdir) noexcept {
    if( !asset_file ) {
        return "";
    }
    // cwd in source root of developer directory - or - WASM (emscripten)
    std::string assetdir0 = "resources";
    if( exists( assetdir0+"/"+asset_file ) ) {
        return assetdir0;
    }
    if( !exe_path || !asset_install_subdir ) {
        return "";
    }
    // launched from installed package
    std::string exedir = jau::fs::dirname(exe_path);
    std::string cwd = jau::fs::get_cwd();
    std::string adir = jau::fs::isAbsolute(exedir) ? exedir : cwd+"/"+exedir;
    std::string assetdir1 = jau::fs::absolute( adir+"/../share/"+asset_install_subdir );
    if( exists( assetdir1+"/"+asset_file ) ) {
        return assetdir1;
    }
    return "";
}
