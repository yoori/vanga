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

#ifndef BASIC_SINGLETON_HPP
#define BASIC_SINGLETON_HPP

#include <memory>
#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>

#include <Gears/Basic/Uncopyable.hpp>
#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/OutputMemoryStream.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>

#include "AtomicCounter.hpp"
#include "AtomicRefCountable.hpp"
#include "Lock.hpp"

namespace Gears
{
  /**
   * All instances of this and descending classes are removed only after
   * exit(3) call or main exit.
   * Useful for singletons.
   */
  class AtExitDestroying: private Uncopyable
  {
  public:
    enum DEFAULT_PRIORITIES
    {
      DP_USUAL_SINGLETON = 0,
      // LoadCounters must be destroyed after all
      DP_LOUD_COUNTER = 16384,
    };

  protected:
    /**
     * Constructor
     * Optionally registers destroy function in atexit
     * Inserts object into the list of destroyable objects
     * @param priority objects with lesser value will be destroyed sooner
     */
    AtExitDestroying(int priority) throw ();

    /**
     * Destructor
     */
    virtual
    ~AtExitDestroying() throw ();

  private:
    typedef StaticInitializedMutex Lock;

  private:
    /**
     * Destroys the registered objects
     */
    static void
    destroy_at_exit_() throw ();

    static Lock mutex_;
    static bool registered_;
    static AtExitDestroying* lower_priority_head_;
    AtExitDestroying* lower_priority_;
    AtExitDestroying* equal_priority_;
    int priority_;
  };

  namespace Helper
  {
    /**
     * Destroys Object on exit
     */
    template <typename Object, typename Pointer, const int PRIORITY>
    class AtExitDestroyer: public AtExitDestroying
    {
    public:
      /**
       * Constructor
       * @param object object to destroy at exit
       */
      AtExitDestroyer(Object* object) throw ();

    protected:
      /**
       * Destructor
       */
      virtual
      ~AtExitDestroyer() throw ();

    private:
      Pointer object_;
    };

    /**
     * Adapter for std::auto_ptr
     */
    template<typename Type>
    class AutoPtr: public std::auto_ptr<Type>
    {
    public:
      AutoPtr(Type* object) throw ();

      Type* in() throw ();

      Type* retn() throw ();
    };

    /**
     * Adapter for simple pointer
     */
    template<typename Type>
    class SimplePtr
    {
    public:
      SimplePtr(Type* object) throw ();

      Type* in() throw ();

      Type* retn() throw ();

    private:
      Type* ptr_;
    };
  }

  /**
   * Singleton
   * Safe to use in multithreaded environment (even before main() call).
   * Single object is destroyed after exit(3) call or main() exit.
   * It is not safe to call instance() at that time.
   */
  template <typename Single, typename Pointer = Helper::AutoPtr<Single>,
    const int PRIORITY = AtExitDestroying::DP_USUAL_SINGLETON>
  class Singleton
  {
  public:
    typedef Single InstanceType;

    /**
     * Optionally creates a new Single object or returns reference to the
     * existing.
     * It is not safe to call it after exit(3) call or main() exit.
     * @return reference to the unique object
     */
    static Single&
    instance() throw (Gears::Exception);

  private:
    typedef StaticInitializedMutex Lock;

  private:
    static Lock mutex_;
    static SigAtomicType initialized_;
    static Single* volatile instance_;
  };


  /**
   * Template class is aimed to allow only one instance of certain type
   * to exist at the given point in time. Lifetime of each of those instances
   * are controlled manually.
   */
  template <typename Determinator>
  class Unique: private Uncopyable
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, DescriptiveException);

  protected:
    /**
     * Constructor
     * Successfully constructs the object only if another one does not exist.
     */
    Unique() throw (Gears::Exception, Exception);
    /**
     * Destructor
     * Allows creating of another object of the same type.
     */
    ~Unique() throw ();

  private:
    typedef StaticInitializedMutex Lock;

  private:
    static Lock mutex_;
    static Unique<Determinator>* volatile existing_;
  };


  /**
   * Class informs if some objects of the specified class have not been
   * destroyed on program shutdown.
   */
  template <typename Determinator>
  class AllDestroyer
  {
  protected:
    /**
     * Constructor.
     * Increases the number of objects created.
     */
    AllDestroyer() throw ();
    /**
     * Constructor.
     * Increases the number of objects created.
     */
    AllDestroyer(const AllDestroyer&) throw ();
    /**
     * Destructor.
     * Decreases the number of objects created.
     */
    ~AllDestroyer() throw ();

  private:
    class LoudCounter:
      public AtomicRefCountable,
      public AtomicCounter
    {
    public:
      LoudCounter() throw();

      void check() throw();

    private:
      virtual ~LoudCounter() throw();
    };

    typedef IntrusivePtr<LoudCounter> LoudCounter_var;

    class LoudCounterHolder
    {
    public:
      typedef Singleton<
        LoudCounterHolder,
        Helper::AutoPtr<LoudCounterHolder>,
        AtExitDestroying::DP_LOUD_COUNTER> Single;

      LoudCounterHolder() throw (Gears::Exception);

      ~LoudCounterHolder() throw ();

      LoudCounter*
      counter() throw ();

    private:
      LoudCounter_var counter_;
    };

    LoudCounter_var counter_;
  };
}

namespace Gears
{
  // AtExitDestroying impl
  inline
  AtExitDestroying::~AtExitDestroying() throw()
  {}

