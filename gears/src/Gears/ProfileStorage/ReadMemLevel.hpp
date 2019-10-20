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

#ifndef READMEMPLEVEL_HPP
#define READMEMPLEVEL_HPP

#include <Gears/Basic/Lock.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>

#include "BaseLevel.hpp"

namespace Gears
{
  template<typename KeyType>
  class MemLevelHolder
  {
  public:
    struct __attribute__ ((__packed__)) ProfileHolderPOD
    {
      uint32_t access_time;
      unsigned char operation;
      unsigned long next_size; // sum size in next layers
    };

    struct ProfileHolder: public ProfileHolderPOD
    {
      ConstSmartMemBuf_var mem_buf;
    };

    typedef std::map<KeyType, ProfileHolder> ProfileHolderMap;

  public:
    MemLevelHolder()
      throw();

    CheckProfileResult
    check_profile_i(const KeyType& key) const
      throw(typename ReadBaseLevel<KeyType>::Exception);

    GetProfileResult
    get_profile_i(const KeyType& key) const
      throw(typename ReadBaseLevel<KeyType>::Exception);

    unsigned long
    size_i() const
      throw();

    uint64_t
    area_size_i() const
      throw();

    unsigned long
    merge_free_size_i() const
      throw();

    Time
    min_access_time_i() const
      throw();

    bool
    get_first_i(
      KeyType& key,
      ConstSmartMemBuf_var& mem_buf) const
      throw();

    static unsigned long
    eval_area_size_(const ProfileHolder& holder)
      throw();

  public:
    ProfileHolderMap profiles_;
    unsigned long size_;
    uint64_t area_size_;
    uint64_t merge_free_size_;
    Time min_access_time_;
  };

  template<typename KeyType, typename KeySerializerType>
  class RWMemLevel;

  template<typename KeyType, typename KeySerializerType>
  class ReadFileLevel;

  //
  // ReadMemLevel
  //
  template<typename KeyType>
  class ReadMemLevel:
    public ReadBaseLevel<KeyType>,
    public MemLevelHolder<KeyType>
  {
    /*
    template<typename KeyType2, typename KeySerializerType>
    friend class RWMemLevel<KeyType2, KeySerializerType>;
    */
  public:
    class KeyIteratorImpl:
      public ReadBaseLevel<KeyType>::KeyIterator
    {
    public:
      KeyIteratorImpl(const ReadMemLevel<KeyType>* read_mem_level)
        throw();

      virtual bool
      get_next(
        KeyType& key,
        ProfileOperation& operation,
        Time& access_time)
        throw();

    private:
      const Gears::IntrusivePtr<const ReadMemLevel<KeyType> > read_mem_level_;
      typename MemLevelHolder<KeyType>::ProfileHolderMap::
        const_iterator profiles_it_;
    };

    class IteratorImpl:
      public ReadBaseLevel<KeyType>::Iterator
    {
    public:
      IteratorImpl(const ReadMemLevel<KeyType>* read_mem_level)
        throw();

      virtual bool
      get_next(
        KeyType& key,
        ProfileOperation& operation,
        Time& access_time)
        throw();

      virtual ConstSmartMemBuf_var
      get_profile()
        throw();

    private:
      const Gears::IntrusivePtr<const ReadMemLevel<KeyType> > read_mem_level_;
      typename MemLevelHolder<KeyType>::ProfileHolderMap::
        const_iterator profiles_it_;
    };

  public:
    virtual CheckProfileResult
    check_profile(const KeyType& key) const
      throw(typename ReadBaseLevel<KeyType>::Exception);

    virtual GetProfileResult
    get_profile(const KeyType& key) const
      throw(typename ReadBaseLevel<KeyType>::Exception);

    virtual typename ReadBaseLevel<KeyType>::KeyIterator_var
    get_key_iterator() const
      throw();

    virtual typename ReadBaseLevel<KeyType>::Iterator_var
    get_iterator(unsigned long read_buffer_size) const
      throw();

    virtual unsigned long
    size() const
      throw();

    virtual unsigned long
    area_size() const
      throw();

    virtual unsigned long
    merge_free_size() const
      throw();

    virtual Time
    min_access_time() const
      throw();

  protected:
    virtual ~ReadMemLevel() throw()
    {}
  };
}

#include "ReadMemLevel.tpp"

#endif /*READMEMLEVEL_HPP*/
