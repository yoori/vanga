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

#ifndef GEARS_THREADING_SCHEDULER_HPP
#define GEARS_THREADING_SCHEDULER_HPP

#include <list>

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/Time.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>
#include <Gears/Basic/Lock.hpp>

#include "Condition.hpp"
#include "ActiveObject.hpp"
#include "ActiveObjectCommonImpl.hpp"

namespace Gears
{
  class Goal: public virtual AtomicRefCountable
  {
  public:
    /**
     * Callback function to be called from the scheduler
     */
    virtual
    void
    deliver() /*throw(Gears::Exception)*/ = 0;
  };

  typedef IntrusivePtr<Goal> Goal_var;

  class Scheduler: public ActiveObjectCommonImpl
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, ActiveObject::Exception);

    /**
     * Constructor
     * @param callback Reference countable callback object to be called
     * for errors
     * @param stack_size stack size for working thread
     * @param delivery_time_adjustment Should delivery_time_shift_ be used
     * for messages' time shift
     */
    Scheduler(
      ActiveObjectCallback* callback,
      size_t stack_size = 0,
      bool delivery_time_adjustment = false)
      /*throw(InvalidArgument, Gears::Exception)*/;

    /**
     * Adds goal to the queue. Goal's reference counter is incremented.
     * On error it is unchanged (and object will be freed in the caller).
     * @param goal Object to enqueue
     * @param time Timestamp to match
     */
    void
    schedule(Goal* goal, const Time& time)
      /*throw(InvalidArgument, Exception, Gears::Exception)*/;

    /**
     * Tries to remove goal from the queue.
     * @param goal Object to remove
     * @return number of entries removed
     */
    unsigned
    unschedule(const Goal* goal)
      /*throw(Gears::Exception)*/;

    /**
     * Clearance of messages' queue
     */
    virtual
    void
    clear() /*throw(Gears::Exception)*/;

  protected:
    /**
     * Destructor
     * Decreases all unmatched messages' reference counters
     */
    virtual
    ~Scheduler() noexcept;

  private:
    class Job: public SingleJob
    {
    public:
      Job(ActiveObjectCallback* callback,
        bool delivery_time_adjustment)
        /*throw(Gears::Exception)*/;

      virtual
      void
      work() noexcept;

      virtual
      void
      terminate() noexcept;

      void
      schedule(Goal* goal, const Time& time)
        /*throw(InvalidArgument, Exception, Gears::Exception)*/;

      unsigned
      unschedule(const Goal* goal)
        /*throw(Gears::Exception)*/;

      void
      clear() noexcept;

    protected:
      virtual ~Job() noexcept;

      /**
       * Element of messages' queue. Composition of Message and associated Time.
       */
      class TimedMessage
      {
      public:
        /**
         * Constructor
         * @param time Associated time
         * @param goal Shared ownership on goal
         */
        TimedMessage(const Time& time, Goal* goal) noexcept;

        /**
         * Holding time
         * @return Associated time
         */
        const Time&
        time() const noexcept;

        /**
         * Calls deliver() on owned goal
         */
        void
        deliver() /*throw(Gears::Exception)*/;

        /**
         * Checks if it holds the goal
         * @param goal goal to check against
         * @return true if they coinside
         */
        bool
        is_goal(const Goal* goal) const noexcept;

      private:
        Time time_;
        Goal_var goal_;
      };

      typedef std::list<TimedMessage> TimedList;

    protected:
      mutable Condition new_event_in_schedule_;
      bool have_new_events_;  // Predicate for condition!

      TimedList messages_;
      bool delivery_time_adjustment_;
      Time delivery_time_shift_;
    };

    typedef IntrusivePtr<Job> Job_var;

  protected:
    Job& job_;
  };

  typedef IntrusivePtr<Scheduler> Scheduler_var;
}

//
// Inlines
//

namespace Gears
{
  //
  // Scheduler::TimedMessage inlines
  //

  inline
  Scheduler::Job::TimedMessage::TimedMessage(
    const Time& time, Goal* goal) noexcept
    : time_(time),
      goal_(Gears::add_ref(goal))
  {}

  inline
  const Time&
  Scheduler::Job::TimedMessage::time() const noexcept
  {
    return time_;
  }

  inline
  void
  Scheduler::Job::TimedMessage::deliver() /*throw(Gears::Exception)*/
  {
    goal_->deliver();
  }

  inline
  bool
  Scheduler::Job::TimedMessage::is_goal(const Goal* goal) const
    noexcept
  {
    return goal == goal_.in();
  }

  //
  // Scheduler inlines
  //

  inline
  void
  Scheduler::schedule(Goal* goal, const Time& time)
    /*throw(InvalidArgument, Exception, Gears::Exception)*/
  {
    job_.schedule(goal, time);
  }

  inline
  unsigned
  Scheduler::unschedule(const Goal* goal)
    /*throw(Gears::Exception)*/
  {
    return job_.unschedule(goal);
  }

  inline
  void
  Scheduler::clear() /*throw(Gears::Exception)*/
  {
    job_.clear();
  }
}

#endif /*GEARS_THREADING_SCHEDULER_HPP*/
