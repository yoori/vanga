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

#include "Config.hpp"
#include "WriterGenerator.hpp"
#include "DescriptorGenerator.hpp"

namespace Cpp
{
  DescriptorGenerator::DescriptorGenerator(
    std::ostream& out,
    std::ostream& out_cpp,
    const char* offset)
    noexcept
    : out_(out),
      out_cpp_(out_cpp),
      offset_(offset)
  {}

  void
  DescriptorGenerator::generate_decl(
    Declaration::StructDescriptor* struct_descriptor)
    noexcept
  {
    out_ << offset_ << "/* " << struct_descriptor->name() <<
      " descriptor declaration */" << std::endl;

    generate_descriptor_base_decl_(struct_descriptor);

    WriterGenerator(out_, out_cpp_, offset_.c_str()).generate_default_buffers_decl(
      struct_descriptor);

    WriterGenerator(out_, out_cpp_, offset_.c_str()).generate_protected_decl(
      struct_descriptor);
  }

  void
  DescriptorGenerator::generate_impl(
    Declaration::StructDescriptor* struct_descriptor)
    noexcept
  {
    Declaration::StructDescriptor::PosedFieldList_var fields =
      struct_descriptor->fields();

    WriterGenerator(out_, out_cpp_, offset_.c_str()).generate_default_buffers_impl(
      struct_descriptor);

    WriterGenerator(out_, out_cpp_, offset_.c_str()).generate_protected_impl(
      struct_descriptor);
  }

  void
  DescriptorGenerator::generate_descriptor_base_decl_(
    const Declaration::StructDescriptor* struct_descriptor) noexcept
  {
    /* struct <descriptor name>_base
     * {
     *   static const bool IS_FIXED = <is fixed>;
     * }
     */
    out_ << offset_ << "struct " << struct_descriptor->name() << BASE_SUFFIX <<
      std::endl <<
      offset_ << "{" << std::endl <<
      offset_ << "  static const bool IS_FIXED = " <<
      (struct_descriptor->is_fixed() ? "true" : "false") <<
      ";" << std::endl;

    unsigned long fixed_size = struct_descriptor->fields()->empty() ? 0 :
      (*struct_descriptor->fields()->rbegin())->pos() +
      (*struct_descriptor->fields()->rbegin())->descriptor()->fixed_size();

    out_ << offset_ << "  static const unsigned FIXED_SIZE = " <<
      fixed_size << ";" << std::endl << std::endl;

    for(Declaration::StructDescriptor::
        PosedFieldList::const_iterator field_it =
          struct_descriptor->fields()->begin();
        field_it != struct_descriptor->fields()->end(); ++field_it)
    {
      out_ << offset_ << "  static const unsigned " <<
        (*field_it)->name() << FIELD_OFFSET_SUFFIX << " = " <<
        (*field_it)->pos() << ";" << std::endl;
    }

    out_ << offset_ << "};" << std::endl << std::endl;
  }
}
