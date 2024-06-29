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
 
// Grammar input for Bison >= 3.2
%require "3.2"
%language "c++"

%locations // produce infix_calc_yy::location type for more accurate location @ error

%define lr.type lalr // default, smallest table and fastest

%define api.prefix {infix_calc_yy} // namespace
%define api.parser.class {parser}
// %define api.pure full // default w/ C++
%define api.token.raw // use scanner token values, no lookup
%define api.token.constructor // use complete symbols
%define api.token.prefix {TOK_}
%define api.value.type variant // use complete symbols
// %define api.value.automove // use rvalue std::move -> we use it manually

%parse-param { std::function<infix_calc_yy::parser::symbol_type(infix_calc::compiler&)> yylex } // for parser-ctor only
%param { infix_calc::compiler& cc} // append: parser-ctor and yylex-call

// %define parse.assert // requires RTTI

// Lookahead Correction (LAC)
// %define parse.lac full 
%define parse.lac none // default, faster, LAC not required on this simple grammar
%define parse.error detailed

// Debugging:
// %define parse.trace

%code requires {
  // # define YYDEBUG 1
  # include <string>
  # include <functional>
  # include "infix_calc.hpp"
  using namespace infix_calc;
}

%code {
  // location: parser implementation file after the usual contents of the parser header file
  # include "infix_calc_parser.hpp"  
  
  void add_func(rpn_calc::rpn_expression_t& expr);
  void clear_funcs();
  void exit_app();
  void print_usage();
  void set_width(float x1, float x2);
  void set_height(float y1, float y2);
}

%printer { yyo << $$; } <*>;

%token <double> UREAL
%token <std::string> IDENTIFIER
%token <std::string> ERROR

%nterm <double> real_number
%nterm <rpn_calc::rpn_token_t> unary_func, arg2_func, arg3_func

%token LPAREN RPAREN COMMA SEMICOLON EOL

%left DRAW CLEAR SET_WIDTH SET_HEIGHT HELP EXIT

%left SUB ADD
%left MUL DIV MOD MOD2
%left ABS SIN COS TAN ARCSIN ARCCOS ARCTAN
%left POW POW2 LOG LOG10 EXP
%left SQRT CEIL FLOOR
%left STEP MIX
%left NEG

%%

program       : command
              | program command
              ;
              
command       : DRAW { cc.rpn_expr.clear(); } expression SEMICOLON { add_func(cc.rpn_expr); cc.rpn_expr.clear(); }
              | CLEAR SEMICOLON { clear_funcs(); }
              | SET_WIDTH real_number COMMA real_number SEMICOLON { set_width($2, $4); }
              | SET_HEIGHT real_number COMMA real_number SEMICOLON { set_height($2, $4); }
              | HELP SEMICOLON { print_usage(); }
              | EXIT SEMICOLON { exit_app(); }     
              | error { print_usage(); }         
              ;
              
expression    : product
              | expression ADD product { cc.put_rpn(rpn_calc::rpn_token_t::ADD); }
              | expression SUB product { cc.put_rpn(rpn_calc::rpn_token_t::SUB); }
              ;

product     : operand
              | product MUL operand { cc.put_rpn(rpn_calc::rpn_token_t::MUL); }
              | product DIV operand { cc.put_rpn(rpn_calc::rpn_token_t::DIV); }
              | product MOD  operand { cc.put_rpn(rpn_calc::rpn_token_t::MOD); }
              | product MOD2 operand { cc.put_rpn(rpn_calc::rpn_token_t::MOD); }
              | product POW  operand { cc.put_rpn(rpn_calc::rpn_token_t::POW); }
              | product POW2 operand { cc.put_rpn(rpn_calc::rpn_token_t::POW); }
              ;

operand     : IDENTIFIER { cc.put_rpn(std::move($1)); }
              | ADD IDENTIFIER  { cc.put_rpn(std::move($2)); }
              | SUB IDENTIFIER  { cc.put_rpn(std::move($2));
                                  cc.put_rpn(rpn_calc::rpn_token_t::NEG);
                                }
              | LPAREN expression RPAREN
              | unary_func LPAREN expression RPAREN { cc.put_rpn($1); }
              | arg2_func LPAREN expression COMMA expression RPAREN { cc.put_rpn($1); }
              | arg3_func LPAREN expression COMMA expression COMMA expression RPAREN { cc.put_rpn($1); }
              | real_number { cc.put_rpn($1); }
              ;

unary_func  : ABS { $$=rpn_calc::rpn_token_t::ABS; } |
              SIN { $$=rpn_calc::rpn_token_t::SIN; } |
              COS { $$=rpn_calc::rpn_token_t::COS; } |
              TAN { $$=rpn_calc::rpn_token_t::TAN; } |
              ARCSIN { $$=rpn_calc::rpn_token_t::ARCSIN; } |
              ARCCOS { $$=rpn_calc::rpn_token_t::ARCCOS; } |
              ARCTAN { $$=rpn_calc::rpn_token_t::ARCTAN; } |
              LOG { $$=rpn_calc::rpn_token_t::LOG; } |
              LOG10 { $$=rpn_calc::rpn_token_t::LOG10; } |
              EXP { $$=rpn_calc::rpn_token_t::EXP; } |
              SQRT { $$=rpn_calc::rpn_token_t::SQRT; } |
              CEIL { $$=rpn_calc::rpn_token_t::CEIL; } |
              FLOOR { $$=rpn_calc::rpn_token_t::FLOOR; } |
              NEG  { $$=rpn_calc::rpn_token_t::NEG; } ;
              
arg2_func  : STEP { $$=rpn_calc::rpn_token_t::STEP; };

arg3_func  : MIX { $$=rpn_calc::rpn_token_t::MIX; };

real_number : UREAL { $$=$1; } |
              ADD UREAL { $$=$2; } |
              SUB UREAL { $$=$2*(-1); } ;

%%

void
infix_calc_yy::parser::error (const location_type& l, const std::string& m)
{
  std::cerr << l << ": " << m << std::endl;
}

