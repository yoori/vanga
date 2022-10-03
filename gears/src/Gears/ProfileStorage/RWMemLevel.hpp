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

#ifndef RWMEMPLEVEL_HPP
#define RWMEMPLEVEL_HPP

#include <Gears/Basic/Lock.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>

#include "BaseLevel.hpp"
#include "ReadMemLevel.hpp"

namespace Gears
{
  //
  // RWMemLevel
  //
  template<typename KeyType, typename KeySerializerType>
  class RWMemLevel:
    public RWBaseLevel<KeyType>,
    protected MemLevelHolder<KeyType>
  {
  public:
    class KeyIteratorImpl: public ReadBaseLevel<KeyType>::KeyIterator
    {
    public:
      KeyIteratorImpl(
        const RWMemLevel<KeyType, KeySerializerType>* read_mem_level)
        noexcept;

      virtual bool
      get_next(
        KeyType& key,
        ProfileOperation& operation,
        Time& access_time)
        noexcept;

    protected:
      virtual ~KeyIteratorImpl() noexcept
      {}

    private:
      const Gears::IntrusivePtr<
        const RWMemLevel<KeyType, KeySerializerType> > rw_mem_level_;
      bool initialized_;
      KeyType cur_key_;
    };

    class IteratorImpl: public ReadBaseLevel<KeyType>::Iterator
    {
    public:
      IteratorImpl(
        const RWMemLevel<KeyType, KeySerializerType>* read_mem_level)
        noexcept;

      virtual bool
      get_next(
        KeyType& key,
        ProfileOperation& operation,
        Time& access_time)
        /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

      virtual ConstSmartMemBuf_var
      get_profile()
        /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

    protected:
      virtual ~IteratorImpl() noexcept
      {}

    private:
      const Gears::IntrusivePtr<
        const RWMemLevel<KeyType, KeySerializerType> > rw_mem_level_;
      bool initialized_;
      KeyType cur_key_;
    };

    IntrusivePtr<ReadMemLevel<KeyType> >
    convert_to_read_mem_level()
      noexcept;

    virtual CheckProfileResult
    check_profile(const KeyType& key) const
      /*throw(typename RWBaseLevel<KeyType>::Exception)*/;

    virtual GetProfileResult
    get_profile(const KeyType& key) const
      /*throw(typename RWBaseLevel<KeyType>::Exception)*/;

    virtual typename ReadBaseLevel<KeyType>::KeyIterator_var
    get_key_iterator() const
      noexcept;

    virtual typename ReadBaseLevel<KeyType>::Iterator_var
    get_iterator(unsigned long read_buffer_size) const
      noexcept;

    virtual unsigned long
    size() const
      noexcept;

    virtual uint64_t
    area_size() const
      noexcept;

    virtual unsigned long
    merge_free_size() const
      noexcept;

    virtual Time
    min_access_time() const
      noexcept;

    virtual ConstSmartMemBuf_var
    save_profile(
      const KeyType& key,
      ConstSmartMemBuf* mem_buf,
      ProfileOperation operation,
      unsigned long next_size,
      const Time& now)
      /*throw(typename RWBaseLevel<KeyType>::Exception)*/;

    virtual unsigned long
    remove_profile(
      const KeyType& key,
      unsigned long next_size)
      /*throw(typename RWBaseLevel<KeyType>::Exception)*/;

    void
    clear_expired(
      const Time& expire_time)
      /*throw(typename RWBaseLevel<KeyType>::Exception)*/;

  protected:
    virtual ~RWMemLevel() noexcept
    {}

    unsigned long
    eval_area_size_(const KeyType& key) const
      noexcept;

  private:
    typedef Gears::RWLock SyncPolicy;

  private:
    mutable SyncPolicy lock_;
    KeySerializerType key_serializer_;
  };
}

#include "RWMemLevel.tpp"

#endif /*RWMEMLEVEL_HPP*/
