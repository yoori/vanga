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

#ifndef GEARS_THREADING_ACTIVEOBJECT_HPP
#define GEARS_THREADING_ACTIVEOBJECT_HPP

#include <vector>
#include <limits.h>

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>
#include <Gears/Basic/Singleton.hpp>
#include <Gears/Basic/SubString.hpp>

namespace Gears
{
  /**
   * Reference countable callback for report errors.
   */
  class ActiveObjectCallback:
    public virtual AtomicRefCountable
  {
  public:
    enum Severity
    {
      CRITICAL_ERROR = 0,
      ERROR = 1,
      WARNING = 2
    };

    virtual
    void
    report_error(
      Severity severity,
      const SubString& description,
      const SubString& error_code = SubString())
      noexcept = 0;

  protected:
    virtual
    ~ActiveObjectCallback() noexcept;
  };

  typedef IntrusivePtr<ActiveObjectCallback>
    ActiveObjectCallback_var;

  class ActiveObject:
    public virtual AtomicRefCountable,
    private AllDestroyer<ActiveObject>
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);
    DECLARE_GEARS_EXCEPTION(NotSupported, Exception);
    DECLARE_GEARS_EXCEPTION(AlreadyActive, Exception);
    DECLARE_GEARS_EXCEPTION(InvalidArgument, Exception);

  public:
    virtual
    void
    activate_object()
      /*throw(AlreadyActive, Exception, Gears::Exception)*/ = 0;

    virtual
    void
    deactivate_object()
      /*throw(Exception, Gears::Exception)*/ = 0;

    virtual
    void
    wait_object()
      /*throw(Exception, Gears::Exception)*/ = 0;

    virtual
    bool
    active()
      /*throw(Gears::Exception)*/ = 0;

    virtual
    void
    clear() /*throw(Gears::Exception)*/;

  public:
    static const char PRINTABLE_NAME[];

  protected:
    enum ACTIVE_STATE
    {
      AS_ACTIVE,
      AS_DEACTIVATING,
      AS_NOT_ACTIVE
    };

  protected:
    virtual
    ~ActiveObject() noexcept;
  };

  typedef IntrusivePtr<ActiveObject>
    ActiveObject_var;
} // namespace Gears

//
// Inlines
//
namespace Gears
{
  //
  // ActiveObjectCallback class
  //

  inline
  ActiveObjectCallback::~ActiveObjectCallback() noexcept
  {}

  //
  // ActiveObject class
  //

  inline
  ActiveObject::~ActiveObject() noexcept
  {}

  inline
  void
  ActiveObject::clear() /*throw(Gears::Exception)*/
  {}
}

#endif /*GEARS_THREADING_ACTIVEOBJECT_HPP*/
