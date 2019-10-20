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

#ifndef GEARS_ACTIVEOBJECTCALLBACK_HPP
#define GEARS_ACTIVEOBJECTCALLBACK_HPP

#include <ostream>

#include <Gears/Threading/ActiveObjectCallback.hpp>
#include <Gears/Logging/StreamLogger.hpp>

namespace Gears
{
  class ActiveObjectCallbackStreamImpl: public ActiveObjectCallback
  {
  public:
    explicit
    ActiveObjectCallbackStreamImpl(
      std::ostream& output_stream,
      const char* message_prefix = "ActiveObject",
      const char* aspect = 0,
      const char* code = 0)
      throw (Gears::Exception);

    virtual void
    report_error(
      Severity severity,
      const SubString& description,
      const SubString& code = SubString())
      throw();
    
  protected:
    virtual
    ~ActiveObjectCallbackStreamImpl() throw() = default;

  protected:
    const Logger_var logger_;
    const std::string message_prefix_;
    const std::string aspect_;
    const std::string code_;
  };
}

namespace Gears
{
  //
  // ActiveObjectCallbackStreamImpl class
  //

  inline
  ActiveObjectCallbackStreamImpl::ActiveObjectCallbackStreamImpl(
    std::ostream& output_stream,
    const char* message_prefix,
    const char* aspect,
    const char* code)
    throw (Gears::Exception)
    : logger_(new OStreamLogger(output_stream)),
      message_prefix_(message_prefix),
      aspect_(aspect),
      code_(code)
  {}

  inline void
  ActiveObjectCallbackStreamImpl::report_error(
    Severity severity,
    const SubString& description,
    const SubString& code)
    throw()
  {
    logger_->log(
      message_prefix_ + description.str(),
      severity,
      aspect_,
      !code.empty() ? code : code_);
  }
}

#endif


