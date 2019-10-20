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

#ifndef PLAIN_PARSING_PROCESSOR_HPP
#define PLAIN_PARSING_PROCESSOR_HPP

#include <sstream>
#include <Gears/Plain/Declaration/SimpleDescriptor.hpp>
#include <Gears/Plain/Declaration/StructDescriptor.hpp>
#include <Gears/Plain/Declaration/SimpleReader.hpp>
#include <Gears/Plain/Declaration/StructReader.hpp>
#include <Gears/Plain/Declaration/SimpleWriter.hpp>
#include <Gears/Plain/Declaration/StructWriter.hpp>
#include <Gears/Plain/Declaration/Namespace.hpp>

#include <Gears/Plain/Code/Element.hpp>
#include <Gears/Plain/Code/NamespaceElement.hpp>
#include <Gears/Plain/Code/TypeElement.hpp>
#include <Gears/Plain/Code/TypeDefElement.hpp>
#include <Gears/Plain/Code/Declarator.hpp>

typedef std::string Identifier;
typedef std::list<Identifier> IdentifierList;

struct TypeSpecifier
{
  TypeSpecifier(const char* name_val) throw();

  TypeSpecifier(const char* name_val, const std::list<TypeSpecifier>& args_val)
    throw();

  std::string full_name() const throw();

  const std::string name;
  const std::list<TypeSpecifier> args;
};

typedef std::list<TypeSpecifier> TypeSpecifierList;

namespace Parsing
{
  class Processor: public Gears::AtomicRefCountable
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);
    DECLARE_GEARS_EXCEPTION(AlreadyDeclared, Exception);
    DECLARE_GEARS_EXCEPTION(IncorrectType, Exception);

  public:
    Processor(Code::ElementList* elements_val) throw();

    void error(
      unsigned long line,
      unsigned long column,
      const char* message) throw();

    Declaration::Namespace_var
    root_namespace() const throw();

    Declaration::BaseType_var
    find_type(const TypeSpecifier& type_name) const throw();

    // namespace declaration
    void open_namespace(const char* name);

    void close_namespace();

    // descriptor declaration
    void open_descriptor(const char* name)
      throw(AlreadyDeclared);

    void add_descriptor_field(
      const TypeSpecifier& field_type, const char* name)
      throw(IncorrectType);

    void close_descriptor() throw();

    // reader declaration
    void open_reader(const char* name, const char* base_type_name)
      throw(IncorrectType);

    void add_reader_field(
      const TypeSpecifier& field_type, const char* name)
      throw(IncorrectType);

    void close_reader() throw();

    void create_auto_reader(const char* name, const char* base_type_name)
      throw(IncorrectType);

    // writer declaration
    void open_writer(const char* name, const char* base_type_name)
      throw (IncorrectType);

    void add_writer_field(
      const TypeSpecifier& field_type,
      const char* name,
      const IdentifierList& mapping_specifiers = IdentifierList())
      throw (IncorrectType);

    void close_writer() throw();

    void create_auto_writer(const char* name, const char* base_type_name)
      throw(IncorrectType);

    void clone_type(const char* new_name, const TypeSpecifier& base_type)
      throw();

  protected:
    enum DescriptorResolve
    {
      DR_DIRECT,
      DR_READER,
      DR_WRITER
    };

  protected:
    virtual ~Processor() throw() {}

    // basic help functions
    Declaration::BaseDescriptor_var
    resolve_descriptor_(const TypeSpecifier& type_specifier)
      throw(IncorrectType);

    Declaration::BaseReader_var
    resolve_reader_(
      const TypeSpecifier& type_specifier,
      Declaration::BaseDescriptor* descriptor = 0)
      throw(IncorrectType);

    Declaration::BaseWriter_var
    resolve_writer_(
      const TypeSpecifier& type_specifier,
      Declaration::BaseDescriptor* descriptor = 0)
      throw(IncorrectType);

    Declaration::BaseTemplate_var
    resolve_template_(const char* name)
      throw(IncorrectType);

    // auto types manipulations
    Declaration::StructReader_var
    resolve_struct_auto_reader_(
      Declaration::StructDescriptor* struct_descriptor)
      throw (Exception);

    Declaration::StructWriter_var
    resolve_struct_auto_writer_(
      Declaration::StructDescriptor* struct_descriptor)
      throw (IncorrectType);

    Declaration::BaseReader_var
    resolve_auto_reader_(const char* name) throw(IncorrectType);

    Declaration::BaseWriter_var
    resolve_auto_writer_(const char* name) throw(IncorrectType);

    Declaration::StructReader_var
    generate_auto_reader_(
      const char* name,
      Declaration::StructDescriptor* struct_descriptor)
      throw(Exception);

    Declaration::StructWriter_var
    generate_auto_writer_(
      const char* name,
      Declaration::StructDescriptor* struct_descriptor)
      throw (IncorrectType);

    // template manipulations
    Declaration::CompleteTemplateDescriptor_var
    init_template_descriptor_(
      const TypeSpecifier& type_specifier,
      DescriptorResolve descriptor_resolving = DR_DIRECT)
      throw(IncorrectType);

    Declaration::BaseReader_var
    init_template_reader_(
      const TypeSpecifier& type_specifier,
      Declaration::BaseDescriptor* descriptor = 0 // complete template descriptor
      )
      throw(IncorrectType);

    Declaration::BaseWriter_var
    init_template_writer_(
      const TypeSpecifier& type_specifier,
      Declaration::BaseDescriptor* descriptor = 0 // complete template descriptor
      )
      throw(IncorrectType);

  private:
    /* parsing context */
    Declaration::Namespace_var global_namespace_;
    Code::NamespaceElement_var current_namespace_;
//  Code::ElementList_var elements_;
//  Code::Declarator_var declarator_;

    // descriptor declaration
    std::string descriptor_name_;
    Declaration::StructDescriptor::FieldList_var descriptor_fields_;

    // reader declaration
    std::string reader_name_;
    Declaration::StructDescriptor_var reader_descriptor_;
    Declaration::StructReader::FieldReaderList_var reader_fields_;

    // writer declaration
    std::string writer_name_;
    Declaration::StructDescriptor_var writer_descriptor_;
    Declaration::StructWriter::FieldWriterList_var writer_fields_;
  };

  typedef Gears::IntrusivePtr<Processor> Processor_var;
}

// Inlines
// TypeSpecifier impl
inline
TypeSpecifier::TypeSpecifier(const char* name_val) throw()
  : name(name_val)
{}

inline
TypeSpecifier::TypeSpecifier(
  const char* name_val, const std::list<TypeSpecifier>& args_val)
  throw()
  : name(name_val), args(args_val)
{}

inline
std::string
TypeSpecifier::full_name() const throw()
{
  if(!args.empty())
  {
    std::ostringstream name_ostr;
    name_ostr << name << "<";
    for(std::list<TypeSpecifier>::const_iterator ait = args.begin();
        ait != args.end(); ++ait)
    {
      if(ait != args.begin())
      {
        name_ostr << ",";
      }
      name_ostr << ait->full_name();
    }
    name_ostr << ">";
    return name_ostr.str();
  }

  return name;
}

namespace Parsing
{
  inline
  Declaration::Namespace_var
  Processor::root_namespace() const throw()
  {
    return global_namespace_;
  }
}

#endif /*PLAIN_PARSING_PROCESSOR_HPP*/
