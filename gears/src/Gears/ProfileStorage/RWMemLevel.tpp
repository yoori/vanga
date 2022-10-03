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

namespace Gears
{
  // RWMemLevel
  template<typename KeyType, typename KeySerializerType>
  RWMemLevel<KeyType, KeySerializerType>::KeyIteratorImpl::KeyIteratorImpl(
    const RWMemLevel<KeyType, KeySerializerType>* rw_mem_level)
    noexcept
    : rw_mem_level_(add_ref(rw_mem_level)),
      initialized_(false)
  {}

  template<typename KeyType, typename KeySerializerType>
  bool
  RWMemLevel<KeyType, KeySerializerType>::KeyIteratorImpl::get_next(
    KeyType& key,
    ProfileOperation& operation,
    Gears::Time& access_time)
    noexcept
  {
    RWMemLevel<KeyType, KeySerializerType>::SyncPolicy::ReadGuard lock(rw_mem_level_->lock_);
    typename MemLevelHolder<KeyType>::ProfileHolderMap::const_iterator it;
    if(!initialized_)
    {
      it = rw_mem_level_->profiles_.begin();
      initialized_ = true;
    }
    else
    {
      it = rw_mem_level_->profiles_.lower_bound(cur_key_);
      if(it == rw_mem_level_->profiles_.end())
      {
        return false;
      }

      ++it;
    }

    if(it == rw_mem_level_->profiles_.end())
    {
      return false;
    }

    cur_key_ = it->first;
    key = cur_key_;
    operation = static_cast<ProfileOperation>(it->second.operation);
    access_time = Gears::Time(it->second.access_time);

    return true;
  }

  template<typename KeyType, typename KeySerializerType>
  RWMemLevel<KeyType, KeySerializerType>::IteratorImpl::IteratorImpl(
    const RWMemLevel<KeyType, KeySerializerType>* rw_mem_level)
    noexcept
    : rw_mem_level_(add_ref(rw_mem_level)),
      initialized_(false)
  {}

  template<typename KeyType, typename KeySerializerType>
  bool
  RWMemLevel<KeyType, KeySerializerType>::IteratorImpl::get_next(
    KeyType& key,
    ProfileOperation& operation,
    Gears::Time& access_time)
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    RWMemLevel<KeyType, KeySerializerType>::SyncPolicy::ReadGuard lock(rw_mem_level_->lock_);
    typename MemLevelHolder<KeyType>::ProfileHolderMap::const_iterator it;
    if(!initialized_)
    {
      it = rw_mem_level_->profiles_.begin();
      initialized_ = true;
    }
    else
    {
      it = rw_mem_level_->profiles_.lower_bound(cur_key_);
      if(it == rw_mem_level_->profiles_.end())
      {
        return false;
      }

      ++it;
    }

    if(it == rw_mem_level_->profiles_.end())
    {
      return false;
    }

    cur_key_ = it->first;
    key = cur_key_;
    operation = static_cast<ProfileOperation>(it->second.operation);
    access_time = Gears::Time(it->second.access_time);

