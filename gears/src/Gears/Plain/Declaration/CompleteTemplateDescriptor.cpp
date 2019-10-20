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

#include <sstream>
#include <cstring>
#include "CompleteTemplateDescriptor.hpp"

namespace Declaration
{
  CompleteTemplateDescriptor::CompleteTemplateDescriptor(
    const char* name,
    const BaseDescriptorList& args)
    throw()
    : BaseType(name),
      BaseDescriptor(name),
      args_(args)
  {}

  CompleteTemplateDescriptor_var
  CompleteTemplateDescriptor::as_complete_template()
    throw()
  {
    return Gears::add_ref(this);
  }

  const BaseDescriptorList&
  CompleteTemplateDescriptor::args() const throw()
  {
    return args_;
  }

  BaseReader_var
  CompleteTemplateDescriptor::complete_template_reader(
    const BaseReaderList& args)
    throw(InvalidParam)
  {
    if(args.size() != args_.size())
    {
      std::ostringstream ostr;
      ostr << "can't init template reader - incorrect number of arguments: " <<
        args.size() << " instead " << args_.size();
      throw InvalidParam(ostr.str());
    }

    int arg_i = 0;
    BaseDescriptorList::const_iterator dit = args_.begin();
    for(BaseReaderList::const_iterator rit = args.begin();
        rit != args.end(); ++rit, ++dit, ++arg_i)
    {
      if(::strcmp((*rit)->descriptor()->name(), (*dit)->name()) != 0)
      {
        std::ostringstream ostr;
        ostr << "can't init template reader - argument #" << arg_i <<
          ": type '" << (*rit)->name() << "' isn't reader of '" <<
          (*dit)->name() << "', it is reader of '" <<
          (*rit)->descriptor()->name() << "'";
        throw InvalidParam(ostr.str());
      }    
    }

    return create_template_reader_(args);
  }

  BaseWriter_var
  CompleteTemplateDescriptor::complete_template_writer(
    const BaseWriterList& args)
    throw(InvalidParam)
  {
    if(args.size() != args_.size())
    {
      std::ostringstream ostr;
      ostr << "can't init template writer - incorrect number of arguments: " <<
        args.size() << " instead " << args_.size();
      throw InvalidParam(ostr.str());
    }

    int arg_i = 0;
    BaseDescriptorList::const_iterator dit = args_.begin();
    for(BaseWriterList::const_iterator rit = args.begin();
        rit != args.end(); ++rit, ++dit, ++arg_i)
    {
      if(::strcmp((*rit)->descriptor()->name(), (*dit)->name()) != 0)
      {
        std::ostringstream ostr;
        ostr << "can't init template writer - argument #" << arg_i <<
          ": type '" << (*rit)->name() << "' isn't writer of '" <<
          (*dit)->name() << "', it is reader of '" <<
          (*rit)->descriptor()->name() << "'";
        throw InvalidParam(ostr.str());
      }    
    }

    return create_template_writer_(args);
  }
}
