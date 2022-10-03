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

#ifndef _CODE_NAMESPACEELEMENT_HPP_
#define _CODE_NAMESPACEELEMENT_HPP_

#include <Gears/Plain/Declaration/Namespace.hpp>
#include "Element.hpp"

namespace Code
{
  class NamespaceElement: public Element
  {
  public:
    NamespaceElement(
      NamespaceElement* owner,
      Declaration::Namespace* namespace_decl) noexcept;

    Declaration::Namespace_var namespace_decl() const noexcept;

    ElementList_var elements() const noexcept;

    void add_element(Element* elem) noexcept;

    Gears::IntrusivePtr<NamespaceElement>
    owner() noexcept;

    virtual void visited(ElementVisitor* visitor) const noexcept;

  protected:
    virtual ~NamespaceElement() noexcept {}
    
  private:
    NamespaceElement* owner_;
    Declaration::Namespace_var namespace_;
    ElementList_var elements_;
  };  

  typedef Gears::IntrusivePtr<NamespaceElement>
    NamespaceElement_var;
}

namespace Code
{
  inline
  NamespaceElement::NamespaceElement(
    NamespaceElement* owner,
    Declaration::Namespace* namespace_decl) noexcept
    : owner_(owner),
      namespace_(Gears::add_ref(namespace_decl)),
      elements_(new ElementList)
  {}

  inline
  Declaration::Namespace_var
  NamespaceElement::namespace_decl() const noexcept
  {
    return namespace_;
  }

  inline
  ElementList_var
  NamespaceElement::elements() const noexcept
  {
    return elements_;
  }

  inline
  void
  NamespaceElement::add_element(Element* elem) noexcept
  {
    elements_->push_back(Gears::add_ref(elem));
  }

  inline
  Gears::IntrusivePtr<NamespaceElement>
  NamespaceElement::owner() noexcept
  {
    return Gears::add_ref(owner_);
  }

  inline
  void
  NamespaceElement::visited(ElementVisitor* visitor) const noexcept
  {
    visitor->visit_i(this);
  }
}

#endif /*_CODE_NAMESPACEELEMENT_HPP_*/
