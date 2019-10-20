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

#ifndef _SIMPLEDESCRIPTOR_HPP_
#define _SIMPLEDESCRIPTOR_HPP_

#include <Gears/Basic/RefCountable.hpp>
#include "BaseDescriptor.hpp"

namespace Declaration
{
  class SimpleDescriptor: public virtual BaseDescriptor
  {
  public:
    SimpleDescriptor(
      const char* name,
      bool is_fixed_val,
      SizeType fixed_size_val)
      throw();

    virtual bool is_fixed() const throw();

    virtual SizeType fixed_size() const throw();

    virtual SimpleDescriptor_var as_simple() throw();

  protected:
    virtual ~SimpleDescriptor() throw() {}
    
  private:
    bool is_fixed_;
    SizeType fixed_size_;
  };

  typedef Gears::IntrusivePtr<SimpleDescriptor>
    SimpleDescriptor_var;
}

namespace Declaration
{
  inline
  SimpleDescriptor::SimpleDescriptor(
    const char* name_val,
    bool is_fixed_val,
    SizeType fixed_size_val)
    throw()
    : BaseType(name_val),
      BaseDescriptor(name_val),
      is_fixed_(is_fixed_val),
      fixed_size_(fixed_size_val)
  {}
  
  inline
  bool
  SimpleDescriptor::is_fixed() const throw()
  {
    return is_fixed_;
  }

  inline
  SizeType
  SimpleDescriptor::fixed_size() const throw()
  {
    return fixed_size_;
  }

  inline
  SimpleDescriptor_var
  SimpleDescriptor::as_simple() throw()
  {
    return Gears::add_ref(this);
  }
}

#endif /*_SIMPLEDESCRIPTOR_HPP_*/
