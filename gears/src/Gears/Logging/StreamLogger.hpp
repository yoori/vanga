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

#ifndef GEARS_STREAMLOGGER_HPP
#define GEARS_STREAMLOGGER_HPP

#include "Logger.hpp"
#include "LogHelper.hpp"

namespace Gears
{
  /**
   * Writes formatted log line into the stream specified.
   */
  class OStreamLogger:
    public Logger,
    protected LogHelper
  {
  public:
    DECLARE_GEARS_EXCEPTION(BadStream, Exception);

    OStreamLogger(
      std::ostream& ostr,
      unsigned long log_level = Logger::INFO)
      noexcept;

    virtual unsigned long
    log_level() noexcept;

    virtual void
    log_level(unsigned long value) noexcept;

    virtual bool
    log(const Gears::SubString& text,
      unsigned long severity = INFO,
      const Gears::SubString& aspect = Gears::SubString(),
      const Gears::SubString& code = Gears::SubString());

  protected:
    virtual
    ~OStreamLogger() noexcept = default;

  protected:
    std::ostream& ostr_;
    Gears::SpinLock lock_;
    SigAtomicType log_level_;
  };
}

//
// Inlines
//
namespace Gears
{
  inline
  OStreamLogger::OStreamLogger(
    std::ostream& ostr,
    unsigned long log_level)
    noexcept
    : ostr_(ostr),
      log_level_(log_level)
  {}
}

#endif
