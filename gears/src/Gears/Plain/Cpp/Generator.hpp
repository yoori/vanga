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

#ifndef PLAIN_CPP_GENERATOR_HPP
#define PLAIN_CPP_GENERATOR_HPP

#include <Gears/Plain/Declaration/SimpleDescriptor.hpp>
#include <Gears/Plain/Declaration/SimpleReader.hpp>
#include <Gears/Plain/Declaration/SimpleWriter.hpp>
#include <Gears/Plain/Declaration/StructDescriptor.hpp>
#include <Gears/Plain/Declaration/StructReader.hpp>
#include <Gears/Plain/Declaration/StructWriter.hpp>

#include <Gears/Plain/Code/Element.hpp>
#include <Gears/Plain/Code/IncludeElement.hpp>
#include <Gears/Plain/Code/TypeElement.hpp>
#include <Gears/Plain/Code/TypeDefElement.hpp>
#include <Gears/Plain/Code/NamespaceElement.hpp>

namespace Cpp
{
  class Generator: public Gears::AtomicRefCountable
  {
  public:
    void generate(
      std::ostream& out,
      std::ostream& out_inl_impl,
      std::ostream& out_cpp,
      Code::ElementList* elements) throw();

  protected:
    virtual ~Generator() throw() {}
  };

  typedef Gears::IntrusivePtr<Generator>
    Generator_var;
}

#endif /*PLAIN_CPP_GENERATOR_HPP*/
