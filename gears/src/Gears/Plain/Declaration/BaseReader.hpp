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

#ifndef _BASEREADER_HPP_
#define _BASEREADER_HPP_

#include <list>
#include "BaseType.hpp"
#include "BaseDescriptor.hpp"

namespace Declaration
{
  class SimpleReader;
  typedef Gears::IntrusivePtr<SimpleReader>
    SimpleReader_var;

  class StructReader;
  typedef Gears::IntrusivePtr<StructReader>
    StructReader_var;

  class BaseReader: public virtual BaseType
  {
  public:
    BaseReader(const char* name_val) throw();

    virtual BaseDescriptor_var descriptor() throw() = 0;

    virtual SimpleReader_var as_simple_reader() throw();

    virtual StructReader_var as_struct_reader() throw();

    virtual BaseReader_var as_reader() throw();

  protected:
    virtual
    ~BaseReader() throw () = default;
  };

  typedef Gears::IntrusivePtr<BaseReader>
    BaseReader_var;

  typedef std::list<BaseReader_var> BaseReaderList;
}

#endif /*_BSEREADER_HPP_*/
