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

#ifndef PLAIN_DECLARATION_BASETEMPLATE_HPP
#define PLAIN_DECLARATION_BASETEMPLATE_HPP

#include <Gears/Basic/Exception.hpp>

#include "BaseType.hpp"
#include "BaseDescriptor.hpp"

namespace Declaration
{
  class BaseTemplate: public BaseType
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);
    DECLARE_GEARS_EXCEPTION(InvalidParam, Exception);

    BaseTemplate(const char* name, unsigned long args_count)
      noexcept;

    virtual BaseTemplate_var
    as_template() noexcept;

    unsigned long
    args() const noexcept;

    CompleteTemplateDescriptor_var
    complete_template_descriptor(
      const BaseDescriptorList& args) const
      /*throw(InvalidParam)*/;

  protected:
    virtual
    ~BaseTemplate() noexcept {}

    virtual CompleteTemplateDescriptor_var
    create_template_descriptor_(
      const char* name,
      const BaseDescriptorList& args) const
      /*throw(InvalidParam)*/ = 0;

  private:
    unsigned long args_count_;
  };

  typedef Gears::IntrusivePtr<BaseTemplate>
    BaseTemplate_var;
}

#endif /*PLAIN_DECLARATION_BASETEMPLATE_HPP*/
