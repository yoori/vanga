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

#ifndef GEARS_DELEGATEPROFILEMAP_HPP
#define GEARS_DELEGATEPROFILEMAP_HPP

namespace Gears
{
  template<typename KeyType>
  class DelegateProfileMap:
    public virtual ProfileMap<KeyType>
  {
  public:
    typedef typename ProfileMap<KeyType>::Exception Exception;

    DelegateProfileMap(ProfileMap<KeyType>* profile_map) noexcept;

    virtual void
    wait_preconditions(const KeyType&, OperationPriority op_priority) const
      /*throw(Exception)*/;

    virtual bool
    check_profile(const KeyType& key) const /*throw(Exception)*/;

    virtual
    ConstSmartMemBuf_var
    get_profile(
      const KeyType& key,
      Gears::Time* last_access_time)
      /*throw(Exception)*/;

    virtual void
    save_profile(
      const KeyType& key,
      Gears::ConstSmartMemBuf* mem_buf,
      const Gears::Time& now,
      OperationPriority priority)
      /*throw(Exception)*/;

    virtual bool
    remove_profile(
      const KeyType& key,
      OperationPriority priority)
      /*throw(Exception)*/;

    virtual void copy_keys(
      typename ProfileMap<KeyType>::KeyList& keys)
      /*throw(Exception)*/;

    virtual void clear_expired(const Gears::Time& expire_time)
      /*throw(Exception)*/;

    virtual unsigned long size() const noexcept;

    virtual unsigned long area_size() const noexcept;

  protected:
    ProfileMap<KeyType>* no_add_ref_delegate_map_() const /*noexcept*/;

  private:
    IntrusivePtr<ProfileMap<KeyType> > profile_map_;
  };
}

namespace Gears
{
  /** DelegateProfileMap */
  template<typename KeyType>
  DelegateProfileMap<KeyType>::
  DelegateProfileMap(
    ProfileMap<KeyType>* profile_map)
    noexcept
    : profile_map_(Gears::add_ref(profile_map))
  {}

  template<typename KeyType>
  Gears::ConstSmartMemBuf_var
  DelegateProfileMap<KeyType>::
  get_profile(
    const KeyType& key,
    Gears::Time* last_access_time)
    /*throw(Exception)*/
  {
    return profile_map_->get_profile(key, last_access_time);
  }

  template<typename KeyType>
  void
  DelegateProfileMap<KeyType>::wait_preconditions(
    const KeyType& key,
    OperationPriority op_priority) const
    /*throw(Exception)*/
  {
    return profile_map_->wait_preconditions(key, op_priority);
  }
  
  template<typename KeyType>
  bool
  DelegateProfileMap<KeyType>::
  check_profile(const KeyType& key) const /*throw(Exception)*/
  {
    return profile_map_->check_profile(key);
  }

  template<typename KeyType>
  void
  DelegateProfileMap<KeyType>::
  save_profile(
    const KeyType& key,
    Gears::ConstSmartMemBuf* mem_buf,
    const Gears::Time& now,
    OperationPriority priority)
    /*throw(Exception)*/
  {
    profile_map_->save_profile(key, mem_buf, now, priority);
  }
  
  template<typename KeyType>
  bool
  DelegateProfileMap<KeyType>::
  remove_profile(
    const KeyType& key,
    OperationPriority priority)
    /*throw(Exception)*/
  {
    return profile_map_->remove_profile(key, priority);
  }

  template<typename KeyType>
  void
  DelegateProfileMap<KeyType>::copy_keys(
    typename ProfileMap<KeyType>::KeyList& keys)
    /*throw(Exception)*/
  {
    profile_map_->copy_keys(keys);
  }

  template<typename KeyType>
  void
  DelegateProfileMap<KeyType>::
  clear_expired(const Gears::Time& expire_time)
    /*throw(Exception)*/
  {
    profile_map_->clear_expired(expire_time);
  }

  template<typename KeyType>
  unsigned long
  DelegateProfileMap<KeyType>::size() const noexcept
  {
    return profile_map_->size();
  }

  template<typename KeyType>
  unsigned long
  DelegateProfileMap<KeyType>::area_size() const noexcept
  {
    return profile_map_->area_size();
  }

  template<typename KeyType>
  ProfileMap<KeyType>*
  DelegateProfileMap<KeyType>::no_add_ref_delegate_map_() const
  {
    return profile_map_.in();
  }
}

#endif /*GEARS_DELEGATEPROFILEMAP_HPP*/
