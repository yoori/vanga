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

#ifndef GEARS_THREADING_THREADRUNNER_HPP
#define GEARS_THREADING_THREADRUNNER_HPP

#include <Gears/Basic/Types.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>
#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/Errno.hpp>
#include <Gears/Basic/ArrayAutoPtr.hpp>
#include <Gears/Basic/AtomicCounter.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>

#include "Semaphore.hpp"

namespace Gears
{
  /**
   * A job performed by ThreadRunner in the threads.
   */
  class ThreadJob: public AtomicRefCountable
  {
  public:
    /**
     * Work process for the job.
     */
    virtual void
    work() throw() = 0;

  protected:
    /**
     * Destructor.
     */
    virtual
    ~ThreadJob() throw();
  };

  typedef IntrusivePtr<ThreadJob> ThreadJob_var;

  /**
   * Creates several threads and executes specified job(s) in them.
   */
  class ThreadRunner
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, DescriptiveException);
    DECLARE_GEARS_EXCEPTION(AlreadyStarted, Exception);
    DECLARE_GEARS_EXCEPTION(PosixException, Exception);

    /**
     * Options for threads.
     */
    struct Options
    {
      /**
       * Constructor
       * @param stack_size stack size for the thread.
       */
      Options(size_t stack_size = 0) throw();

      // Default stack size for threads
      static const size_t DEFAULT_STACK_SIZE = 1024 * 1024;

      size_t stack_size;
    };

  public:
    /**
     * Constructor
     * @param job to be executed in all threads.
     * @param number_of_jobs number of jobs to run concurrently.
     * @param options threads options
     */
    ThreadRunner(
      ThreadJob* job,
      unsigned int number_of_jobs,
      const Options& options = Options())
      throw(Gears::Exception, PosixException);

    /**
     * Constructor
     * @param functor functor producing jobs
     * @param number_of_jobs number of jobs to produce and run concurrently
     * @param options threads options
     */
    template <typename Functor>
    ThreadRunner(
      size_t number_of_jobs,
      Functor functor,
      const Options& options = Options())
      throw(Gears::Exception, PosixException);

    /**
     * Constructor
     * @param begin beginning of the container with jobs
     * @param end end of the container with jobs
     * @param options threads options
     */
    template <typename ForwardIterator>
    ThreadRunner(
      ForwardIterator begin,
      ForwardIterator end,
      const Options& options = Options())
      throw(Gears::Exception, PosixException);

    /**
     * Destructor
     * Waits for threads' completion if they are not terminated yet.
     */
    ~ThreadRunner() throw();

    /**
     * Number of jobs to execute
     * @return number of jobs
     */
    size_t
    number_of_jobs() const throw();

    /**
     * Return if jobs are running or not. Thread unsafe.
     * @return jobs running status
     */
    bool
    running() const throw();

    /**
     * Creates threads and runs the jobs. If creation of a thread fails,
     * no jobs will run. Thread unsafe.
     */
    void
    start() throw(AlreadyStarted, PosixException, Gears::Exception);

    /**
     * Waits for termination of previously started threads.
     * Thread unsafe.
     */
    void
    wait_for_completion() throw(PosixException);

  private:
    class PThreadAttr
    {
    public:
      PThreadAttr() throw(PosixException);
      ~PThreadAttr() throw();
      operator pthread_attr_t*() throw();
    private:
      pthread_attr_t attr_;
    };

    struct JobInfo
    {
      ThreadRunner* runner;
      ThreadJob_var job;
      pthread_t thread_id;
    };

  private:
    static void*
    thread_func_(void* arg) throw();

    void
    thread_func_(ThreadJob& job) throw();

  private:
    Options options_;
    Semaphore start_semaphore_;
    volatile SigAtomicType is_running_;
    ArrayIndexType number_of_jobs_;
    ArrayAutoPtr<JobInfo> jobs_;
  };
}

namespace Gears
{
  //
  // ThreadJob class
  //

  inline
  ThreadJob::~ThreadJob() throw()
  {}

  //
  // ThreadRunner::Options class
  //

  inline
  ThreadRunner::Options::Options(size_t stack_size) throw()
    : stack_size(stack_size < PTHREAD_STACK_MIN ? DEFAULT_STACK_SIZE :
        stack_size)
  {}

  //
  // ThreadRunner::PThreadAttr class
  //

  inline
  ThreadRunner::PThreadAttr::PThreadAttr() throw(PosixException)
  {
    static const char* FUN = "ThreadRunner::PThreadAttr::PThreadAttr()";

    const int res = ::pthread_attr_init(&attr_);

    if(res)
    {
      throw_errno_exception<PosixException>(res, FUN,
        ": failed to initialize attribute");
    }
  }

  inline
  ThreadRunner::PThreadAttr::~PThreadAttr() throw()
  {
    ::pthread_attr_destroy(&attr_);
  }

  inline
  ThreadRunner::PThreadAttr::operator pthread_attr_t*() throw()
  {
    return &attr_;
  }

  //
  // ThreadRunner class
  //

  inline
  ThreadRunner::~ThreadRunner() throw()
  {
    try
    {
      wait_for_completion();
    }
    catch (...)
    {}
  }

  inline
  size_t
  ThreadRunner::number_of_jobs() const throw()
  {
    return number_of_jobs_;
  }

  inline
  bool
  ThreadRunner::running() const throw()
  {
    return is_running_ != 0;
  }

  inline
  void*
  ThreadRunner::thread_func_(void* arg) throw()
  {
    JobInfo* info = static_cast<JobInfo*>(arg);
    info->runner->thread_func_(*info->job);
    return 0;
  }
}

#endif /*GEARS_THREADING_THREADRUNNER_HPP*/
