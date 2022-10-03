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

#ifndef _CODE_INCLUDEELEMENT_HPP_
#define _CODE_INCLUDEELEMENT_HPP_

#include "Element.hpp"

namespace Code
{
  class IncludeElement: public Element
  {
  public:
    IncludeElement(const char* file_val) noexcept;

    const char* file() const noexcept;

    virtual void visited(ElementVisitor* visitor) const noexcept;

  protected:
    virtual ~IncludeElement() noexcept {}
    
  private:
    std::string file_;
  };
}

namespace Code
{
  inline
  IncludeElement::IncludeElement(const char* file_val) noexcept
    : file_(file_val)
  {}

  inline
  const char*
  IncludeElement::file() const noexcept
  {
    return file_.c_str();
  }
  
  inline
  void
  IncludeElement::visited(ElementVisitor* visitor) const noexcept
  {
    visitor->visit_i(this);
  }
}

#endif /*_CODE_INCLUDEELEMENT_HPP_*/
