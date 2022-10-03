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

#include <Gears/Basic/Errno.hpp>

#include "Condition.hpp"

namespace Gears
{
  Condition::Condition()
    /*throw(ResourceError, Gears::Exception)*/
  {
    const int res = pthread_cond_init(&cond_, NULL);
    if(res)
    {
      Gears::throw_errno_exception<ResourceError>(res,
        "Condition::Condition()");
    }
  }

  Condition::~Condition() noexcept
  {
    // todo: should be able to detect the errors;
    pthread_cond_destroy(&cond_);
  }

  void
  Condition::wait(Mutex& lock)
    /*throw(ResourceError, Gears::Exception)*/
  {
    // When we call pthread_cond_wait mutex_ should be locked,
    // else - U.B.
    // "pthread_cond_wait" release the mutex_ and block thread
    // while another thread call signal().
    // Mutex used to protect data need for condition calculation.
    const int res = ::pthread_cond_wait(&cond_, &lock.mutex_i());
    if(res)
    {
      Gears::throw_errno_exception<ResourceError>(res,
        "Condition::wait()");
    }
  }

  bool
  Condition::timed_wait(
    Mutex& lock,
    const Gears::Time* time,
    bool time_is_relative)
    /*throw(ResourceError, Gears::Exception)*/
  {
    if(time == 0)
    {
      wait(lock);
      return true; //We don't reach INFINITY time and cannot reach timeout
    }

    Gears::Time real_time(time_is_relative ?
      Gears::Time::get_time_of_day() + *time : *time);

    // When we call pthread_cond_wait mutex_ should be locked,
    // else - U.B.
    // "pthread_cond_wait" release the mutex and block thread
    // while another thread call signal().
    // Mutex used to protect data need for condition calculation.
    const timespec RESTRICT = { real_time.tv_sec, real_time.tv_usec * 1000 };
    const int res = pthread_cond_timedwait(&cond_, &lock.mutex_i(), &RESTRICT);

    if(res == ETIMEDOUT)
    {
      return false;
    }

    if(res)
    {
      Gears::throw_errno_exception<ResourceError>(res,
        "Gears::Condition::timed_wait()");
    }

    return true;
  }

  /// Signal one waiting thread.
  void
  Condition::signal() /*throw(ResourceError, Gears::Exception)*/
  {
    // function may fail if:
    // EINVAL The value cond_ does not refer to an initialized condition
    // variable.
    const int res = pthread_cond_signal(&cond_);
    if(res)
    {
      Gears::throw_errno_exception<ResourceError>(res,
        "Gears::Condition::signal()");
    }
  }

  /// Signal *all* waiting threads.
  void
  Condition::broadcast() /*throw(ResourceError, Gears::Exception)*/
  {
    // function may fail if:
    // EINVAL The value cond_ does not refer to an initialized condition
    // variable.
    const int res = pthread_cond_broadcast(&cond_);
    if(res)
    {
      Gears::throw_errno_exception<ResourceError>(res,
        "Gears::Condition::broadcast()");
    }
  }

  //
  // class ConditionGuard
  //

  ConditionGuard::ConditionGuard(
    Mutex& lock,
    Condition& condition)
    noexcept
    : Mutex::WriteGuard(lock),
      condition_(condition)
  {}

  void
  ConditionGuard::wait()
    /*throw(Condition::ResourceError, Gears::Exception)*/
  {
    condition_.wait(lock_);
  }

  bool
  ConditionGuard::timed_wait(
    const Time* time,
    bool time_is_relative)
    /*throw(Condition::ResourceError, Gears::Exception)*/
  {
    return condition_.timed_wait(lock_, time, time_is_relative);
  }
} //namespace Gears
