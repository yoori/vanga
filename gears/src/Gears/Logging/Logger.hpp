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

#ifndef GEARS_LOGGER_HPP
#define GEARS_LOGGER_HPP

#include <cstdarg>
#include <signal.h>

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>
#include <Gears/Basic/Time.hpp>
#include <Gears/Basic/Lock.hpp>

/**
 * Common namespace for all logging related classes
 */
namespace Gears
{
  DECLARE_GEARS_EXCEPTION(LoggerException, Gears::DescriptiveException);

  class BasicLogger;
  class Logger;

  /**
   * Supplies simple usage for BaseLogger interface
   */
  class BasicLogger: public Gears::AtomicRefCountable
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, LoggerException);

  public:
    /**
     * Logger records severities.
     */
    enum Severity
    {
      EMERGENCY = 0,
      ALERT = 1,
      CRITICAL = 2,
      ERROR = 3,
      WARNING = 4,
      NOTICE = 5,
      INFO = 6,
      DEBUG = 7,
      TRACE = 8
    };

    /**
     * Gets logger trace level.
     * @return current trace level
     */
    virtual unsigned long
    log_level() noexcept = 0;

    /**
     * Sets logger trace level.
     * Records with severity value higher than trace
     * level should not be logged.
     * @param value new log level.
     */
    virtual void
    log_level(unsigned long value) noexcept = 0;

    /**
     * Logs text with severity, aspect and code specified.
     * @param text text to be logged
     * @param severity log record severity
     * @param aspect log record aspect
     * @param code log record code
     * @return success status
     */
    virtual bool
    log(const Gears::SubString& text,
      unsigned long severity = INFO,
      const Gears::SubString& aspect = Gears::SubString(),
      const Gears::SubString& code = Gears::SubString())
      = 0;

  protected:
    BasicLogger() noexcept = default;

    /**
     * Destructor
     */
    virtual
    ~BasicLogger() noexcept = default;
  };

  /**
   * Base class for all of the loggers
   */
  class Logger: public BasicLogger
  {
  protected:
    /**
     * Destructor
     */
    virtual
    ~Logger() noexcept = default;
  };

  typedef Gears::IntrusivePtr<Logger> Logger_var;

  /**
   * Simple class proxy for Logger
   * holds own logger, tranship calls to held logger
   * Immutable
   */
  class SimpleLoggerHolder: public Logger
  {
  public:
    /*
     * Construct holder with logger to hold
     * @param logger logger to hold
     */
    explicit
    SimpleLoggerHolder(Logger* logger) noexcept;

    /**
     * Gets logger trace level.
     * @return current trace level
     */
    virtual unsigned long
    log_level() noexcept;

    /**
     * Sets logger trace level.
     * Records with severity value higher than trace
     * level should not be logged.
     * @param value new log level.
     */
    virtual void
    log_level(unsigned long value) noexcept;

    /**
     * Logs text with severity, aspect and code specified.
     * @param text text to be logged
     * @param severity log record severity
     * @param aspect log record aspect
     * @param code log record code
     * @return success status
     */
    virtual
    bool
    log(const Gears::SubString& text,
      unsigned long severity = INFO,
      const Gears::SubString& aspect = Gears::SubString(),
      const Gears::SubString& code = Gears::SubString())
      noexcept;

  protected:
    /**
     * Destructor
     */
    virtual
    ~SimpleLoggerHolder() noexcept = default;

    mutable Logger_var logger_;
  };

  /*
   * Class proxy for Logger
   * Holds own logger, tranship calls to held logger
   * Thread safe
   */
  class LoggerHolder : public SimpleLoggerHolder
  {
  public:
    /*
     * Construct holder with logger to hold
     * @param logger logger to hold
     */
    explicit
    LoggerHolder(Logger* logger = 0) noexcept;

    /*
     * Set in held logger
     * @param logger logger to hold
     */
    void
    logger(Logger* logger) noexcept;

    /**
     * Gets logger trace level.
     * @return current trace level
     */
    virtual unsigned long
    log_level() noexcept;

    /**
     * Sets logger trace level.
     * Records with severity value higher than trace
     * level should not be logged.
     * @param value new log level.
     */
    virtual void
    log_level(unsigned long value) noexcept;

    /**
     * Logs text with severity, aspect and code specified.
     * @param text text to be logged
     * @param severity log record severity
     * @param aspect log record aspect
     * @param code log record code
     * @return success status
     */
    virtual
    bool
    log(const Gears::SubString& text,
      unsigned long severity = INFO,
      const Gears::SubString& aspect = Gears::SubString(),
      const Gears::SubString& code = Gears::SubString())
      noexcept;

  protected:
    /**
     * Destructor
     */
    virtual
    ~LoggerHolder() noexcept = default;

  private:
    Gears::SpinLock mutex_;
    volatile SigAtomicType log_level_;
  };

  typedef Gears::IntrusivePtr<LoggerHolder> LoggerHolder_var;

  /**
   * Logger uses another logger and puts predefined aspect and/or error_code
   * if required.
   */
  class LoggerDefaultHolder: public LoggerHolder
  {
  public:
    /**
     * Constructor
     * @param logger logger to hold
     * @param aspect aspect to use if unspecified in log() call
     * @param code error code to use if unspecified in log() call
     */
    explicit
    LoggerDefaultHolder(
      Logger* logger = 0,
      const Gears::SubString& aspect = Gears::SubString(),
      const Gears::SubString& code = Gears::SubString())
      /*throw(Gears::Exception)*/;

    /**
     * Logs text with severity, aspect and code specified.
     * @param text text to be logged
     * @param severity log record severity
     * @param aspect log record aspect
     * @param code log record code
     * @return success status
     */
    virtual
    bool
    log(const Gears::SubString& text,
      unsigned long severity = INFO,
      const Gears::SubString& aspect = Gears::SubString(),
      const Gears::SubString& code = Gears::SubString())
      noexcept;

  protected:
    virtual
    ~LoggerDefaultHolder() noexcept = default;

  protected:
    std::string aspect_;
    std::string code_;
  };

  /**
   * NullLogger
   */
  class NullLogger: public Logger
  {
  public:
    /**
     * Gets logger trace level
     * @return zero
     */
    virtual unsigned long
    log_level() noexcept;

    /**X
     * Does nothing
     * @param value new log level
     */
    virtual void
    log_level(unsigned long value) noexcept;

    /**
     * Ignores passed log record information
     * @param text text to be logged
     * @param severity log record severity
     * @param aspect log record aspect
     * @param code log record code
     * @return true
     */
    virtual bool
    log(const Gears::SubString& text,
      unsigned long severity = INFO,
      const Gears::SubString& aspect = Gears::SubString(),
      const Gears::SubString& code = Gears::SubString())
      noexcept;

  protected:
    virtual
    ~NullLogger() noexcept = default;
  };
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace Gears
{
  //
  // NullLogger class
  //

  inline
  unsigned long
  NullLogger::log_level() noexcept
  {
    return 0;
  }

  inline
  void
  NullLogger::log_level(unsigned long /*level*/) noexcept
  {}

  inline
  bool
  NullLogger::log(
    const Gears::SubString& /*text*/,
    unsigned long /*severity*/,
    const Gears::SubString& /*aspect*/,
    const Gears::SubString& /*code*/)
    noexcept
  {
    return true;
  }
}

#endif