  namespace Helper
  {
    // AtExitDestroyer impl
    template <typename Object, typename Pointer, const int PRIORITY>
    AtExitDestroyer<Object, Pointer, PRIORITY>::AtExitDestroyer(
      Object* object) throw ()
      : AtExitDestroying(PRIORITY), object_(object)
    {}

    template <typename Object, typename Pointer, const int PRIORITY>
    AtExitDestroyer<Object, Pointer, PRIORITY>::~AtExitDestroyer() throw ()
    {}

    //
    // class AutoPtr
    //
    template <typename Type>
    AutoPtr<Type>::AutoPtr(Type* object) throw ()
      : std::auto_ptr<Type>(object)
    {}

    template <typename Type>
    Type*
    AutoPtr<Type>::in() throw ()
    {
      return this->get();
    }

    template <typename Type>
    Type*
    AutoPtr<Type>::retn() throw ()
    {
      return this->release();
    }

    //
    // class SimplePtr
    //
    template <typename Type>
    SimplePtr<Type>::SimplePtr(Type* object) throw ()
      : ptr_(object)
    {}

    template <typename Type>
    Type*
    SimplePtr<Type>::in() throw ()
    {
      return ptr_;
    }

    template <typename Type>
    Type*
    SimplePtr<Type>::retn() throw ()
    {
      Type* ptr(ptr_);
      ptr_ = 0;
      return ptr;
    }
  }

  // Singleton impl

  // All of these are initialized statically, before instance calls
  template <typename Single, typename Pointer, const int PRIORITY>
  typename Singleton<Single, Pointer, PRIORITY>::Lock
  Singleton<Single, Pointer, PRIORITY>::mutex_ = STATIC_MUTEX_INITIALIZER;

  template <typename Single, typename Pointer, const int PRIORITY>
  SigAtomicType
  Singleton<Single, Pointer, PRIORITY>::initialized_ = 0;

  template <typename Single, typename Pointer, const int PRIORITY>
  Single*
  volatile Singleton<Single, Pointer, PRIORITY>::instance_ = 0;

  template <typename Single, typename Pointer, const int PRIORITY>
  Single&
  Singleton<Single, Pointer, PRIORITY>::instance() throw(Gears::Exception)
  {
    if(!initialized_)
    {
      {
        Lock::WriteGuard guard(mutex_);
        if(!instance_)
        {
          Pointer single(new Single);
          new Helper::AtExitDestroyer<Single, Pointer, PRIORITY>(single.in());
          instance_ = single.retn();
        }
      }
      initialized_ = true;
    }
    return *instance_;
  }

  /* Unique impl */
  template <typename Determinator>
  typename Unique<Determinator>::Lock
  Unique<Determinator>::mutex_ = STATIC_MUTEX_INITIALIZER;

  template<typename Determinator>
  Unique<Determinator>* volatile
  Unique<Determinator>::existing_ = NULL;

  template<typename Determinator>
  Unique<Determinator>::Unique() throw(Exception, Gears::Exception)
  {
    static const char* FUN = "Unique<...>::Unique()";

    Lock::WriteGuard guard(mutex_);

    if(existing_)
    {
      ErrorStream ostr;
      ostr << FUN << ": another unique " << existing_ << " still exists";
      throw Exception(ostr);
    }

    existing_ = this;
  }

  template<typename Determinator>
  Unique<Determinator>::~Unique() throw()
  {
    Lock::WriteGuard guard(mutex_);
    assert(existing_ == this);
    existing_ = 0;
  }

  /* AllDestroyer::LoudCounter impl */
  template<typename Determinator>
  AllDestroyer<Determinator>::LoudCounter::LoudCounter() throw()
    : AtomicCounter(0)
  {}

  template<typename Determinator>
  AllDestroyer<Determinator>::LoudCounter::~LoudCounter() throw()
  {}

  template<typename Determinator>
  void
  AllDestroyer<Determinator>::LoudCounter::check() throw()
  {
    int counter = *this;
    if(counter)
    {
      ErrorStream ostr;
      ostr << "Not been removed " << counter << " of " <<
        Determinator::PRINTABLE_NAME << "\n",
      ::write(STDERR_FILENO, ostr.str().data(), ostr.str().size());
    }
  }

  /* AllDestroyer::LoudCounterHolder class */
  template<typename Determinator>
  AllDestroyer<Determinator>::LoudCounterHolder::LoudCounterHolder()
    throw(Gears::Exception)
    : counter_(new LoudCounter)
  {}

  template<typename Determinator>
  AllDestroyer<Determinator>::LoudCounterHolder::~LoudCounterHolder() throw()
  {
    counter_->check();
  }

  template<typename Determinator>
  typename AllDestroyer<Determinator>::LoudCounter*
  AllDestroyer<Determinator>::LoudCounterHolder::counter() throw()
  {
    return counter_;
  }

  /* AllDestroyer impl */
  template<typename Determinator>
  AllDestroyer<Determinator>::AllDestroyer() throw()
    : counter_(Gears::add_ref(
        AllDestroyer<Determinator>::LoudCounterHolder::Single::
          instance().counter()))
  {
    counter_->add(1);
  }

  template<typename Determinator>
  AllDestroyer<Determinator>::AllDestroyer(const AllDestroyer& another)
    throw()
    : counter_(another.counter_)
  {
    counter_->add(1);
  }

  template<typename Determinator>
  AllDestroyer<Determinator>::~AllDestroyer() throw()
  {
    counter_->add(-1);
  }
}

#endif /*BASIC_SINGLETON_HPP*/
