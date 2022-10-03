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

#ifndef GEARS_MEMBUF_HPP
#define GEARS_MEMBUF_HPP

#include "BufAllocator.hpp"

#ifdef DEV_DEBUG
#ifndef DEV_MEMBUF_BOUNDS
#define DEV_MEMBUF_BOUNDS 1024
#endif
#else
#ifdef DEV_MEMBUF_BOUNDS
#undef DEV_MEMBUF_BOUNDS
#endif
#define DEV_MEMBUF_BOUNDS 0
#endif

namespace Gears
{
  /**
   * Memory buffer entity.
   * Give capacity logic and size of buffer used by user.
   * Allow copy and move construction and move assignment.
   */
  class MemBuf
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, DescriptiveException);
    DECLARE_GEARS_EXCEPTION(OutOfMemory, Exception);
    DECLARE_GEARS_EXCEPTION(RangeError, Exception);

    /**
     * Construct empty object without memory allocation.
     * @param allocator Memory allocator will be using for allocation,
     * if not specified using special default allocator.
     */
    explicit
    MemBuf(BasicBufAllocator* allocator = 0) noexcept;

    /**
     * Construct memory buffer and mark all size bytes as used.
     * @param size bytes to be allocated.
     * @param allocator Memory allocator will be using for allocation,
     * if not specified using special default allocator.
     */
    explicit
    MemBuf(std::size_t size, BasicBufAllocator* allocator = 0)
      /*throw (OutOfMemory)*/;

    /**
     * Copy constructor
     * @param right copying content
     * Physically allocated memory enough for store right parameter size(),
     * bytes, but have not guarantee for right capacity()!
     */
    MemBuf(const MemBuf& right) /*throw (OutOfMemory)*/;

    /**
     * Copy constructor with allocator specified
     * @param right copying content
     * @param allocator Memory allocator will be using for allocation,
     * if not specified using special default allocator.
     */
    MemBuf(const MemBuf& right, BasicBufAllocator* allocator)
      /*throw (OutOfMemory)*/;

    /**
     * Move constructor
     * @param right moving content
     */
    MemBuf(MemBuf&& right) noexcept;

    /**
     * Construct MemBuf object that able to store size bytes,
     * initialized content via ptr
     * @param ptr pointer to initial data for new MemBuf.
     * @param size bytes should copy from ptr source.
     * @param allocator Memory allocator will be using for allocation,
     * if not specified using special default allocator.
     */
    MemBuf(const void* ptr, std::size_t size,
      BasicBufAllocator* allocator = 0)
      /*throw (RangeError, OutOfMemory)*/;

    /**
     * Frees allocated memory, while debug mode on, checks
     * buffer boundaries for buffer overrun.
     */
    ~MemBuf() noexcept;

    /**
     * @return true if buffer size used by user is zero.
     */
    bool
    empty() const noexcept;

    /**
     * Free allocated memory, set logical size and capacity
     * to zero.
     */
    void
    clear() noexcept;

    /**
     * @return buffer size used by user.
     */
    std::size_t
    size() const noexcept;

    /**
     * @return really allocated memory by this MemBuf object.
     */
    std::size_t
    capacity() const noexcept;

    /**
     * @param offset from begin of user data in bytes
     * @return pointer on user data.
     */
    void*
    data(std::size_t offset = 0) noexcept;

    /**
     * @param offset from begin of user data in bytes
     * @return pointer on user data.
     * Constant version.
     */
    const void*
    data(std::size_t offset = 0) const noexcept;

    /**
     * @param offset from begin of user data in bytes
     * @return pointer on user data.
     */
    template <typename DataType>
    DataType*
    get(std::size_t offset = 0) noexcept;

    /**
     * @param offset from begin of user data in bytes
     * @return pointer on user data.
     * Constant version.
     */
    template <typename DataType>
    const DataType*
    get(std::size_t offset = 0) const noexcept;

    /**
     * Assigns new content for the buffer.
     * @param ptr pointer to data for MemBuf.
     * @param size bytes should copy from ptr source.
     */
    void
    assign(const void* ptr, std::size_t size)
      /*throw (Gears::Exception, OutOfMemory)*/;

    /**
     * Simply allocated new memory buffer.
     * Old buffer content is lost.
     * @param size in bytes of new memory buffer.
     * Set capacity and user size to parameter value.
     */
    void
    alloc(std::size_t size) /*throw (Gears::Exception, OutOfMemory)*/;

    /**
     * Modifying logical buffer size.
     * Doesn't allocate physical memory
     * @param size must be less than or equal to MemBuf capacity.
     * Throw RangeError, if you exceed really allocate memory.
     */
    void
    resize(std::size_t size) /*throw (RangeError)*/;

    /**
     * swap between this object and the other
     * @param right object
     */
    void
    swap(MemBuf& right) noexcept;

    /**
     * Assignment operator is prohibited.
     */
    MemBuf&
    operator =(MemBuf& right) noexcept = delete;

    /**
     * Move operator. Calls swap().
     * @param right buffer will move to this object
     * @return reference to this object
     */
    MemBuf&
    operator =(MemBuf&& right) noexcept;

    /**
     * @return pointer to memory allocator
     */
    BasicBufAllocator_var
    get_allocator() noexcept;

  private:
    mutable BasicBufAllocator_var allocator_;

    BasicBufAllocator::Pointer ptr_;
    //! memory size used for store user structures.
    std::size_t size_;
    //! really allocated bytes.
    std::size_t capacity_;
  };

  /**
   * MemBuf with predefined allocator value
   */
  template <typename AllocatorValue>
  class MemBufTmpl
  {
  public:
    /**
     * Constructor
     */
    template <typename... T>
    explicit
    MemBufTmpl(T&&... data) /*throw (Gears::Exception)*/;

    /**
     * Aggregated MemBuf
     * @return aggregated MemBuf
     */
    MemBuf&
    membuf() noexcept;

  private:
    MemBuf mem_buf_;
  };


  /**
   * MemBuf with reference counting.
   * MemBuf is either Generics::MemBuf or const Generics::MemBuf
   */
  template <typename MemBuf>
  class SmartTmplMemBuf :
    public AtomicRefCountable
  {
  public:
    /**
     * Constructor
     */
    template <typename... T>
    explicit
    SmartTmplMemBuf(T&&... data) /*throw (Gears::Exception)*/;

    /**
     * Aggregated MemBuf
     * @return aggregated MemBuf
     */
    MemBuf&
    membuf() noexcept;

    /**
     * Aggregated MemBuf
     * @return aggregated MemBuf
     */
    const MemBuf&
    membuf() const noexcept;

  protected:
    /**
     * Destructor
     */
    virtual
    ~SmartTmplMemBuf() noexcept = default;

  private:
    MemBuf mem_buf_;
  };

  typedef SmartTmplMemBuf<MemBuf> SmartMemBuf;
  typedef SmartTmplMemBuf<const MemBuf> ConstSmartMemBuf;

  typedef IntrusivePtr<SmartMemBuf> SmartMemBuf_var;
  typedef IntrusivePtr<ConstSmartMemBuf>
    ConstSmartMemBuf_var;

  /**
   * SmartMemBuf with predefined allocator value.
   */
  template <typename MemBuf, typename AllocatorValue>
  class SmartMemBufTmpl: public SmartTmplMemBuf<MemBuf>
  {
  public:
    /**
     * Constructor
     */
    template <typename... T>
    explicit
    SmartMemBufTmpl(T&&... data) /*throw (Gears::Exception)*/;

  protected:
    virtual
    ~SmartMemBufTmpl() noexcept = default;
  };

  /**
   * Functor may be used for Generics::BoundedMap container,
   * for example.
   */
  struct ConstSmartMemBufSize
  {
    std::size_t
    operator()(ConstSmartMemBuf* smb) const noexcept;
  };


  /**
   * Transfer ownership of underlying MemBuf data from SmartMemBuf
   * to newly created ConstSmartMemBuf disallowing any future modification
   * of the buffer. Thread safe.
   */
  ConstSmartMemBuf_var
  transfer_membuf(SmartMemBuf* ptr) /*throw (Gears::Exception)*/;

  struct MemBufBoundsGuard
  {
    explicit
    MemBufBoundsGuard(size_t bounds) noexcept;
  };
}

