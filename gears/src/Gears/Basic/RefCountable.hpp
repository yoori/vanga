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

#ifndef REFERENCECOUNTING_REFCOUNTABLE_HPP
#define REFERENCECOUNTING_REFCOUNTABLE_HPP

#include "Macro.hpp"

/* RefCountable
 * base for ref countable objects without thread sharing !!!
 */

namespace Gears
{
  class RefCountable
  {
  public:
    void
    add_ref() const noexcept;

    void
    remove_ref() const noexcept;

  protected:
    RefCountable() noexcept;

    RefCountable(const volatile RefCountable& impl) noexcept;

    virtual
    ~RefCountable() noexcept;

  private:
    mutable unsigned long ref_count_;
  };
}

namespace Gears
{
  inline
  RefCountable::RefCountable() noexcept
    : ref_count_(1)
  {}

  inline
  RefCountable::RefCountable(const volatile RefCountable&)
    noexcept
    : ref_count_(1)
  {}

  inline
  RefCountable::~RefCountable() noexcept
  {}

  inline
  void
  RefCountable::add_ref() const noexcept
  {
    ++ref_count_;
  }

  inline
  void
  RefCountable::remove_ref() const noexcept
  {
    if(!--ref_count_)
    {
      delete this;
    }
  }
}

#endif /*REFERENCECOUNTING_REFCOUNTABLE_HPP*/
