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

#ifndef GEARS_THREADING_SEMAPHORE_HPP
#define GEARS_THREADING_SEMAPHORE_HPP

#include <semaphore.h>
#include <errno.h>

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/Errno.hpp>
#include <Gears/Basic/Uncopyable.hpp>
#include <Gears/Basic/Time.hpp>

#include "Condition.hpp"

namespace Gears
{
  /**
   * Semaphore: unnamed semaphore wrapper
   *   for macosx implemented with using conditional variable
   */
  class Semaphore: private Uncopyable
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, DescriptiveException);

    Semaphore(int count) throw(Exception);

    ~Semaphore() throw();

    void acquire() throw(Exception);

    bool try_acquire() throw(Exception);

    void release() throw(Exception);

    int value() throw(Exception);

  private:
#if __APPLE__
    Mutex lock_;
    Condition cond_;
    unsigned long value_;
#else
    sem_t semaphore_;
#endif
  };
}

#endif /*GEARS_THREADING_SEMAPHORE_HPP*/
