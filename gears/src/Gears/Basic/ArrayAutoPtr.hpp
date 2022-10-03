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

#ifndef GEARS_BASIC_ARRAYAUTOPTR_HPP
#define GEARS_BASIC_ARRAYAUTOPTR_HPP

#include <algorithm>

namespace Gears
{
  template<typename ElementType>
  class ArrayAutoPtr;

  /**
   * Auxiliary type to enable copies and assignments
   */
  template<typename U>
  class ArrayAutoPtrRef
  {
  public:
    U* u_ptr;

  private:
    ArrayAutoPtrRef(U* src) noexcept;

    friend class ArrayAutoPtr<U>;
  };

  /**
   * An ArrayAutoPtr provides semantic of
   * STRICT OWNERSHIP over an array pointer.
   */
  template<typename ElementType>
  class ArrayAutoPtr
  {
  public:
    /**
     * Constructor
     * @param size size of the array to allocate (zero - not to allocate)
     */
    explicit
    ArrayAutoPtr(size_t size = 0) noexcept;

    /**
     * Move constructor
     * Transforms ownership from src to the constructed object
     * @param src former owner of the array
     */
    ArrayAutoPtr(ArrayAutoPtr<ElementType>& src) noexcept;

    /**
     * Destructor
     * Deallocates owned array
     */
    ~ArrayAutoPtr() noexcept;

    /**
     * Assignment operator
     * Transforms ownership from src to the object
     * @param src former owner of the array
     */
    ArrayAutoPtr<ElementType>&
    operator=(ArrayAutoPtr<ElementType>& src) noexcept;

    /**
     * Accessor for the array
     * @return pointer to stored array
     */
    ElementType*
    get() const noexcept;

    /**
     * Accessor for element of the array
     * @param index index of the required element of the array
     * @return reference to element
     */
    ElementType&
    operator[](unsigned index) noexcept;

    /**
     * Accessor for constant element of the array
     * @param index index of the required element of the array
     * @return constant reference to element
     */
    const ElementType&
    operator[](unsigned index) const noexcept;

    /**
     * Releases ownership
     * @return previously stored pointer to the array
     */
    ElementType*
    release() noexcept;

    /**
     * Releases stored array (if any) and allocated a new one (if size is
     * positive)
     * @param size size of a new array
     */
    void
    reset(unsigned size) noexcept;

    /**
     * Releases stored array (if any) and resets the pointer with a new one.
     * May lead to problems (if ptr is not pointer to array of T).
     * @param ptr new pointer to hold
     */
    void
    unsafe_reset(ElementType* ptr) noexcept;

    /**
     * Never implemented thus usage will lead to error messages.
     */
    template<typename U>
    void
    unsafe_reset(U*) noexcept;

    /**
     * Swaps pointers of the object and src
     * @param src another object to swap pointers with
     */
    void
    swap(ArrayAutoPtr<ElementType>& src) noexcept;

    /**X
     * Special conversions to enable copies and assignments
     * This allows constructs such as
     * ArrayAutoPtr<Class1>  func_returning_ArrayAutoPtr(.....);
     */

    ArrayAutoPtr(ArrayAutoPtrRef<ElementType> src) noexcept;

    ArrayAutoPtr&
    operator=(ArrayAutoPtrRef<ElementType> rh) noexcept;

    operator ArrayAutoPtrRef<ElementType>() noexcept;

  private:
    ElementType* ptr_;
  };

  typedef ArrayAutoPtr<char> ArrayChar;
  typedef ArrayAutoPtr<unsigned char> ArrayByte;
  typedef ArrayAutoPtr<wchar_t> ArrayWChar;
}

//
// Implementation
//
namespace Gears
{
  //
  // ArrayAutoPtrRef class
  //

  template<typename U>
  ArrayAutoPtrRef<U>::ArrayAutoPtrRef(U* src) noexcept
    : u_ptr(src)
  {}

  //
  // ArrayAutoPtr class
  //

  template<typename ElementType>
  ElementType*
  ArrayAutoPtr<ElementType>::get() const noexcept
  {
    return ptr_;
  }

  template<typename ElementType>
  ElementType&
  ArrayAutoPtr<ElementType>::operator [](unsigned index) noexcept
  {
    return ptr_[index];
  }

  template<typename ElementType>
  const ElementType&
  ArrayAutoPtr<ElementType>::operator[](unsigned index) const noexcept
  {
    return ptr_[index];
  }

  template<typename ElementType>
  ElementType*
  ArrayAutoPtr<ElementType>::release() noexcept
  {
    ElementType* ptr(ptr_);
    ptr_ = 0;
    return ptr;
  }

  template<typename ElementType>
  void
  ArrayAutoPtr<ElementType>::unsafe_reset(ElementType* ptr) noexcept
  {
    if(ptr_ != ptr)
    {
      if(ptr_)
      {
        delete [] ptr_;
      }

      ptr_ = ptr;
    }
  }

  template<typename ElementType>
  void
  ArrayAutoPtr<ElementType>::reset(unsigned size) noexcept
  {
    ElementType* ptr = size ? new ElementType[size] : 0;

    if(ptr_)
    {
      delete [] ptr_;
    }

    ptr_ = ptr;
  }

  template<typename ElementType>
  void
  ArrayAutoPtr<ElementType>::swap(ArrayAutoPtr<ElementType>& src) noexcept
  {
    std::swap(ptr_, src.ptr_);
  }

  template<typename ElementType>
  ArrayAutoPtr<ElementType>::ArrayAutoPtr(size_t size) noexcept
    : ptr_(size ? new ElementType[size] : 0)
  {}

  template<typename ElementType>
  ArrayAutoPtr<ElementType>::ArrayAutoPtr(ArrayAutoPtr<ElementType>& src) noexcept
    : ptr_(src.release())
  {}

  template<typename ElementType>
  ArrayAutoPtr<ElementType>::~ArrayAutoPtr() noexcept
  {
    unsafe_reset(0);
  }

  template<typename ElementType>
  ArrayAutoPtr<ElementType>&
  ArrayAutoPtr<ElementType>::operator=(ArrayAutoPtr<ElementType>& src) noexcept
  {
    if(this != &src)
    {
      unsafe_reset(src.release());
    }

    return *this;
  }

  template<typename ElementType>
  ArrayAutoPtr<ElementType>::ArrayAutoPtr(ArrayAutoPtrRef<ElementType> src) noexcept
    : ptr_(src.u_ptr)
  {}

  template<typename ElementType>
  ArrayAutoPtr<ElementType>&
  ArrayAutoPtr<ElementType>::operator=(ArrayAutoPtrRef<ElementType> rh) noexcept
  {
    unsafe_reset(rh.u_ptr);
    return *this;
  }

  template<typename ElementType>
  ArrayAutoPtr<ElementType>::operator ArrayAutoPtrRef<ElementType>() noexcept
  {
    return ArrayAutoPtrRef<ElementType>(release());
  }
}

#endif /*GEARS_BASIC_ARRAYAUTOPTR_HPP*/
