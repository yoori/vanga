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

#ifndef _CODE_TYPEDEFELEMENT_HPP_
#define _CODE_TYPEDEFELEMENT_HPP_

#include <Gears/Plain/Declaration/BaseType.hpp>
#include "Element.hpp"

namespace Code
{
  class TypeDefElement: public Element
  {
  public:
    TypeDefElement(
      const char* type_name_val,
      Declaration::BaseType* base_type_val)
      noexcept;

    const char* type_name() const noexcept;

    Declaration::BaseType_var base_type() const noexcept;

    virtual void visited(ElementVisitor* visitor) const noexcept;

  protected:
    virtual ~TypeDefElement() noexcept {}
    
  private:
    std::string type_name_;
    Declaration::BaseType_var base_type_;
  };
}

namespace Code
{
  inline
  TypeDefElement::TypeDefElement(
    const char* type_name_val,
    Declaration::BaseType* base_type_val)
    noexcept
    : type_name_(type_name_val),
      base_type_(Gears::add_ref(base_type_val))
  {}

  inline
  const char*
  TypeDefElement::type_name() const noexcept
  {
    return type_name_.c_str();
  }

  inline
  Declaration::BaseType_var
  TypeDefElement::base_type() const noexcept
  {
    return base_type_;
  }
  
  inline
  void
  TypeDefElement::visited(ElementVisitor* visitor) const noexcept
  {
    visitor->visit_i(this);
  }
}

#endif /*_CODE_TYPEDEFELEMENT_HPP_*/
