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

#ifndef GEARS_BUFALLOCATOR_HPP_
#define GEARS_BUFALLOCATOR_HPP_

#include <ostream>

#include "Exception.hpp"
#include "AtomicCounter.hpp"
#include "AtomicRefCountable.hpp"
#include "IntrusivePtr.hpp"
#include "Lock.hpp"

namespace Gears
{
  struct BasicBufAllocator: public AtomicRefCountable
  {
    /**
     * Exception means free memory exhausted.
     */
    DECLARE_GEARS_EXCEPTION(OutOfMemory, DescriptiveException);

    //class OutOfMemory_tag_ {};
    //typedef Gears::CompositeException<OutOfMemory_tag_, DescriptiveException> OutOfMemory;

    typedef void* Pointer;
    typedef const void* ConstPointer;

    /**
     * @param size mean request for size bytes for code needs,
     * but allocator can give >= size value bytes.
     * Really allocated value return to client by set size
     * in this case.
     * @return pointer to begin allocated memory, size available
     * for using returns in size.
     */
    virtual
    Pointer
    allocate(size_t& size) throw (Exception, OutOfMemory) = 0;

    /**
     * All size T objects in the area pointed
     * by
     * @param ptr shall be destroyed prior to
     * this call.
     * @param size shall match the
     * value passed to allocate to
     * obtain this memory. Does not
     * throw exceptions. [Note: p shall not be null.]
     */
    virtual void
    deallocate(Pointer ptr, size_t size) throw () = 0;

    /**
     * Approximated cached memory size.
     * @return cached memory size.
     */
    virtual size_t
    cached() const
      throw (Gears::Exception);

    /**
     * Print detailed approximate cached memory information.
     */
    virtual void
    print_cached(std::ostream& ostr) const
      throw (Gears::Exception);

    /**
     * @return application level default allocator. Usually
     * simple new/delete behavior.
     */
    static BasicBufAllocator*
    get_default_allocator() throw (Gears::Exception);

  protected:
    /**
     * protected destructor because reference counting.
     */
    virtual
    ~BasicBufAllocator() throw() = default;
      
    /**
     * Align number to 2^mask number
     * @param number is number to align
     * @param mask power of 2 to be aligned.
     */ 
    static void
    align_(size_t& number, size_t mask) throw();

  private:
    typedef StaticInitializedMutex Lock;

  private:
    /// Application level default allocator object.
    static Lock default_allocator_creation_mutex_;
    static volatile SigAtomicType default_allocator_initialized_;
    static BasicBufAllocator* volatile default_allocator_;
  };

  typedef IntrusivePtr<BasicBufAllocator>
    BasicBufAllocator_var;

  class DefaultBufAllocator:
    public BasicBufAllocator
  {
   public:
    /// default power of 2 for alignment value.
    static const size_t DEF_ALIGN = 10;

    explicit
    DefaultBufAllocator(size_t align_code = DEF_ALIGN) throw ();

    /**
     * Align request size bytes according to MASK_
     * and allocate memory.
     * @param size at minimum memory to be allocated.
     * @return pointer to allocated memory block
     */
    virtual Pointer
    allocate(size_t& size) throw (Gears::Exception, OutOfMemory);

    /**
     * Deallocate 
     * @param ptr pointer to releasing memory block.
     * @param size not used in this allocator.
     */
    virtual void
    deallocate(Pointer ptr, size_t size) throw ();

  protected:
    /**
     * Destructor
     */
    virtual
    ~DefaultBufAllocator() throw () = default;

  private:
    const size_t MASK_;
  };

  /**
   * Returns aligned pointer
   */
  class AlignBufAllocator: public BasicBufAllocator
  {
   public:
    /// default power of 2 for alignment value.
    static const size_t DEF_PTR_ALIGN = 9;
    static const size_t DEF_ALIGN = 10;

    explicit
    AlignBufAllocator(
      size_t ptr_align_code = DEF_PTR_ALIGN,
      size_t align_code = DEF_ALIGN)
      throw ();

    /**
     * Align request size bytes according to MASK_
     * and allocate memory aligned according to ALIGN_.
     * @param size at minimum memory to be allocated.
     * @return pointer to allocated memory block
     */
    virtual
    Pointer
    allocate(size_t& size) throw (Gears::Exception, OutOfMemory);

    /**
     * Deallocate 
     * @param ptr pointer to releasing memory block.
     * @param size not used in this allocator.
     */
    virtual
    void
    deallocate(Pointer ptr, size_t size) throw ();

  protected:
    /**
     * Destructor
     */
    virtual
    ~AlignBufAllocator() throw () = default;

  private:
    const size_t ALIGN_;
    const size_t MASK_;
  };
}

#endif /*GEARS_BUFALLOCATOR_HPP_*/
