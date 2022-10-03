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

#include <algorithm>

#include <Gears/Basic/OutputMemoryStream.hpp>

#include "ThreadRunner.hpp"

namespace Gears
{
  ThreadRunner::ThreadRunner(
    ThreadJob* job,
    unsigned int number_of_jobs,
    const Options& options)
    /*throw(Gears::Exception, PosixException)*/
    : options_(options),
      start_semaphore_(0),
      is_running_(false),
      number_of_jobs_(static_cast<ArrayIndexType>(number_of_jobs)),
      jobs_(number_of_jobs_)
  {
    for(ArrayIndexType i = 0; i < number_of_jobs_; ++i)
    {
      jobs_[i].runner = this;
      jobs_[i].job = Gears::add_ref(job);
    }
  }

  template<typename Functor>
  ThreadRunner::ThreadRunner(
    size_t number_of_jobs,
    Functor functor,
    const Options& options)
    /*throw(Gears::Exception, PosixException)*/
    : options_(options),
      start_semaphore_(0),
      is_running_(false),
      number_of_jobs_(number_of_jobs),
      jobs_(number_of_jobs_)
  {
    for(size_t i = 0; i < number_of_jobs_; ++i)
    {
      jobs_[i].runner = this;
      jobs_[i].job = functor();
    }
  }

  template<typename ForwardIterator>
  ThreadRunner::ThreadRunner(
    ForwardIterator begin,
    ForwardIterator end,
    const Options& options)
    /*throw(Gears::Exception, PosixException)*/
    : options_(options),
      start_semaphore_(0),
      is_running_(false),
      number_of_jobs_(std::distance(begin, end)),
      jobs_(number_of_jobs_)
  {
    for(size_t i = 0; i < number_of_jobs_; i++)
    {
      jobs_[i].runner = this;
      jobs_[i].job = Gears::add_ref(*begin++);
    }
  }

  void
  ThreadRunner::wait_for_completion() /*throw(PosixException)*/
  {
    static const char* FUN = "ThreadRunner::wait_for_completion()";

    if(is_running_)
    {
      ErrorStream ostr;
      for(ArrayIndexType i = 0; i < number_of_jobs_; i++)
      {
        const int res = pthread_join(jobs_[i].thread_id, 0);
        if(res)
        {
          char error[sizeof(PosixException)];
          Gears::ErrnoHelper::compose_safe(
            error,
            sizeof(error),
            res,
            FUN,
            "join failure");
          ostr << error << "\n";
        }
      }
      start_semaphore_.acquire();
      is_running_ = false;

      const SubString& str = ostr.str();
      if(str.size())
      {
        throw PosixException(str);
      }
    }
  }

  void
  ThreadRunner::start() /*throw(AlreadyStarted, PosixException, Gears::Exception)*/
  {
    static const char* FUN = "ThreadRunner::start()";

    if(is_running_)
    {
      ErrorStream ostr;
      ostr << FUN << ": already started";
      throw AlreadyStarted(ostr.str());
    }

    PThreadAttr attr;
    const int res = ::pthread_attr_setstacksize(attr, options_.stack_size);
    if(res)
    {
      char stack_size[32];
      snprintf(stack_size, sizeof(stack_size), "%lu",
        static_cast<unsigned long>(options_.stack_size));
      Gears::throw_errno_exception<Exception>(
        FUN,
        "tried to set stack size ",
        stack_size);
    }

    is_running_ = true;
    size_t number_of_jobs = number_of_jobs_;

    try
    {
      for(number_of_jobs_ = 0; number_of_jobs_ < number_of_jobs;
        number_of_jobs_++)
      {
        const int res = pthread_create(
          &jobs_[number_of_jobs_].thread_id,
          attr,
          thread_func_,
          &jobs_[number_of_jobs_]);

        if(res)
        {
          throw_errno_exception<Exception>(res, FUN, "thread start");
        }
      }
    }
    catch(...)
    {
      is_running_ = false;
      start_semaphore_.release();
      try
      {
        wait_for_completion();
      }
      catch (...)
      {
      }
      number_of_jobs_ = number_of_jobs;
      throw;
    }

    start_semaphore_.release();
  }

  void
  ThreadRunner::thread_func_(ThreadJob& job) noexcept
  {
    start_semaphore_.acquire();
    start_semaphore_.release();

    if (is_running_)
    {
      job.work();
    }
  }
}
