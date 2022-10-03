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

#include <iostream>
#include <Gears/Basic/Errno.hpp>

#include "ThreadValue.hpp"

namespace Gears
{
  template<typename Type>
  ThreadValue<Type>::ThreadValue()
    /*throw(Exception)*/
  {
    int res = ::pthread_key_create(&key_, destroy_function_);
    if(res != 0)
    {
      throw_errno_exception<Exception>(res, "ThreadValue::ThreadValue()");
    }
  }

  template<typename Type>
  ThreadValue<Type>::~ThreadValue() noexcept
  {
    try
    {
      reset(0); // set NULL value (required by pthread_key_delete spec)
    }
    catch(const Gears::Exception& ex)
    {
      std::cerr << "ThreadValue::~ThreadValue(): caught eh::Exception: " <<
        ex.what();
    }

    int res = ::pthread_key_delete(key_);
    if(res != 0)
    {
      char message[1024];
      ErrnoHelper::compose_safe(message, sizeof(message), res, "ThreadValue::~ThreadValue()");
      std::cerr << message << std::endl;
    }
  }

  template<typename Type>
  void
  ThreadValue<Type>::reset(Type* value)
    /*throw(Exception)*/
  {
    reset_();

    int res = ::pthread_setspecific(key_, value);
    if(res != 0)
    {
      throw_errno_exception<Exception>(res, "ThreadValue::set()");
    }
  }

  template<typename Type>
  Type*
  ThreadValue<Type>::get() const noexcept
  {
    return static_cast<Type*>(::pthread_getspecific(key_));
  }

  template<typename Type>
  Type*
  ThreadValue<Type>::release() /*throw(Exception)*/
  {
    Type* ptr = get();

    if(ptr)
    {
      int res = ::pthread_setspecific(key_, 0);
      if(res != 0)
      {
        delete ptr;
        throw_errno_exception<Exception>(res, "ThreadValue::release()");
      }
    }

    return ptr;
  }

  template<typename Type>
  void
  ThreadValue<Type>::reset_() noexcept
  {
    // destroy value without set null
    Type* obj = get();
    if(obj)
    {
      delete obj;
    }
  }

  template<typename Type>
  void
  ThreadValue<Type>::destroy_function_(void* obj)
  {
    delete static_cast<Type*>(obj);
  }
}
