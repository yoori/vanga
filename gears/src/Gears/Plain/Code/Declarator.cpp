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

#include <Gears/Plain/Declaration/ListTemplate.hpp>
#include "Declarator.hpp"

namespace Code
{
  Declarator::Declarator(
    Declaration::Namespace* root_namespace_val,
    Code::ElementList* elements_val)
    throw()
    : root_namespace_(Gears::add_ref(root_namespace_val)),
      elements_(Gears::add_ref(elements_val)),
      current_namespace_(Gears::add_ref(root_namespace_val))
  {
    root_namespace_->add_type(Declaration::BaseType_var(
      new Declaration::CompatibilityListTemplate()));

    root_namespace_->add_type(Declaration::BaseType_var(
      new Declaration::ArrayTemplate()));

    root_namespace_->add_type(Declaration::BaseType_var(
      new Declaration::SimpleType(
        "char",
        true,
        1,
        Declaration::SimpleReader::CppReadTraits(
          "char",
          "PlainTypes::CharCastPolicy::read_cast",
          false,
          "char"),
        Declaration::SimpleWriter::CppWriteTraitsGenerator_var(
          new Declaration::CppWriteTraitsGeneratorNoSpecifiersImpl(
            Declaration::SimpleWriter::CppWriteTraits_var(
              new Declaration::SimpleWriter::CppWriteTraits(
                "char&",
                "PlainTypes::CharCastPolicy::write_cast",
                "char",
                "PlainTypes::CharCastPolicy::read_cast",
                "char",
                "char"))))
      )));

    root_namespace_->add_type(Declaration::BaseType_var(
      new Declaration::SimpleType(
        "uint",
        true,
        4,
        Declaration::SimpleReader::CppReadTraits(
          "uint32_t",
          "PlainTypes::UIntCastPolicy::read_cast",
          false,
          "uint32_t"),
        Declaration::SimpleWriter::CppWriteTraitsGenerator_var(
          new Declaration::CppWriteTraitsGeneratorNoSpecifiersImpl(
            Declaration::SimpleWriter::CppWriteTraits_var(
              new Declaration::SimpleWriter::CppWriteTraits(
                "uint32_t&",
                "PlainTypes::UIntCastPolicy::write_cast",
                "uint32_t",
                "PlainTypes::UIntCastPolicy::read_cast",
                "uint32_t",
                "uint32_t"))))
      )));

    root_namespace_->add_type(Declaration::BaseType_var(
      new Declaration::SimpleType(
        "int",
        true,
        4,
        Declaration::SimpleReader::CppReadTraits(
          "int32_t",
          "PlainTypes::IntCastPolicy::read_cast",
          false,
          "int32_t"),
        Declaration::SimpleWriter::CppWriteTraitsGenerator_var(
          new Declaration::CppWriteTraitsGeneratorNoSpecifiersImpl(
            Declaration::SimpleWriter::CppWriteTraits_var(
              new Declaration::SimpleWriter::CppWriteTraits(
                "int32_t&",
                "PlainTypes::IntCastPolicy::write_cast",
                "int32_t",
                "PlainTypes::IntCastPolicy::read_cast",
                "int32_t",
                "int32_t"))))
      )));

    root_namespace_->add_type(Declaration::BaseType_var(
      new Declaration::SimpleType(
        "uint64",
        true,
        8,
        Declaration::SimpleReader::CppReadTraits(
          "uint64_t",
          "PlainTypes::UInt64CastPolicy::read_cast",
          false,
          "uint64_t"),
        Declaration::SimpleWriter::CppWriteTraitsGenerator_var(
          new Declaration::CppWriteTraitsGeneratorNoSpecifiersImpl(
            Declaration::SimpleWriter::CppWriteTraits_var(
              new Declaration::SimpleWriter::CppWriteTraits(
                "uint64_t&",
                "PlainTypes::UInt64CastPolicy::write_cast",
                "uint64_t",
                "PlainTypes::UInt64CastPolicy::read_cast",
                "uint64_t",
                "uint64_t"))))
      )));

    root_namespace_->add_type(Declaration::BaseType_var(
      new Declaration::SimpleType(
        "string",
        false,
        4,
        Declaration::SimpleReader::CppReadTraits(
          "const char*",
          "PlainTypes::String::read_cast",
          false,
          "const char*"),
        Declaration::SimpleWriter::CppWriteTraitsGenerator_var(
          new Declaration::CppWriteTraitsGeneratorNoSpecifiersImpl(
            Declaration::SimpleWriter::CppWriteTraits_var(
              new Declaration::SimpleWriter::CppWriteTraits(
                "std::string&",
                "",
                "const std::string&",
                "",
                "std::string",
                "PlainTypes::String"))))
      )));

    root_namespace_->add_type(Declaration::BaseType_var(
      new Declaration::SimpleType(
        "bytes",
        false,
        8,
        Declaration::SimpleReader::CppReadTraits(
          "PlainTypes::ConstBuf",
          "PlainTypes::Buffer::read_cast",
          false,
          "PlainTypes::ConstBuf"),
        Declaration::SimpleWriter::CppWriteTraitsGenerator_var(
          new Declaration::CppWriteTraitsGeneratorNoSpecifiersImpl(
            Declaration::SimpleWriter::CppWriteTraits_var(
              new Declaration::SimpleWriter::CppWriteTraits(
                "Generics::MemBuf&",
                "",
                "const Generics::MemBuf&",
                "",
                "Generics::MemBuf",
                "PlainTypes::Buffer"))))
      )));
  }

  void Declarator::open_namespace(const char* name) throw()
  {
    current_namespace_ = current_namespace_->add_namespace(name);
  }

  void Declarator::close_namespace() throw()
  {
    current_namespace_ = current_namespace_->owner();
  }

  Declaration::Namespace_var
  Declarator::current_namespace() throw()
  {
    return current_namespace_;
  }
  
  Declaration::StructDescriptor_var
  Declarator::declare_struct(
    const char* name,
    Declaration::StructDescriptor::FieldList* fields)
    throw()
  {
    Declaration::StructDescriptor_var new_struct_descriptor(
      new Declaration::StructDescriptor(name, fields));
    current_namespace_->add_type(new_struct_descriptor);
    elements_->push_back(
      Code::Element_var(new Code::TypeElement(new_struct_descriptor)));
    return new_struct_descriptor;
  }

  Declaration::StructReader_var
  Declarator::declare_struct_reader(
    const char* name,
    Declaration::StructDescriptor* struct_descriptor,
    Declaration::StructReader::FieldReaderList* fields)
    throw()
  {
    Declaration::StructReader_var new_struct_reader(
      new Declaration::StructReader(name, struct_descriptor, fields));
    current_namespace_->add_type(new_struct_reader);
    elements_->push_back(Code::Element_var(
      new Code::TypeElement(new_struct_reader)));
    return new_struct_reader;
  }

  Declaration::StructWriter_var
  Declarator::declare_struct_writer(
    const char* name,
    Declaration::StructDescriptor* struct_descriptor,
    Declaration::StructWriter::FieldWriterList* fields)
    throw()
  {
    Declaration::StructWriter_var new_struct_writer(
      new Declaration::StructWriter(name, struct_descriptor, fields));
    current_namespace_->add_type(new_struct_writer);
    elements_->push_back(Code::Element_var(
      new Code::TypeElement(new_struct_writer)));
    return new_struct_writer;
  }
}
