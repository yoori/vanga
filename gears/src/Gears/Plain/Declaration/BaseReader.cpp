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

#include "BaseReader.hpp"
#include "SimpleReader.hpp"
#include "StructReader.hpp"

namespace Declaration
{
  BaseReader::BaseReader(const char* name_val) noexcept
    : BaseType(name_val)
  {}

  BaseReader_var
  BaseReader::as_reader() noexcept
  {
    return Gears::add_ref(this);
  }

  SimpleReader_var
  BaseReader::as_simple_reader() noexcept
  {
    return SimpleReader_var();
  }

  StructReader_var
  BaseReader::as_struct_reader() noexcept
  {
    return StructReader_var();
  }
}
