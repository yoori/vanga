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

/**
* @file   StreamLogger.hpp
* @author Karen Aroutiounov
*/

#include <memory>
//#include <Gears/Basic/Function.hpp>
#include <Gears/Basic/OutputMemoryStream.hpp>

#include "StreamLogger.hpp"

namespace Gears
{
  unsigned long
  OStreamLogger::log_level() noexcept
  {
    return log_level_;
  }

  void
  OStreamLogger::log_level(unsigned long value) noexcept
  {
    // lock for memory barrier
    Gears::SpinLock::WriteGuard guard(lock_);
    log_level_ = value;
  }

  bool
  OStreamLogger::log(
    const Gears::SubString& text,
    unsigned long severity,
    const Gears::SubString& aspect,
    const Gears::SubString& code)
    noexcept
  {
    static const char* FUN = "OStreamLogHandler::publish()";

    if(severity > static_cast<unsigned long>(log_level_))
    {
      return true;
    }

    const Gears::ExtendedTime now =
      Gears::Time::get_time_of_day().get_time(Time::TZ_GMT);

    BufferList buffers;
    get_buffer_(buffers);

    prepare_buf_(
      **buffers.begin(),
      &now,
      text,
      &severity,
      &aspect,
      &code);

    ostr_ << (*buffers.begin())->data() << std::flush;

    if (!ostr_.good())
    {
      Gears::ErrorStream ostr;
      ostr << FUN << ": stream is dead";
      throw BadStream(ostr.str());
    }

    return false;
  }
}
