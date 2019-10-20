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
    IntrusivePtr() throw();

    IntrusivePtr(ObjectType* ptr) throw();

    IntrusivePtr(const IntrusivePtr<ObjectType>& s_ptr) throw();

    template<typename OtherType>
    IntrusivePtr(const IntrusivePtr<OtherType>& s_ptr) throw();

    ~IntrusivePtr() throw();

    IntrusivePtr<ObjectType>&
    operator=(ObjectType* ptr) throw();

    IntrusivePtr<ObjectType>&
    operator=(const IntrusivePtr<ObjectType>& s_ptr) throw();

    template<typename OtherType>
    IntrusivePtr<ObjectType>&
    operator=(const IntrusivePtr<OtherType>& s_ptr) throw();

    void
    swap(IntrusivePtr<ObjectType>& sptr) throw();

    // accessors
    operator ObjectType*() const throw();

    ObjectType* operator->() const throw();

    ObjectType* in() const throw();

    ObjectType* retn() throw();

  private:
    ObjectType* ptr_;
  };

  template<typename ObjectType>
  ObjectType*
  add_ref(ObjectType* ptr) throw();

  template<typename ObjectType>
  ObjectType*
  add_ref(const IntrusivePtr<ObjectType>& ptr) throw();
}

namespace Gears
{
  template<typename ObjectType>
  IntrusivePtr<ObjectType>::IntrusivePtr() throw()
    : ptr_(0)
  {}

  template<typename ObjectType>
  IntrusivePtr<ObjectType>::IntrusivePtr(ObjectType* ptr) throw()
    : ptr_(ptr)
  {}

  template<typename ObjectType>
  IntrusivePtr<ObjectType>::IntrusivePtr(
    const IntrusivePtr<ObjectType>& s_ptr)
    throw()
    : ptr_(add_ref(s_ptr.in()))
  {}

  template<typename ObjectType>
  template<typename OtherType>
  IntrusivePtr<ObjectType>::IntrusivePtr(
    const IntrusivePtr<OtherType>& s_ptr)
    throw()
    : ptr_(add_ref(s_ptr.in()))
  {}

  template<typename ObjectType>
  IntrusivePtr<ObjectType>::~IntrusivePtr() throw()
  {
    if(ptr_)
    {
      ptr_->remove_ref();
    }
  }

  template<typename ObjectType>
  IntrusivePtr<ObjectType>&
  IntrusivePtr<ObjectType>::operator=(ObjectType* ptr) throw()
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
    throw()
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
    throw()
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
    throw()
  {
    ObjectType* new_ptr(sptr.in());
    sptr.retn();
    sptr = ptr_;
    ptr_ = new_ptr;
  }

  template<typename ObjectType>
  IntrusivePtr<ObjectType>::operator ObjectType*() const throw()
  {
    return ptr_;
  }

  template<typename ObjectType>
  ObjectType*
  IntrusivePtr<ObjectType>::operator->() const throw()
  {
    return ptr_;
  }

  template<typename ObjectType>
  ObjectType*
  IntrusivePtr<ObjectType>::in() const throw()
  {
    return ptr_;
  }

  template<typename ObjectType>
  ObjectType*
  IntrusivePtr<ObjectType>::retn() throw()
  {
    ObjectType* ret(ptr_);
    ptr_ = 0;
    return ret;
  }

  template<typename ObjectType>
  ObjectType*
  add_ref(ObjectType* ptr) throw()
  {
    if(ptr)
    {
      ptr->add_ref();
    }

    return ptr;
  }

  template<typename ObjectType>
  ObjectType*
  add_ref(const IntrusivePtr<ObjectType>& ptr) throw()
  {
    return add_ref(ptr.in());
  }
}

#endif // __cplusplus

#endif // GEARS_INTRUSIVEPTR_HPP
