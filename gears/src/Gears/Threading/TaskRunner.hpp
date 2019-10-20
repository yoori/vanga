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

#ifndef GEARS_THREADING_TASKRUNNER_HPP
#define GEARS_THREADING_TASKRUNNER_HPP

#include <deque>

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/Lock.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>

#include "Semaphore.hpp"
#include "ActiveObject.hpp"
#include "Scheduler.hpp"

namespace Gears
{
  /**
   * General Task to be processed by TaskRunner.
   */
  class Task: public virtual AtomicRefCountable
  {
  public:
    /**
     * Method is called by TaskRunner when the object's order arrives.
     */
    virtual void
    execute() throw(Gears::Exception) = 0;

  protected:
    virtual
    ~Task() throw();
  };

  typedef IntrusivePtr<Task> Task_var;

  /**
   * Performs tasks in several threads parallelly.
   */
  class TaskRunner : public ActiveObjectCommonImpl
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, ActiveObject::Exception);
    DECLARE_GEARS_EXCEPTION(Overflow, Exception);
    DECLARE_GEARS_EXCEPTION(NotActive, Exception);

    /**
     * Constructor
     * @param callback not null callback is called on errors
     * @param threads_number number of working threads
     * @param stack_size their stack sizes
     * @param max_pending_tasks maximum task queue length
     */
    TaskRunner(
      ActiveObjectCallback* callback,
      unsigned int threads_number,
      size_t stack_size = 0,
      unsigned long max_pending_tasks = 0)
      throw(InvalidArgument, Exception, Gears::Exception);

    /**
     * Enqueues a task
     * @param task task to enqueue. Number of references is not increased
     * @param timeout maximal absolute wait time before fail on mutex lock
     * until the task is put in the queue. NULL timeout means no wait.
     * If you put limitations on the size of the queue, and it's full,
     * method waits for the release up to timeout
     */
    void
    enqueue_task(Task* task, const Time* timeout = 0)
      throw(InvalidArgument, Overflow, NotActive, Gears::Exception);

    /**
     * Returns number of tasks recently being enqueued
     * This number does not have much meaning in MT environment
     * @return number of tasks enqueued
     */
    unsigned long
    task_count() throw();

    /**
     * Waits for the moment task queue is empty and returns control.
     * In MT environment tasks can be added at the very same moment of
     * return of control.
     */
    void
    wait_for_queue_exhausting() throw(Gears::Exception);

    /**
     * Clear task queue
     */
    virtual void
    clear() throw(Gears::Exception);

  protected:
    virtual
    ~TaskRunner() throw();

  private:
    class TaskRunnerJob: public SingleJob
    {
    public:
      TaskRunnerJob(
        ActiveObjectCallback* callback,
        unsigned long number_of_threads,
        unsigned long max_pending_tasks)
        throw(Gears::Exception);

      virtual void
      work() throw();

      virtual void
      terminate() throw();

      void
      enqueue_task(Task* task, const Time* timeout)
        throw(InvalidArgument, Overflow, NotActive, Gears::Exception);

      unsigned long
      task_count() throw();

      void
      wait_for_queue_exhausting() throw(Gears::Exception);

      void
      clear() throw(Gears::Exception);

    protected:
      virtual
      ~TaskRunnerJob() throw();

    private:
      typedef std::deque<Task_var> Tasks;

    private:
      const unsigned long NUMBER_OF_THREADS_;
      Tasks tasks_;
      Semaphore new_task_;
      Semaphore not_full_;
      const bool LIMITED_;
    };

    typedef IntrusivePtr<TaskRunnerJob>
      TaskRunnerJob_var;

    TaskRunnerJob& job_;
  };

  typedef IntrusivePtr<TaskRunner>
    TaskRunner_var;

  /**
   * Task with specified RC implementation
   */
  class TaskImpl:
    public virtual Task,
    public virtual AtomicRefCountable
  {
  protected:
    /**
     * Destructor
     */
    virtual
    ~TaskImpl() throw();
  };

  /**
   * Should be put into the Planner.
   * When time arrives, it puts itself into TaskRunner.
   */
  class TaskGoal:
    public Goal,
    public Task,
    public virtual AtomicRefCountable
  {
  public:
    /**
     * Constructor
     * @param task_runner TaskRunner to put the object into.
     */
    TaskGoal(TaskRunner* task_runner) throw(Gears::Exception);

    /**
     * Implementation of Goal::deliver.
     * Puts the object into the TaskRunner.
     */
    virtual void
    deliver() throw(Gears::Exception);

  protected:
    /**
     * Destructor
     */
    virtual
    ~TaskGoal() throw();

  private:
    TaskRunner_var task_runner_;
  };

  /**
   * Reusable version of TaskGoal
   */
  class GoalTask:
    public Goal,
    public Task
  {
  public:
    /**
     * Constructor
     * After the object construction call deliver() to put the object into the
     * TaskRunner or schedule_() to put it into the Planner.
     * @param planner Planner to put the object into.
     * @param task_runner TaskRunner to put the object into.
     * or in Planner otherwisee
     */
    GoalTask(Scheduler* planner, TaskRunner* task_runner) throw(Gears::Exception);

    /**
     * Implementation of Goal::deliver.
     * Puts the object into the TaskRunner.
     */
    virtual void
    deliver() throw(Gears::Exception);

    /**
     * Put the object into the Planner. Call this in execute().
     * @param when time of putting the object into the TaskRunner
     */
    void
    schedule(const Time& time) throw(Gears::Exception);

  protected:
    /**
     * Destructor
     */
    virtual
    ~GoalTask() throw();

  private:
    Scheduler_var planner_;
    TaskRunner_var task_runner_;
  };
}

