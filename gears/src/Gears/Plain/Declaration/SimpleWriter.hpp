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

#ifndef _SIMPLEWRITER_HPP_
#define _SIMPLEWRITER_HPP_

#include <Gears/Basic/OutputMemoryStream.hpp>
#include <Gears/Basic/RefCountable.hpp>
#include "BaseWriter.hpp"

namespace Declaration
{
  class SimpleWriter: public BaseWriter
  {
    /* required override holder_traits, cast_fun */
  public:
    struct CppWriteTraits: public Gears::AtomicRefCountable
    {
      CppWriteTraits(
        const char* write_type_reference_val,
        const char* write_type_cast_val,
        const char* write_type_const_reference_val,
        const char* write_type_const_cast_val,
        const char* write_type_val,
        const char* holder_type_name_val,
        const char* field_type_suffix_val = "")
        : write_type_name(write_type_reference_val),
          write_type_cast(write_type_cast_val),
          write_type_const_name(write_type_const_reference_val),
          write_type_const_cast(write_type_const_cast_val),
          write_type(write_type_val),
          holder_type_name(holder_type_name_val),
          field_type_suffix(field_type_suffix_val)
      {}

      // write accessor return type
      std::string write_type_name;
      // for fixed: void* -> write_type_name&
      std::string write_type_cast;

      // const write accessor return type
      std::string write_type_const_name;
      // for fixed: const void* -> const write_type_name&
      std::string write_type_const_cast;

      std::string write_type;

      // for not fixed type: holder type
      std::string holder_type_name;

      std::string field_type_suffix;

    protected:
      virtual ~CppWriteTraits() throw()
      {}
    };

    typedef Gears::IntrusivePtr<CppWriteTraits>
      CppWriteTraits_var;

    class CppWriteTraitsGenerator: public Gears::AtomicRefCountable
    {
    public:
      virtual bool
      check_mapping_specifier(const char* mapping_specifier)
        throw() = 0;

      virtual CppWriteTraits_var
      generate(const MappingSpecifierSet& mapping_specifiers)
        throw() = 0;

    protected:
      virtual ~CppWriteTraitsGenerator() throw()
      {}
    };

    typedef Gears::IntrusivePtr<CppWriteTraitsGenerator>
      CppWriteTraitsGenerator_var;

    SimpleWriter(
      const char* name_val,
      CppWriteTraitsGenerator* cpp_write_traits_generator)
      throw();

    virtual SimpleWriter_var as_simple_writer() throw();

    virtual void
    check_mapping_specifiers(
      const Declaration::MappingSpecifierSet& mapping_specifiers)
      throw(InvalidMappingSpecifier);

    CppWriteTraitsGenerator_var
    cpp_write_traits_generator() const throw();

  protected:
    virtual ~SimpleWriter() throw() {}

  private:
    CppWriteTraitsGenerator_var cpp_write_traits_generator_;
  };

  typedef Gears::IntrusivePtr<SimpleWriter>
    SimpleWriter_var;

  class CppWriteTraitsGeneratorNoSpecifiersImpl:
    public SimpleWriter::CppWriteTraitsGenerator
  {
  public:
    CppWriteTraitsGeneratorNoSpecifiersImpl(
      SimpleWriter::CppWriteTraits* cpp_write_traits)
      : cpp_write_traits_(Gears::add_ref(cpp_write_traits))
    {}

    virtual bool
    check_mapping_specifier(const char* /*mapping_specifier*/)
      throw()
    {
      return false;
    }

    virtual SimpleWriter::CppWriteTraits_var
    generate(const MappingSpecifierSet& /*mapping_specifiers*/)
      throw()
    {
      return cpp_write_traits_;
    }

  protected:
    virtual ~CppWriteTraitsGeneratorNoSpecifiersImpl() throw()
    {}

  protected:
    SimpleWriter::CppWriteTraits_var cpp_write_traits_;
  };
}

namespace Declaration
{
  // SimpleWriter impl
  inline
  SimpleWriter::SimpleWriter(
    const char* name_val,
    CppWriteTraitsGenerator* cpp_write_traits_generator)
    throw()
    : BaseType(name_val),
      BaseWriter(name_val),
      cpp_write_traits_generator_(
        Gears::add_ref(cpp_write_traits_generator))
  {}

  inline
  SimpleWriter_var
  SimpleWriter::as_simple_writer() throw()
  {
    return Gears::add_ref(this);
  }

  inline
  void
  SimpleWriter::check_mapping_specifiers(
    const Declaration::MappingSpecifierSet& mapping_specifiers)
    throw(InvalidMappingSpecifier)
  {
    for(Declaration::MappingSpecifierSet::const_iterator it =
          mapping_specifiers.begin();
        it != mapping_specifiers.end(); ++it)
    {
      if(!cpp_write_traits_generator_->check_mapping_specifier(it->c_str()))
      {
        Gears::ErrorStream ostr;
        ostr << "Incorrect specifier: " << *it;
        throw InvalidMappingSpecifier(ostr.str());
      }
    }
  }

  inline
  SimpleWriter::CppWriteTraitsGenerator_var
  SimpleWriter::cpp_write_traits_generator() const throw()
  {
    return cpp_write_traits_generator_;
  }
}

#endif /*_SIMPLEWRITER_HPP_*/
