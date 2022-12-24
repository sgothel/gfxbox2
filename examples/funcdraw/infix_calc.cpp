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

static const char *EvalStatusString[] = {
             "No Error",
         /*-----------------------*/
             "Too complex",
             "Unresolved variables",
             "Division by zero",
             "Overflow",
             "Undefined",
             "RPN Underflow",
         /*-----------------------*/
};

std::string to_string(RPNStatus status) noexcept {
    return std::string(EvalStatusString[static_cast<unsigned int>(status)]);
}

std::string to_string(const rpn_token_t ts) noexcept {
    switch(ts) {
        case rpn_token_t::UREAL:
            return "<ureal>";
        case rpn_token_t::VARIABLE:
            return "<var>";
        case rpn_token_t::SUB:
            return "-";
        case rpn_token_t::ADD:
            return "+";
        case rpn_token_t::MUL:
            return "*";
        case rpn_token_t::DIV:
            return "/";
        case rpn_token_t::MOD:
            return "%";
        case rpn_token_t::ABS:
            return "abs";
        case rpn_token_t::SIN:
            return "sin";
        case rpn_token_t::COS:
            return "cos";
        case rpn_token_t::TAN:
            return "tan";
        case rpn_token_t::ARCSIN:
            return "asin";
        case rpn_token_t::ARCCOS:
            return "acos";
        case rpn_token_t::ARCTAN:
            return "atan";
        case rpn_token_t::POW:
            return "pow";
        case rpn_token_t::LOG:
            return "ln";
        case rpn_token_t::LOG10:
            return "log";
        case rpn_token_t::EXP:
            return "exp";
        case rpn_token_t::SQRT:
            return "sqrt";
        case rpn_token_t::NEG:
            return "neg";
    }
    return "unknown";
}

std::string to_string(const rpn_token& v) noexcept {
    switch(v.ts) {
        case rpn_token_t::UREAL:
            return std::to_string(v.value);
        case rpn_token_t::VARIABLE:
            return "'"+v.id+"'";
        default:
            return to_string(v.ts);
    }
    return "unknown";
}

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
        fin = ::stdin;
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
    if( nullptr != fin && ::stdin != fin ) {
        ::fclose(fin);
    }
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

void compiler::put_rpn(rpn_token_t ts) {
    rpn_stack.push_back( { ts, 0.0, "" } );
}

void compiler::put_rpn(double value) {
    rpn_stack.push_back( { rpn_token_t::UREAL, value, "" } );
}

void compiler::put_rpn(const std::string& variable_name) {
    rpn_stack.push_back( { rpn_token_t::VARIABLE, 0.0, variable_name } );
}

void compiler::put_rpn(std::string&& variable_name) {
    rpn_stack.push_back( { rpn_token_t::VARIABLE, 0.0, std::move(variable_name) } );
}

std::string to_string(const std::vector<rpn_token>& rpn_stack) noexcept {
    std::string r;
    for(const rpn_token& t : rpn_stack) {
        r.append(to_string(t));
        if( t.ts == rpn_token_t::UREAL || t.ts == rpn_token_t::VARIABLE ) {
            r.append(", ");
        } else {
            r.append("; ");
        }
    }
    return r;
}

std::string to_string(const variable_set& variables) noexcept {
    std::string r;
    for (const auto& [key, value] : variables) {
        r.append(key).append(" = ").append(std::to_string(value)).append(", ");
    }
    return r;
}