//
// Inlines
//

namespace Gears
{
  //
  // Task class
  //

  inline
  Task::~Task() throw()
  {}

  //
  // TaskImpl class
  //

  inline
  TaskImpl::~TaskImpl() throw()
  {}

  //
  // TaskGoal class
  //

  inline
  TaskGoal::TaskGoal(TaskRunner* task_runner)
    throw(Gears::Exception)
    : task_runner_(Gears::add_ref(task_runner))
  {}

  inline
  TaskGoal::~TaskGoal() throw()
  {}

  inline void
  TaskGoal::deliver() throw(Gears::Exception)
  {
    task_runner_->enqueue_task(this);
  }

  //
  // GoalTask class
  //

  inline
  GoalTask::GoalTask(Scheduler* planner, TaskRunner* task_runner)
    throw(Gears::Exception)
    : planner_(Gears::add_ref(planner)),
      task_runner_(Gears::add_ref(task_runner))
  {}

  inline
  GoalTask::~GoalTask() throw()
  {}

  inline
  void
  GoalTask::deliver() throw(Gears::Exception)
  {
    task_runner_->enqueue_task(this);
  }

  inline
  void
  GoalTask::schedule(const Time& when) throw(Gears::Exception)
  {
    planner_->schedule(this, when);
  }

  //
  // TaskRunner::TaskRunnerJob class
  //

  inline
  unsigned long
  TaskRunner::TaskRunnerJob::task_count() throw()
  {
    LockType::ReadGuard guard(mutex());
    return tasks_.size();
  }

  //
  // TaskRunner class
  //

  inline
  void
  TaskRunner::enqueue_task(Task* task, const Time* timeout)
    throw(InvalidArgument, Overflow, NotActive, Gears::Exception)
  {
    job_.enqueue_task(task, timeout);
  }

  inline
  unsigned long
  TaskRunner::task_count() throw()
  {
    return job_.task_count();
  }

  inline
  void
  TaskRunner::wait_for_queue_exhausting() throw(Gears::Exception)
  {
    job_.wait_for_queue_exhausting();
  }

  inline
  void
  TaskRunner::clear() throw(Gears::Exception)
  {
    job_.clear();
  }
}

#endif
