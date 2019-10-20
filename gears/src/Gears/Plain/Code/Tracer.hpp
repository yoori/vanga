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

#ifndef _TRACER_HPP_
#define _TRACER_HPP_

#include "Element.hpp"
#include <Gears/Plain/Code/IncludeElement.hpp>
#include <Gears/Plain/Code/TypeElement.hpp>
#include <Gears/Plain/Code/TypeDefElement.hpp>
#include <Gears/Plain/Code/NamespaceElement.hpp>

#include <Gears/Plain/Declaration/BaseDescriptor.hpp>
#include <Gears/Plain/Declaration/BaseTemplate.hpp>
#include <Gears/Plain/Declaration/SimpleType.hpp>
#include <Gears/Plain/Declaration/StructDescriptor.hpp>
#include <Gears/Plain/Declaration/StructReader.hpp>
#include <Gears/Plain/Declaration/StructWriter.hpp>
#include <Gears/Plain/Declaration/CompleteTemplateDescriptor.hpp>
#include <Gears/Plain/Declaration/Namespace.hpp>

namespace Code
{
  class Tracer:
    public Gears::AtomicRefCountable,
    public Code::ElementVisitor
  {
  public:
    void generate(
      std::ostream& out,
      Code::ElementList* elements) throw();

    virtual void visit_i(const Code::Element*) throw();

    virtual void visit_i(const Code::IncludeElement*) throw();

    virtual void visit_i(const Code::NamespaceElement*) throw();

    virtual void visit_i(const Code::TypeDefElement*) throw();

    virtual void visit_i(const Code::TypeElement*) throw();

  protected:
    virtual ~Tracer() throw() {}

    void generate_descriptor_(
      const Declaration::StructDescriptor* descriptor) throw();

    void generate_reader_(
      Declaration::StructReader* reader) throw();

    void generate_writer_(
      Declaration::StructWriter* writer) throw();

  private:
    std::ostream* out_;
    std::string offset_;
  };

  typedef Gears::IntrusivePtr<Tracer>
    Tracer_var;
}

namespace Code
{
  inline
  void
  Tracer::visit_i(
    const Code::Element*) throw()
  {}

  inline
  void
  Tracer::visit_i(
    const Code::IncludeElement* elem) throw()
  {
    *out_ << offset_ << "#include <" << elem->file() << ">" << std::endl;
  }

  inline
  void
  Tracer::visit_i(
    const Code::NamespaceElement* elem) throw()
  {
    Declaration::Namespace_var namespace_decl = elem->namespace_decl();
    std::string prev_offset;

    if(namespace_decl.in() && namespace_decl->name()[0])
    {
      prev_offset = offset_;
      *out_ << offset_ << "namespace " << namespace_decl->name() << std::endl <<
        offset_ << "{";
      offset_ = offset_ + "  ";
    }
    
    for(Code::ElementList::const_iterator el_it = elem->elements()->begin();
        el_it != elem->elements()->end(); ++el_it)
    {
      visit(*el_it);
    }

    if(namespace_decl.in() && namespace_decl->name()[0])
    {
      offset_ = prev_offset;
      *out_ << offset_ << "}" << std::endl;
    }
  }

  inline
  void
  Tracer::visit_i(
    const Code::TypeDefElement* elem) throw()
  {
    *out_ << offset_ << "typedef " << elem->base_type()->name() << " " <<
      elem->type_name() << ";" << std::endl;
  }

  inline
  void
  Tracer::visit_i(
    const Code::TypeElement* elem) throw()
  {
    Declaration::BaseType_var type = elem->type();
    Declaration::BaseDescriptor_var descriptor = type->as_descriptor();

    if(descriptor.in())
    {
      Declaration::StructDescriptor_var struct_descriptor =
        descriptor->as_struct();

      assert(struct_descriptor.in());
      generate_descriptor_(struct_descriptor);
    }
    else
    {
      Declaration::BaseReader_var reader = type->as_reader();
      if(reader.in())
      {
        Declaration::StructReader_var struct_reader =
          reader->as_struct_reader();
        assert(struct_reader.in());
        
        generate_reader_(struct_reader);
      }
      else
      {
        Declaration::BaseWriter_var writer = type->as_writer();
        assert(writer.in());

        Declaration::StructWriter_var struct_writer =
          writer->as_struct_writer();
        assert(struct_writer.in());

        generate_writer_(struct_writer);
      }
    }
  }

  inline
  void
  Tracer::generate_descriptor_(
    const Declaration::StructDescriptor* struct_descriptor) throw()
  {
    *out_ << std::endl << offset_ <<
      "struct " << struct_descriptor->name() << std::endl <<
      offset_ << "{" << std::endl;

    for(Declaration::StructDescriptor::
        PosedFieldList::const_iterator field_it =
          struct_descriptor->fields()->begin();
        field_it != struct_descriptor->fields()->end(); ++field_it)
    {
      *out_ << offset_ << "  " <<
        (*field_it)->descriptor()->name() << " " << (*field_it)->name() <<
        ";" << std::endl;
    }

    *out_ << offset_ << "};" << std::endl;
  }

  inline
  void
  Tracer::generate_reader_(
    Declaration::StructReader* struct_reader) throw()
  {
    *out_ << std::endl <<
      offset_ << "reader " << struct_reader->name() << "<" <<
      struct_reader->descriptor()->name() << ">" << std::endl <<
      offset_ << "{" << std::endl;

    for(Declaration::StructReader::
        FieldReaderList::const_iterator field_it =
          struct_reader->fields()->begin();
        field_it != struct_reader->fields()->end(); ++field_it)
    {
      *out_ << offset_ << "  " <<
        (*field_it)->reader()->name() << " " << (*field_it)->name() <<
        ";" << std::endl;
    }

    *out_ << offset_ << "};" << std::endl;
  }

  inline
  void
  Tracer::generate_writer_(
    Declaration::StructWriter* struct_writer) throw()
  {
    *out_ << std::endl <<
      offset_ << "writer " << struct_writer->name() << "<" <<
      struct_writer->descriptor()->name() << ">" << std::endl <<
      offset_ << "{" << std::endl;

    for(Declaration::StructWriter::
        FieldWriterList::const_iterator field_it =
          struct_writer->fields()->begin();
        field_it != struct_writer->fields()->end(); ++field_it)
    {
      *out_ << offset_ << "  " <<
        (*field_it)->writer()->name() << " " << (*field_it)->name() <<
        ";" << std::endl;
    }

    *out_ << offset_ << "};" << std::endl;
  }
  
  inline
  void
  Tracer::generate(
    std::ostream& out,
    Code::ElementList* elements) throw()
  {
    out_ = &out;
    
    for(Code::ElementList::const_iterator el_it =
          elements->begin();
        el_it != elements->end(); ++el_it)
    {
      visit(*el_it);
    }

    out_ = 0;
  }
}

#endif /*_TRACER_HPP_*/
