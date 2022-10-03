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

#ifndef PLAIN_DECLARATION_BASEDESCRIPTOR_HPP
#define PLAIN_DECLARATION_BASEDESCRIPTOR_HPP

#include <list>
#include "BaseType.hpp"

namespace Declaration
{
  class SimpleDescriptor;
  typedef Gears::IntrusivePtr<SimpleDescriptor>
    SimpleDescriptor_var;

  class StructDescriptor;
  typedef Gears::IntrusivePtr<StructDescriptor>
    StructDescriptor_var;

  class TemplateDescriptor;
  typedef Gears::IntrusivePtr<TemplateDescriptor>
    TemplateDescriptor_var;

  class CompleteTemplateDescriptor;
  typedef Gears::IntrusivePtr<CompleteTemplateDescriptor>
    CompleteTemplateDescriptor_var;

  typedef unsigned long SizeType;

  class BaseDescriptor: public virtual BaseType
  {
  public:
    BaseDescriptor(const char* name_val);

    virtual SimpleDescriptor_var as_simple() noexcept;

    virtual StructDescriptor_var as_struct() noexcept;

    virtual CompleteTemplateDescriptor_var as_complete_template() noexcept;

    virtual bool is_fixed() const noexcept = 0;

    virtual SizeType fixed_size() const noexcept = 0;

    virtual BaseDescriptor_var as_descriptor() noexcept;

  protected:
    virtual ~BaseDescriptor() noexcept {}
  };

  typedef Gears::IntrusivePtr<BaseDescriptor>
    BaseDescriptor_var;

  typedef std::list<BaseDescriptor_var> BaseDescriptorList;
}

#endif /*PLAIN_DECLARATION_BASEDESCRIPTOR_HPP*/
