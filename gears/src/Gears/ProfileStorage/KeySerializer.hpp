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

#ifndef GEARS_KEYSERIALIZER_HPP_
#define GEARS_KEYSERIALIZER_HPP_

#include <Gears/Basic/Exception.hpp>

namespace Gears
{
  struct StringSerializer
  {
    static void
    read(std::string& key, void* buf, unsigned long buf_size)
    {
      key.assign(static_cast<const char*>(buf), buf_size);
    }

    static void
    write(void* buf, unsigned long buf_size, const std::string& key)
    {
      ::memcpy(buf, key.data(), buf_size);
    }

    static unsigned long
    size(const std::string& key)
    {
      return key.size();
    }
  };

  struct NumericSerializer
  {
    static void
    read(uint64_t& key, void* buf, unsigned long buf_size)
    {
      assert(buf_size == 8);
      key = *static_cast<const uint64_t*>(buf);
    }

    static void
    write(void* buf, unsigned long buf_size, uint64_t key)
    {
      assert(buf_size == 8);
      *static_cast<uint64_t*>(buf) = key;
    }

    static unsigned long
    size(uint64_t)
    {
      return 8;
    }
  };
}

#endif /*GEARS_KEYSERIALIZER_HPP_*/
