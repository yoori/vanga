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

#ifndef THREADING_LOCK_HPP
#define THREADING_LOCK_HPP

#include <pthread.h>

#include <Gears/Basic/Uncopyable.hpp>

/**
 * StaticInitializedMutex: exclusive lock policy, must be initialized statically
 * Mutex: exclusive lock policy
 * SpinLock: exclusive spin lock policy
 * RWLock: read/write lock policy
 * NullLock: single thread policy
 */

#define STATIC_MUTEX_INITIALIZER { PTHREAD_MUTEX_INITIALIZER }

namespace Gears
{
  template<typename LockType>
  class BaseReadGuard
  {
  public:
    BaseReadGuard(LockType& lock) noexcept
      : lock_(lock)
    {
      lock_.lock_read();
    }

    ~BaseReadGuard() noexcept
    {
      lock_.unlock();
    }

  protected:
    LockType& lock_;
  };

  template<typename LockType>
  class BaseWriteGuard
  {
  public:
    BaseWriteGuard(LockType& lock) noexcept
      : lock_(lock)
    {
      lock_.lock();
    }

    ~BaseWriteGuard() noexcept
    {
      lock_.unlock();
    }

  protected:
    LockType& lock_;
  };

  class StaticInitializedMutex
  {
    // for static initialization all members is public and no c-tor, d-tor
  public:
    typedef BaseWriteGuard<StaticInitializedMutex> ReadGuard;
    typedef BaseWriteGuard<StaticInitializedMutex> WriteGuard;

    void lock_read() noexcept;

    void lock() noexcept;

    void unlock() noexcept;

    pthread_mutex_t mutex_;
  };

  class Mutex:
    protected StaticInitializedMutex,
    private Uncopyable
  {
  public:
    typedef BaseWriteGuard<Mutex> ReadGuard;
    typedef BaseWriteGuard<Mutex> WriteGuard;

  public:
    Mutex() noexcept;

    Mutex(int pshared) noexcept;

    ~Mutex() noexcept;

    pthread_mutex_t& mutex_i() noexcept;

    using StaticInitializedMutex::lock_read;

    using StaticInitializedMutex::lock;

    using StaticInitializedMutex::unlock;
  };

  class RWLock: private Uncopyable
  {
  public:
    typedef BaseReadGuard<RWLock> ReadGuard;
    typedef BaseWriteGuard<RWLock> WriteGuard;

  public:
    RWLock() noexcept;

    RWLock(int pshared) noexcept;

    ~RWLock() noexcept;

    void lock_read() noexcept;

    void lock() noexcept;

    void unlock() noexcept;

  protected:
    pthread_rwlock_t mutex_;
  };

#ifdef PTHREAD_SPINLOCK_DEFINED
  class SpinLock: private Uncopyable
  {
  public:
    typedef BaseWriteGuard<SpinLock> ReadGuard;
    typedef BaseWriteGuard<SpinLock> WriteGuard;

  public:
    SpinLock(int pshared = PTHREAD_PROCESS_PRIVATE) noexcept;

    ~SpinLock() noexcept;

    void lock() noexcept;

    void unlock() noexcept;

  protected:
    pthread_spinlock_t mutex_;
  };

#else
  typedef Mutex SpinLock;
#endif

  struct NullLock: private Uncopyable
  {
  public:
    struct NullGuard
    {
      NullGuard(NullLock&) noexcept {}
    };

    typedef NullGuard ReadGuard;
    typedef NullGuard WriteGuard;

  public:
    NullLock() noexcept {};

    void lock() noexcept {};

    void unlock() noexcept {};
  };
}

namespace Gears
{
  /* StaticInitializedMutex */
  inline
  void
  StaticInitializedMutex::lock_read() noexcept
  {
    ::pthread_mutex_lock(&mutex_);
  }

  inline
  void
  StaticInitializedMutex::lock() noexcept
  {
    ::pthread_mutex_lock(&mutex_);
  }

  inline
  void
  StaticInitializedMutex::unlock() noexcept
  {
    ::pthread_mutex_unlock(&mutex_);
  }

  /* Mutex */
  inline
  Mutex::Mutex() noexcept
  {
    pthread_mutex_init(&mutex_, 0);
  }

  inline
  Mutex::Mutex(int pshared) noexcept
  {
    pthread_mutexattr_t mutex_attributes;
    pthread_mutexattr_init(&mutex_attributes);
    pthread_mutexattr_setpshared(&mutex_attributes, pshared);
    pthread_mutex_init(&mutex_, &mutex_attributes);
  }

  inline
  Mutex::~Mutex() noexcept
  {
    pthread_mutex_destroy(&mutex_);
  }

  inline
  pthread_mutex_t&
  Mutex::mutex_i() noexcept
  {
    return mutex_;
  }

  /* RWLock */
  inline
  RWLock::RWLock() noexcept
  {
    ::pthread_rwlock_init(&mutex_, 0);
  }

  inline
  RWLock::~RWLock() noexcept
  {
    ::pthread_rwlock_destroy(&mutex_);
  }

  inline
  void
  RWLock::lock() noexcept
  {
    ::pthread_rwlock_wrlock(&mutex_);
  }

  inline
  void
  RWLock::lock_read() noexcept
  {
    ::pthread_rwlock_rdlock(&mutex_);
  }

  inline
  void
  RWLock::unlock() noexcept
  {
    ::pthread_rwlock_unlock(&mutex_);
  }

#ifdef PTHREAD_SPINLOCK_DEFINED
  /* SpinLock */
  inline
  SpinLock::SpinLock(int pshared) noexcept
  {
    ::pthread_spin_init(&mutex_, pshared);
  }

  inline
  SpinLock::~SpinLock() noexcept
  {
    ::pthread_spin_destroy(&mutex_);
  }

  inline
  void
  SpinLock::lock() noexcept
  {
    ::pthread_spin_lock(&mutex_);
  }

  inline
  void
  SpinLock::unlock() noexcept
  {
    ::pthread_spin_unlock(&mutex_);
  }
#endif /*PTHREAD_SPINLOCK_DEFINED*/
}

#endif
