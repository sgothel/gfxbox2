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
        case rpn_token_t::CEIL:
            return "ceil";
        case rpn_token_t::FLOOR:
            return "floor";
        case rpn_token_t::STEP:
            return "step";
        case rpn_token_t::MIX:
            return "mix";
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
    double lleft_op3, left_op2, right_op1;

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
        if( count == 3 ) {
            right_op1  = res.back().value;
            res.pop_back();
            left_op2 = res.back().value;
            res.pop_back();
            lleft_op3 = res.back().value;
            res.pop_back();
        } else if( count == 2 ) {
            right_op1  = res.back().value;
            res.pop_back();
            left_op2 = res.back().value;
            res.pop_back();
        } if( count == 1 ) {
            right_op1  = res.back().value;
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
                    if ( ( ( left_op2 > 0 && right_op1 > 0 )
                           ||
                           ( left_op2 < 0 && right_op1 < 0 )
                         ) &&
                         std::abs(left_op2) >=
                             std::numeric_limits<double>::max() - std::abs(right_op1)
                       )
                    {
                        status = RPNStatus::Overflow;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, left_op2 + right_op1, "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::SUB:
                if( !check_stack_cntr(2) ) { break; }
                if( get_stack_ureal(2) ) {
                    if ( ( ( left_op2 > 0 && right_op1 < 0 )
                           ||
                           ( left_op2 < 0 && right_op1 > 0 )
                         ) &&
                         std::abs(left_op2) >=
                                 std::numeric_limits<double>::max() - std::abs(right_op1)
                       )
                    {
                        status =RPNStatus::Overflow;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, left_op2 - right_op1, "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::MUL:
                if( !check_stack_cntr(2) ) { break; }
                if( get_stack_ureal(2) ) {
                    if( std::abs(right_op1) > 1 &&
                        std::abs(left_op2) >=
                            std::numeric_limits<double>::max() / std::abs(right_op1) )
                    {
                        status =RPNStatus::Overflow;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, left_op2 * right_op1, "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::DIV:
                if( !check_stack_cntr(2) ) { break; }
                if( get_stack_ureal(2) ) {
                    if (right_op1 == 0)
                    {
                        status = RPNStatus::Division_by_zero;
                        break;
                    }
                    if( std::abs(right_op1) < 1 &&
                        std::abs(left_op2) >=
                            std::numeric_limits<double>::max() * std::abs(right_op1) )
                    {
                        status = RPNStatus::Overflow;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, left_op2 / right_op1, "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::MOD:
                if( !check_stack_cntr(2) ) { break; }
                if( get_stack_ureal(2) ) {
                    if (right_op1 == 0)
                    {
                        status = RPNStatus::Division_by_zero;
                        break;
                    }
                    if( std::abs(right_op1) < 1 &&
                        std::abs(left_op2) >=
                            std::numeric_limits<double>::max() * std::abs(right_op1) )
                    {
                        status = RPNStatus::Overflow;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, std::fmod(left_op2, right_op1), "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::POW:
                if( !check_stack_cntr(2) ) { break; }
                if( get_stack_ureal(2) ) {
                    res.push_back( { rpn_token_t::UREAL, std::pow(left_op2, right_op1), "" } );
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::SQRT:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    if (right_op1<0)
                    {
                        status=RPNStatus::Undefined;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, std::sqrt(right_op1), "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::LOG:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    if (right_op1<=0)
                    {
                        status=RPNStatus::Undefined;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, std::log(right_op1), "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::LOG10:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    if (right_op1<=0)
                    {
                        status=RPNStatus::Undefined;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, std::log10(right_op1), "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::EXP:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    res.push_back( { rpn_token_t::UREAL, std::exp(right_op1), "" } );
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::ABS:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    res.push_back( { rpn_token_t::UREAL, std::abs(right_op1), "" } );
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::SIN:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    res.push_back( { rpn_token_t::UREAL, std::sin(right_op1), "" } );
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::COS:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    res.push_back( { rpn_token_t::UREAL, std::cos(right_op1), "" } );
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::TAN:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    res.push_back( { rpn_token_t::UREAL, std::tan(right_op1), "" } );
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::ARCSIN:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    if (std::abs(right_op1)>1)
                    {
                        status=RPNStatus::Undefined;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, std::asin(right_op1), "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::ARCCOS:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    if (std::abs(right_op1)>1)
                    {
                        status=RPNStatus::Undefined;
                    } else {
                        res.push_back( { rpn_token_t::UREAL, std::acos(right_op1), "" } );
                    }
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::ARCTAN:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    res.push_back( { rpn_token_t::UREAL, std::atan(right_op1), "" } );
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::CEIL:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    res.push_back( { rpn_token_t::UREAL, std::ceil(right_op1), "" } );
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::FLOOR:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    res.push_back( { rpn_token_t::UREAL, std::floor(right_op1), "" } );
                } else {
                    res.push_back(t);
                }
                break;

            case rpn_token_t::STEP:
                if( !check_stack_cntr(2) ) { break; }
                res.push_back(t);
                break;

            case rpn_token_t::MIX:
                if( !check_stack_cntr(3) ) { break; }
                res.push_back(t);
                break;

            case rpn_token_t::NEG:
                if( !check_stack_cntr(1) ) { break; }
                if( get_stack_ureal(1) ) {
                    res.push_back( { rpn_token_t::UREAL, 0 - right_op1, "" } );
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

static double step(double edge, double x) noexcept {
    return x < edge ? 0 : 1;
}
static double mix(double x, double y, double a) noexcept {
    return x * ( 1 - a ) + y * a;
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
    double lleft_op3, left_op2, right_op1;

    std::vector<double> stack;
    stack.reserve(expr.size());

    auto get_stack = [&](size_t count) noexcept -> bool {
        if( stack.size() < count ) {
            status = RPNStatus::RPN_Underflow;
            return false;
        }
        if( count == 3 ) {
            right_op1 = stack.back(); stack.pop_back();
            left_op2  = stack.back(); stack.pop_back();
            lleft_op3  = stack.back(); stack.pop_back();
        } else if( count == 2 ) {
            right_op1 = stack.back(); stack.pop_back();
            left_op2  = stack.back(); stack.pop_back();
        } else if( count == 1 ) {
            right_op1 = stack.back(); stack.pop_back();
        }
        return true;
    };

    for (size_t i=0; i<expr.size() && status==RPNStatus::No_Error; i++)
    {
        const rpn_token& t = expr[i];
        switch (t.ts)
        {
            case rpn_token_t::UREAL:
                stack.push_back(t.value);
                break;

            case rpn_token_t::VARIABLE:
                {
                    if (auto var_iter = variables.find(t.id); var_iter != variables.end()) {
                        stack.push_back(var_iter->second);
                    } else {
                        // variable t.id undefined!
                        fprintf(stderr, "variable '%s' undefined\n", t.id.c_str());
                        status = RPNStatus::Unresolved_Variables; // FIXME: Refine!
                    }
                }
                break;

            case rpn_token_t::ADD:
                if( !get_stack(2) ) { break; }
                if ( ( ( left_op2 > 0 && right_op1 > 0 )
                       ||
                       ( left_op2 < 0 && right_op1 < 0 )
                     ) &&
                     std::abs(left_op2) >=
                         std::numeric_limits<double>::max() - std::abs(right_op1)
                   )
                {
                    status = RPNStatus::Overflow;
                } else {
                    stack.push_back(left_op2 + right_op1);
                }
                break;

            case rpn_token_t::SUB:
                if( !get_stack(2) ) { break; }
                if ( ( ( left_op2 > 0 && right_op1 < 0 )
                       ||
                       ( left_op2 < 0 && right_op1 > 0 )
                     ) &&
                     std::abs(left_op2) >=
                             std::numeric_limits<double>::max() - std::abs(right_op1)
                   )
                {
                    status =RPNStatus::Overflow;
                } else {
                    stack.push_back(left_op2 - right_op1);
                }
                break;

            case rpn_token_t::MUL:
                if( !get_stack(2) ) { break; }
                if( std::abs(right_op1) > 1 &&
                    std::abs(left_op2) >=
                        std::numeric_limits<double>::max() / std::abs(right_op1) )
                {
                    status =RPNStatus::Overflow;
                } else {
                    stack.push_back(left_op2 * right_op1);
                }
                break;

            case rpn_token_t::DIV:
                if( !get_stack(2) ) { break; }
                if (right_op1 == 0)
                {
                    status = RPNStatus::Division_by_zero;
                    break;
                }
                if( std::abs(right_op1) < 1 &&
                    std::abs(left_op2) >=
                        std::numeric_limits<double>::max() * std::abs(right_op1) )
                {
                    status = RPNStatus::Overflow;
                } else {
                    stack.push_back(left_op2 / right_op1);
                }
                break;

            case rpn_token_t::MOD:
                if( !get_stack(2) ) { break; }
                if (right_op1 == 0)
                {
                    status = RPNStatus::Division_by_zero;
                    break;
                }
                if( std::abs(right_op1) < 1 &&
                    std::abs(left_op2) >=
                        std::numeric_limits<double>::max() * std::abs(right_op1) )
                {
                    status = RPNStatus::Overflow;
                } else {
                    stack.push_back(std::fmod(left_op2, right_op1));
                }
                break;

            case rpn_token_t::POW:
                if( !get_stack(2) ) { break; }
                stack.push_back(std::pow(left_op2,right_op1));
                break;

            case rpn_token_t::SQRT:
                if( !get_stack(1) ) { break; }
                if (right_op1<0)
                {
                    status=RPNStatus::Undefined;
                } else {
                    stack.push_back(std::sqrt (right_op1));
                }
                break;

            case rpn_token_t::LOG:
                if( !get_stack(1) ) { break; }
                if (right_op1<=0)
                {
                    status=RPNStatus::Undefined;
                } else {
                    stack.push_back(std::log (right_op1));
                }
                break;

            case rpn_token_t::LOG10:
                if( !get_stack(1) ) { break; }
                if (right_op1<=0)
                {
                    status=RPNStatus::Undefined;
                } else {
                    stack.push_back(std::log10(right_op1));
                }
                break;

            case rpn_token_t::EXP:
                if( !get_stack(1) ) { break; }
                stack.push_back(std::exp(right_op1));
                break;

            case rpn_token_t::ABS:
                if( !get_stack(1) ) { break; }
                stack.push_back(std::abs(right_op1));
                break;

            case rpn_token_t::SIN:
                if( !get_stack(1) ) { break; }
                stack.push_back(std::sin(right_op1));
                break;

            case rpn_token_t::COS:
                if( !get_stack(1) ) { break; }
                stack.push_back(std::cos(right_op1));
                break;

            case rpn_token_t::TAN:
                if( !get_stack(1) ) { break; }
                stack.push_back(std::tan(right_op1));
                break;

            case rpn_token_t::ARCSIN:
                if( !get_stack(1) ) { break; }
                if (std::abs(right_op1)>1)
                {
                    status=RPNStatus::Undefined;
                } else {
                    stack.push_back(std::asin (right_op1));
                }
                break;

            case rpn_token_t::ARCCOS:
                if( !get_stack(1) ) { break; }
                if (std::abs(right_op1)>1)
                {
                    status=RPNStatus::Undefined;
                } else {
                    stack.push_back(std::acos (right_op1));
                }
                break;

            case rpn_token_t::ARCTAN:
                if( !get_stack(1) ) { break; }
                stack.push_back(std::atan (right_op1));
                break;

            case rpn_token_t::CEIL:
                if( !get_stack(1) ) { break; }
                stack.push_back(std::ceil(right_op1));
                break;

            case rpn_token_t::FLOOR:
                if( !get_stack(1) ) { break; }
                stack.push_back(std::floor(right_op1));
                break;

            case rpn_token_t::STEP:
                if( !get_stack(2) ) { break; }
                stack.push_back(step(left_op2, right_op1));
                break;

            case rpn_token_t::MIX:
                if( !get_stack(3) ) { break; }
                stack.push_back(mix(lleft_op3, left_op2, right_op1));
                break;

            case rpn_token_t::NEG:
                if( !get_stack(1) ) { break; }
                stack.push_back(0 - right_op1);
                break;

            default :
                break;
        }
    }

    if( status == RPNStatus::No_Error ) {
        if( stack.size() < 1 ) {
            status = RPNStatus::RPN_Underflow;
        } else {
            result = stack.back(); stack.pop_back();
        }
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
