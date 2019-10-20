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

#ifndef GEARS_TRANSACTIONMAP_HPP
#define GEARS_TRANSACTIONMAP_HPP

#include <Gears/Basic/AtomicRefCountable.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>
#include <Gears/Basic/AtomicCounter.hpp>
#include <Gears/Basic/Lock.hpp>

namespace Gears
{
  class TransactionBase: public virtual AtomicRefCountable
  {
    typedef Mutex SyncPolicy;

  public:
    class TransactionHolderBase
    {
    public:
      SyncPolicy::Mutex lock_;
      AtomicCounter lock_count_;

    public:
      TransactionHolderBase(): lock_count_(1)
      {}

      virtual void
      add_ref() const throw() = 0;

      virtual void
      remove_ref() const throw() = 0;

    protected:
      virtual ~TransactionHolderBase() throw ();
    };

    typedef IntrusivePtr<TransactionHolderBase>
      TransactionHolderBase_var;

    /**
     * lock holder->lock_ for write
     */
    TransactionBase(TransactionHolderBase* holder) throw ();

  protected:
    virtual
    ~TransactionBase() throw ();

  private:
    TransactionHolderBase_var holder_;
    SyncPolicy::WriteGuard locker_;
  };

  /**
   * TransactionImplType must inherit TransactionBase.
   * TransactionMap must guarantee that only one Transaction object can be
   * created for each key. And lock application if it try get already
   * opened transaction, and wait it close.
   */
  template <typename KeyType, typename TransactionImplType>
  class TransactionMap
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);
    DECLARE_GEARS_EXCEPTION(MaxWaitersReached, Exception);

    typedef TransactionImplType Transaction;

    typedef IntrusivePtr<TransactionImplType>
      Transaction_var;

    typedef typename TransactionImplType::ArgType TransactionArgType;

  public:
    TransactionMap(unsigned long max_waiters = 0) throw();

    Transaction_var
    get_transaction(
      const KeyType& key,
      bool check_max_waiters = true,
      const TransactionArgType& arg = TransactionArgType())
      throw (MaxWaitersReached, Exception);

    virtual ~TransactionMap() throw ();

  protected:
    /**
     * Override AtomicImpl : delegate transaction close into map,
     * if need delete object for erase from open_transaction_map_.
     */
    class TransactionHolder: public TransactionBase::TransactionHolderBase
    {
    public:
      TransactionHolder(
        TransactionMap& transaction_map,
        const KeyType& key)
        throw ();

      void
      add_ref() const throw();

      void
      remove_ref() const throw();

    protected:
      virtual
      ~TransactionHolder() throw();

    private:
      mutable AtomicCounter ref_count_;
      KeyType key_;
      TransactionMap& transactions_map_;
    };

  protected:
    /**
     * child classes must override this method,
     * can be implemented as strategy
     */
    virtual Transaction_var
    create_transaction_impl_(
      TransactionHolder* holder,
      const KeyType& key,
      const TransactionArgType& arg)
      throw (Gears::Exception) = 0;

  private:
    typedef Mutex SyncPolicy;

    typedef IntrusivePtr<TransactionHolder>
      TransactionHolder_var;

  private:
    /**
     * remove Transaction from open_transaction_map_
     */
    void
    close_i_(const KeyType& key) throw ();

  private:
    const unsigned long max_waiters_;
    SyncPolicy open_transaction_map_lock_;
    typedef std::map<KeyType, TransactionHolder*> OpenedTransactionMap;
    OpenedTransactionMap open_transaction_map_;
  };
}

//
// Implementation
//

namespace Gears
{
  inline
  TransactionBase::TransactionHolderBase::~TransactionHolderBase() throw ()
  {}

  inline
  TransactionBase::TransactionBase(TransactionHolderBase* holder)
    throw ()
    : holder_(Gears::add_ref(holder)),
      locker_(holder->lock_)
  {}

  inline
  TransactionBase::~TransactionBase() throw ()
  {
    holder_->lock_count_ += -1;
  }

  template <typename KeyType, typename TransactionImplType>
  TransactionMap<KeyType, TransactionImplType>::TransactionHolder::
    TransactionHolder(TransactionMap& transactions_map, const KeyType& key)
    throw ()
    : ref_count_(1),
      key_(key),
      transactions_map_(transactions_map)
  {}

