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

#ifndef GEARS_THREADING_ACTIVEOBJECTCOMMONIMPL_HPP
#define GEARS_THREADING_ACTIVEOBJECTCOMMONIMPL_HPP

#include <Gears/Basic/AtomicRefCountable.hpp>
#include <Gears/Basic/Lock.hpp>

#include "ThreadRunner.hpp"
#include "ActiveObject.hpp"

namespace Gears
{
  /**
   * General implementation Active Object logic by default.
   * May be supplement with special logic in concrete Active Object
   * through virtual methods override.
   */
  class ActiveObjectCommonImpl:
    public virtual ActiveObject,
    public virtual AtomicRefCountable
  {
  public:
    typedef ActiveObject::Exception Exception;
    typedef ActiveObject::NotSupported NotSupported;
    typedef ActiveObject::AlreadyActive AlreadyActive;
    typedef ActiveObject::InvalidArgument InvalidArgument;

    /**
     * Start threads that will perform SingleJob
     */
    virtual
    void
    activate_object()
      /*throw(AlreadyActive, Exception, Gears::Exception)*/;

    /**
     * Initiate stopping of Active object
     * Acquires mutex and informs SingleJob
     */
    virtual
    void
    deactivate_object()
      /*throw(Exception, Gears::Exception)*/;

    /**
     * Waits for deactivation completion
     * Acquires mutex and waits for threads completion
     */
    virtual
    void
    wait_object() /*throw(Exception, Gears::Exception)*/;

    /**
     * Current status
     * @return Returns true if active and not going to deactivate
     */
    virtual
    bool
    active() /*throw(Gears::Exception)*/;

  protected:
    /**
     * ActiveObjectCommonImpl expects the only object will be a job for
     * all ThreadRunner's threads. This object must be a descendant of
     * this class.
     */
    class SingleJob: public ThreadJob
    {
    public:
      typedef ActiveObject::Exception Exception;
      typedef ActiveObject::NotSupported NotSupported;
      typedef ActiveObject::AlreadyActive AlreadyActive;
      typedef ActiveObject::InvalidArgument InvalidArgument;

      /**
       * Constructor
       * @param callback callback to be called for error reporting
       */
      SingleJob(ActiveObjectCallback* callback)
        /*throw(InvalidArgument, Gears::Exception)*/;

      /**
       * Stored callback
       * @return stored callback
       */
      const ActiveObjectCallback_var&
      callback() noexcept;

      /**
       * Mutex for operations synchronizations
       * @return stored mutex
       */
      Mutex&
      mutex() noexcept;

      void
      make_terminate() noexcept;

      void
      terminated() noexcept;

      bool
      is_terminating() noexcept;

      /**
       * Function must inform the object to stop jobs to work.
       */
      virtual
      void
      terminate() noexcept = 0;

    protected:
      typedef Mutex LockType;

    protected:
      /**
       * Destructor
       */
      virtual
      ~SingleJob() noexcept;

    private:
      mutable LockType mutex_;
      ActiveObjectCallback_var callback_;
      volatile SigAtomicType terminating_;
    };

    typedef IntrusivePtr<SingleJob>
      SingleJob_var;

    typedef Mutex LockType;

  protected:
    /**
     * Constructor
     * Initializes SINGLE_JOB_ with the provided job and
     * creates ThreadRunner.
     * @param job job to execute in threads
     * @param threads_number number of threads to execute the job in
     * @param stack_size stack size for threads
     */
    ActiveObjectCommonImpl(
      SingleJob* job,
      unsigned int threads_number = 1,
      std::size_t stack_size = 0)
      /*throw(InvalidArgument)*/;

    /**
     * Destructor
     */
    virtual
    ~ActiveObjectCommonImpl() noexcept;

    /**
     * Provides number of work threads created
     * @return number of created threads
     */
    size_t
    threads_number_() noexcept;

    /**
     * @return mutex that used in clients code for
     * data protection.
     */
    Mutex&
    mutex_() noexcept;

    const SingleJob_var SINGLE_JOB_;

  private:
    ThreadRunner thread_runner_;

    mutable Mutex termination_mutex_;
    Mutex& work_mutex_;

    volatile SigAtomicType active_state_;
  };
}

//
// Inlines
//
namespace Gears
{
  //
  // ActiveObjectCommonImpl inlines
  //

  inline
  size_t
  ActiveObjectCommonImpl::threads_number_() noexcept
  {
    return thread_runner_.number_of_jobs();
  }

  inline
  Mutex&
  ActiveObjectCommonImpl::mutex_() noexcept
  {
    return work_mutex_;
  }

  //
  // ActiveObjectCommonImpl::SingleJob class
  //

  inline
  ActiveObjectCommonImpl::SingleJob::SingleJob(
    ActiveObjectCallback* callback)
    /*throw(InvalidArgument, Gears::Exception)*/
    : callback_(Gears::add_ref(callback)),
      terminating_(false)
  {
    static const char* FUN = "ActiveObjectCommonImpl::SingleJob::SingleJob()";

    if(!callback)
    {
      ErrorStream ostr;
      ostr << FUN << ": callback == 0";
      throw InvalidArgument(ostr.str());
    }
  }

  inline
  ActiveObjectCommonImpl::SingleJob::~SingleJob() noexcept
  {}

  inline
  const ActiveObjectCallback_var&
  ActiveObjectCommonImpl::SingleJob::callback() noexcept
  {
    return callback_;
  }

  inline
  Mutex&
  ActiveObjectCommonImpl::SingleJob::mutex() noexcept
  {
    return mutex_;
  }

  inline
  void
  ActiveObjectCommonImpl::SingleJob::make_terminate() noexcept
  {
    terminating_ = true;
    terminate();
  }

  inline
  void
  ActiveObjectCommonImpl::SingleJob::terminated() noexcept
  {
    terminating_ = false;
  }

  inline
  bool
  ActiveObjectCommonImpl::SingleJob::is_terminating() noexcept
  {
    return terminating_;
  }
}

#endif /*GEARS_THREADING_ACTIVEOBJECTCOMMONIMPL_HPP*/
