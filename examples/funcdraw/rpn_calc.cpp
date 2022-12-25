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

#include "rpn_calc.hpp"

namespace rpn_calc {

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

std::string to_string(const rpn_stack_t& rpn_expr) noexcept {
    std::string r;
    for(const rpn_token& t : rpn_expr) {
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

RPNStatus reduce(rpn_stack_t& res, const rpn_stack_t& source) noexcept {
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

bool rpn_expression_t::resolved(const variable_set& variables) const noexcept {
    for( const rpn_token& t : expr ) {
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

RPNStatus rpn_expression_t::eval(double& result, const variable_set& variables) const noexcept {
    RPNStatus status = RPNStatus::No_Error;
    double left_operand, right_operand;

    double stack[expr.size()];
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

    for (size_t i=0; i<expr.size() && status==RPNStatus::No_Error; i++)
    {
        const rpn_token& t = expr[i];
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

} // namespace rpn_calc

std::ostream& std::operator<<(std::ostream& os, const rpn_calc::rpn_token_t ts) {
    os << rpn_calc::to_string(ts);
    return os;
}

std::ostream& std::operator<<(std::ostream& os, const rpn_calc::rpn_token& rpn) {
    os << rpn_calc::to_string(rpn);
    return os;
}
