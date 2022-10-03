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

#ifndef PLAIN_STRUCTDESCRIPTOR_HPP_
#define PLAIN_STRUCTDESCRIPTOR_HPP_

#include <list>
#include <cstring>

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/RefCountable.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>

#include "BaseDescriptor.hpp"

namespace Declaration
{
  class StructDescriptor:
    public virtual BaseDescriptor
  {
  public:
    class Field: public Gears::AtomicRefCountable
    {
    public:
      Field(BaseDescriptor* descriptor_val, const char* name_val)
        /*throw (Gears::Exception)*/;

      Field(const Field& other)
        /*throw (Gears::Exception)*/;

      const char*
      name() const noexcept;

      BaseDescriptor_var descriptor() const noexcept;

    protected:
      virtual ~Field() noexcept {}

    private:
      BaseDescriptor_var descriptor_;
      std::string name_;
    };

    typedef Gears::IntrusivePtr<Field>
      Field_var;

    struct FieldList: public std::list<Field_var>,
      public Gears::AtomicRefCountable
    {
    protected:
      virtual ~FieldList() noexcept {}
    };

    typedef Gears::IntrusivePtr<FieldList>
      FieldList_var;

    class PosedField: public Field
    {
    public:
      PosedField(const Field& field_val, unsigned long pos_val)
        noexcept;

      unsigned long
      pos() const noexcept;

    protected:
      virtual ~PosedField() noexcept {}

    private:
      unsigned long pos_;
    };

    typedef Gears::IntrusivePtr<PosedField>
      PosedField_var;

    struct PosedFieldList: public std::list<PosedField_var>,
      public Gears::AtomicRefCountable
    {
    protected:
      virtual ~PosedFieldList() noexcept {}
    };

    typedef Gears::IntrusivePtr<PosedFieldList>
      PosedFieldList_var;

  public:
    StructDescriptor(
      const char* name_val,
      FieldList* fields_val) noexcept;

    PosedFieldList_var fields() const noexcept;

    PosedField_var find_field(const char* name) const noexcept;

    virtual bool is_fixed() const noexcept;

    virtual SizeType fixed_size() const noexcept;

    virtual StructDescriptor_var as_struct() noexcept;

  protected:
    virtual ~StructDescriptor() noexcept {}

  private:
    PosedFieldList_var fields_;
    bool is_fixed_;
    SizeType fixed_size_;
  };

  typedef Gears::IntrusivePtr<StructDescriptor>
    StructDescriptor_var;
}

namespace Declaration
{
  inline
  StructDescriptor::Field::Field(
    BaseDescriptor* descriptor_val,
    const char* name_val)
    /*throw (Gears::Exception)*/
    : descriptor_(Gears::add_ref(descriptor_val)),
      name_(name_val)
  {}

  inline
  StructDescriptor::Field::Field(
    const Field& other)
    /*throw (Gears::Exception)*/
    : Gears::AtomicRefCountable(),
      descriptor_(other.descriptor_),
      name_(other.name_)
  {}

  inline
  const char*
  StructDescriptor::Field::name() const
    noexcept
  {
    return name_.c_str();
  }

  inline
  StructDescriptor::PosedField::PosedField(
    const StructDescriptor::Field& field_val,
    unsigned long pos_val)
    noexcept
    : StructDescriptor::Field(field_val),
      pos_(pos_val)
  {}

  inline
  unsigned long
  StructDescriptor::PosedField::pos() const
    noexcept
  {
    return pos_;
  }

  inline
  BaseDescriptor_var
  StructDescriptor::Field::descriptor() const
    noexcept
  {
    return descriptor_;
  }
  
  inline
  StructDescriptor::StructDescriptor(
    const char* name_val,
    FieldList* fields_val)
    noexcept
    : BaseType(name_val),
      BaseDescriptor(name_val),
      fields_(new PosedFieldList()),
      is_fixed_(true)
  {
    unsigned long pos = 0;
    for(FieldList::const_iterator fit = fields_val->begin();
        fit != fields_val->end(); ++fit)
    {
      is_fixed_ &= (*fit)->descriptor()->is_fixed();
      fields_->push_back(PosedField_var(new PosedField(*(*fit), pos)));
      pos += (*fit)->descriptor()->fixed_size();
    }

    fixed_size_ = pos;
  }

  inline
  StructDescriptor::PosedFieldList_var
  StructDescriptor::fields() const noexcept
  {
    return fields_;
  }

  inline
  StructDescriptor::PosedField_var
  StructDescriptor::find_field(const char* name) const noexcept
  {
    for(StructDescriptor::PosedFieldList::const_iterator fit =
          fields_->begin();
        fit != fields_->end(); ++fit)
    {
      if(strcmp(name, (*fit)->name()) == 0)
      {
        return *fit;
      }
    }

    return StructDescriptor::PosedField_var();
  }

  inline
  bool
  StructDescriptor::is_fixed() const noexcept
  {
    return is_fixed_;
  }

  inline
  SizeType
  StructDescriptor::fixed_size() const noexcept
  {
    return fixed_size_;
  }

  inline
  StructDescriptor_var
  StructDescriptor::as_struct() noexcept
  {
    return Gears::add_ref(this);
  }
}

#endif /*PLAIN_STRUCTDESCRIPTOR_HPP_*/
