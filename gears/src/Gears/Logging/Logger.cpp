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

#include "Logger.hpp"

namespace Gears
{
  //
  // SimpleLoggerHolder class
  //

  SimpleLoggerHolder::SimpleLoggerHolder(Logger* logger) noexcept
    : logger_(Gears::add_ref(logger))
  {}

  unsigned long
  SimpleLoggerHolder::log_level() noexcept
  {
    return logger_->log_level();
  }

  void
  SimpleLoggerHolder::log_level(unsigned long value) noexcept
  {
    logger_->log_level(value);
  }

  bool
  SimpleLoggerHolder::log(
    const Gears::SubString& text,
    unsigned long severity,
    const Gears::SubString& aspect,
    const Gears::SubString& code)
    noexcept
  {
    return logger_->log(text, severity, aspect, code);
  }

  //
  // LoggerHolder class
  //

  LoggerHolder::LoggerHolder(Logger* logger) noexcept
    : SimpleLoggerHolder(logger),
      log_level_(logger ? logger->log_level() : 0)
  {}

  void
  LoggerHolder::logger(Logger* new_logger) noexcept
  {
    Logger_var nl(Gears::add_ref(new_logger));

    {
      Gears::SpinLock::WriteGuard guard(mutex_);
      std::swap(logger_, nl);
      log_level_ = logger_ ? logger_->log_level() : 0;
    }
  }

  unsigned long
  LoggerHolder::log_level() noexcept
  {
    return log_level_;
  }

  void
  LoggerHolder::log_level(unsigned long value) noexcept
  {
    Logger_var logger;

    {
      Gears::SpinLock::WriteGuard guard(mutex_);
      if (!logger_)
      {
        return;
      }
      logger = logger_;
      log_level_ = value;
    }

    logger->log_level(value);
  }

  bool
  LoggerHolder::log(
    const Gears::SubString& text,
    unsigned long severity,
    const Gears::SubString& aspect,
    const Gears::SubString& code) noexcept
  {
    if (severity > static_cast<unsigned long>(log_level_))
    {
      return true;
    }

    Logger_var logger;

    {
      Gears::SpinLock::WriteGuard guard(mutex_);
      logger = logger_;
    }

    return logger ? logger->log(text, severity, aspect, code) : false;
  }

  //
  // LoggerDefaultHolder class
  //

  LoggerDefaultHolder::LoggerDefaultHolder(
    Logger* logger,
    const Gears::SubString& aspect,
    const Gears::SubString& code)
    /*throw (Gears::Exception)*/
    : LoggerHolder(logger),
      aspect_(aspect.str()),
      code_(code.str())
  {}

  bool
  LoggerDefaultHolder::log(
    const Gears::SubString& text,
    unsigned long severity,
    const Gears::SubString& aspect,
    const Gears::SubString& code)
    noexcept
  {
    return LoggerHolder::log(
      text,
      severity,
      !aspect.empty() ? aspect : aspect_,
      !code.empty() ? code : code_);
  }
}