    return true;
  }

  template<typename KeyType, typename KeySerializerType>
  Gears::ConstSmartMemBuf_var
  RWMemLevel<KeyType, KeySerializerType>::IteratorImpl::get_profile()
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    RWMemLevel<KeyType, KeySerializerType>::SyncPolicy::ReadGuard lock(rw_mem_level_->lock_);
    typename MemLevelHolder<KeyType>::ProfileHolderMap::const_iterator
      it = rw_mem_level_->profiles_.find(cur_key_);

    if(it != rw_mem_level_->profiles_.end())
    {
      return it->second.mem_buf;
    }

    return Gears::ConstSmartMemBuf_var();
  }

  template<typename KeyType, typename KeySerializerType>
  IntrusivePtr<ReadMemLevel<KeyType> >
  RWMemLevel<KeyType, KeySerializerType>::convert_to_read_mem_level()
    noexcept
  {
    RWMemLevel<KeyType, KeySerializerType>::SyncPolicy::WriteGuard lock(lock_);
    IntrusivePtr<ReadMemLevel<KeyType> > read_mem_level(
      new ReadMemLevel<KeyType>());
    read_mem_level->profiles_.swap(this->profiles_);
    read_mem_level->size_ = this->size_;
    read_mem_level->area_size_ = this->area_size_;
    read_mem_level->merge_free_size_ = this->merge_free_size_;
    return read_mem_level;
  }

  template<typename KeyType, typename KeySerializerType>
  CheckProfileResult
  RWMemLevel<KeyType, KeySerializerType>::check_profile(const KeyType& key)
    const /*throw(typename RWBaseLevel<KeyType>::Exception)*/
  {
    SyncPolicy::ReadGuard lock(lock_);
    return this->check_profile_i(key);
  }

  template<typename KeyType, typename KeySerializerType>
  GetProfileResult
  RWMemLevel<KeyType, KeySerializerType>::get_profile(
    const KeyType& key)
    const /*throw(typename RWBaseLevel<KeyType>::Exception)*/
  {
    SyncPolicy::ReadGuard lock(lock_);
    return this->get_profile_i(key);
  }

  template<typename KeyType, typename KeySerializerType>
  typename ReadBaseLevel<KeyType>::KeyIterator_var
  RWMemLevel<KeyType, KeySerializerType>::get_key_iterator() const
    noexcept
  {
    return new KeyIteratorImpl(this);
  }

  template<typename KeyType, typename KeySerializerType>
  typename ReadBaseLevel<KeyType>::Iterator_var
  RWMemLevel<KeyType, KeySerializerType>::get_iterator(
    unsigned long /*read_buffer_size*/)
    const noexcept
  {
    return new IteratorImpl(this);
  }

  template<typename KeyType, typename KeySerializerType>
  unsigned long
  RWMemLevel<KeyType, KeySerializerType>::size() const noexcept
  {
    SyncPolicy::ReadGuard lock(lock_);
    return this->size_i();
  }

  template<typename KeyType, typename KeySerializerType>
  uint64_t
  RWMemLevel<KeyType, KeySerializerType>::area_size() const noexcept
  {
    SyncPolicy::ReadGuard lock(lock_);
    return this->area_size_i();
  }

  template<typename KeyType, typename KeySerializerType>
  unsigned long
  RWMemLevel<KeyType, KeySerializerType>::merge_free_size() const noexcept
  {
    SyncPolicy::ReadGuard lock(lock_);
    return this->merge_free_size_i();
  }

  template<typename KeyType, typename KeySerializerType>
  Gears::Time
  RWMemLevel<KeyType, KeySerializerType>::min_access_time() const
    noexcept
  {
    return Gears::Time::ZERO;
  }

  template<typename KeyType, typename KeySerializerType>
  Gears::ConstSmartMemBuf_var
  RWMemLevel<KeyType, KeySerializerType>::save_profile(
    const KeyType& key,
    Gears::ConstSmartMemBuf* mem_buf,
    ProfileOperation operation,
    unsigned long next_size,
    const Gears::Time& now)
    /*throw(typename RWBaseLevel<KeyType>::Exception)*/
  {
    Gears::ConstSmartMemBuf_var old_mem_buf;

    typename MemLevelHolder<KeyType>::ProfileHolder new_holder;
    new_holder.operation = operation;
    new_holder.next_size = next_size;
    new_holder.mem_buf = Gears::add_ref(mem_buf);
    new_holder.access_time = now.tv_sec;
    unsigned long key_area_size = this->eval_area_size_(key);
    unsigned long new_holder_area_size =
      this->MemLevelHolder<KeyType>::eval_area_size_(new_holder);

    SyncPolicy::WriteGuard lock(lock_);
    typename MemLevelHolder<KeyType>::ProfileHolderMap::iterator it =
      this->profiles_.find(key);
    if(it == this->profiles_.end())
    {
      ++this->size_;
      this->area_size_ += key_area_size + new_holder_area_size;
      this->profiles_.insert(std::make_pair(key, new_holder));
    }
    else
    {
      unsigned long old_holder_area_size =
        this->MemLevelHolder<KeyType>::eval_area_size_(it->second);
      this->area_size_ -= old_holder_area_size;
      this->area_size_ += new_holder_area_size;
      it->second.mem_buf.swap(old_mem_buf);
      it->second = new_holder;
    }

    return old_mem_buf;
  }

  template<typename KeyType, typename KeySerializerType>
  unsigned long
  RWMemLevel<KeyType, KeySerializerType>::remove_profile(
    const KeyType& key,
    unsigned long next_size)
    /*throw(typename RWBaseLevel<KeyType>::Exception)*/
  {
    // deallocate outside lock
    Gears::ConstSmartMemBuf_var old_mem_buf;
    typename MemLevelHolder<KeyType>::ProfileHolder new_holder;
    new_holder.operation = PO_ERASE;
    new_holder.next_size = next_size;
    new_holder.access_time = 0; // non actual for remove operation
    unsigned long key_area_size = this->eval_area_size_(key);
    unsigned long new_holder_area_size =
      this->MemLevelHolder<KeyType>::eval_area_size_(new_holder);

    SyncPolicy::WriteGuard lock(lock_);
    typename MemLevelHolder<KeyType>::ProfileHolderMap::iterator it =
      this->profiles_.find(key);
    if(it == this->profiles_.end())
    {
      this->area_size_ += key_area_size + new_holder_area_size;
      this->profiles_.insert(std::make_pair(key, new_holder));
    }
    else
    {
      it->second.mem_buf.swap(old_mem_buf); // deallocate old mem_buf outside lock

      if(it->second.operation == PO_INSERT)
      {
        ++this->size_;
        this->area_size_ -= key_area_size +
          this->MemLevelHolder<KeyType>::eval_area_size_(it->second);
        this->profiles_.erase(it);
      }
      else // PO_REWRITE
      {
        ++this->size_;
        this->area_size_ -=
          this->MemLevelHolder<KeyType>::eval_area_size_(it->second);
        this->area_size_ += new_holder_area_size;
        it->second = new_holder;
      }
    }

    return this->area_size_;
  }

  template<typename KeyType, typename KeySerializerType>
  void
  RWMemLevel<KeyType, KeySerializerType>::clear_expired(
    const Gears::Time& /*expire_time*/)
    /*throw(typename RWBaseLevel<KeyType>::Exception)*/
  {
  }

  template<typename KeyType, typename KeySerializerType>
  unsigned long
  RWMemLevel<KeyType, KeySerializerType>::eval_area_size_(
    const KeyType& key) const
    noexcept
  {
    return 3*sizeof(void*) + key_serializer_.size(key);
  }
}
