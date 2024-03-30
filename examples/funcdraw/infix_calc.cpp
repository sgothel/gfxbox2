/*
 * Author: Sven Gothel <sgothel@jausoft.com>
 * Copyright (c) 1993-2023 Gothel Software e.K.
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

#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <functional>
#include <fstream>
#include <ostream>

#include "infix_calc.hpp"
#include "infix_calc_parser.hpp"
#if defined(SCANNER_REFLEX)
    #include "infix_calc_scanner2.hpp"
#elif defined(SCANNER_FLEX)
    #include "infix_calc_scanner1.hpp"
    infix_calc_yy::parser::symbol_type infix_calc_yylex(infix_calc_yy::location& loc, yyscan_t yyscanner);
#else
    #error Either FLEX or REFLEX should be used
#endif

namespace infix_calc {

struct backend_t {
#if defined(SCANNER_REFLEX)
    infix_calc_yy::scanner scan;
#elif defined(SCANNER_FLEX)
    yyscan_t scan;
    YY_BUFFER_STATE buffer;
#endif
    infix_calc_yy::location loc;

    backend_t() :
#if defined(SCANNER_REFLEX)
    scan(),
#elif defined(SCANNER_FLEX)
    scan(nullptr),
    buffer(nullptr),
#endif
    loc()
    { }
};

compiler::compiler() {
    backend = reinterpret_cast<void*>(new backend_t());
}

compiler::~compiler() {
    backend_t* p = reinterpret_cast<backend_t*>(backend);
    delete p;
}

infix_calc_yy::location& compiler::location() {
    return reinterpret_cast<backend_t*>(backend)->loc;
}

bool compiler::scan_begin(const std::string& fname)
{
    FILE* fin = nullptr;
    if (fname.empty () || fname == "-") {
        #if defined(__EMSCRIPTEN__)
            fin = nullptr; // FIXME
        #else
            fin = ::stdin;
        #endif
    } else {
        fin = ::fopen(fname.c_str(), "r");
    }
    if( nullptr == fin ) {
        std::cerr << "cannot open " << fname << ": " << ::strerror(errno) << std::endl;
        return false;
    }
#if defined(SCANNER_REFLEX)
    infix_calc_yy::scanner& scan = reinterpret_cast<backend_t*>(backend)->scan;
    scan.switch_streams(fin);
#elif defined(SCANNER_FLEX)
    yyscan_t& scan = reinterpret_cast<backend_t*>(backend)->scan;
    infix_calc_yylex_init(&scan);
    infix_calc_yyset_in(fin, scan);
#endif
    return true;
}

bool compiler::scan_begin(const int fd)
{
    FILE* fin = nullptr;
    if( 0 > fd ) {
        std::cerr << "invalid fd " << std::to_string(fd) << std::endl;
        return false;
    }
    fin = ::fdopen(fd, "r");
    if( nullptr == fin ) {
        std::cerr << "cannot open fd " << std::to_string(fd) << ": " << ::strerror(errno) << std::endl;
        return false;
    }
#if defined(SCANNER_REFLEX)
    infix_calc_yy::scanner& scan = reinterpret_cast<backend_t*>(backend)->scan;
    scan.switch_streams(fin);
#elif defined(SCANNER_FLEX)
    yyscan_t& scan = reinterpret_cast<backend_t*>(backend)->scan;
    infix_calc_yylex_init(&scan);
    infix_calc_yyset_in(fin, scan);
#endif
    return true;
}

bool compiler::scan_begin(const char* bytes, int len) {
#if defined(SCANNER_REFLEX)
    infix_calc_yy::scanner& scan = reinterpret_cast<backend_t*>(backend)->scan;
    scan.switch_streams(reflex::Input(bytes, len));
#elif defined(SCANNER_FLEX)
    yyscan_t& scan = reinterpret_cast<backend_t*>(backend)->scan;
    infix_calc_yylex_init(&scan);
    infix_calc_yyset_debug(99, scan);
    reinterpret_cast<backend_t*>(backend)->buffer = infix_calc_yy_scan_bytes(bytes, len, scan);
#endif
    return true;
}

void compiler::scan_end()
{
#if defined(SCANNER_REFLEX)
    infix_calc_yy::scanner& scan = reinterpret_cast<backend_t*>(backend)->scan;
    scan.switch_streams(reflex::Input());
#elif defined(SCANNER_FLEX)
    yyscan_t& scan = reinterpret_cast<backend_t*>(backend)->scan;
    FILE* fin = infix_calc_yyget_in(scan);
    #if defined(__EMSCRIPTEN__)
        if( nullptr != fin ) {
            ::fclose(fin);
        }
    #else
        if( nullptr != fin && ::stdin != fin ) {
            ::fclose(fin);
        }
    #endif
    if( nullptr != reinterpret_cast<backend_t*>(backend)->buffer ) {
        infix_calc_yy_delete_buffer( reinterpret_cast<backend_t*>(backend)->buffer, scan );
        reinterpret_cast<backend_t*>(backend)->buffer = nullptr;
    }
    infix_calc_yylex_destroy(scan);
    scan = 0;
#endif
}

bool compiler::parse (const std::string &fname)
{
  typedef std::function<infix_calc_yy::parser::symbol_type(infix_calc::compiler&)> yylex_func_t;
  yylex_func_t yylex_func = [&](compiler& cc) -> infix_calc_yy::parser::symbol_type {
#if defined(SCANNER_REFLEX)
      infix_calc_yy::scanner& scan = reinterpret_cast<backend_t*>(cc.backend)->scan;
      return scan.yylex(cc.location());
#elif defined(SCANNER_FLEX)
      yyscan_t& scan = reinterpret_cast<backend_t*>(cc.backend)->scan;
      return infix_calc_yylex(cc.location(), scan);
#endif
  };
  location().initialize (&fname);
  if( !scan_begin(fname) ) {
      return false;
  }
  infix_calc_yy::parser parse(yylex_func, *this);
  int res = parse ();
  scan_end();
  return 0 == res;
}

bool compiler::parse (const int fd)
{
  typedef std::function<infix_calc_yy::parser::symbol_type(infix_calc::compiler&)> yylex_func_t;
  yylex_func_t yylex_func = [&](compiler& cc) -> infix_calc_yy::parser::symbol_type {
#if defined(SCANNER_REFLEX)
      infix_calc_yy::scanner& scan = reinterpret_cast<backend_t*>(cc.backend)->scan;
      return scan.yylex(cc.location());
#elif defined(SCANNER_FLEX)
      yyscan_t& scan = reinterpret_cast<backend_t*>(cc.backend)->scan;
      return infix_calc_yylex(cc.location(), scan);
#endif
  };
  if( !scan_begin(fd) ) {
      return false;
  }
  infix_calc_yy::parser parse(yylex_func, *this);
  int res = parse ();
  scan_end();
  return 0 == res;
}

bool compiler::parse (const char* bytes, int len) {
    typedef std::function<infix_calc_yy::parser::symbol_type(infix_calc::compiler&)> yylex_func_t;
    yylex_func_t yylex_func = [&](compiler& cc) -> infix_calc_yy::parser::symbol_type {
  #if defined(SCANNER_REFLEX)
        infix_calc_yy::scanner& scan = reinterpret_cast<backend_t*>(cc.backend)->scan;
        return scan.yylex(cc.location());
  #elif defined(SCANNER_FLEX)
        yyscan_t& scan = reinterpret_cast<backend_t*>(cc.backend)->scan;
        return infix_calc_yylex(cc.location(), scan);
  #endif
    };
    if( !scan_begin(bytes, len) ) {
        return false;
    }
    infix_calc_yy::parser parse(yylex_func, *this);
    // parse.set_debug_level(99);
    int res = parse ();
    scan_end();
    return 0 == res;
}

static infix_calc_yy::location eof_loc;

infix_calc_yy::parser::symbol_type make_YYEOF() {
    return infix_calc_yy::parser::make_YYEOF(eof_loc);
}

infix_calc_yy::parser::symbol_type make_UREAL(const std::string &s, double f, const infix_calc_yy::parser::location_type& loc)
{
  errno = 0;
  double r = ::strtod(s.c_str(), nullptr);
  if( ERANGE == errno ) {
    return infix_calc_yy::parser::make_ERROR("integer is out of range: " + s, loc);
  } else {
    return infix_calc_yy::parser::make_UREAL(r*f, loc);
  }
}

} // namespace infix_calc

std::ostream& std::operator<<(std::ostream& os, const infix_calc_yy::location& loc) {
    return infix_calc_yy::operator <<(os, loc);
}
