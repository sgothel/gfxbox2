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

#ifndef INFIX_CALC_HPP
#define INFIX_CALC_HPP

#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <ostream>

#include "rpn_calc.hpp"

namespace infix_calc_yy {
    class scanner; // fwd
    class parser; // fwd
    class location; // fwd
}

namespace infix_calc {

    class compiler {
        private:
            friend class infix_calc_yy::parser;

            bool scan_begin(const std::string& fname);
            bool scan_begin(const int fd);
            bool scan_begin(const char* bytes, int len);
            void scan_end();

            void put_rpn(rpn_calc::rpn_token_t ts) { rpn_expr.push_back(ts); }
            void put_rpn(double value) { rpn_expr.push_back(value); }
            void put_rpn(const std::string& variable_name) { rpn_expr.push_back(variable_name); }
            void put_rpn(std::string&& variable_name) { rpn_expr.push_back(std::move(variable_name)); }

            void* backend;

        public:
            rpn_calc::variable_set variables;
            rpn_calc::rpn_expression_t rpn_expr;

            compiler();
            ~compiler();

            // Run the parser on file f, returns true on success.
            bool parse (const std::string& fname);
            // Run the parser on file descriptor fd, returns true on success.
            bool parse (const int fd);
            // Run the parser on given bytes, returns true on success.
            bool parse (const char* bytes, int len);

            // Reduce the RPN, replacing this RPM stack if no error occurs
            rpn_calc::RPNStatus reduce() noexcept { return rpn_expr.reduce(); }

            // Checks whether all variables in RPN are resolved with given variables.
            bool resolved() const noexcept { return rpn_expr.resolved(variables); }

            // Evaluate the parsed RPN w/ given variables.
            rpn_calc::RPNStatus eval(double& result) const noexcept {
                return rpn_expr.eval(result, variables);
            }
            infix_calc_yy::location& location();
    };

} // namespace infix_calc

namespace std {
    ostream& operator<<(ostream& os, const infix_calc_yy::location& loc);
}
#endif /* INFIX_CALC_HPP */

