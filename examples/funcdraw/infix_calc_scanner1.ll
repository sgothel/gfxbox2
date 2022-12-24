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
%top{
    // This scanner is using a reentrant flex w/ prefix 
    //    
    # include "infix_calc.hpp"
    # include "infix_calc_parser.hpp"
    
    # include "infix_calc_scanner_warnings.hpp"
    
    # define YY_DECL \
        infix_calc_yy::parser::symbol_type infix_calc_yylex(infix_calc_yy::location& loc, yyscan_t yyscanner)
        
    // Code run each time a pattern is matched.
    # define YY_USER_ACTION  loc.columns (yyleng);
    
    // %option c++       
    // %option bison-bridge
    
    // %option debug    
}

%option noyywrap nounput noinput batch

%option prefix="infix_calc_yy"
%option reentrant

%{
    namespace infix_calc {
        infix_calc_yy::parser::symbol_type make_UREAL(const std::string &s, double f, const infix_calc_yy::parser::location_type& loc);
    }
%}

id    [a-zA-Z][a-zA-Z_0-9]*
ureal [0-9]+("."[0-9]+)?([eE][-+]?[0-9]+)?
pi    [pP][iI]
blank [ \t\r]

%%
%{
    // Called at yylex entry
    loc.step ();
%}

{blank}+    loc.step ();
\n+         loc.lines (yyleng); loc.step ();

\(          return infix_calc_yy::parser::make_LPAREN(loc);
\)          return infix_calc_yy::parser::make_RPAREN(loc);

"draw"      return infix_calc_yy::parser::make_DRAW(loc);
"clear"     return infix_calc_yy::parser::make_CLEAR(loc);
"help"      return infix_calc_yy::parser::make_HELP(loc);
"exit"      return infix_calc_yy::parser::make_EXIT(loc);

"abs"       return infix_calc_yy::parser::make_ABS(loc);
"sin"       return infix_calc_yy::parser::make_SIN(loc);
"cos"       return infix_calc_yy::parser::make_COS(loc);
"tan"       return infix_calc_yy::parser::make_TAN(loc);
"asin"      return infix_calc_yy::parser::make_ARCSIN(loc);
"acos"      return infix_calc_yy::parser::make_ARCCOS(loc);
"atan"      return infix_calc_yy::parser::make_ARCTAN(loc);
"^"         return infix_calc_yy::parser::make_POW(loc);
"sqrt"      return infix_calc_yy::parser::make_SQRT(loc);
"ln"        return infix_calc_yy::parser::make_LOG(loc);
"log"       return infix_calc_yy::parser::make_LOG10(loc);
"exp"       return infix_calc_yy::parser::make_EXP(loc);
"neg"       return infix_calc_yy::parser::make_NEG(loc);

{ureal}{pi} return infix_calc::make_UREAL (yytext, M_PI, loc);
{ureal}[e]  return infix_calc::make_UREAL (yytext, M_E, loc);
{ureal}     return infix_calc::make_UREAL (yytext, 1.0, loc);

{pi}        return infix_calc_yy::parser::make_UREAL(M_PI, loc);
[e]         return infix_calc_yy::parser::make_UREAL(M_E, loc);

\-          return infix_calc_yy::parser::make_SUB(loc);
"+"         return infix_calc_yy::parser::make_ADD(loc);
\*          return infix_calc_yy::parser::make_MUL(loc);
\/          return infix_calc_yy::parser::make_DIV(loc);
\%          return infix_calc_yy::parser::make_MOD(loc);

{id}        return infix_calc_yy::parser::make_IDENTIFIER(yytext, loc);

.           return infix_calc_yy::parser::make_ERROR("Unrecognized characters '"+std::string(yytext)+"'", loc);

<<EOF>>     return infix_calc_yy::parser::make_YYEOF (loc);

%%
