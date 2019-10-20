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

#ifndef _BASEWRITER_HPP_
#define _BASEWRITER_HPP_

#include <list>
#include <set>
#include <Gears/Basic/Exception.hpp>
#include "BaseType.hpp"

namespace Declaration
{
  typedef std::set<std::string> MappingSpecifierSet;

  class SimpleWriter;
  typedef Gears::IntrusivePtr<SimpleWriter>
    SimpleWriter_var;

  class StructWriter;
  typedef Gears::IntrusivePtr<StructWriter>
    StructWriter_var;

  class BaseWriter: public virtual BaseType
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);
    DECLARE_GEARS_EXCEPTION(InvalidMappingSpecifier, Exception);

    BaseWriter(const char* name_val) throw();

    virtual BaseDescriptor_var descriptor() throw() = 0;

    virtual void
    check_mapping_specifiers(
      const MappingSpecifierSet& mapping_specifiers)
      throw(InvalidMappingSpecifier) = 0;

    /* non fixed field */
    virtual SimpleWriter_var as_simple_writer() throw();

    virtual StructWriter_var as_struct_writer() throw();

    virtual BaseWriter_var as_writer() throw();

  protected:
    virtual ~BaseWriter() throw() {}
  };

  typedef Gears::IntrusivePtr<BaseWriter>
    BaseWriter_var;

  typedef std::list<BaseWriter_var> BaseWriterList;
}

#endif /*_BASEWRITER_HPP_*/
