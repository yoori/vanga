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

#ifndef GEARS_THREADKEY_HPP
#define GEARS_THREADKEY_HPP

#include <pthread.h>

#include <Gears/Basic/Errno.hpp>

namespace Gears
{
  /**
   * Performs access to thread-specific data stored as pointers
   */
  template <typename Data>
  class ThreadKey
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);

    /**
     * Create thread-specific key
     * @param destructor optional destructor called for data on thread
     * termination
     */
    explicit
    ThreadKey(void (*destructor)(void*) = 0) /*throw (Exception)*/;

    /**
     * Store data for the current thread
     * @param data to store
     */
    void
    set_data(Data* data) /*throw (Exception)*/;

    /**
     * Get stored data for the current thread
     * @return stored data
     */
    Data*
    get_data() noexcept;

  private:
    ThreadKey(const ThreadKey&);

  private:
    pthread_key_t key_;
  };
}

namespace Gears
{
  template <typename Data>
  ThreadKey<Data>::ThreadKey(void (*destructor)(void*)) /*throw (Exception)*/
  {
    static const char* FUN = "ThreadKey<>::ThreadKey()";

    const int RES = pthread_key_create(&key_, destructor);
    if (RES)
    {
      Gears::throw_errno_exception<Exception>(RES, FUN,
        ": Failed to create key");
    }
  }

  template <typename Data>
  void
  ThreadKey<Data>::set_data(Data* data) /*throw (Exception)*/
  {
    static const char* FUN = "ThreadKey<>::set_data()";

    const int RES = pthread_setspecific(key_, data);
    if (RES)
    {
      Gears::throw_errno_exception<Exception>(RES, FUN,
        ": Failed to set data");
    }
  }

  template <typename Data>
  Data*
  ThreadKey<Data>::get_data() noexcept
  {
    return static_cast<Data*>(pthread_getspecific(key_));
  }
}

#endif