  template <typename KeyType, typename TransactionImplType>
  TransactionMap<KeyType, TransactionImplType>::TransactionHolder::
    ~TransactionHolder() throw ()
  {}

  template <typename KeyType, typename TransactionImplType>
  void
  TransactionMap<KeyType, TransactionImplType>::TransactionHolder::
    add_ref() const throw ()
  {
    ref_count_.add(1);
  }

  template <typename KeyType, typename TransactionImplType>
  void
  TransactionMap<KeyType, TransactionImplType>::TransactionHolder::
    remove_ref() const throw ()
  {
    // We must first lock open_transaction_map_lock_ to avoid
    // using holder_ in get_transaction, it can be removed here.
    // 1) map_lock_ 2) remove_ref 3) remove open_transac if need
    SyncPolicy::WriteGuard lock(transactions_map_.open_transaction_map_lock_);

    int result_counter = ref_count_.add_and_fetch(-1);
    assert(result_counter >= 0);
    if(result_counter == 0)
    {
      transactions_map_.close_i_(key_);
      delete this;
    }
  }

  template <typename KeyType, typename TransactionImplType>
  TransactionMap<KeyType, TransactionImplType>::
    TransactionMap(unsigned long max_waiters)
    throw ()
    : max_waiters_(max_waiters)
  {}

  /**
   * lock open_transaction_map_lock_,  find key:
   * If not exists simple create Transaction -> TransactionHolder and
   * insert holder into open_transaction_map_.
   * If exists save _var to holder and create Transaction object with
   * this holder, outside open_transaction_map_lock_ zone.
   */
  template <typename KeyType, typename TransactionImplType>
  typename TransactionMap<KeyType, TransactionImplType>::Transaction_var
  TransactionMap<KeyType, TransactionImplType>::
  get_transaction(
    const KeyType& key,
    bool check_max_waiters,
    const TransactionArgType& arg)
    throw (MaxWaitersReached, Exception)
  {
    static const char* FUN = "TransactionMap::get_transaction()";

    try
    {
      TransactionHolder_var holder;
      unsigned long lock_count;
      bool max_waiters_reached = false;

      {
        SyncPolicy::WriteGuard lock(open_transaction_map_lock_);

        typename OpenedTransactionMap::iterator it =
          open_transaction_map_.find(key);
        if (it == open_transaction_map_.end())  // not exists
        {
          holder = new TransactionHolder(*this, key);
          Transaction_var transac(
            create_transaction_impl_(holder, key, arg));
          open_transaction_map_.insert(std::make_pair(key, holder.in()));
          return transac;
        }

        // will be created additional transaction object with this holder,
        // we must add reference to this holder, because transaction use it
        holder = Gears::add_ref(it->second);
        lock_count = it->second->lock_count_;
        if(check_max_waiters &&
           max_waiters_ != 0 &&
           lock_count >= max_waiters_)
        {
          max_waiters_reached = true;
        }
        else
        {
          it->second->lock_count_ += 1;
        }
      }

      if(max_waiters_reached)
      {
        // throw outside lock
        ErrorStream ostr;
        ostr << FUN << ": already opened " << lock_count <<
          " transactions, max waiters = " << max_waiters_;
        throw MaxWaitersReached(ostr.str());
      }

      try
      {
        // exist already opened transaction
        return create_transaction_impl_(holder, key, arg);
      }
      catch(...)
      {
        holder->lock_count_ += -1;
        throw;
      }
    }
    catch (const MaxWaitersReached&)
    {
      throw;
    }
    catch (const Gears::Exception& ex)
    {
      ErrorStream ostr;
      ostr << FUN << ": cannot create transaction: " << ex.what();
      throw Exception(ostr.str());
    }
  }

  template <typename KeyType, typename TransactionImplType>
  TransactionMap<KeyType, TransactionImplType>::~TransactionMap() throw ()
  {}

  template <typename KeyType, typename TransactionImplType>
  void
  TransactionMap<KeyType, TransactionImplType>::close_i_(const KeyType& key)
    throw ()
  {
    typename OpenedTransactionMap::iterator it =
      open_transaction_map_.find(key);
    if (it != open_transaction_map_.end())
    {
      open_transaction_map_.erase(it);
    }
  }
}

#endif /*GEARS_TRANSACTIONMAP_HPP*/
