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

#include <algorithm>
#include <cassert>

#include "Errno.hpp"
#include "BufAllocator.hpp"

namespace Gears
{
  //
  // BasicBufAllocator impl
  //

  BasicBufAllocator::Lock
  BasicBufAllocator::default_allocator_creation_mutex_;

  volatile SigAtomicType
  BasicBufAllocator::default_allocator_initialized_;

  BasicBufAllocator* volatile
  BasicBufAllocator::default_allocator_;

  BasicBufAllocator*
  BasicBufAllocator::get_default_allocator() /*throw (Gears::Exception)*/
  {
    if(!default_allocator_initialized_)
    {
      {
        BasicBufAllocator::Lock::WriteGuard guard(
          default_allocator_creation_mutex_);

        if(!default_allocator_)
        {
          default_allocator_ = new DefaultBufAllocator();
        }
      }
      default_allocator_initialized_ = true;
    }

    return default_allocator_;
  }

  inline void
  BasicBufAllocator::align_(size_t& number, size_t mask) noexcept
  {
    number += (-number) & mask;
  }

  size_t
  BasicBufAllocator::cached() const /*throw (Gears::Exception)*/
  {
    return 0;
  }

  void
  BasicBufAllocator::print_cached(std::ostream& ostr) const
    /*throw(Gears::Exception)*/
  {
    ostr << '0';
  }

  //
  // class DefaultBufAllocator impl
  //

  const size_t DefaultBufAllocator::DEF_ALIGN;

  DefaultBufAllocator::DefaultBufAllocator(size_t align_code)
    noexcept
    : MASK_((1 << align_code) - 1)
  {}

  BasicBufAllocator::Pointer
  DefaultBufAllocator::allocate(size_t& size)
    /*throw(Gears::Exception, OutOfMemory)*/
  {
    align_(size, MASK_);
    return new unsigned char[size];
  }

  void
  DefaultBufAllocator::deallocate(Pointer ptr, size_t size)
    noexcept
  {
    (void)size;
    assert(!(size & MASK_));
    delete [] static_cast<unsigned char*>(ptr);
  }

  //
  // class AlignBufAllocator
  //

  const size_t AlignBufAllocator::DEF_PTR_ALIGN;
  const size_t AlignBufAllocator::DEF_ALIGN;

  AlignBufAllocator::AlignBufAllocator(
    size_t ptr_align_code,
    size_t align_code)
    noexcept
    : ALIGN_(1 << ptr_align_code), MASK_((1 << align_code) - 1)
  {}

  BasicBufAllocator::Pointer
  AlignBufAllocator::allocate(size_t& size)
    /*throw(Gears::Exception, OutOfMemory)*/
  {
    static const char* FUN = "AlignBufAllocator::allocate()";

    align_(size, MASK_);
    void* ptr;
    if (int res = posix_memalign(&ptr, ALIGN_, size))
    {
      throw_errno_exception<OutOfMemory>(res, FUN,
        ": failed to allocate aligned memory of size ", size);
    }
    return ptr;
  }

  void
  AlignBufAllocator::deallocate(Pointer ptr, size_t size)
    noexcept
  {
    (void)size;
    assert(!(size & MASK_));
    free(ptr);
  }
}