//////////////////////////////////////////////////////////////////////////
// Inlines
//////////////////////////////////////////////////////////////////////////

namespace Gears
{
  //
  // MemBuf class
  //
  inline bool
  MemBuf::empty() const noexcept
  {
    return !size_;
  }

  inline void*
  MemBuf::data(std::size_t offset) noexcept
  {
    return static_cast<unsigned char*>(ptr_) + offset + DEV_MEMBUF_BOUNDS;
  }

  inline const void*
  MemBuf::data(std::size_t offset) const noexcept
  {
    return static_cast<unsigned char*>(ptr_) + offset + DEV_MEMBUF_BOUNDS;
  }

  template <typename DataType>
  DataType*
  MemBuf::get(std::size_t offset) noexcept
  {
    return static_cast<DataType*>(data(offset));
  }

  template <typename DataType>
  const DataType*
  MemBuf::get(std::size_t offset) const noexcept
  {
    return static_cast<const DataType*>(data(offset));
  }

  inline std::size_t
  MemBuf::size() const noexcept
  {
    return size_;
  }

  inline std::size_t
  MemBuf::capacity() const noexcept
  {
    return capacity_ - 2 * DEV_MEMBUF_BOUNDS;
  }

  inline
  BasicBufAllocator_var
  MemBuf::get_allocator() noexcept
  {
    return allocator_;
  }

