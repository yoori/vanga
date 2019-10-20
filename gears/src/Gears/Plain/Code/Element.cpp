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

#include "Element.hpp"
#include "IncludeElement.hpp"
#include "NamespaceElement.hpp"
#include "TypeDefElement.hpp"
#include "TypeElement.hpp"

namespace Code
{  
  void ElementVisitor::visit_i(const IncludeElement* elem) throw()
  {
    visit_i(static_cast<const Element*>(elem));
  }

  void ElementVisitor::visit_i(const NamespaceElement* elem) throw()
  {
    visit_i(static_cast<const Element*>(elem));
  }

  void ElementVisitor::visit_i(const TypeDefElement* elem) throw()
  {
    visit_i(static_cast<const Element*>(elem));
  }  

  void ElementVisitor::visit_i(const TypeElement* elem) throw()
  {
    visit_i(static_cast<const Element*>(elem));
  }

  void Element::visited(ElementVisitor* visitor) const throw()
  {
    visitor->visit_i(this);
  }
}
