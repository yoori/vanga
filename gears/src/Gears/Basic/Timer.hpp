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

#ifndef GEARS_BASIC_TIMER_HPP
#define GEARS_BASIC_TIMER_HPP

#include <sys/resource.h>

#include "Time.hpp"

namespace Gears
{
  template<typename Clock>
  class BaseTimer
  {
  public:
    BaseTimer() throw ();

    void
    start() throw ();

    void
    stop() throw ();

    Time
    elapsed_time() const throw ();

    const Time&
    start_time() const throw();

    const Time&
    stop_time() const throw();

  private:
    Clock clock_;
    bool started_;
    Time start_;
    Time stop_;
  };

  class ClockTimeOfDay
  {
  public:
    Time operator ()() const throw();
  };

  class ClockCPUUsage
  {
  public:
    Time operator ()() const throw();
  };

  typedef BaseTimer<ClockTimeOfDay> Timer;

  typedef BaseTimer<ClockCPUUsage> CPUTimer;
}

namespace Gears
{
  inline
  Time
  ClockTimeOfDay::operator()() const throw()
  {
    return Time::get_time_of_day();
  }

  inline
  Time
  ClockCPUUsage::operator()() const throw()
  {
    rusage usage;
    ::getrusage(RUSAGE_SELF, &usage);
    return Gears::Time(usage.ru_utime) + usage.ru_stime;
  }

  template<typename ClockType>
  BaseTimer<ClockType>::BaseTimer() throw()
    : started_(false)
  {}

  template<typename ClockType>
  void
  BaseTimer<ClockType>::start() throw()
  {
    started_ = true;
    start_ = clock_();
  }

  template<typename ClockType>
  void
  BaseTimer<ClockType>::stop() throw()
  {
    Time stop = clock_();
    if (started_)
    {
      stop_ = stop;
      started_ = false;
    }
  }

  template<typename ClockType>
  Time
  BaseTimer<ClockType>::elapsed_time() const throw()
  {
    return stop_ - start_;
  }

  template<typename ClockType>
  const Time&
  BaseTimer<ClockType>::start_time() const throw()
  {
    return start_;
  }

  template<typename ClockType>
  const Time&
  BaseTimer<ClockType>::stop_time() const throw()
  {
    return stop_;
  }
}

#endif /*GEARS_BASIC_TIMER_HPP*/