  //
  // MemBufTmpl class
  //
  template <typename AllocatorValue>
  template <typename... T>
  MemBufTmpl<AllocatorValue>::MemBufTmpl(T&&... data) /*throw (Gears::Exception)*/
    : mem_buf_(std::forward<T>(data)..., AllocatorValue::ALLOCATOR)
  {}

  template <typename AllocatorValue>
  MemBuf&
  MemBufTmpl<AllocatorValue>::membuf() noexcept
  {
    return mem_buf_;
  }

  //
  // SmartTmplMemBuf class
  //
  template <typename MemBuf>
  template <typename... T>
  SmartTmplMemBuf<MemBuf>::SmartTmplMemBuf(T&&... data)
    /*throw (Gears::Exception)*/
    : mem_buf_(std::forward<T>(data)...)
  {}

  template <typename MemBuf>
  MemBuf&
  SmartTmplMemBuf<MemBuf>::membuf() noexcept
  {
    return mem_buf_;
  }

  template <typename MemBuf>
  const MemBuf&
  SmartTmplMemBuf<MemBuf>::membuf() const noexcept
  {
    return mem_buf_;
  }

  //
  // SmartMemBufTmpl class
  //
  template <typename MemBuf, typename AllocatorValue>
  template <typename... T>
  SmartMemBufTmpl<MemBuf, AllocatorValue>::SmartMemBufTmpl(T&&... data)
    /*throw (Gears::Exception)*/
    : SmartTmplMemBuf<MemBuf>(std::forward<T>(data)...,
      AllocatorValue::ALLOCATOR)
  {}

  //
  // ConstSmartMemBufSize class
  //
  inline std::size_t
  ConstSmartMemBufSize::operator()(ConstSmartMemBuf* smb) const noexcept
  {
    return smb->membuf().size();
  }

  static MemBufBoundsGuard membuf_bounds_guard(DEV_MEMBUF_BOUNDS);
}

#endif
