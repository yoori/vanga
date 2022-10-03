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

#ifndef GEARS_THREAD_VALUE_HPP
#define GEARS_THREAD_VALUE_HPP

#include <pthread.h>
#include <Gears/Basic/Exception.hpp>

namespace Gears
{
  template<typename Type>
  class ThreadValue
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, DescriptiveException);

    ThreadValue() /*throw(Exception)*/;

    ~ThreadValue() noexcept;

    void
    reset(Type* value) /*throw(Exception)*/;

    Type*
    get() const noexcept;

    Type*
    release() /*throw(Exception)*/;

  private:
    void
    reset_() noexcept;

    static
    void
    destroy_function_(void* obj);

  private:
    pthread_key_t key_;
  };
}

#include "ThreadValue.tpp"

#endif /*GEARS_THREAD_VALUE_HPP*/
