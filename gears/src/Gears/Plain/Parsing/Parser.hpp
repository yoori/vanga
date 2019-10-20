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

#ifndef PLAIN_PARSER_HPP
#define PLAIN_PARSER_HPP

#include <iostream>
#include <Gears/Basic/RefCountable.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>
#include <Gears/Plain/Code/Element.hpp>
#include <Gears/Plain/Declaration/Namespace.hpp>

namespace Parsing
{
  class Parser:
    public Gears::AtomicRefCountable
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);

    bool parse(
      std::ostream& error_ostr,
      Code::ElementList* elements,
      std::istream& istr,
      Declaration::Namespace_var* root_namespace = 0)
      throw(Exception);

  protected:
    virtual
    ~Parser() throw () = default;
  };

  typedef Gears::IntrusivePtr<Parser> Parser_var;
}

#endif /*PLAIN_PARSER_HPP*/
