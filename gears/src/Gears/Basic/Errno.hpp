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

#ifndef GEARS_EH_ERRNO_HPP
#define GEARS_EH_ERRNO_HPP

#include <algorithm>
#include <errno.h>

#include "Macro.hpp"
#include "Exception.hpp"
#include "SubString.hpp"
#include "StringManip.hpp"

OPEN_NAMESPACE(Gears)

  //
  // Errno throw helper
  //
  namespace ErrnoHelper
  {
    // There are two versions of strerror_r functions with
    // different behavior
    inline const char*
    error_message(const char* /*buf*/, const char* function_result)
    {
      return function_result;
    }

    inline const char*
    error_message(const char* buf, int /*function_result*/)
    {
      return buf;
    }

    template <typename... Args>
    void
    compose_safe(char* string, size_t string_size, int error, Args... args)
      throw ()
    {
      char error_buf[128];
      char buf[128];

      StringManip::int_to_str(error, error_buf, sizeof(error_buf));
      StringManip::concat(
        string,
        string_size,
        args...,
        ": errno = ",
        error_buf,
        ": ",
        error_message(buf, strerror_r(error, buf, sizeof(buf))));
    }
  }

  template <typename ExceptionType, typename... ArgTypes>
  void
  throw_errno_value_exception(int error, ArgTypes... args) throw(ExceptionType)
  {
    char string[sizeof(ExceptionType)];
    ErrnoHelper::compose_safe(string, sizeof(string), error, args...);
    throw ExceptionType(string);
  }

  template <typename ExceptionType, typename... ArgTypes>
  void
  throw_errno_exception(ArgTypes... args) throw(ExceptionType)
  {
    throw_errno_value_exception<ExceptionType>(errno, args...);
  }

CLOSE_NAMESPACE

#endif
