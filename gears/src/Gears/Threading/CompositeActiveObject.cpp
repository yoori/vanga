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

#include <algorithm>
#include <functional>
#include <cassert>

#include <Gears/Basic/OutputMemoryStream.hpp>

#include "CompositeActiveObject.hpp"

namespace Gears
{
  CompositeActiveObject::CompositeActiveObject(
    bool synch_termination,
    bool clear_on_exit) noexcept
    : active_state_(AS_NOT_ACTIVE),
      synchronous_(synch_termination),
      clear_on_exit_(clear_on_exit)
  {}

  CompositeActiveObject::~CompositeActiveObject() noexcept
  {
    if(clear_on_exit_)
    {
      try
      {
        clear();
      }
      catch (...)
      {}
    }

    try
    {
      clear_children();
    }
    catch (...)
    {}
  }

  void
  CompositeActiveObject::activate_object()
    /*throw(CompositeAlreadyActive, ChildException, Gears::Exception)*/
  {
    static const char* FUN = "CompositeActiveObject::activate_object()";

    Lock::WriteGuard guard(lock_);

    if(active_state_ != AS_NOT_ACTIVE)
    {
      ErrorStream ostr;
      ostr << FUN << ": already active";
      throw CompositeAlreadyActive(ostr.str());
    }

    ActiveObjectList::iterator it(child_objects_.begin());
    try
    {
      for(; it != child_objects_.end(); ++it)
      {
        (*it)->activate_object();
      }
    }
    catch(const Gears::Exception& e)
    {
      ErrorStream all_errors;
      try
      {
        active_state_ = AS_ACTIVE;
        ActiveObjectList::reverse_iterator rit(it);
        deactivate_object_i(rit);
        wait_for_some_objects(rit, child_objects_.rend());
      }
      catch (const Gears::Exception& e)
      {
        all_errors << e.what();
      }

      active_state_ = AS_NOT_ACTIVE;
      ErrorStream ostr;
      ostr << FUN << ": " << e.what();
      const SubString& all_errors_str = all_errors.str();
      if(all_errors_str.size())
      {
        ostr << all_errors_str;
      }
      throw ChildException(ostr.str());
    }

    active_state_ = AS_ACTIVE;
  }

  void
  CompositeActiveObject::deactivate_object()
    /*throw(Exception, Gears::Exception)*/
  {
    Lock::WriteGuard guard(lock_);
    deactivate_object_i(child_objects_.rbegin());
  }

  void
  CompositeActiveObject::wait_object() /*throw(Exception, Gears::Exception)*/
  {
    ActiveObjectList copy_of_child_objects;

    {
      ConditionGuard cond_guard(lock_, deactivated_);
      while (active_state_ == AS_ACTIVE)
      {
        cond_guard.wait();
      }

      if(active_state_ != AS_DEACTIVATING)
      {
        return;
      }

      std::copy(
        child_objects_.begin(),
        child_objects_.end(),
        std::back_inserter(copy_of_child_objects));
    }

    wait_for_some_objects(
      copy_of_child_objects.rbegin(),
      copy_of_child_objects.rend());

    {
      Lock::WriteGuard guard(lock_);
      if(active_state_ == AS_DEACTIVATING)
      {
        active_state_ = AS_NOT_ACTIVE;
      }
    }
  }

  void
  CompositeActiveObject::add_child_object(ActiveObject* child)
    /*throw(Exception, Gears::Exception)*/
  {
    static const char* FUN = "CompositeActiveObject::add_child_object()";

    Lock::WriteGuard guard(lock_);

    try
    {
      if(active_state_ == AS_ACTIVE)
      {
        if(!child->active())
        {
          child->activate_object();
        }
      }
      else
      {
        if(child->active())
        {
          child->deactivate_object();
          child->wait_object();
        }
      }

      child_objects_.push_back(Gears::add_ref(child));
    }
    catch(const Gears::Exception& ex)
    {
      ErrorStream ostr;
      ostr << FUN << "Can't add object. Caught Gears::Exception: " << ex.what();
      throw Exception(ostr.str());
    }
  }

  void
  CompositeActiveObject::clear_children() /*throw(Exception, Gears::Exception)*/
  {
    Lock::WriteGuard guard(lock_);

    if(active_state_ != AS_NOT_ACTIVE)
    {
      ActiveObjectList::reverse_iterator rit(child_objects_.rbegin());
      deactivate_object_i(rit);
      wait_for_some_objects(rit, child_objects_.rend());
      active_state_ = AS_NOT_ACTIVE;
    }

    child_objects_.clear();
  }

  void
  CompositeActiveObject::wait_for_some_objects(
    ActiveObjectList::reverse_iterator rit,
    ActiveObjectList::reverse_iterator rend)
    /*throw(Exception, Gears::Exception)*/
  {
    static const char* FUN = "CompositeActiveObject::wait_for_some_objects()";

    for(; rit != rend; ++rit)
    {
      try
      {
        (*rit)->wait_object();
      }
      catch(const Gears::Exception& ex)
      {
        ErrorStream all_errors;
        all_errors << ex.what() << std::endl;
        for(; rit != rend; ++rit)
        {
          try
          {
            (*rit)->wait_object();
          }
          catch(const Gears::Exception& ex)
          {
            all_errors << ex.what() << std::endl;
          }
        }

        ErrorStream ostr;
        ostr << FUN <<
          ": Can't wait child active object. Caught Gears::Exception:\n";
        ostr << all_errors.str();
        throw Exception(ostr.str());
      }
    }
  }

  void
  CompositeActiveObject::deactivate_object_i(
    ActiveObjectList::reverse_iterator rit)
    /*throw(Exception, Gears::Exception)*/
  {
    static const char* FUN = "CompositeActiveObject::deactivate_object_i()";

    if(active_state_ != AS_ACTIVE)
    {
      return;
    }

    deactivated_.broadcast(); // TO CHECK

    ErrorStream all_errors;

    for(; rit != child_objects_.rend(); ++rit)
    {
      try
      {
        (*rit)->deactivate_object();
        if(synchronous_)
        {
          (*rit)->wait_object();
        }
      }
      catch(const Gears::Exception& ex)
      {
        all_errors << ex.what() << std::endl;
      }
    }

    const SubString& all_errors_str = all_errors.str();

    if(all_errors_str.size())
    {
      ErrorStream ostr;
      ostr << FUN <<
        ": Can't deactivate child active object. Caught Gears::Exception:\n";
      ostr << all_errors_str;
      throw Exception(ostr.str());
    }

    active_state_ = AS_DEACTIVATING;
  }

  void
  CompositeActiveObject::clear() /*throw(Gears::Exception)*/
  {
    Lock::WriteGuard guard(lock_);

    for(ActiveObjectList::iterator it(child_objects_.begin());
      it != child_objects_.end(); ++it)
    {
      (*it)->clear();
    }
  }
}
