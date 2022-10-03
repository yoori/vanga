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

#include "BaseType.hpp"
#include "BaseDescriptor.hpp"
#include "BaseReader.hpp"
#include "BaseWriter.hpp"
#include "BaseTemplate.hpp"

namespace Declaration
{
  BaseType::BaseType(const char* name_val)
    : name_(name_val)
  {}

  const char*
  BaseType::name() const
  {
    return name_.c_str();
  }

  BaseDescriptor_var
  BaseType::as_descriptor() noexcept
  {
    return BaseDescriptor_var();
  }

  BaseReader_var
  BaseType::as_reader() noexcept
  {
    return BaseReader_var();
  }

  BaseWriter_var
  BaseType::as_writer() noexcept
  {
    return BaseWriter_var();
  }

  BaseTemplate_var
  BaseType::as_template() noexcept
  {
    return BaseTemplate_var();
  }
}

