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

#ifndef GEARS_ATOMICREFCOUNTABLE_HPP
#define GEARS_ATOMICREFCOUNTABLE_HPP

#include <assert.h>
#include "AtomicCounter.hpp"

namespace Gears
{
  class AtomicRefCountable
  {
  public:
    void
    add_ref() const throw();

    void
    remove_ref() const throw();

  protected:
    AtomicRefCountable() throw();

    AtomicRefCountable(const volatile AtomicRefCountable&) throw();

    virtual
    ~AtomicRefCountable() throw();

  protected:
    mutable Gears::AtomicCounter ref_count_;
  };
}

namespace Gears
{
  inline
  AtomicRefCountable::AtomicRefCountable() throw()
    : ref_count_(1)
  {}

  inline
  AtomicRefCountable::AtomicRefCountable(
    const volatile AtomicRefCountable&)
    throw()
    : ref_count_(1)
  {}

  inline
  AtomicRefCountable::~AtomicRefCountable() throw()
  {}

  inline
  void
  AtomicRefCountable::add_ref() const throw()
  {
    ref_count_.add(1);
  }

  inline
  void
  AtomicRefCountable::remove_ref() const throw()
  {
    int result_counter = ref_count_.add_and_fetch(-1);
    assert(result_counter >= 0);
    if(result_counter == 0)
    {
      delete this;
    }
  }
}

#endif
