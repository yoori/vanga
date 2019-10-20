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

#include "Processor.hpp"
#include "YYParserAdapter.hpp"
#include "Parser.hpp"

#include <src/Gears/Plain/Parsing/YYParser.yy.hpp>

namespace Parsing
{
  bool Parser::parse(
    std::ostream& error_ostr,
    Code::ElementList* elements,
    std::istream& istr,
    Declaration::Namespace_var* root_namespace)
    throw(Exception)
  {
    Parsing::Processor_var processor = new Parsing::Processor(elements);

    YYParserAdapter parse_adapter;
    parse_adapter.processor = processor;
    parse_adapter.scanner = new yy::PlainScanner(&istr, &error_ostr);
    yy::PlainParser parser(parse_adapter);
    if(!parser.parse())
    {
      if(root_namespace)
      {
        *root_namespace = processor->root_namespace();
      }

      return true;
    }

    return false;
  }
}
