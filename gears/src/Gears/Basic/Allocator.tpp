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

namespace Gears
{
  /* BasicFixedBufferAllocator impl */
  template<typename Elem, const size_t SIZE, typename Buffer,
    typename BufferInitializer>
  BasicFixedBufferAllocator<Elem, SIZE, Buffer, BufferInitializer>::
  BasicFixedBufferAllocator() noexcept
    : allocated_(false)
  {
    buffer_[SIZE - 1] = '\0';
  }

  template<typename Elem, const size_t SIZE, typename Buffer,
    typename BufferInitializer>
  BasicFixedBufferAllocator<Elem, SIZE, Buffer, BufferInitializer>::
  BasicFixedBufferAllocator(
    BufferInitializer buffer_initializer) noexcept
    : buffer_(buffer_initializer), allocated_(false)
  {
    buffer_[SIZE - 1] = '\0';
  }

  template<typename Elem, const size_t SIZE, typename Buffer,
    typename BufferInitializer>
  typename BasicFixedBufferAllocator<Elem, SIZE, Buffer, BufferInitializer>::
    Allocator::value_type*
  BasicFixedBufferAllocator<Elem, SIZE, Buffer, BufferInitializer>::allocate(
    typename Allocator::size_type size, const void*) noexcept
  {
    if (allocated_ || size >= SIZE)
    {
      return 0;
    }
    allocated_ = true;
    return buffer_;
  }

  template<typename Elem, const size_t SIZE, typename Buffer,
    typename BufferInitializer>
  void
  BasicFixedBufferAllocator<Elem, SIZE, Buffer, BufferInitializer>::deallocate(
    typename Allocator::value_type* ptr, typename Allocator::size_type size)
    noexcept
  {
    if (!allocated_ || ptr != buffer_ || size >= SIZE)
    {
      return;
    }
    allocated_ = false;
  }

  /* ArrayBuffer class */
  template<typename Elem, const size_t SIZE, typename Initializer>
  ArrayBuffer<Elem, SIZE, Initializer>::ArrayBuffer(
    Initializer /*initializer*/) noexcept
  {}

  template<typename Elem, const size_t SIZE, typename Initializer>
  ArrayBuffer<Elem, SIZE, Initializer>::operator Elem*() noexcept
  {
    return buffer_;
  }

  /* FixedBufferAllocator impl */

  template<typename Elem, const size_t SIZE>
  FixedBufferAllocator<Elem, SIZE>::FixedBufferAllocator(Elem* buffer) noexcept
    : BasicFixedBufferAllocator<Elem, SIZE, Elem*>(buffer)
  {}

  /* StackAllocator impl */

  template<typename Elem, const size_t SIZE>
  StackAllocator<Elem, SIZE>::StackAllocator(size_t /*allocator_initializer*/)
    noexcept
  {}
}
