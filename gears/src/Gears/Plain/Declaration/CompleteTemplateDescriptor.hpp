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

#ifndef PLAIN_DECLARATION_COMPLETETEMPLATEDESCRIPTOR_HPP
#define PLAIN_DECLARATION_COMPLETETEMPLATEDESCRIPTOR_HPP

#include "BaseDescriptor.hpp"
#include "BaseReader.hpp"
#include "BaseWriter.hpp"

namespace Declaration
{
  class CompleteTemplateDescriptor: public virtual BaseDescriptor
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);
    DECLARE_GEARS_EXCEPTION(InvalidParam, Exception);

    CompleteTemplateDescriptor(
      const char* name,
      const BaseDescriptorList& args)
      throw();

    virtual bool is_fixed() const throw() = 0;

    virtual SizeType fixed_size() const throw() = 0;

    CompleteTemplateDescriptor_var
    as_complete_template() throw();

    BaseReader_var
    complete_template_reader(const BaseReaderList& args)
      throw(InvalidParam);

    BaseWriter_var
    complete_template_writer(const BaseWriterList& args)
      throw(InvalidParam);

    const BaseDescriptorList& args() const throw();

  protected:
    virtual ~CompleteTemplateDescriptor() throw() {}

    virtual BaseReader_var
    create_template_reader_(const BaseReaderList& args)
      throw(InvalidParam) = 0;

    virtual BaseWriter_var
    create_template_writer_(const BaseWriterList& args)
      throw(InvalidParam) = 0;

  private:
    BaseDescriptorList args_;
  };

  typedef Gears::IntrusivePtr<CompleteTemplateDescriptor>
    CompleteTemplateDescriptor_var;
}

#endif /*PLAIN_DECLARATION_COMPLETETEMPLATEDESCRIPTOR_HPP*/
