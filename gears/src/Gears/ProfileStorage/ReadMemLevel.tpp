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
  // MemLevelHolder
  template<typename KeyType>
  MemLevelHolder<KeyType>::MemLevelHolder()
    throw()
    : size_(0),
      area_size_(0),
      merge_free_size_(0)
  {}

  template<typename KeyType>
  CheckProfileResult
  MemLevelHolder<KeyType>::check_profile_i(
    const KeyType& key) const
    throw(typename ReadBaseLevel<KeyType>::Exception)
  {
    CheckProfileResult result;

    typename ProfileHolderMap::const_iterator it = profiles_.find(key);
    if(it != profiles_.end())
    {
      result.operation = static_cast<ProfileOperation>(it->second.operation);
      
      if(it->second.operation == PO_ERASE)
      {
        result.size = it->second.next_size;
      }
      else
      {
        result.size = it->second.mem_buf->membuf().size();
      }
    }
    else
    {
      result.operation = PO_NOT_FOUND;
      result.size = 0;
    }

    return result;
  }

  template<typename KeyType>
  GetProfileResult
  MemLevelHolder<KeyType>::get_profile_i(
    const KeyType& key) const
    throw(typename ReadBaseLevel<KeyType>::Exception)
  {
    GetProfileResult result;

    typename ProfileHolderMap::const_iterator it = profiles_.find(key);
    if(it != profiles_.end())
    {
      result.mem_buf = it->second.mem_buf;
      result.operation = static_cast<ProfileOperation>(it->second.operation);
    }
    else
    {
      result.operation = PO_NOT_FOUND;
    }

    return result;
  }

  template<typename KeyType>
  unsigned long
  MemLevelHolder<KeyType>::size_i() const
    throw()
  {
    return size_;
  }

  template<typename KeyType>
  uint64_t
  MemLevelHolder<KeyType>::area_size_i() const
    throw()
  {
    return area_size_;
  }

  template<typename KeyType>
  uint64_t
  MemLevelHolder<KeyType>::merge_free_size_i() const
    throw()
  {
    return merge_free_size_;
  }

  template<typename KeyType>
  Gears::Time
  MemLevelHolder<KeyType>::min_access_time_i() const
    throw()
  {
    return min_access_time_;
  }

  template<typename KeyType>
  bool
  MemLevelHolder<KeyType>::get_first_i(
    KeyType& key,
    Gears::ConstSmartMemBuf_var& mem_buf) const
    throw()
  {
    typename ProfileHolderMap::const_iterator it = profiles_.begin();
    while(it != profiles_.end())
    {
      if(it->second.erased)
      {
        key = it->first;
        mem_buf = it->second.mem_buf;
        return true;
      }

      ++it;
    }

    return false;
  }

  template<typename KeyType>
  unsigned long
  MemLevelHolder<KeyType>::eval_area_size_(
    const ProfileHolder& holder)
    throw()
  {
    return sizeof(ProfileHolder) + (
      holder.mem_buf.in() ? holder.mem_buf->membuf().size() : 0);
  }

  // ReadMemLevel::KeyIteratorImpl
  template<typename KeyType>
  ReadMemLevel<KeyType>::KeyIteratorImpl::KeyIteratorImpl(
    const ReadMemLevel<KeyType>* read_mem_level)
    throw()
    : read_mem_level_(add_ref(read_mem_level)),
      profiles_it_(read_mem_level_->profiles_.end())
  {}

  template<typename KeyType>
  bool
  ReadMemLevel<KeyType>::KeyIteratorImpl::get_next(
    KeyType& key,
    ProfileOperation& operation,
    Gears::Time& access_time)
    throw()
  {
    if(profiles_it_ == read_mem_level_->profiles_.end())
    {
      profiles_it_ = read_mem_level_->profiles_.begin();
    }
    else
    {
      ++profiles_it_;
    }

    if(profiles_it_ != read_mem_level_->profiles_.end())
    {
      key = profiles_it_->first;
      operation = static_cast<ProfileOperation>(profiles_it_->second.operation);
      access_time = Gears::Time(profiles_it_->second.access_time);
      return true;
    }

    return false;
  }

  // ReadMemLevel::IteratorImpl
  template<typename KeyType>
  ReadMemLevel<KeyType>::IteratorImpl::IteratorImpl(
    const ReadMemLevel<KeyType>* read_mem_level)
    throw()
    : read_mem_level_(add_ref(read_mem_level)),
      profiles_it_(read_mem_level_->profiles_.end())
  {}

  template<typename KeyType>
  bool
  ReadMemLevel<KeyType>::IteratorImpl::get_next(
    KeyType& key,
    ProfileOperation& operation,
    Gears::Time& access_time)
    throw()
  {
    if(profiles_it_ == read_mem_level_->profiles_.end())
    {
      profiles_it_ = read_mem_level_->profiles_.begin();
    }
    else
    {
      ++profiles_it_;
    }

    if(profiles_it_ != read_mem_level_->profiles_.end())
    {
      key = profiles_it_->first;
      operation = static_cast<ProfileOperation>(profiles_it_->second.operation);
      access_time = Gears::Time(profiles_it_->second.access_time);
      return true;
    }

    return false;
  }

  template<typename KeyType>
  Gears::ConstSmartMemBuf_var
  ReadMemLevel<KeyType>::IteratorImpl::get_profile()
    throw()
  {
    assert(profiles_it_ != read_mem_level_->profiles_.end());
    return profiles_it_->second.mem_buf;
  }

  template<typename KeyType>
  CheckProfileResult
  ReadMemLevel<KeyType>::check_profile(const KeyType& key)
    const throw(typename ReadBaseLevel<KeyType>::Exception)
  {
    return this->check_profile_i(key);
  }

  template<typename KeyType>
  GetProfileResult
  ReadMemLevel<KeyType>::get_profile(
    const KeyType& key)
    const throw(typename ReadBaseLevel<KeyType>::Exception)
  {
    return this->get_profile_i(key);
  }

  template<typename KeyType>
  typename ReadBaseLevel<KeyType>::KeyIterator_var
  ReadMemLevel<KeyType>::get_key_iterator() const
    throw()
  {
    return new KeyIteratorImpl(this);
  }

  template<typename KeyType>
  typename ReadBaseLevel<KeyType>::Iterator_var
  ReadMemLevel<KeyType>::get_iterator(
    unsigned long /*read_buffer_size*/) const
    throw()
  {
    return new IteratorImpl(this);
  }

  template<typename KeyType>
  unsigned long
  ReadMemLevel<KeyType>::size() const throw()
  {
    return this->size_i();
  }

  template<typename KeyType>
  uint64_t
  ReadMemLevel<KeyType>::area_size() const throw()
  {
    return this->area_size_i();
  }

  template<typename KeyType>
  unsigned long
  ReadMemLevel<KeyType>::merge_free_size() const throw()
  {
    return this->merge_free_size_i();
  }

  template<typename KeyType>
  Gears::Time
  ReadMemLevel<KeyType>::min_access_time() const throw()
  {
    return this->min_access_time_i();
  }
}
