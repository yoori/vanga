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

#include <cassert>
#include <algorithm>

#include "TaskRunner.hpp"

namespace Gears
{
  //
  // TaskRunner::TaskRunnerJob class
  //

  TaskRunner::TaskRunnerJob::TaskRunnerJob(
    ActiveObjectCallback* callback,
    unsigned long number_of_threads,
    unsigned long max_pending_tasks)
    throw(Gears::Exception)
    : SingleJob(callback),
      NUMBER_OF_THREADS_(number_of_threads),
      new_task_(0),
      not_full_(static_cast<int>(std::min<unsigned long>(max_pending_tasks, SEM_VALUE_MAX))),
      LIMITED_(max_pending_tasks)
  {}

  TaskRunner::TaskRunnerJob::~TaskRunnerJob() throw()
  {}

  void
  TaskRunner::TaskRunnerJob::clear() throw(Gears::Exception)
  {
    LockType::WriteGuard guard(mutex());
    if(LIMITED_)
    {
      for(size_t i = tasks_.size(); i; i--)
      {
        not_full_.release();
      }
    }
    tasks_.clear();
  }

  void
  TaskRunner::TaskRunnerJob::enqueue_task(
    Task* task,
    const Time* /*timeout*/)
    throw(InvalidArgument, Overflow, NotActive, Gears::Exception)
  {
    static const char* FUN = "TaskRunner::TaskRunnerJob::enqueue_task()";

    if(!task)
    {
      ErrorStream ostr;
      ostr << FUN << ": task is NULL";
      throw InvalidArgument(ostr.str());
    }

    // Producer
    if(LIMITED_)
    {
//    if(!(timeout ? not_full_.timed_acquire(timeout) :
//      not_full_.try_acquire()))
      if(not_full_.try_acquire())
      {
        ErrorStream ostr;
        ostr << FUN << ": TaskRunner overflow";
        throw Overflow(ostr.str());
      }
    }

    {
      LockType::WriteGuard guard(mutex());
      try
      {
        tasks_.push_back(Gears::add_ref(task));
      }
      catch (...)
      {
        if(LIMITED_)
        {
          not_full_.release();
        }
        throw;
      }
    }
    // Wake any working thread
    new_task_.release();
  }

  void
  TaskRunner::TaskRunnerJob::wait_for_queue_exhausting() throw(Gears::Exception)
  {
    for(;;)
    {
      {
        LockType::ReadGuard guard(mutex());
        if(tasks_.empty())
        {
          return;
        }
      }
      Gears::Time wait(0, 300000);
      select(0, 0, 0, 0, &wait);
    }
  }

  void
  TaskRunner::TaskRunnerJob::work() throw()
  {
    static const char* FUN = "TaskRunner::TaskRunnerJob::work()";

    try
    {
      for(;;)
      {
        Task_var task;
        {
          new_task_.acquire();
          LockType::WriteGuard guard(mutex());
          if(is_terminating())
          {
            break;
          }
          assert(!tasks_.empty());
          task = tasks_.front();
          tasks_.pop_front();
        }

        // Tell any blocked thread that the queue is ready for a "new item"
        if(LIMITED_)
        {
          not_full_.release();
        }

        try
        {
          task->execute();
        }
        catch (const Gears::Exception& ex)
        {
          callback()->report_error(
            ActiveObjectCallback::ERROR,
            SubString(ex.what()));
        }
      }
    }
    catch (const Gears::Exception& e)
    {
      ErrorStream ostr;
      ostr << FUN << ": Gears::Exception: " << e.what();
      callback()->report_error(
        ActiveObjectCallback::CRITICAL_ERROR,
        ostr.str());
    }
  }

  void
  TaskRunner::TaskRunnerJob::terminate() throw()
  {
    for(unsigned long i = NUMBER_OF_THREADS_; i; i--)
    {
      new_task_.release();
    }
  }

  //
  // TaskRunner class
  //

  TaskRunner::TaskRunner(
    ActiveObjectCallback* callback,
    unsigned int threads_number,
    size_t stack_size,
    unsigned long max_pending_tasks)
    throw(InvalidArgument, Exception, Gears::Exception)
    : ActiveObjectCommonImpl(
        TaskRunnerJob_var(new TaskRunnerJob(
          callback,
          threads_number,
          max_pending_tasks)),
        threads_number, stack_size),
      job_(static_cast<TaskRunnerJob&>(*SINGLE_JOB_))
  {}

  TaskRunner::~TaskRunner() throw()
  {}
}
