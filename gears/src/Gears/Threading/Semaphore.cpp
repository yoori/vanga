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

#include "Semaphore.hpp"

namespace Gears
{
#if __APPLE__
  //
  // Condition based implementation
  //
  Semaphore::Semaphore(int count) /*throw(Exception)*/
    : value_(count)
  {}

  Semaphore::~Semaphore() noexcept
  {}

  void
  Semaphore::acquire() /*throw(Exception)*/
  {
    ConditionGuard guard(lock_, cond_);

    while(value_ == 0)
    {
      guard.wait();
    }

    --value_;
  }

  bool
  Semaphore::try_acquire() /*throw(Exception)*/
  {
    ConditionGuard guard(lock_, cond_);

    if(value_ == 0)
    {
      return false;
    }

    --value_;

    return true;
  }

  void
  Semaphore::release() /*throw(Exception)*/
  {
    Mutex::WriteGuard guard(lock_);
    ++value_;
    cond_.signal();
  }

  int
  Semaphore::value() /*throw(Exception)*/
  {
    Mutex::ReadGuard guard(lock_);
    return static_cast<int>(value_);
  }

#else

  Semaphore::Semaphore(int count) /*throw(Exception)*/
  {
    if(::sem_init(&semaphore_, 0, count))
    {
      throw_errno_exception<Exception>(
        "Semaphore::Semaphore()");
    }
  }

  Semaphore::~Semaphore() noexcept
  {
    ::sem_destroy(&semaphore_);
  }

  void
  Semaphore::acquire() /*throw(Exception)*/
  {
    while(true)
    {
      if(!::sem_wait(&semaphore_))
      {
        break;
      }

      if(errno != EINTR)
      {
        throw_errno_exception<Exception>(
          "Semaphore::acquire()");
      }
    }
  }

  bool
  Semaphore::try_acquire() /*throw(Exception)*/
  {
    if(!sem_trywait(&semaphore_))
    {
      return true;
    }

    if(errno != EAGAIN)
    {
      throw_errno_exception<Exception>(
        "Semaphore::try_acquire()");
    }

    return false;
  }

  void
  Semaphore::release() /*throw(Exception)*/
  {
    if(::sem_post(&semaphore_))
    {
      throw_errno_exception<Exception>("Semaphore::release()");
    }
  }

  int
  Semaphore::value() /*throw(Exception)*/
  {
    int value;
    if(::sem_getvalue(&semaphore_, &value) < 0)
    {
      throw_errno_exception<Exception>("Semaphore::value()");
    }

    return value;
  }

#endif
}
