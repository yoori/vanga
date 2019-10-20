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

#ifndef PROFILESTORAGE_PROFILEMAP_HPP
#define PROFILESTORAGE_PROFILEMAP_HPP

#include <list>
#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>
#include <Gears/Basic/Time.hpp>
#include <Gears/Basic/MemBuf.hpp>

namespace Gears
{
  enum OperationPriority
  {
    OP_RUNTIME,
    OP_BACKGROUND
  };

  template<typename KeyType>
  class ProfileMap: public virtual Gears::AtomicRefCountable
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, DescriptiveException);
    DECLARE_GEARS_EXCEPTION(CorruptedRecord, Exception);

    typedef KeyType KeyTypeT;
    typedef std::list<KeyType> KeyList;

    virtual void
    wait_preconditions(const KeyType&, OperationPriority) const
      throw(Exception)
    {}

    virtual bool
    check_profile(const KeyType& key) const throw(Exception) = 0;

    virtual Gears::ConstSmartMemBuf_var
    get_profile(
      const KeyType& key,
      Gears::Time* last_access_time = 0)
      throw(Exception) = 0;

    virtual void
    save_profile(
      const KeyType& key,
      Gears::ConstSmartMemBuf* mem_buf,
      const Gears::Time& now = Gears::Time::get_time_of_day(),
      OperationPriority op_priority = OP_RUNTIME)
      throw(Exception) = 0;

    virtual bool
    remove_profile(
      const KeyType& key,
      OperationPriority op_priority = OP_RUNTIME)
      throw(Exception) = 0;

    virtual void
    clear_expired(const Gears::Time& /*expire_time*/)
      throw(Exception)
    {
      throw Exception("clear_expired isn't supported");
    }

    virtual void
    copy_keys(KeyList& /*keys*/) throw(Exception)
    {
      throw Exception("copy_keys isn't supported");
    };

    virtual unsigned long
    size() const throw() = 0;

    virtual unsigned long
    area_size() const throw() = 0;

  protected:
    virtual
    ~ProfileMap() throw () = default;
  };
}

#endif /*PROFILESTORAGE_PROFILEMAP_HPP*/
