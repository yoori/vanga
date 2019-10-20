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

#include "Scheduler.hpp"

namespace Gears
{
  //
  // Scheduler::Job class
  //

  Scheduler::Job::Job(
    ActiveObjectCallback* callback,
    bool delivery_time_adjustment)
    throw(Gears::Exception)
    : SingleJob(callback),
      have_new_events_(false),
      delivery_time_adjustment_(delivery_time_adjustment)
  {}

  Scheduler::Job::~Job() throw()
  {}

  void
  Scheduler::Job::terminate() throw()
  {
    have_new_events_ = true;
    new_event_in_schedule_.signal(); // wake the working thread
  }

  void
  Scheduler::Job::schedule(Goal* goal, const Time& time)
    throw(InvalidArgument, Exception, Gears::Exception)
  {
    static const char* FUN = "Scheduler::Job::schedule()";

    if(!goal)
    {
      ErrorStream ostr;
      ostr << FUN << ": goal is null";
      throw InvalidArgument(ostr.str());
    }

    Time tm(time > Time::ZERO ? time : Time::ZERO);

    bool signal;

    {
      /** sch 1: add message into list */
      LockType::WriteGuard guard(mutex());

      TimedList::iterator itor(messages_.end());

      while (itor != messages_.begin())
      {
        --itor;
        if(itor->time() < tm)
        {
          ++itor;
          break;
        }
      }

      signal = (itor == messages_.begin());

      messages_.insert(itor, TimedMessage(tm, goal));
      if(signal)
      {
        have_new_events_ = true;
      }
    }

    if(signal)
    {
      /** sch 2: new events into schedule signal */
      new_event_in_schedule_.signal();
    }
  }

  unsigned
  Scheduler::Job::unschedule(const Goal* goal)
    throw(Gears::Exception)
  {
    unsigned removed = 0;

    {
      LockType::WriteGuard guard(mutex());

      for(TimedList::iterator itor(messages_.begin());
        itor != messages_.end();)
      {
        if(itor->is_goal(goal))
        {
          itor = messages_.erase(itor);
          removed++;
        }
        else
        {
          ++itor;
        }
      }
    }

    return removed;
  }

  void
  Scheduler::Job::work() throw()
  {
    static const char* FUN = "Scheduler::Job::work()";

    try
    {
      TimedList pending;
      Time abs_time;
      Time cur_time;

      for(;;)
      {
        Time* pabs_time = 0;

        {
          /** svc 1: make list of pending tasks */
          LockType::WriteGuard guard(mutex());

          if(is_terminating())
          {
            break;
          }
          cur_time = Time::get_time_of_day();

          while (!messages_.empty())  // pump messages to pending.
          {
            abs_time = messages_.front().time();

            if(delivery_time_adjustment_)
            {
              abs_time = abs_time > delivery_time_shift_ ?
                abs_time - delivery_time_shift_ :
                Time::ZERO;
            }

            // pump all overdue event to pending list.
            //  They will call immediately
            if(abs_time <= cur_time)
            {
              pending.splice(pending.end(), messages_, messages_.begin());
            }
            else
            {
              pabs_time = &abs_time;  // first event in the future
              break;
            }
          }
        } // end data lock

        if(pending.empty())
        {
          /** svc 2: wait semaphore signal */
          bool new_event_in_schedule = true;

          {
            ConditionGuard cond_guard(mutex(), new_event_in_schedule_);

            while (!have_new_events_)
            {
              try
              {
                new_event_in_schedule = cond_guard.timed_wait(pabs_time);
              }
              catch (const Gears::Exception& e)
              {
                callback()->report_error(
                  ActiveObjectCallback::CRITICAL_ERROR,
                  SubString(e.what()));
                new_event_in_schedule = false;
              }
              if(!have_new_events_)
              {
                new_event_in_schedule = false;
                break;
              }
            } // while

            if(is_terminating())
            {
              break;
            }

            if(new_event_in_schedule)
            {
              have_new_events_ = false;
            }
          } // Unlock ConditionGuard condition

          if(new_event_in_schedule)
          {
            if(delivery_time_adjustment_ && pabs_time)
            {
              Time wake_tm(Time::get_time_of_day());
              if(wake_tm > *pabs_time) // if OVERDUE event
              {
                Time shift = wake_tm - *pabs_time; // OVERDUE event
                delivery_time_shift_ = shift / 2;
              }
            }
            continue;
          }
        } // if(pending.empty())

        /** svc 3: deliver pending tasks */
        while (!pending.empty())
        {
          try
          {
            pending.front().deliver();
          }
          catch (const Gears::Exception& ex)
          {
            callback()->report_error(
              ActiveObjectCallback::ERROR,
              SubString(ex.what()));
          }
          pending.pop_front();
        }
      }
    }
    catch (const Gears::Exception& e)
    {
      ErrorStream ostr;
      ostr << FUN << ": Gears::Exception caught: " << e.what();
      callback()->report_error(
        ActiveObjectCallback::CRITICAL_ERROR,
        ostr.str());
    }
  }

  void
  Scheduler::Job::clear() throw()
  {
    LockType::WriteGuard guard(mutex());
    messages_.clear();
  }

  //
  // Scheduler class
  //

  Scheduler::Scheduler(ActiveObjectCallback* callback, size_t stack_size,
    bool delivery_time_adjustment) throw(InvalidArgument, Gears::Exception)
    : ActiveObjectCommonImpl(
        Job_var(new Job(callback, delivery_time_adjustment)),
        1, stack_size),
      job_(static_cast<Job&>(*SINGLE_JOB_))
  {}

  Scheduler::~Scheduler() throw()
  {}
}
