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

#ifndef GEARS_TRANSACTIONPROFILEMAP_HPP
#define GEARS_TRANSACTIONPROFILEMAP_HPP

#include "TransactionMap.hpp"
#include "ProfileMap.hpp"
#include "DelegateProfileMap.hpp"

namespace Gears
{
  template <typename KeyType>
  class TransactionProfileMap;

  template <typename KeyType>
  class ProfileTransactionImpl: public TransactionBase
  {
  public:
    typedef OperationPriority ArgType;

  public:
    ProfileTransactionImpl(
      TransactionProfileMap<KeyType>& profile_map,
      TransactionBase::TransactionHolderBase* holder,
      const KeyType& key,
      OperationPriority op_priority)
      throw ();

    /**
     * Simply delegate to ExpireProfileMap::get_profile
     */
    virtual
    Gears::ConstSmartMemBuf_var
    get_profile(Gears::Time* last_access_time = 0)
      throw (typename ProfileMap<KeyType>::Exception);

    /**
     * Simply delegate to ExpireProfileMap::save_profile
     */
    virtual void
    save_profile(
      Gears::ConstSmartMemBuf* mem_buf,
      const Gears::Time& now = Gears::Time::get_time_of_day())
      throw (typename ProfileMap<KeyType>::Exception);

    /**
     * Simply delegate to ExpireProfileMap::remove_profile
     */
    virtual
    bool remove_profile()
      throw(typename ProfileMap<KeyType>::Exception);

  protected:
    /**
     * Virtual empty destructor, protected as RC-object
     */
    virtual
    ~ProfileTransactionImpl() throw ();

  private:
    TransactionProfileMap<KeyType>& profile_map_;
    KeyType key_;
    OperationPriority op_priority_;
  };

  /**
   * ProfileMapType is RC-object
   */
  template <typename KeyType>
  class TransactionProfileMap:
    public virtual DelegateProfileMap<KeyType>,
    public virtual Gears::AtomicRefCountable,
    protected virtual TransactionMap<KeyType, ProfileTransactionImpl<KeyType> >
  {
    friend class ProfileTransactionImpl<KeyType>;

  public:
    DECLARE_GEARS_EXCEPTION(Exception, typename ProfileMap<KeyType>::Exception);

    typedef ProfileTransactionImpl<KeyType> ProfileTransactionImplType;

    typedef TransactionMap<KeyType, ProfileTransactionImplType>
      BaseTransactionMapType;

    typedef typename BaseTransactionMapType::MaxWaitersReached
      MaxWaitersReached;

    typedef typename BaseTransactionMapType::Transaction
      Transaction;

    typedef typename BaseTransactionMapType::Transaction_var
      Transaction_var;

  public:
    TransactionProfileMap(ProfileMap<KeyType>* base_map,
      unsigned long max_waiters = 0,
      bool create_transaction_on_get = false)
      throw();

    virtual bool
    check_profile(const KeyType& key) const
      throw(typename ProfileMap<KeyType>::Exception);

    virtual Gears::ConstSmartMemBuf_var
    get_profile(
      const KeyType& key,
      Gears::Time* last_access_time = 0)
      throw(typename ProfileMap<KeyType>::Exception);

    virtual void
    save_profile(
      const KeyType& key,
      Gears::ConstSmartMemBuf* mem_buf,
      const Gears::Time& now = Gears::Time::get_time_of_day(),
      OperationPriority op_priority = OP_RUNTIME)
      throw(typename ProfileMap<KeyType>::Exception);

    virtual bool
    remove_profile(
      const KeyType& key,
      OperationPriority op_priority = OP_RUNTIME)
      throw(typename ProfileMap<KeyType>::Exception);

    Transaction_var
    get_transaction(
      const KeyType& key,
      bool check_max_waiters = true,
      OperationPriority op_priority = OP_RUNTIME)
      throw (MaxWaitersReached, Exception);

  protected:
    virtual ~TransactionProfileMap() throw () {}

  private:
    typedef typename BaseTransactionMapType::TransactionHolder
      TransactionHolder;

  private:
    virtual Transaction_var
    create_transaction_impl_(
      TransactionHolder* holder,
      const KeyType& key,
      const OperationPriority& arg)
      throw (Gears::Exception)
    {
      return new ProfileTransactionImplType(*this, holder, key, arg);
    }

    Gears::ConstSmartMemBuf_var
    get_profile_i_(
      const KeyType& key,
      Gears::Time* last_access_time)
      throw(typename ProfileMap<KeyType>::Exception);

    void
    save_profile_i_(
      const KeyType& key,
      Gears::ConstSmartMemBuf* mem_buf,
      const Gears::Time& now,
      OperationPriority op_priority)
      throw(typename ProfileMap<KeyType>::Exception);

    bool
    remove_profile_i_(
      const KeyType& key,
      OperationPriority op_priority)
      throw(typename ProfileMap<KeyType>::Exception);

  private:
    const bool create_transaction_on_get_;
  };
}

//
// Implementations
//

