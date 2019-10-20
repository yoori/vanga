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

#ifndef BASIC_ATOMICCOUNTER_HPP
#define BASIC_ATOMICCOUNTER_HPP

//#include <Config.hpp>
#include <Gears/Basic/Macro.hpp>

#if __APPLE__
#  include <libkern/OSAtomic.h>
#else
// GCC
#  include <signal.h>

#  ifdef __GNUC__
#    if __GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 2
#      include <ext/atomicity.h>
#    else
#      include <bits/atomicity.h>
#    endif
#  endif
#endif

#include <Gears/Basic/Uncopyable.hpp>

OPEN_NAMESPACE(Gears)

  typedef volatile sig_atomic_t SigAtomicType;

  class AtomicCounter: public Uncopyable
  {
  public:
    AtomicCounter() {}

    explicit AtomicCounter(int value);

    void add(int value);

    int fetch_and_add(int value);

    int add_and_fetch(int value);

    AtomicCounter& operator=(int value);

    AtomicCounter& operator++();

    AtomicCounter& operator--();

    long operator++(int);

    long operator--(int);

    AtomicCounter& operator+=(int value);

    AtomicCounter& operator-=(int value);

    operator int() const;

  protected:
#if __APPLE__
    typedef int32_t Word;
#else
    // GCC
    typedef _Atomic_word Word;
#endif

  private:
    AtomicCounter(const AtomicCounter&);
    AtomicCounter& operator=(const AtomicCounter&);

  private:
    mutable volatile Word value_;
  };

CLOSE_NAMESPACE

OPEN_NAMESPACE(Gears)

  inline
  AtomicCounter::AtomicCounter(int value)
    : value_(value)
  {}

#if __APPLE__
  inline void
  AtomicCounter::add(int value)
  {
    ::OSAtomicAdd32(value, &value_);
  }

  inline int
  AtomicCounter::fetch_and_add(int value)
  {
    return ::OSAtomicAdd32(value, &value_) - value;
  }

  inline int
  AtomicCounter::add_and_fetch(int value)
  {
    return ::OSAtomicAdd32(value, &value_);
  }

#else
  // GCC
  inline void
  AtomicCounter::add(int value)
  {
    __gnu_cxx::__atomic_add(&value_, value);
  }
  
  inline int
  AtomicCounter::fetch_and_add(int value)
  {
    return __gnu_cxx::__exchange_and_add(&value_, value);
  }
  
  inline int
  AtomicCounter::add_and_fetch(int value)
  {
    return __gnu_cxx::__exchange_and_add(&value_, value) + value;
  }

#endif

  inline AtomicCounter&
  AtomicCounter::operator=(int value)
  {
    value_ = value;
    return *this;
  }

  inline AtomicCounter&
  AtomicCounter::operator++()
  {
    add(1);
    return *this;
  }

  inline AtomicCounter&
  AtomicCounter::operator--()
  {
    add(-1);
    return *this;
  }

  inline long
  AtomicCounter::operator++(int)
  {
    return fetch_and_add(1);
  }

  inline long
  AtomicCounter::operator--(int)
  {
    return fetch_and_add(-1);
  }

  inline AtomicCounter&
  AtomicCounter::operator+=(int value)
  {
    add(value);
    return *this;
  }

  inline AtomicCounter&
  AtomicCounter::operator-=(int value)
  {
    add(-value);
    return *this;
  }

  inline
  AtomicCounter::operator int() const
  {
    return const_cast<AtomicCounter*>(this)->fetch_and_add(0);
  }

CLOSE_NAMESPACE

#endif /*BASIC_ATOMICCOUNTER_HPP*/
