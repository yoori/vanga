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

#ifndef PLAIN_CODE_ELEMENT_HPP_
#define PLAIN_CODE_ELEMENT_HPP_

#include <list>
#include <iostream>

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>

namespace Code
{
  class Element;
  class IncludeElement;
  class NamespaceElement;
  class TypeDefElement;
  class TypeElement;

  class ElementVisitor
  {
  public:
    virtual
    ~ElementVisitor() noexcept {}

    virtual void
    visit(const Element*) noexcept;

    virtual void
    visit_i(const Element*) noexcept = 0;

    virtual void
    visit_i(const IncludeElement*) noexcept;

    virtual void
    visit_i(const NamespaceElement*) noexcept;

    virtual void
    visit_i(const TypeDefElement*) noexcept;

    virtual void
    visit_i(const TypeElement*) noexcept;
  };
  
  class Element: public virtual Gears::AtomicRefCountable
  {
  public:
    virtual void
    visited(ElementVisitor* visitor) const noexcept;

  protected:
    virtual
    ~Element() noexcept {}
  };

  typedef Gears::IntrusivePtr<Element> Element_var;

  struct ElementList:
    public std::list<Element_var>,
    public Gears::AtomicRefCountable
  {
  protected:
    virtual
    ~ElementList() noexcept {}
  };
  
  typedef Gears::IntrusivePtr<ElementList>
    ElementList_var;
}

namespace Code
{
  inline void
  ElementVisitor::visit(const Element* elem) noexcept
  {
    elem->visited(this);
  }
}

#endif /*PLAIN_CODE_ELEMENT_HPP_*/