RPNStatus reduce(std::vector<rpn_token>& res, const std::vector<rpn_token>& source) noexcept {
    res.clear();
    RPNStatus status = RPNStatus::No_Error;
    double left_operand, right_operand;

    auto check_stack_cntr = [&](size_t min) noexcept -> bool {
        if( res.size() < min ) {
            status = RPNStatus::RPN_Underflow;
            return false;
        } else {
            return true;
        }
    };
    auto get_stack_ureal = [&](size_t count) noexcept -> bool {
        const size_t sz = res.size();
        if( 0 == sz ) {
            return 0 == count;
        }
        const size_t hi = sz - 1;
        for(size_t i=0; i<count; ++i) {
            if( hi < i || res[hi-i].ts != rpn_token_t::UREAL ) {
                return false;
            }
        }
        if( count >= 1 ) {
            right_operand  = res.back().value;
            res.pop_back();
        }
        if( count >= 2 ) {
            left_operand = res.back().value;
            res.pop_back();
        }
        return true;
    };

    for (size_t i=0; i<source.size() && status==RPNStatus::No_Error; i++)
    {
        const rpn_token& t = source[i];
        switch (t.ts)
        {
            case rpn_token_t::UREAL:
                res.push_back(t);
                break;

            case rpn_token_t::VARIABLE:
                res.push_back(t);
                break;

            case rpn_token_t::ADD:
                if( !check_stack_cntr(2) ) { break; }
                if( get_stack_ureal(2) ) {
                    if ( ( ( left_operand > 0 && right_operand > 0 )
                           ||
                           ( left_operand < 0 && right_operand < 0 )
                         ) &&
                         std::abs(left_operand) >=
                             std::numeric_limits<double>::max() - std::abs(right_operand)
                       )
                    {
                        status = RPNStatus::Overflow;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, left_operand + right_operand, "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::SUB:
                if( !check_stack_cntr(2) ) { break; }
                if( get_stack_ureal(2) ) {
                    if ( ( ( left_operand > 0 && right_operand < 0 )
                           ||
                           ( left_operand < 0 && right_operand > 0 )
                         ) &&
                         std::abs(left_operand) >=
                                 std::numeric_limits<double>::max() - std::abs(right_operand)
                       )
                    {
                        status =RPNStatus::Overflow;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, left_operand - right_operand, "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::MUL:
                if( !check_stack_cntr(2) ) { break; }
                if( get_stack_ureal(2) ) {
                    if( std::abs(right_operand) > 1 &&
                        std::abs(left_operand) >=
                            std::numeric_limits<double>::max() / std::abs(right_operand) )
                    {
                        status =RPNStatus::Overflow;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, left_operand * right_operand, "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::DIV:
                if( !check_stack_cntr(2) ) { break; }
                if( get_stack_ureal(2) ) {
                    if (right_operand == 0)
                    {
                        status = RPNStatus::Division_by_zero;
                        break;
                    }
                    if( std::abs(right_operand) < 1 &&
                        std::abs(left_operand) >=
                            std::numeric_limits<double>::max() * std::abs(right_operand) )
                    {
                        status = RPNStatus::Overflow;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, left_operand / right_operand, "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::MOD:
                if( !check_stack_cntr(2) ) { break; }
                if( get_stack_ureal(2) ) {
                    if (right_operand == 0)
                    {
                        status = RPNStatus::Division_by_zero;
                        break;
                    }
                    if( std::abs(right_operand) < 1 &&
                        std::abs(left_operand) >=
                            std::numeric_limits<double>::max() * std::abs(right_operand) )
                    {
                        status = RPNStatus::Overflow;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, std::fmod(left_operand, right_operand), "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::POW:
                if( !check_stack_cntr(2) ) { break; }
                if( get_stack_ureal(2) ) {
                    res.push_back( { rpn_token_t::UREAL, std::pow(left_operand, right_operand), "" } );
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::SQRT:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    if (right_operand<0)
                    {
                        status=RPNStatus::Undefined;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, std::sqrt(right_operand), "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::LOG:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    if (right_operand<=0)
                    {
                        status=RPNStatus::Undefined;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, std::log(right_operand), "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::LOG10:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    if (right_operand<=0)
                    {
                        status=RPNStatus::Undefined;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, std::log10(right_operand), "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::EXP:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    res.push_back( { rpn_token_t::UREAL, std::exp(right_operand), "" } );
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::ABS:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    res.push_back( { rpn_token_t::UREAL, std::abs(right_operand), "" } );
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::SIN:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    res.push_back( { rpn_token_t::UREAL, std::sin(right_operand), "" } );
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::COS:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    res.push_back( { rpn_token_t::UREAL, std::cos(right_operand), "" } );
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::TAN:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    res.push_back( { rpn_token_t::UREAL, std::tan(right_operand), "" } );
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::ARCSIN:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    if (std::abs(right_operand)>1)
                    {
                        status=RPNStatus::Undefined;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, std::asin(right_operand), "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::ARCCOS:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    if (std::abs(right_operand)>1)
                    {
                        status=RPNStatus::Undefined;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, std::acos(right_operand), "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::ARCTAN:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    res.push_back( { rpn_token_t::UREAL, std::atan(right_operand), "" } );
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::NEG:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    res.push_back( { rpn_token_t::UREAL, 0 - right_operand, "" } );
                } else {
                    res.push_back(t);
                }
                break;

            default :
                break;
        }
    }

    return status;
}

bool resolved(const variable_set& variables, const std::vector<rpn_token>& rpn_stack) noexcept {
    for( const rpn_token& t : rpn_stack ) {
        switch (t.ts)
        {
            case rpn_token_t::VARIABLE:
                {
                    if (auto var_iter = variables.find(t.id); var_iter == variables.end()) {
                        // variable t.id undefined!
                        fprintf(stderr, "variable '%s' undefined\n", t.id.c_str());
                        return false;
                    }
                }
                break;

            default :
                break;
        }
    }
    return true;
}

RPNStatus eval(double& result, const variable_set& variables, const std::vector<rpn_token>& rpn_stack) noexcept {
    RPNStatus status = RPNStatus::No_Error;
    double left_operand, right_operand;

    double stack[rpn_stack.size()];
    size_t stack_cntr=0;
    stack[0] = 0.0;

    auto get_stack = [&](size_t count) noexcept -> bool {
        if( stack_cntr < count ) {
            status = RPNStatus::RPN_Underflow;
            return false;
        }
        if( count >= 1 ) {
            right_operand = stack[--stack_cntr];
        }
        if( count >= 2 ) {
            left_operand  = stack[--stack_cntr];
        }
        return true;
    };

    for (size_t i=0; i<rpn_stack.size() && status==RPNStatus::No_Error; i++)
    {
        const rpn_token& t = rpn_stack[i];
        switch (t.ts)
        {
            case rpn_token_t::UREAL:
                stack[stack_cntr++] = t.value;
                break;

            case rpn_token_t::VARIABLE:
                {
                    if (auto var_iter = variables.find(t.id); var_iter != variables.end()) {
                        stack[stack_cntr++] = var_iter->second;
                    } else {
                        // variable t.id undefined!
                        fprintf(stderr, "variable '%s' undefined\n", t.id.c_str());
                        status = RPNStatus::Unresolved_Variables; // FIXME: Refine!
                    }
                }
                break;

            case rpn_token_t::ADD:
                if( !get_stack(2) ) { break; }
                if ( ( ( left_operand > 0 && right_operand > 0 )
                       ||
                       ( left_operand < 0 && right_operand < 0 )
                     ) &&
                     std::abs(left_operand) >=
                         std::numeric_limits<double>::max() - std::abs(right_operand)
                   )
                {
                    status = RPNStatus::Overflow;
                } else {
                    stack[stack_cntr++] = left_operand + right_operand;
                }
                break;

            case rpn_token_t::SUB:
                if( !get_stack(2) ) { break; }
                if ( ( ( left_operand > 0 && right_operand < 0 )
                       ||
                       ( left_operand < 0 && right_operand > 0 )
                     ) &&
                     std::abs(left_operand) >=
                             std::numeric_limits<double>::max() - std::abs(right_operand)
                   )
                {
                    status =RPNStatus::Overflow;
                } else {
                    stack[stack_cntr++] = left_operand - right_operand;
                }
                break;

            case rpn_token_t::MUL:
                if( !get_stack(2) ) { break; }
                if( std::abs(right_operand) > 1 &&
                    std::abs(left_operand) >=
                        std::numeric_limits<double>::max() / std::abs(right_operand) )
                {
                    status =RPNStatus::Overflow;
                } else {
                    stack[stack_cntr++] = left_operand * right_operand;
                }
                break;

            case rpn_token_t::DIV:
                if( !get_stack(2) ) { break; }
                if (right_operand == 0)
                {
                    status = RPNStatus::Division_by_zero;
                    break;
                }
                if( std::abs(right_operand) < 1 &&
                    std::abs(left_operand) >=
                        std::numeric_limits<double>::max() * std::abs(right_operand) )
                {
                    status = RPNStatus::Overflow;
                } else {
                    stack[stack_cntr++] = left_operand / right_operand;
                }
                break;

            case rpn_token_t::MOD:
                if( !get_stack(2) ) { break; }
                if (right_operand == 0)
                {
                    status = RPNStatus::Division_by_zero;
                    break;
                }
                if( std::abs(right_operand) < 1 &&
                    std::abs(left_operand) >=
                        std::numeric_limits<double>::max() * std::abs(right_operand) )
                {
                    status = RPNStatus::Overflow;
                } else {
                    stack[stack_cntr++] = std::fmod(left_operand, right_operand);
                }
                break;

            case rpn_token_t::POW:
                if( !get_stack(2) ) { break; }
                stack[stack_cntr++] = std::pow(left_operand,right_operand);
                break;

            case rpn_token_t::SQRT:
                if( !get_stack(1) ) { break; }
                if (right_operand<0)
                {
                    status=RPNStatus::Undefined;
                } else {
                    stack[stack_cntr++] = std::sqrt (right_operand);
                }
                break;

            case rpn_token_t::LOG:
                if( !get_stack(1) ) { break; }
                if (right_operand<=0)
                {
                    status=RPNStatus::Undefined;
                } else {
                    stack[stack_cntr++] = std::log (right_operand);
                }
                break;

            case rpn_token_t::LOG10:
                if( !get_stack(1) ) { break; }
                if (right_operand<=0)
                {
                    status=RPNStatus::Undefined;
                } else {
                    stack[stack_cntr++] = std::log10(right_operand);
                }
                break;

            case rpn_token_t::EXP:
                if( !get_stack(1) ) { break; }
                stack[stack_cntr++] = std::exp(right_operand);
                break;

            case rpn_token_t::ABS:
                if( !get_stack(1) ) { break; }
                stack[stack_cntr++] = std::abs(right_operand);
                break;

            case rpn_token_t::SIN:
                if( !get_stack(1) ) { break; }
                stack[stack_cntr++] = std::sin(right_operand);
                break;

            case rpn_token_t::COS:
                if( !get_stack(1) ) { break; }
                stack[stack_cntr++] = std::cos(right_operand);
                break;

            case rpn_token_t::TAN:
                if( !get_stack(1) ) { break; }
                stack[stack_cntr++] = std::tan(right_operand);
                break;

            case rpn_token_t::ARCSIN:
                if( !get_stack(1) ) { break; }
                if (std::abs(right_operand)>1)
                {
                    status=RPNStatus::Undefined;
                } else {
                    stack[stack_cntr++] = std::asin (right_operand);
                }
                break;

            case rpn_token_t::ARCCOS:
                if( !get_stack(1) ) { break; }
                if (std::abs(right_operand)>1)
                {
                    status=RPNStatus::Undefined;
                } else {
                    stack[stack_cntr++] = std::acos (right_operand);
                }
                break;

            case rpn_token_t::ARCTAN:
                if( !get_stack(1) ) { break; }
                stack[stack_cntr++] = std::atan (right_operand);
                break;

            case rpn_token_t::NEG:
                if( !get_stack(1) ) { break; }
                stack[stack_cntr++] = 0 - right_operand;
                break;

            default :
                break;
        }
    }

    if( status == RPNStatus::No_Error ) {
        result = stack[0];
    }
    return (status);
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

std::ostream& std::operator<<(std::ostream& os, const infix_calc::rpn_token_t ts) {
    os << infix_calc::to_string(ts);
    return os;
}

std::ostream& std::operator<<(std::ostream& os, const infix_calc::rpn_token& rpn) {
    os << infix_calc::to_string(rpn);
    return os;
}
