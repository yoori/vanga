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
    add_ref() const noexcept;

    void
    remove_ref() const noexcept;

  protected:
    AtomicRefCountable() noexcept;

    AtomicRefCountable(const volatile AtomicRefCountable&) noexcept;

    virtual
    ~AtomicRefCountable() noexcept;

  protected:
    mutable Gears::AtomicCounter ref_count_;
  };
}

namespace Gears
{
  inline
  AtomicRefCountable::AtomicRefCountable() noexcept
    : ref_count_(1)
  {}

  inline
  AtomicRefCountable::AtomicRefCountable(
    const volatile AtomicRefCountable&)
    noexcept
    : ref_count_(1)
  {}

  inline
  AtomicRefCountable::~AtomicRefCountable() noexcept
  {}

  inline
  void
  AtomicRefCountable::add_ref() const noexcept
  {
    ref_count_.add(1);
  }

  inline
  void
  AtomicRefCountable::remove_ref() const noexcept
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
