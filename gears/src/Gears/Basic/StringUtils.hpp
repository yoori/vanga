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

#ifndef GEARS_STRINGUTILS_HPP
#define GEARS_STRINGUTILS_HPP

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <Gears/Basic/Exception.hpp>

namespace Gears
{
  /**
   * Contain help string manipulation routines
   */
  namespace StringManip
  {
    size_t
    strlcpy(char* dst, const char* src, size_t size) noexcept;
  } // namespace StringManip
}

namespace Gears
{
namespace StringManip
{
  inline
  size_t
  strlcpy(char* dst, const char* src, size_t size) noexcept
  {
    const char* const saved_src = src;

    if (size)
    {
      while (--size)
      {
        if (!(*dst++ = *src++))
        {
          return src - saved_src - 1;
        }
      }
      *dst = '\0';
    }

    while (*src++);

    return src - saved_src - 1;
  }
}
} /*Gears*/

#endif
