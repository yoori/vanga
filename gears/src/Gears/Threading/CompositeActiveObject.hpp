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

#ifndef GEARS_THREADING_COMPOSITEACTIVEOBJECT_HPP
#define GEARS_THREADING_COMPOSITEACTIVEOBJECT_HPP

#include <list>

#include "ActiveObject.hpp"
#include "Condition.hpp"

namespace Gears
{
  /**
   * class CompositeActiveObject
   * This implements Active Object for control of several Active Objects.
   * Use this class to delegate active/inactive state control
   * over some set of Active Objects into one Active Object. In other words,
   * it usefully for central active/deactivate state management.
   * You SHOULD NOT change active/deactivate status of any added child
   * object into this CompositeActiveObject by yourself. Use it simply,
   * and activate or deactivate they through this holder. It will give
   * you guarantee of straight activation order, first added - first
   * activated, and guarantee for stopping object, last added, deactivate
   * and join first (reverse order). Optionally, you can switch off
   * stop ordering, it will be faster than consequently stopping and
   * safe for independent ActiveObjects (other words, use parallel stopping
   * if allow application logic). This behavior tune by the constructor
   * parameter synch_termination.
   * CompositeActiveObject is reference countable object.
   */
  class CompositeActiveObject: public virtual ActiveObject
  {
  public:
    DECLARE_GEARS_EXCEPTION(ChildException, ActiveObject::Exception);
    DECLARE_GEARS_EXCEPTION(CompositeAlreadyActive, ActiveObject::AlreadyActive);

    /**
     * Construct empty not active container for
     * ActiveObjects.
     * @param synch_termination true means do immediately wait after each
     * child Active Object deactivation.
     * @param clear_on_exit whether to call clear() in destructor or not
     */
    CompositeActiveObject(
      bool synch_termination = false,
      bool clear_on_exit = true) noexcept;

    // ActiveObject interface
    /**
     * Activate all owned active objects. For empty case, simply change
     * status to Active. Throw CompositeAlreadyActive,
     * if you try to activate twice. All object activated successfully
     * or stay deactivate, if we were not able to activate any in the set.
     */
    virtual void
    activate_object()
      /*throw(CompositeAlreadyActive, ChildException, Gears::Exception)*/;

    /**
     * Deactivate (initiate stopping) all owned Active Objects.
     * Do nothing for empty and already deactivating object.
     * Perform deactivation as LIFO, last added Active Object
     * will start deactivation first.
     */
    virtual void
    deactivate_object() /*throw(Exception, Gears::Exception)*/;

    /**
     * Waits for deactivation all owned completion.
     * Perform waits as LIFO, last added Active Object will wait first.
     * That logic correspond deactivate_object method.
     */
    virtual void
    wait_object() /*throw(Exception, Gears::Exception)*/;

    /**
     * Calls clear() for all owned objects
     */
    virtual void
    clear() /*throw(Gears::Exception)*/;

    /**
     * Thread-safe check status of this Active Object.
     * @return true if active.
     */
    virtual bool
    active() /*throw(Gears::Exception)*/;

    /**
     * Deactivate and wait for stop for all owned Active Objects.
     * Clears list of the objects.
     */
    void
    clear_children() /*throw(Exception, Gears::Exception)*/;

    /**
     * This method fills CompositeActiveObject with other Active
     * objects. Control for consistent state, any time all object
     * that it own must be active or not active.
     * You delegate active state control to this container. Use
     * residual object only for work flow, do not change active status
     * through it.
     * @param child object that should go under the management of container.
     */
    void
    add_child_object(ActiveObject* child) /*throw(Exception, Gears::Exception)*/;

  protected:
    typedef std::list<ActiveObject_var> ActiveObjectList;

    typedef Mutex Lock;

  protected:
    /**
     * Perform deactivating all owned objects, and waits for
     * its completion.
     */
    virtual
    ~CompositeActiveObject() noexcept;

    /**
     * Simply calls wait_object for the given interval of objects
     */
    void
    wait_for_some_objects(ActiveObjectList::reverse_iterator rbegin,
      ActiveObjectList::reverse_iterator rend)
      /*throw(Exception, Gears::Exception)*/;

    /**
     * Thread-unsafe deactivation logic
     */
    void
    deactivate_object_i(ActiveObjectList::reverse_iterator rit)
      /*throw(Exception, Gears::Exception)*/;

  private:
    Lock lock_;
    mutable Condition deactivated_;

    volatile SigAtomicType active_state_;
    bool synchronous_;
    bool clear_on_exit_;

    ActiveObjectList child_objects_;
  };

  typedef IntrusivePtr<CompositeActiveObject>
    CompositeActiveObject_var;
}

namespace Gears
{
  inline bool
  CompositeActiveObject::active() /*throw(Gears::Exception)*/
  {
    return active_state_ == AS_ACTIVE;
  }
}

#endif
