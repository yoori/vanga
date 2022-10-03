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

#ifndef _CODE_TYPEELEMENT_HPP_
#define _CODE_TYPEELEMENT_HPP_

#include <Gears/Plain/Declaration/BaseType.hpp>
#include "Element.hpp"

namespace Code
{
  class TypeElement: public Element
  {
  public:
    TypeElement(Declaration::BaseType* type_val) noexcept;

    Declaration::BaseType_var type() const noexcept;

    virtual void visited(ElementVisitor* visitor) const noexcept;

  private:
    Declaration::BaseType_var type_;
  };
}

namespace Code
{
  inline
  TypeElement::TypeElement(Declaration::BaseType* type_val)
    noexcept
    : type_(Gears::add_ref(type_val))
  {}

  inline
  Declaration::BaseType_var
  TypeElement::type() const noexcept
  {
    return type_;
  }

  inline
  void
  TypeElement::visited(ElementVisitor* visitor) const noexcept
  {
    visitor->visit_i(this);
  }
}

#endif /*_CODE_TYPEELEMENT_HPP_*/
