/* 
 * This file is part of the Vanga distribution (https://github.com/yoori/vanga).
 * Vanga is library that implement multinode decision tree constructing algorithm
 * for regression prediction
 *
 * Copyright (c) 2014 Yuri Kuznecov <yuri.kuznecov@gmail.com>.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PLAIN_PARSING_YYSCANNER_HPP
#define PLAIN_PARSING_YYSCANNER_HPP

#ifndef YY_DECL
# define YY_DECL \
    yy::PlainParser::token_type \
    yy::PlainScanner::lex( \
      yy::PlainParser::semantic_type* yylval, \
      yy::PlainParser::location_type* yylloc)

#endif

#ifndef __FLEX_LEXER_H
# define yyFlexLexer PlainFlexLexer
# include "FlexLexer.h"
# undef yyFlexLexer
#endif

#include <iostream>
#include <Gears/Basic/RefCountable.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>
#include <src/Gears/Plain/Parsing/YYParser.yy.hpp>

namespace yy
{
  class PlainScanner:
    public PlainFlexLexer,
    public Gears::AtomicRefCountable
  {
  public:
    PlainScanner(std::istream* in, std::ostream* out);

    /* lex adapter */
    virtual yy::PlainParser::token_type lex(
      yy::PlainParser::semantic_type* yyval,
      yy::location* loc);

    void set_debug(bool debug);

  protected:
    virtual ~PlainScanner() throw();
  };

  typedef Gears::IntrusivePtr<PlainScanner>
    PlainScanner_var;
}

#endif /*PLAIN_PARSING_YYSCANNER_HPP*/
