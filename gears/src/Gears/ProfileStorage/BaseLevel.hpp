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

#ifndef BASELEVEL_HPP
#define BASELEVEL_HPP

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>
#include <Gears/Basic/Time.hpp>
#include <Gears/Basic/MemBuf.hpp>

namespace Gears
{
  enum ProfileState
  {
    PS_NOT_FOUND,
    PS_ERASED,
    PS_FOUND
  };

  enum ProfileOperation
  {
    PO_NOT_FOUND = 0,
    PO_INSERT,
    PO_REWRITE,
    PO_ERASE
  };

  struct CheckProfileResult
  {
    ProfileOperation operation;
    unsigned long size;
  };

  struct GetProfileResult
  {
    GetProfileResult()
      : operation(PO_NOT_FOUND)
    {}

    ProfileOperation operation;
    ConstSmartMemBuf_var mem_buf;
    Time access_time;
  };

  template<typename KeyType>
  class ReadBaseLevel: public virtual Gears::AtomicRefCountable
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);

    class KeyIterator: public virtual Gears::AtomicRefCountable
    {
    public:
      virtual bool
      get_next(KeyType& key, ProfileOperation& operation, Time& access_time)
        throw(Exception) = 0;

    protected:
      virtual
      ~KeyIterator() throw () = default;
    };

    typedef IntrusivePtr<KeyIterator> KeyIterator_var;

    class Iterator: public virtual Gears::AtomicRefCountable
    {
    public:
      virtual bool
      get_next(KeyType& key, ProfileOperation& operation, Time& access_time)
        throw(Exception) = 0;

      virtual ConstSmartMemBuf_var
      get_profile()
        throw(Exception) = 0;

    protected:
      virtual
      ~Iterator() throw () = default;
    };

    typedef IntrusivePtr<Iterator> Iterator_var;

  public:
    virtual CheckProfileResult
    check_profile(const KeyType& key) const
      throw(Exception) = 0;

    virtual GetProfileResult
    get_profile(const KeyType& key) const
      throw(Exception) = 0;

    virtual KeyIterator_var
    get_key_iterator() const
      throw() = 0;

    virtual Iterator_var
    get_iterator(unsigned long read_buffer_size) const
      throw(Exception) = 0;

    // number of profiles
    virtual unsigned long
    size() const throw() = 0;

    // estimated physical size
    virtual uint64_t
    area_size() const throw() = 0;

    // size that will be freed on merge this level with next levels
    virtual unsigned long
    merge_free_size() const throw() = 0;

    virtual Time
    min_access_time() const throw() = 0;

  protected:
    virtual
    ~ReadBaseLevel() throw () = default;
  };

  template<typename KeyType>
  class RWBaseLevel: public ReadBaseLevel<KeyType>
  {
  public:
    virtual ConstSmartMemBuf_var
    save_profile(
      const KeyType& key,
      ConstSmartMemBuf* mem_buf,
      ProfileOperation operation,
      unsigned long next_size,
      const Time& now)
      throw(typename ReadBaseLevel<KeyType>::Exception) = 0;

    virtual unsigned long
    remove_profile(
      const KeyType& key,
      unsigned long next_size)
      throw(typename ReadBaseLevel<KeyType>::Exception) = 0;

  protected:
    virtual
    ~RWBaseLevel() throw () = default;
  };
}

#endif /*BASELEVEL_HPP*/
