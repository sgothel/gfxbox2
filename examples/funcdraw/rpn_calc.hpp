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

#ifndef RPN_CALC_HPP
#define RPN_CALC_HPP

#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <ostream>

namespace rpn_calc {

    enum class RPNStatus : unsigned int {
        No_Error,
        /*-----------------------*/
        Too_Complex,
        Unresolved_Variables,
        Division_by_zero,
        Overflow,
        Undefined,
        RPN_Underflow,
        /*-----------------------*/
    };
    std::string to_string(RPNStatus status) noexcept;

    enum class rpn_token_t : unsigned int {
        UREAL, VARIABLE,
        SUB, ADD,
        MUL, DIV, MOD,
        ABS, SIN, COS, TAN, ARCSIN, ARCCOS, ARCTAN,
        POW, LOG, LOG10, EXP,
        SQRT, NEG
    };
    std::string to_string(const rpn_token_t ts) noexcept;

    struct rpn_token {
        /** Terminal Symbol (TOKEN) */
        rpn_token_t ts;
        /** Real value */
        double value;
        /** Variable identifier */
        std::string id;
    };
    typedef std::vector<rpn_token> rpn_stack_t;
    std::string to_string(const rpn_token& v) noexcept;
    std::string to_string(const rpn_stack_t& rpn_expr) noexcept;

    // Reduce the given source RPN into destination res RPN
    RPNStatus reduce(rpn_stack_t& res, const rpn_stack_t& source) noexcept;

    typedef std::map<std::string, double> variable_set;
    std::string to_string(const variable_set& variables) noexcept;

    struct rpn_expression_t {
        rpn_stack_t expr;

        void clear() noexcept { expr.clear(); }

        void push_back(rpn_token_t ts) {
            expr.push_back( { ts, 0.0, "" } );
        }

        void push_back(double value) {
            expr.push_back( { rpn_token_t::UREAL, value, "" } );
        }

        void push_back(const std::string& variable_name) {
            expr.push_back( { rpn_token_t::VARIABLE, 0.0, variable_name } );
        }

        void push_back(std::string&& variable_name) {
            expr.push_back( { rpn_token_t::VARIABLE, 0.0, std::move(variable_name) } );
        }

        // Reduce this RPN in place
        RPNStatus reduce() noexcept {
            rpn_stack_t dest;
            RPNStatus s = rpn_calc::reduce(dest, expr);
            if( RPNStatus::No_Error == s ) {
                expr = dest;
            }
            return s;
        }

        // Checks whether all variables in RPN are resolved with given variables.
        bool resolved(const variable_set& variables) const noexcept;

        // Evaluate the given RPN and variables.
        RPNStatus eval(double& result, const variable_set& variables) const noexcept;

        std::string toString() const noexcept { return to_string(expr); }
    };

} // namespace rpn_calc

namespace std {
    ostream& operator<<(ostream& os, const rpn_calc::rpn_token_t rpn);
    ostream& operator<<(ostream& os, const rpn_calc::rpn_token& rpn);
}
#endif /* RPN_CALC_HPP */

