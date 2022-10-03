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

#ifndef BASIC_OPTIONAL_HPP_
#define BASIC_OPTIONAL_HPP_

#include "Macro.hpp"

OPEN_NAMESPACE(Gears)

  template<typename ObjectType>
  class Optional
  {
  public:
    Optional(): defined_(false), val_() {}

    template<typename...Args>
    explicit Optional(bool defined, Args... data): defined_(defined), val_(std::forward<Args>(data)...) {}

    explicit Optional(const ObjectType& val): defined_(true), val_(val) {}

    Optional(const ObjectType* val): defined_(val), val_(val ? *val : ObjectType())
    {}

    const ObjectType*
    operator->() const noexcept
    {
      return &val_;
    }

    ObjectType*
    operator->() noexcept
    {
      return &val_;
    }

    const ObjectType&
    operator*() const noexcept
    {
      return val_;
    }

    ObjectType&
    operator*() noexcept
    {
      return val_;
    }

    bool
    present() const noexcept
    {
      return defined_;
    }

    void
    set(const ObjectType& val)
    {
      val_ = val;
      defined_ = true;
    }

    ObjectType&
    fill()
    {
      defined_ = true;
      return val_;
    }

    Optional&
    operator=(const ObjectType& val)
    {
      set(val);
      return *this;
    }

    Optional&
    operator=(const Optional<ObjectType>& val)
    {
      if(val.present())
      {
        set(*val);
      }
      else
      {
        clear();
      }

      return *this;
    }

    template<typename RightType>
    Optional&
    operator=(const Optional<RightType>& val)
    {
      if(val.present())
      {
        set(*val);
      }
      else
      {
        clear();
      }

      return *this;
    }

    /*
    template<typename RightType>
    Optional&
    operator=(const RightType& val)
    {
      set(val);
      return *this;
    }
    */

    template<typename RightType>
    bool
    operator==(const Optional<RightType>& right) const
    {
      return &right == this || (present() == right.present() &&
        (!present() || **this == *right));
    }

    template <typename RightType>
    bool
    operator !=(const Optional<RightType>& right) const
    {
      return !(*this == right);
    }

    void
    clear()
    {
      defined_ = false;
      val_ = ObjectType();
    }

  protected:
    template <typename CompatibleType>
    Optional(const CompatibleType& val, bool defined)
      : defined_(defined), val_(defined ? ObjectType(val) : ObjectType())
    {}

    void
    present_(bool new_state) noexcept
    {
      defined_ = new_state;
    }

  private:
    bool defined_;
    ObjectType val_;
  };

CLOSE_NAMESPACE

#endif /*BASIC_OPTIONAL_HPP_*/