namespace Gears
{
  /* ProfileTransactionImpl class */
  template <typename KeyType>
  ProfileTransactionImpl<KeyType>::
  ProfileTransactionImpl(
    TransactionProfileMap<KeyType>& profile_map,
    TransactionBase::TransactionHolderBase* holder,
    const KeyType& key,
    OperationPriority op_priority)
    throw ()
    : TransactionBase(holder),
      profile_map_(profile_map),
      key_(key),
      op_priority_(op_priority)
  {}

  template <typename KeyType>
  ProfileTransactionImpl<KeyType>::~ProfileTransactionImpl() throw ()
  {}

  template <typename KeyType>
  Gears::ConstSmartMemBuf_var
  ProfileTransactionImpl<KeyType>::get_profile(
    Gears::Time* last_access_time)
    throw(typename ProfileMap<KeyType>::Exception)
  {
    return profile_map_.get_profile_i_(key_, last_access_time);
  }

  template <typename KeyType>
  void
  ProfileTransactionImpl<KeyType>::save_profile(
    Gears::ConstSmartMemBuf* mem_buf,
    const Gears::Time& now)
    throw(typename ProfileMap<KeyType>::Exception)
  {
    profile_map_.save_profile_i_(key_, mem_buf, now, op_priority_);
  }

  template <typename KeyType>
  bool
  ProfileTransactionImpl<KeyType>::remove_profile()
    throw(typename ProfileMap<KeyType>::Exception)
  {
    return profile_map_.remove_profile_i_(key_, op_priority_);
  }

  template <typename KeyType>
  TransactionProfileMap<KeyType>::
  TransactionProfileMap(ProfileMap<KeyType>* base_map,
    unsigned long max_waiters,
    bool create_transaction_on_get)
    throw()
    : DelegateProfileMap<KeyType>(base_map),
      BaseTransactionMapType(max_waiters),
      create_transaction_on_get_(create_transaction_on_get)
  {}

  template <typename KeyType>  
  bool
  TransactionProfileMap<KeyType>::check_profile(
    const KeyType& key) const
    throw(typename ProfileMap<KeyType>::Exception)
  {
    return this->no_add_ref_delegate_map_()->check_profile(key);
  }

  template <typename KeyType>  
  Gears::ConstSmartMemBuf_var
  TransactionProfileMap<KeyType>::get_profile(
    const KeyType& key,
    Gears::Time* last_access_time)
    throw(typename ProfileMap<KeyType>::Exception)
  {
    if(create_transaction_on_get_)
    {
      return this->get_transaction(key, false)->get_profile(last_access_time);
    }
    else
    {
      return this->no_add_ref_delegate_map_()->get_profile(
        key, last_access_time);
    }
  }

  template <typename KeyType>  
  void
  TransactionProfileMap<KeyType>::save_profile(
    const KeyType& key,
    Gears::ConstSmartMemBuf* mem_buf,
    const Gears::Time& now,
    OperationPriority op_priority)
    throw(typename ProfileMap<KeyType>::Exception)
  {
    this->get_transaction(key, false, op_priority)->save_profile(mem_buf, now);
  }

  template <typename KeyType>
  bool
  TransactionProfileMap<KeyType>::remove_profile(
    const KeyType& key,
    OperationPriority op_priority)
    throw(typename ProfileMap<KeyType>::Exception)
  {
    return this->get_transaction(key, false, op_priority)->remove_profile();
  }

  template <typename KeyType>  
  typename TransactionProfileMap<KeyType>::Transaction_var
  TransactionProfileMap<KeyType>::get_transaction(
    const KeyType& key,
    bool check_max_waiters,
    OperationPriority op_priority)
    throw (MaxWaitersReached, Exception)
  {
    this->no_add_ref_delegate_map_()->wait_preconditions(key, op_priority);

    return BaseTransactionMapType::get_transaction(
      key,
      check_max_waiters);
  }

  template <typename KeyType>  
  Gears::ConstSmartMemBuf_var
  TransactionProfileMap<KeyType>::get_profile_i_(
    const KeyType& key,
    Gears::Time* last_access_time)
    throw(typename ProfileMap<KeyType>::Exception)
  {
    return this->no_add_ref_delegate_map_()->get_profile(
      key, last_access_time);
  }

  template <typename KeyType>  
  void
  TransactionProfileMap<KeyType>::save_profile_i_(
    const KeyType& key,
    Gears::ConstSmartMemBuf* mem_buf,
    const Gears::Time& now,
    OperationPriority op_priority)
    throw(typename ProfileMap<KeyType>::Exception)
  {
    this->no_add_ref_delegate_map_()->save_profile(key, mem_buf, now, op_priority);
  }

  template <typename KeyType>  
  bool
  TransactionProfileMap<KeyType>::remove_profile_i_(
    const KeyType& key,
    OperationPriority op_priority)
    throw(typename ProfileMap<KeyType>::Exception)
  {
    return this->no_add_ref_delegate_map_()->remove_profile(key, op_priority);
  }
}


#endif /*GEARS_TRANSACTIONPROFILEMAP_HPP*/
