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

#ifndef _BASETYPE_HPP_
#define _BASETYPE_HPP_

#include <string>

#include <Gears/Basic/AtomicRefCountable.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>

/*
 * BaseType ->
 *   as_descriptor: BaseDescriptor ->
 *     as_simple: SimpleDescriptor
 *     as_struct: StructDescriptor
 *     as_complete_template: CompleteTemplateDescriptor
 *   as_reader: BaseReader ->
 *     as_simple_reader: SimpleReader
 *     as_struct_reader: StructReader
 *   as_writer: BaseWriter ->
 *     as_simple_writer: SimpleWriter
 *     as_struct_writer: StructWriter
 *   as_template: BaseTemplate
 */
namespace Declaration
{
  class BaseDescriptor;
  typedef Gears::IntrusivePtr<BaseDescriptor>
    BaseDescriptor_var;

  class BaseReader;
  typedef Gears::IntrusivePtr<BaseReader>
    BaseReader_var;

  class BaseWriter;
  typedef Gears::IntrusivePtr<BaseWriter>
    BaseWriter_var;

  class BaseTemplate;
  typedef Gears::IntrusivePtr<BaseTemplate>
    BaseTemplate_var;

  struct BaseType: public virtual Gears::AtomicRefCountable
  {
    BaseType(const char* name_val);

    const char*
    name() const; // local name

    virtual BaseDescriptor_var
    as_descriptor() noexcept;

    virtual BaseReader_var
    as_reader() noexcept;

    virtual BaseWriter_var
    as_writer() noexcept;

    virtual BaseTemplate_var
    as_template() noexcept;

  protected:
    virtual ~BaseType() noexcept {}

  private:
    std::string name_;
  };

  typedef Gears::IntrusivePtr<BaseType> BaseType_var;
}

#endif
