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

#ifndef _STRUCTREADER_HPP_
#define _STRUCTREADER_HPP_

#include <list>
#include "StructDescriptor.hpp"
#include "BaseReader.hpp"

namespace Declaration
{
  class StructReader:
    public virtual Gears::AtomicRefCountable,
    public BaseReader
  {
  public:
    class FieldReader: public Gears::AtomicRefCountable
    {
    public:
      FieldReader(
        StructDescriptor::Field* field_val,
        BaseReader* reader_val)
        throw();

      const char* name() const throw();

      StructDescriptor::Field_var descriptor_field() const throw();

      BaseReader_var reader() const throw();

    protected:
      virtual
      ~FieldReader() throw () = default;

    private:
      StructDescriptor::Field_var field_;
      BaseReader_var reader_;
    };

    typedef Gears::IntrusivePtr<FieldReader>
      FieldReader_var;

    struct FieldReaderList:
      public std::list<FieldReader_var>,
      public Gears::AtomicRefCountable
    {
    protected:
      virtual ~FieldReaderList() throw() {}
    };

    typedef Gears::IntrusivePtr<FieldReaderList>
      FieldReaderList_var;

  public:
    StructReader(
      const char* name_val,
      BaseDescriptor* descriptor_val,
      FieldReaderList* fields_val);

    FieldReaderList_var fields() const;

    virtual BaseDescriptor_var descriptor() throw();

    virtual StructReader_var as_struct_reader() throw();

  protected:
    virtual
    ~StructReader() throw () = default;

  private:
    BaseDescriptor_var descriptor_;
    FieldReaderList_var fields_;
  };

  typedef Gears::IntrusivePtr<StructReader>
    StructReader_var;
}

namespace Declaration
{
  inline
  StructReader::FieldReader::FieldReader(
    StructDescriptor::Field* field_val,
    BaseReader* reader_val)
    throw()
    : field_(Gears::add_ref(field_val)),
      reader_(Gears::add_ref(reader_val))
  {}

  inline
  const char*
  StructReader::FieldReader::name() const throw()
  {
    return field_->name();
  }

  inline
  StructDescriptor::Field_var
  StructReader::FieldReader::descriptor_field() const throw()
  {
    return field_;
  }

  inline
  BaseReader_var
  StructReader::FieldReader::reader() const throw()
  {
    return reader_;
  }

  inline
  StructReader::StructReader(
    const char* name_val,
    BaseDescriptor* descriptor_val,
    FieldReaderList* fields_val)
    : BaseType(name_val),
      BaseReader(name_val),
      descriptor_(Gears::add_ref(descriptor_val)),
      fields_(Gears::add_ref(fields_val))
  {}

  inline
  BaseDescriptor_var
  StructReader::descriptor() throw()
  {
    return descriptor_;
  }

  inline
  StructReader::FieldReaderList_var
  StructReader::fields() const
  {
    return fields_;
  }

  inline
  StructReader_var
  StructReader::as_struct_reader() throw()
  {
    return Gears::add_ref(this);
  }
}

#endif /*_STRUCTREADER_HPP_*/
