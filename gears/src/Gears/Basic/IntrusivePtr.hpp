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

#ifndef GEARS_INTRUSIVEPTR_HPP
#define GEARS_INTRUSIVEPTR_HPP

#ifdef __cplusplus

namespace Gears
{
  template<typename ObjectType>
  class IntrusivePtr
  {
  public:
    typedef ObjectType value_type;

  public:
    IntrusivePtr() noexcept;

    IntrusivePtr(ObjectType* ptr) noexcept;

    IntrusivePtr(const IntrusivePtr<ObjectType>& s_ptr) noexcept;

    template<typename OtherType>
    IntrusivePtr(const IntrusivePtr<OtherType>& s_ptr) noexcept;

    ~IntrusivePtr() noexcept;

    IntrusivePtr<ObjectType>&
    operator=(ObjectType* ptr) noexcept;

    IntrusivePtr<ObjectType>&
    operator=(const IntrusivePtr<ObjectType>& s_ptr) noexcept;

    template<typename OtherType>
    IntrusivePtr<ObjectType>&
    operator=(const IntrusivePtr<OtherType>& s_ptr) noexcept;

    void
    swap(IntrusivePtr<ObjectType>& sptr) noexcept;

    // accessors
    operator ObjectType*() const noexcept;

    ObjectType* operator->() const noexcept;

    ObjectType* in() const noexcept;

    ObjectType* retn() noexcept;

  private:
    ObjectType* ptr_;
  };

  template<typename ObjectType>
  ObjectType*
  add_ref(ObjectType* ptr) noexcept;

  template<typename ObjectType>
  ObjectType*
  add_ref(const IntrusivePtr<ObjectType>& ptr) noexcept;
}

namespace Gears
{
  template<typename ObjectType>
  IntrusivePtr<ObjectType>::IntrusivePtr() noexcept
    : ptr_(0)
  {}

  template<typename ObjectType>
  IntrusivePtr<ObjectType>::IntrusivePtr(ObjectType* ptr) noexcept
    : ptr_(ptr)
  {}

  template<typename ObjectType>
  IntrusivePtr<ObjectType>::IntrusivePtr(
    const IntrusivePtr<ObjectType>& s_ptr)
    noexcept
    : ptr_(add_ref(s_ptr.in()))
  {}

  template<typename ObjectType>
  template<typename OtherType>
  IntrusivePtr<ObjectType>::IntrusivePtr(
    const IntrusivePtr<OtherType>& s_ptr)
    noexcept
    : ptr_(add_ref(s_ptr.in()))
  {}

  template<typename ObjectType>
  IntrusivePtr<ObjectType>::~IntrusivePtr() noexcept
  {
    if(ptr_)
    {
      ptr_->remove_ref();
    }
  }

  template<typename ObjectType>
  IntrusivePtr<ObjectType>&
  IntrusivePtr<ObjectType>::operator=(ObjectType* ptr) noexcept
  {
    if(ptr_)
    {
      ptr_->remove_ref();
    }
    ptr_ = ptr;

    return *this;
  }

  template<typename ObjectType>
  IntrusivePtr<ObjectType>&
  IntrusivePtr<ObjectType>::operator=(
    const IntrusivePtr<ObjectType>& s_ptr)
    noexcept
  {
    ObjectType* new_ptr(add_ref(s_ptr.in()));
    if(ptr_)
    {
      ptr_->remove_ref();
    }
    ptr_ = new_ptr;

    return *this;
  }


  template<typename ObjectType>
  template<typename OtherType>
  IntrusivePtr<ObjectType>&
  IntrusivePtr<ObjectType>::operator=(const IntrusivePtr<OtherType>& s_ptr)
    noexcept
  {
    OtherType* new_ptr(add_ref(s_ptr.in()));
    if(ptr_)
    {
      ptr_->remove_ref();
    }
    ptr_ = new_ptr;

    return *this;
  }

  template <typename ObjectType>
  void
  IntrusivePtr<ObjectType>::swap(IntrusivePtr<ObjectType>& sptr)
    noexcept
  {
    ObjectType* new_ptr(sptr.in());
    sptr.retn();
    sptr = ptr_;
    ptr_ = new_ptr;
  }

  template<typename ObjectType>
  IntrusivePtr<ObjectType>::operator ObjectType*() const noexcept
  {
    return ptr_;
  }

  template<typename ObjectType>
  ObjectType*
  IntrusivePtr<ObjectType>::operator->() const noexcept
  {
    return ptr_;
  }

  template<typename ObjectType>
  ObjectType*
  IntrusivePtr<ObjectType>::in() const noexcept
  {
    return ptr_;
  }

  template<typename ObjectType>
  ObjectType*
  IntrusivePtr<ObjectType>::retn() noexcept
  {
    ObjectType* ret(ptr_);
    ptr_ = 0;
    return ret;
  }

  template<typename ObjectType>
  ObjectType*
  add_ref(ObjectType* ptr) noexcept
  {
    if(ptr)
    {
      ptr->add_ref();
    }

    return ptr;
  }

  template<typename ObjectType>
  ObjectType*
  add_ref(const IntrusivePtr<ObjectType>& ptr) noexcept
  {
    return add_ref(ptr.in());
  }
}

#endif // __cplusplus

#endif // GEARS_INTRUSIVEPTR_HPP
