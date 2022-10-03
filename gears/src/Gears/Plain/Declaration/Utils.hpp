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

#ifndef PLAIN_DECLARATION_HPP
#define PLAIN_DECLARATION_HPP

#include "BaseDescriptor.hpp"
#include "SimpleDescriptor.hpp"
#include "StructDescriptor.hpp"
#include "BaseReader.hpp"
#include "SimpleReader.hpp"
#include "StructReader.hpp"
#include "BaseWriter.hpp"
#include "SimpleWriter.hpp"
#include "StructWriter.hpp"

namespace Declaration
{
  void type_traits(std::ostream& out, Declaration::BaseType* type)
    noexcept;  
}

namespace Declaration
{
  inline
  void type_traits(
    std::ostream& out, Declaration::BaseType* type)
    noexcept
  {
    Declaration::BaseDescriptor_var type_descriptor =
      type->as_descriptor();

    if(type_descriptor.in())
    {
      Declaration::SimpleDescriptor_var simple_descriptor =
        type_descriptor->as_simple();
      Declaration::StructDescriptor_var struct_descriptor =
        type_descriptor->as_struct();
      Declaration::CompleteTemplateDescriptor_var complete_template_descriptor =
        type_descriptor->as_complete_template();
      
      out << " descriptor";
      if(simple_descriptor.in() || struct_descriptor.in() ||
         complete_template_descriptor.in())
      {
        out << "( " << (simple_descriptor.in() ? "simple " : "") <<
          (struct_descriptor.in() ? "struct " : "") <<
          (complete_template_descriptor.in() ? "complete-template " : "") << ")";
      }
    }

    Declaration::BaseReader_var type_reader =
      type->as_reader();

    if(type_reader.in())
    {
      Declaration::SimpleReader_var simple_reader =
        type_reader->as_simple_reader();
      Declaration::StructReader_var struct_reader =
        type_reader->as_struct_reader();
      
      out << " reader";
      if(simple_reader.in() || struct_reader.in())
      {
        out << "( " << (simple_reader.in() ? "simple " : "") <<
          (struct_reader.in() ? "struct " : "") << ")";
      }
    }
    
    Declaration::BaseWriter_var type_writer =
      type->as_writer();

    if(type_writer.in())
    {
      Declaration::SimpleWriter_var simple_writer =
        type_writer->as_simple_writer();
      Declaration::StructWriter_var struct_writer =
        type_writer->as_struct_writer();

      out << " writer";
      if(simple_writer.in() || struct_writer.in())
      {
        out << "( " << (simple_writer.in() ? "simple " : "") <<
          (struct_writer.in() ? "struct " : "") << ")";
      }
    }    
  }
}

#endif /*PLAIN_DECLARATION_HPP*/
