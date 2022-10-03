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

#ifndef PLAIN_CPP_READERGENERATOR_HPP
#define PLAIN_CPP_READERGENERATOR_HPP

#include <string>
#include <iostream>

#include <Gears/Plain/Declaration/SimpleDescriptor.hpp>
#include <Gears/Plain/Declaration/SimpleReader.hpp>
#include <Gears/Plain/Declaration/StructDescriptor.hpp>
#include <Gears/Plain/Declaration/StructReader.hpp>
#include <Gears/Plain/Declaration/CompleteTemplateDescriptor.hpp>

namespace Cpp
{
  class ReaderGenerator
  {
  public:
    ReaderGenerator(std::ostream& out, const char* offset)
      noexcept;

    void generate_decl(
      Declaration::StructReader* struct_reader)
      noexcept;

    void generate_impl(
      Declaration::StructReader* struct_reader)
      noexcept;

  private:
    void generate_ctor_impl_(
      const Declaration::StructReader* struct_reader)
      noexcept;

    void generate_field_funs_impl_(
      const Declaration::StructReader* struct_reader)
      noexcept;

  private:
    std::ostream& out_;
    std::string offset_;
  };
}

#endif /*PLAIN_CPP_READERGENERATOR_HPP*/
