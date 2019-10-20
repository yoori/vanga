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
    add_ref() const throw();

    void
    remove_ref() const throw();

  protected:
    RefCountable() throw();

    RefCountable(const volatile RefCountable& impl) throw();

    virtual
    ~RefCountable() throw();

  private:
    mutable unsigned long ref_count_;
  };
}

namespace Gears
{
  inline
  RefCountable::RefCountable() throw()
    : ref_count_(1)
  {}

  inline
  RefCountable::RefCountable(const volatile RefCountable&)
    throw()
    : ref_count_(1)
  {}

  inline
  RefCountable::~RefCountable() throw()
  {}

  inline
  void
  RefCountable::add_ref() const throw()
  {
    ++ref_count_;
  }

  inline
  void
  RefCountable::remove_ref() const throw()
  {
    if(!--ref_count_)
    {
      delete this;
    }
  }
}

#endif /*REFERENCECOUNTING_REFCOUNTABLE_HPP*/
