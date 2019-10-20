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
#include <Gears/Basic/OutputMemoryStream.hpp>

#include "ActiveObjectCommonImpl.hpp"

namespace Gears
{
  //
  // ActiveObjectCommonImpl class
  //

  ActiveObjectCommonImpl::ActiveObjectCommonImpl(
    SingleJob* job,
    unsigned int threads_number,
    std::size_t stack_size)
    throw(InvalidArgument)
    : SINGLE_JOB_(Gears::add_ref(job)),
      thread_runner_(job, threads_number,
        ThreadRunner::Options(stack_size)),
      work_mutex_(job->mutex()),
      active_state_(AS_NOT_ACTIVE)
  {
    static const char* FUN = "ActiveObjectCommonImpl::ActiveObjectCommonImpl()";

    if(!threads_number)
    {
      ErrorStream ostr;
      ostr << FUN << ": threads_number == 0";
      throw InvalidArgument(ostr.str());
    }
  }

  ActiveObjectCommonImpl::~ActiveObjectCommonImpl() throw()
  {
    static const char* FUN = "ActiveObjectCommonImpl::~ActiveObjectCommonImpl()";

    try
    {
      ErrorStream ostr;
      bool error = false;

      {
        LockType::ReadGuard guard(work_mutex_);

        if(active_state_ == AS_ACTIVE)
        {
          ostr << FUN << ": wasn't deactivated.";
          error = true;
        }

        if(active_state_ != AS_NOT_ACTIVE)
        {
          if(error)
          {
            ostr << std::endl;
          }
          ostr << FUN << ": didn't wait for deactivation, still active.";
          error = true;
        }
      }

      if(error)
      {
        {
          LockType::WriteGuard guard(work_mutex_);
          SINGLE_JOB_->make_terminate();
        }

        thread_runner_.wait_for_completion();

        {
          LockType::WriteGuard guard(work_mutex_);
          SINGLE_JOB_->terminated();
        }

        const ActiveObjectCallback_var& callback = SINGLE_JOB_->callback();
        if(!callback)
        {
          std::cerr << ostr.str() << std::endl;
        }
        else
        {
          callback->report_error(
            ActiveObjectCallback::WARNING,
            ostr.str());
        }
      }
    }
    catch (const Gears::Exception& ex)
    {
      try
      {
        std::cerr << FUN << ": Gears::Exception: " << ex.what() << std::endl;
      }
      catch (...)
      {
        // Nothing we can do
      }
    }
  }

  void
  ActiveObjectCommonImpl::activate_object()
    throw(AlreadyActive, Exception, Gears::Exception)
  {
    static const char* FUN = "ActiveObjectCommonImpl::activate_object()";

    LockType::WriteGuard guard(work_mutex_);

    if(active_state_ != AS_NOT_ACTIVE)
    {
      ErrorStream ostr;
      ostr << FUN << ": still active";
      throw ActiveObject::AlreadyActive(ostr.str());
    }

    try
    {
      active_state_ = AS_ACTIVE;
      thread_runner_.start();
    }
    catch (const Gears::Exception& ex)
    {
      active_state_ = AS_NOT_ACTIVE;

      ErrorStream ostr;
      ostr << FUN << ": start failure: " << ex.what();
      throw Exception(ostr.str());
    }
  }

  void
  ActiveObjectCommonImpl::wait_object()
    throw(Exception, Gears::Exception)
  {
    static const char* FUN = "ActiveObjectCommonImpl::wait_object()";

    LockType::WriteGuard termination_guard(termination_mutex_);
    if(active_state_ != AS_NOT_ACTIVE)
    {
      try
      {
        thread_runner_.wait_for_completion();
        SINGLE_JOB_->terminated();
      }
      catch (const Gears::Exception& ex)
      {
        ErrorStream ostr;
        ostr << FUN << ": waiting failure: " << ex.what();
        throw Exception(ostr.str());
      }
    }

    LockType::WriteGuard guard(work_mutex_);
    if(active_state_ == AS_DEACTIVATING)
    {
      active_state_ = AS_NOT_ACTIVE;
    }
  }

  void
  ActiveObjectCommonImpl::deactivate_object()
    throw(Exception, Gears::Exception)
  {
    LockType::WriteGuard guard(work_mutex_);
    if(active_state_ == AS_ACTIVE)
    {
      active_state_ = AS_DEACTIVATING;
      SINGLE_JOB_->make_terminate();
    }
  }

  bool
  ActiveObjectCommonImpl::active() throw(Gears::Exception)
  {
    return active_state_ == AS_ACTIVE;
  }
} // namespace Gears
