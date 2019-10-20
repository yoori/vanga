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

#ifndef PLAIN_BASE_HPP
#define PLAIN_BASE_HPP

#include <stdint.h>
#include <Gears/Basic/Exception.hpp>

namespace PlainTypes
{
  DECLARE_GEARS_EXCEPTION(CorruptedStruct, Gears::DescriptiveException);

  class ConstBuf
  {
  public:
    ConstBuf(const void* buf, unsigned long buf_size)
      : buf_(static_cast<const unsigned char*>(buf)),
        buf_size_(buf_size)
    {}

    const unsigned char* get() const
    {
      return buf_;
    }

    unsigned long size() const
    {
      return buf_size_;
    }

  protected:
    const unsigned char* buf_;
    unsigned long buf_size_;
  };

  struct CharCastPolicy
  {
    static char read_cast(const void* buf);

    static char& write_cast(void* buf);
  };

  struct IntCastPolicy
  {
    static int32_t read_cast(const void* buf);

    static int32_t& write_cast(void* buf);
  };

  struct UIntCastPolicy
  {
    static uint32_t read_cast(const void* buf);

    static uint32_t& write_cast(void* buf);
  };

  struct UInt64CastPolicy
  {
    static uint64_t read_cast(const void* buf);

    static uint64_t& write_cast(void* buf);
  };
}

namespace PlainTypes
{
  inline
  char
  CharCastPolicy::read_cast(const void* buf)
  {
    return *static_cast<const int32_t*>(buf);
  }

  inline
  char&
  CharCastPolicy::write_cast(void* buf)
  {
    return *static_cast<char*>(buf);
  }

  inline
  int32_t
  IntCastPolicy::read_cast(const void* buf)
  {
    return *static_cast<const int32_t*>(buf);
  }

  inline
  int32_t&
  IntCastPolicy::write_cast(void* buf)
  {
    return *static_cast<int32_t*>(buf);
  }

  inline
  uint32_t
  UIntCastPolicy::read_cast(const void* buf)
  {
    return *static_cast<const uint32_t*>(buf);
  }

  inline uint32_t&
  UIntCastPolicy::write_cast(void* buf)
  {
    return *static_cast<uint32_t*>(buf);
  }

  inline uint64_t
  UInt64CastPolicy::read_cast(const void* buf)
  {
    return *static_cast<const uint64_t*>(buf);
  }

  inline
  uint64_t&
  UInt64CastPolicy::write_cast(void* buf)
  {
    return *static_cast<uint64_t*>(buf);
  }
}

#endif /*PLAIN_BASE_HPP*/
