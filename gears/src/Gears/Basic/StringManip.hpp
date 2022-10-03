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

#ifndef GEARS_STRINGMANIP_HPP
#define GEARS_STRINGMANIP_HPP

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <cassert>
#include <string>
#include <algorithm>
#include <limits>

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/StringUtils.hpp>
#include <Gears/Basic/SubString.hpp>

namespace Gears
{
  /**
   * Contain general string manipulation routines
   */
  namespace StringManip
  {
    DECLARE_GEARS_EXCEPTION(InvalidFormatException, DescriptiveException);

    /**
     * Adds the substring to the string. If there is no space left
     * the function terminates it with nul otherwise it returns the space
     * amount left.
     * @param buffer string buffer to copy to.
     * @param size space amount left in the string buffer.
     * @param substr substring to copy.
     * @return space amount left in the string buffer.
     */
    size_t
    add(char* buffer, size_t size, const SubString& substr)
      noexcept;

    /**
     * Converts integer value into string
     * @param value value to convert
     * @param str buffer to convert to (zero terminated)
     * @param size its size
     * @return number of characters written (without trailing zero)
     * (0 indicates error)
     */
    template <typename Integer>
    size_t
    int_to_str(Integer value, char* str, size_t size) noexcept;

    /**
     * Safely concatenates several strings to the string buffer.
     * @param buffer string buffer to concatenate to
     * @param size the size of the buffer
     * @param f arg to append
     * @param args args to append
     */
    template <typename First, typename... Args>
    void
    concat(char* buffer, size_t size, First f, Args... args)
      noexcept;

    template <typename Integer>
    bool
    str_to_int(const SubString& str, Integer& value) noexcept;
  } // namespace StringManip
}

namespace Gears
{
  namespace StringManip
  {
    inline
    size_t
    append(char* buffer, size_t size, const char* str) noexcept
    {
      size_t length = strlcpy(buffer, str, size);
      return length < size ? length : size;
    }

    inline
    size_t
    append(char* buffer, size_t size, char* str) noexcept
    {
      return append(buffer, size, const_cast<const char*>(str));
    }

    inline
    size_t
    append(char* buffer, size_t size, const SubString& str) noexcept
    {
      //assert(size);
      if (str.size() >= size)
      {
        CharTraits<char>::copy(buffer, str.data(), size - 1);
        buffer[size - 1] = '\0';
        return size;
      }

      CharTraits<char>::copy(buffer, str.data(), str.size());
      return str.size();
    }

    inline
    size_t
    append(char* buffer, size_t size, const std::string& str) noexcept
    {
      return append(buffer, size, SubString(str));
    }

    template <typename Integer>
    size_t
    append(char* buffer, size_t size, Integer integer) noexcept
    {
      size_t res = int_to_str(integer, buffer, size);

      if (!res)
      {
        //assert(size);
        *buffer = '\0';
        return size;
      }

      return res - 1;
    }

    inline
    void
    concat(char* buffer, size_t size)
      noexcept
    {
      assert(size);
      *buffer = '\0';
    }

    template <typename First, typename... Args>
    void
    concat(char* buffer, size_t size, First f, Args... args)
      noexcept
    {
      size_t length = append(buffer, size, f);
      if (length < size)
      {
        concat(buffer + length, size - length, args...);
      }
    }

    //
    // int_to_str function
    //
    namespace IntToStrHelper
    {
      template <typename Integer, const bool is_signed>
      struct IntToStrSign;

      template <typename Integer>
      struct IntToStrSign<Integer, false>
      {
        static size_t
        convert(Integer value, char* str) noexcept;
      };

      template <typename Integer>
      size_t
      IntToStrSign<Integer, false>::convert(Integer value, char* str) noexcept
      {
        char* ptr = str;
        do
        {
          *ptr++ = '0' + value % 10;
        }
        while (value /= 10);
        size_t size = ptr - str;
        for (*ptr-- = '\0'; str < ptr; str++, ptr--)
        {
          std::swap(*str, *ptr);
        }
        return size;
      }

      template <typename Integer>
      struct IntToStrSign<Integer, true>
      {
        static size_t
        convert(Integer value, char* str) noexcept;
      };

      template <typename Integer>
      size_t
      IntToStrSign<Integer, true>::convert(Integer value, char* str) noexcept
      {
        if (value < -std::numeric_limits<Integer>::max())
        {
          return 0;
        }
        if (value < 0)
        {
          *str = '-';
          return IntToStrSign<Integer, false>::convert(-value, str + 1) + 1;
        }
        return IntToStrSign<Integer, false>::convert(value, str);
      }

      template <typename Integer, const bool is_integer>
      struct IntToStrInt;

      template <typename Integer>
      struct IntToStrInt<Integer, true>
      {
        static size_t
        convert(Integer value, char* str, size_t size) noexcept;
      };

      template <typename Integer>
      size_t
      IntToStrInt<Integer, true>::convert(Integer value, char* str,
        size_t size) noexcept
      {
        if (size < std::numeric_limits<Integer>::digits10 + 3)
        {
          return 0;
        }

        return IntToStrSign<Integer,
          std::numeric_limits<Integer>::is_signed>::convert(value, str);
      }
    }

    template <typename Integer>
    size_t
    int_to_str(Integer value, char* str, size_t size) noexcept
    {
      return IntToStrHelper::IntToStrInt<Integer,
        std::numeric_limits<Integer>::is_integer>::convert(value, str, size);
    }

    template <typename Integer>
    bool
    str_to_int(const SubString& str, Integer& value) noexcept
    {
      const char* src = str.begin();
      const char* const END = str.end();
      if (src == END)
      {
        return false;
      }
      bool negative = false;
      switch (*src)
      {
      case '-':
        if (!std::numeric_limits<Integer>::is_signed)
        {
          return false;
        }
        negative = true;
      case '+':
        if (++src == END)
        {
          return false;
        }
      }
      value = 0;
      const Integer LIMIT = std::numeric_limits<Integer>::max() / 10;
      if (negative)
      {
        do
        {
          unsigned char ch = static_cast<unsigned char>(*src) -
            static_cast<unsigned char>('0');
          if (ch > 9 || value < -LIMIT || (value == -LIMIT &&
            ch > static_cast<unsigned char>(
              -(std::numeric_limits<Integer>::min() + LIMIT * 10))))
          {
            return false;
          }
          value = value * static_cast<Integer>(10) -
            static_cast<Integer>(ch);
        }
        while (++src != END);
      }
      else
      {
        do
        {
          unsigned char ch = static_cast<unsigned char>(*src) -
            static_cast<unsigned char>('0');
          if (ch > 9 || value > LIMIT || (value == LIMIT &&
            ch > static_cast<unsigned char>(
              std::numeric_limits<Integer>::max() - LIMIT * 10)))
          {
            return false;
          }
          value = value * static_cast<Integer>(10) +
            static_cast<Integer>(ch);
        }
        while (++src != END);
      }

      return true;
    }
  }
} /*Gears*/

#endif
