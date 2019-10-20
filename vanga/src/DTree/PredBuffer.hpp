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

#ifndef PREDBUFFER_HPP_
#define PREDBUFFER_HPP_

#include <Gears/Basic/AtomicRefCountable.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>
#include <Gears/Basic/Singleton.hpp>
#include <Gears/Basic/Lock.hpp>

namespace Vanga
{
  // helpers for force buffers allocation
  template<typename ValueType>
  struct Buffer:
    public std::vector<ValueType>,
    public Gears::AtomicRefCountable
  {
  protected:
    virtual ~Buffer() throw()
    {}
  };

  template<typename ValueType>
  class BufferProviderImpl;

  template<typename ValueType>
  struct BufferPtr:
    public Gears::AtomicRefCountable
  {
  public:
    BufferPtr(
      BufferProviderImpl<ValueType>* provider,
      Gears::IntrusivePtr<Buffer<ValueType> >& buffer)
      : provider_(Gears::add_ref(provider))
    {
      buffer_.swap(buffer);
    };

    Buffer<ValueType>&
    buf()
    {
      return *buffer_;
    }

  protected:
    virtual ~BufferPtr() throw();

  protected:
    Gears::IntrusivePtr<BufferProviderImpl<ValueType> > provider_;
    Gears::IntrusivePtr<Buffer<ValueType> > buffer_;
  };

  template<typename ValueType>
  class BufferProviderImpl: public Gears::AtomicRefCountable
  {
    friend class BufferPtr<ValueType>;

  public:
    BufferProviderImpl() throw();

    Gears::IntrusivePtr<BufferPtr<ValueType> >
    get();

  protected:
    typedef Gears::SpinLock SyncPolicy;

  protected:
    virtual ~BufferProviderImpl() throw()
    {};

    void
    release_(Gears::IntrusivePtr<Buffer<ValueType> >& buffer);

  protected:
    SyncPolicy::Mutex lock_;
    std::vector<Gears::IntrusivePtr<Buffer<ValueType> > > pred_buffers_;
  };

  template<typename ValueType>
  class BufferProvider:
    public Gears::Singleton<
      BufferProviderImpl<ValueType>,
      Gears::IntrusivePtr<BufferProviderImpl<ValueType> > >
  {};
}

namespace Vanga
{
  template<typename ValueType>
  BufferProviderImpl<ValueType>::BufferProviderImpl() throw()
  {
    pred_buffers_.reserve(128);
  }

  template<typename ValueType>
  Gears::IntrusivePtr<BufferPtr<ValueType> >
  BufferProviderImpl<ValueType>::get()
  {
    Gears::IntrusivePtr<Buffer<ValueType> > res;

    {
      SyncPolicy::WriteGuard lock(lock_);
      if(!pred_buffers_.empty())
      {
        res.swap(pred_buffers_.back());
        pred_buffers_.pop_back();
      }
    }

    if(!res)
    {
      res = new Buffer<ValueType>();
      res->reserve(1024*1024);
    }

    return new BufferPtr<ValueType>(this, res);
  }

  template<typename ValueType>
  void
  BufferProviderImpl<ValueType>::release_(
    Gears::IntrusivePtr<Buffer<ValueType> >& buffer)
  {
    buffer->clear();

    SyncPolicy::WriteGuard lock(lock_);
    pred_buffers_.push_back(Gears::IntrusivePtr<Buffer<ValueType> >());
    pred_buffers_.back().swap(buffer);
  }

  template<typename ValueType>
  BufferPtr<ValueType>::~BufferPtr() throw()
  {
    provider_->release_(buffer_);
  }
}

#endif /*PREDBUFFER_HPP_*/
