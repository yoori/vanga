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

#ifndef GEARS_THREADING_CONDITIONLOCK_HPP
#define GEARS_THREADING_CONDITIONLOCK_HPP

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/Uncopyable.hpp>
#include <Gears/Basic/Time.hpp>
#include <Gears/Basic/Lock.hpp>

namespace Gears
{
  /**
   * @class Condition
   *
   * @brief Condition variable wrapper, which allows threads
   * to block until shared data changes state.
   *
   * A condition variable enables threads to atomically block and
   * test the condition under the protection of a mutual exclusion
   * lock (mutex) until the condition is satisfied.  That is,
   * the mutex must have been held by the thread before calling
   * wait or signal on the condition.  If the condition is false,
   * a thread blocks on a condition variable and atomically
   * releases the mutex that is waiting for the condition to
   * change.  If another thread changes the condition, it may wake
   * up waiting threads by signaling the associated condition
   * variable.  The waiting threads, upon awakening, reacquire the
   * mutex and re-evaluate the condition.
   */
  class Condition: private Uncopyable
  {
  public:
    // Can be raised if system API errors occurred
    DECLARE_GEARS_EXCEPTION(ResourceError, DescriptiveException);

    /**
     *  Constructor initialize condition variable
     */
    Condition() throw(ResourceError, Gears::Exception);

    /**
     * Destructor destroy condition variable
     * It shall be safe to destroy an initialized condition variable
     * upon which no threads are currently blocked.
     * Attempting to destroy a condition variable upon which other threads
     * are currently blocked results in undefined behavior.
     */
    ~Condition() throw();

    /**
     * Block on condition.
     * @param mutex
     * Wait functions shall block on a condition variable. It
     * shall be called with mutex locked by the calling thread
     * or undefined behavior results.
     */
    void
    wait(Mutex& mutex) throw(ResourceError, Gears::Exception);

    /**
     * Block on condition, or until absolute time-of-day has passed.
     * Wait functions shall block on a condition variable.
     * @param mutex Method shall be called with mutex locked by the
     * calling thread or undefined behavior results.
     * @param time pointer to absolute time or time interval in
     * dependency of third parameter. If pointer = 0 use blocking wait()
     * semantics.
     * This is useful if we choose time interval and sometime need
     * infinity waiting.
     * @param time_is_relative = true time parameter should be time interval.
     * Implementation add this time interval to current system time.
     * @return false if timeout.
     */
    bool
    timed_wait(Mutex& mutex,
      const Time* time,
      bool time_is_relative = false)
      throw(ResourceError, Gears::Exception);

    /**
     * Signal one waiting thread. This method shall unblock at least one
     * of the threads that are blocked on Condition
     * (if any threads are blocked on this).
     */
    void
    signal() throw(ResourceError, Gears::Exception);

    /**
     * Signal *all* waiting threads. This method shall unblock all threads
     * currently blocked on Condition
     */
    void
    broadcast() throw(ResourceError, Gears::Exception);

  private:
    pthread_cond_t cond_;
  };

  /**
   * @class ConditionGuard
   *
   * @brief ConditionGuard is useful guard that locks associated with
   *  Condition mutex in constructor and unlock in destructor
   *  And it will delegate calls to used Condition while created.
   *  Available only for mutex lock policy.
   */
  class ConditionGuard: private Mutex::WriteGuard
  {
  public:
    /**
     * Constructor
     * @param conditional methods calls will be delegate to this object
     * @param mutex lock mutex for conditional using
     */
    ConditionGuard(Mutex& mutex, Condition& condition)
      throw();

    /**
     * Block on condition. Delegate call to conditional.
     */
    void
    wait() throw(Condition::ResourceError, Gears::Exception);

    /**
     * Block on condition or until absolute time-of-day has passed.
     * Delegate call to conditional.
     * @param time pointer to absolute time or time interval in dependency
     * of second parameter. If pointer = 0 use blocking wait() semantics.
     * This is useful if we choose time interval and sometime need
     * infinity waiting.
     * @param time_is_relative if = true time parameter should be time
     * interval.
     * Implementation add this time interval to current system time.
     */
    bool
    timed_wait(
      const Time* time,
      bool time_is_relative = false)
      throw(Condition::ResourceError, Gears::Exception);

  private:
    Condition& condition_;
  };
} // namespace Gears

#endif
