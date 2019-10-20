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

#ifndef GEARS_ALLOCATOR_HPP
#define GEARS_ALLOCATOR_HPP

#include "Exception.hpp"

namespace Gears
{
  template<
    typename Elem,
    const size_t SIZE,
    typename Buffer,
    typename BufferInitializer = Buffer>
  class BasicFixedBufferAllocator : public std::allocator<Elem>
  {
  public:
    typedef std::allocator<Elem> Allocator;

    /**
     * Constructor without parameters
     */
    BasicFixedBufferAllocator() throw ();

    /**
     * Constructor with buffer_ init value
     * @param buffer_initializer initializer for buffer_
     */
    BasicFixedBufferAllocator(BufferInitializer buffer_initializer) throw ();

    /**
     * Allocation function
     * Allows to allocate SIZE bytes one time in a row
     * @param size should be equal to SIZE
     * @return pointer to size_ternal buffer
     */
    typename Allocator::pointer
    allocate(typename Allocator::size_type size, const void* = 0)
      throw ();

    /**
     * Deallocation function
     * Deallocates previously allocated memory
     * @param ptr should be equal to the pointer returned by allocate()
     * @param size should be equal to SIZE
     */
    void
    deallocate(typename Allocator::pointer ptr,
      typename Allocator::size_type size) throw ();

  private:
    Buffer buffer_;
    bool allocated_;
  };

  /**
   * Simple buffer allocator
   * Allows a single allocation on preallocated buffer
   */
  template <typename Elem, const size_t SIZE>
  class FixedBufferAllocator :
    public BasicFixedBufferAllocator<Elem, SIZE, Elem*>
  {
  public:
    /**
     * Constructor
     * @param buffer preallocated buffer of size not less than SIZE
     */
    FixedBufferAllocator(Elem* buffer) throw ();
  };

  template <typename Elem, const size_t SIZE, typename Initializer>
  class ArrayBuffer
  {
  public:
    ArrayBuffer(Initializer initializer = Initializer()) throw ();

    operator Elem*() throw ();

  private:
    Elem buffer_[SIZE];
  };

  /**
   * Simple stack allocator
   * Required for disuse of heap for OutputStream
   */
  template <typename Elem, const size_t SIZE>
  class StackAllocator :
    public BasicFixedBufferAllocator<
      Elem, SIZE, ArrayBuffer<Elem, SIZE, size_t>, size_t>
  {
  public:
    /**
     * Constructor
     */
    StackAllocator(size_t allocator_initializer) throw ();
  };
} /*Gears*/

#include "Allocator.tpp"

#endif /*GEARS_ALLOCATOR_HPP*/
