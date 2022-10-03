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
#include "CompleteTemplateDescriptor.hpp"
#include "BaseTemplate.hpp"

namespace Declaration
{
  BaseTemplate::BaseTemplate(
    const char* name,
    unsigned long args_count_val)
    noexcept
    : BaseType(name),
      args_count_(args_count_val)
  {}

  BaseTemplate_var
  BaseTemplate::as_template() noexcept
  {
    return Gears::add_ref(this);
  }

  unsigned long
  BaseTemplate::args() const noexcept
  {
    return args_count_;
  }

  CompleteTemplateDescriptor_var
  BaseTemplate::complete_template_descriptor(
    const BaseDescriptorList& args) const
    /*throw(InvalidParam)*/
  {
    if(args.size() != args_count_)
    {
      std::ostringstream ostr;
      ostr << "can't init template descriptor - incorrect number of arguments: " <<
        args.size() << " instead " << args_count_;
      throw InvalidParam(ostr.str());
    }

    std::ostringstream name_ostr;
    name_ostr << name() << "<";
    for(BaseDescriptorList::const_iterator dit =
          args.begin();
        dit != args.end(); ++dit)
    {
      if(dit != args.begin())
      {
        name_ostr << ",";
      }
      name_ostr << (*dit)->name();
    }
    name_ostr << ">";

    return create_template_descriptor_(name_ostr.str().c_str(), args);
  }
}
