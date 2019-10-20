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

#ifndef PLAIN_DECLARATION_LISTTEMPLATE_HPP
#define PLAIN_DECLARATION_LISTTEMPLATE_HPP

#include <Gears/Basic/RefCountable.hpp>
#include "BaseTemplate.hpp"
#include "CompleteTemplateDescriptor.hpp"

namespace Declaration
{
  /* BaseArrayTemplate */
  class BaseArrayTemplate:
    public virtual Gears::AtomicRefCountable,
    public BaseTemplate
  {
  public:
    BaseArrayTemplate(
      const char* name,
      unsigned long header_size) throw();

  protected:
    virtual ~BaseArrayTemplate() throw() {}

    virtual CompleteTemplateDescriptor_var
    create_template_descriptor_(
      const char* name,
      const BaseDescriptorList& args) const
      throw(InvalidParam);

  private:
    CompleteTemplateDescriptor_var create_array_simple_type_(
      BaseDescriptor* descriptor) const
      throw();

    CompleteTemplateDescriptor_var create_array_struct_type_(
      BaseDescriptor* descriptor) const
      throw();

  private:
    unsigned long header_size_;
  };

  /* ArrayTemplate */
  class ArrayTemplate: public BaseArrayTemplate
  {
  public:
    ArrayTemplate() throw();

  protected:
    virtual ~ArrayTemplate() throw() {}
  };

  /* CompatibilityListTemplate */
  class CompatibilityListTemplate: public BaseArrayTemplate
  {
  public:
    CompatibilityListTemplate() throw();

  protected:
    virtual ~CompatibilityListTemplate() throw() {}
  };
}

#endif /*PLAIN_DECLARATION_LISTTEMPLATE_HPP*/
