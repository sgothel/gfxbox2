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

namespace infix_calc_yy {
    class scanner; // fwd
    class parser; // fwd
    class location; // fwd
}

namespace infix_calc {

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
    std::string to_string(const rpn_token& v) noexcept;

    std::string to_string(const std::vector<rpn_token>& rpn_stack) noexcept;

    typedef std::map<std::string, double> variable_set;

    std::string to_string(const variable_set& variables) noexcept;

    // Reduce the given source RPN into the new dest
    RPNStatus reduce(std::vector<rpn_token>& result, const std::vector<rpn_token>& source) noexcept;

    // Checks whether all variables in RPN are resolved with given variables.
    bool resolved(const variable_set& variables, const std::vector<rpn_token>& rpn_stack) noexcept;

    // Evaluate the given RPN and variables.
    RPNStatus eval(double& result, const variable_set& variables, const std::vector<rpn_token>& rpn_stack) noexcept;

    class compiler {
        private:
            friend class infix_calc_yy::parser;

            bool scan_begin(const std::string& fname);
            bool scan_begin(const int fd);
            bool scan_begin(const char* bytes, int len);
            void scan_end();

            void put_rpn(rpn_token_t ts);
            void put_rpn(double value);
            void put_rpn(const std::string& variable_name);
            void put_rpn(std::string&& variable_name);

            void* backend;

        public:
            variable_set variables;
            std::vector<rpn_token> rpn_stack;

            compiler();
            ~compiler();

            // Run the parser on file f, returns true on success.
            bool parse (const std::string& fname);
            // Run the parser on file descriptor fd, returns true on success.
            bool parse (const int fd);
            // Run the parser on given bytes, returns true on success.
            bool parse (const char* bytes, int len);

            // Reduce the RPN, replacing this RPM stack if no error occurs
            RPNStatus reduce() noexcept {
                std::vector<rpn_token> dest;
                RPNStatus s = infix_calc::reduce(dest, rpn_stack);
                if( RPNStatus::No_Error == s ) {
                    rpn_stack = dest;
                }
                return s;
            }

            // Checks whether all variables in RPN are resolved with given variables.
            bool resolved() noexcept {
                return infix_calc::resolved(variables, rpn_stack);
            }

            // Evaluate the parsed RPN w/ given variables.
            RPNStatus eval(double& result) const noexcept {
                return infix_calc::eval(result, variables, rpn_stack);
            }
            infix_calc_yy::location& location();
    };

} // namespace infix_calc

namespace std {
    ostream& operator<<(ostream& os, const infix_calc_yy::location& loc);
    ostream& operator<<(ostream& os, const infix_calc::rpn_token_t rpn);
    ostream& operator<<(ostream& os, const infix_calc::rpn_token& rpn);
}
#endif /* INFIX_CALC_HPP */

