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

#ifndef GEARS_TYPE_TRAITS_HPP
#define GEARS_TYPE_TRAITS_HPP

#include <limits>

namespace Gears
{
  template<typename Type>
  struct RemoveConst
  {
    typedef Type Result;
  };

  template<typename Type>
  struct RemoveConst<const Type>
  {
    typedef Type Result;
  };

  template<typename Integer>
  Integer
  safe_next(Integer number) noexcept
  {
    return number < std::numeric_limits<Integer>::max() ? number + 1 : number;
  }
}

#endif /*GEARS_TYPE_TRAITS_HPP*/
