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

#ifndef INTERVALMAP_HPP
#define INTERVALMAP_HPP

#include <limits>
#include <set>
#include <map>

#include "Optional.hpp"
#include "IntervalSet.hpp"

namespace Gears
{
  // IntervalSet
  // KeyType must have operator<
  // ValueType must have operator==
  //
  template<typename KeyType, typename ValueType, typename CrossFunType>
  class IntervalMap:
    protected std::map<Interval<KeyType>, ValueType>
  {
  public:
    typedef typename std::map<Interval<KeyType>, ValueType>::const_iterator
      const_iterator;
    typedef typename std::map<Interval<KeyType>, ValueType>::value_type
      value_type;

  public:
    IntervalMap(const CrossFunType& cross_fun = CrossFunType());

    const_iterator
    begin() const;

    const_iterator
    end() const;

    const_iterator
    find(const KeyType& val) const;

    size_t
    size() const;

    void
    insert(const value_type& val);

    void
    clear();

    template<typename ResultCrossFunType, typename RightCrossFunType>
    void
    cross(IntervalMap<KeyType, ValueType, RightCrossFunType>& res,
      const Interval<KeyType>& min,
      const ValueType& value,
      const ResultCrossFunType& result_cross_fun)
      const;

    template<typename RightCrossFunType>
    void
    cross(const IntervalMap<KeyType, ValueType, RightCrossFunType>& res);

  protected:
    void
    insert_(const value_type& val);

    template<typename ResultCrossFunType, typename RightCrossFunType>
    void
    cross_(IntervalMap<KeyType, ValueType, RightCrossFunType>& res,
      const Interval<KeyType>& interval,
      const ValueType& value,
      const ResultCrossFunType& result_cross_fun)
      const;

  private:
    CrossFunType cross_fun_;
  };
}

namespace Gears
{
  // IntervalMap
  template<typename KeyType, typename ValueType, typename CrossFunType>
  IntervalMap<KeyType, ValueType, CrossFunType>::
  IntervalMap(const CrossFunType& cross_fun)
    : cross_fun_(cross_fun)
  {}

  template<typename KeyType, typename ValueType, typename CrossFunType>
  typename IntervalMap<KeyType, ValueType, CrossFunType>::const_iterator
  IntervalMap<KeyType, ValueType, CrossFunType>::
  begin() const
  {
    return std::map<Interval<KeyType>, ValueType>::begin();
  }

  template<typename KeyType, typename ValueType, typename CrossFunType>
  typename IntervalMap<KeyType, ValueType, CrossFunType>::const_iterator
  IntervalMap<KeyType, ValueType, CrossFunType>::
  end() const
  {
    return std::map<Interval<KeyType>, ValueType>::end();
  }

  template<typename KeyType, typename ValueType, typename CrossFunType>
  void
  IntervalMap<KeyType, ValueType, CrossFunType>::
  insert(const value_type& ins)
  {
    typedef typename std::map<Interval<KeyType>, ValueType>::iterator iterator;

    std::cerr << "insert([" << ins.first.min << ", " << ins.first.max <<
      ")," << ins.second << ")" << std::endl;
    iterator it = this->lower_bound(Interval<KeyType>(ins.first.min, 0));
    iterator end_it = this->upper_bound(Interval<KeyType>(ins.first.max, 0));

    // MIN <= it->first.min
    if(it != this->begin() && (--it)->first.max < ins.first.min)
    {
      ++it;
    }

    while(end_it != this->end() && ins.first.max < end_it->first.max)
    {
      ++end_it;
    }

    // MIN <= it->first.min || MIN <= it->first.max
    if(it != end_it)
    {
      Optional<KeyType> ins_val_start_point;

      if(it->first.min < ins.first.min)
      {
        // required split of first interval
        // if its value don't override new interval value
        ValueType val = cross_fun_(it->second, ins.second);
        if(val == it->second)
        {
          if(!(it->first.max < ins.first.max)) // it->first.max >= ins.first.max
          {
            return;
          }

          // new interval have external part
          if(val == ins.second) // it->second is max && it->second == ins.second
          {
            ins_val_start_point = it->first.min;
            this->erase(it++);
          }
          else // it->second is max && it->second != ins.second
          {
            ins_val_start_point = it->first.max;
          }
        }
        else // split: ins.second is max
        {
          insert_(std::make_pair(
            Interval<KeyType>(it->first.min, ins.first.min),
            it->second));

          if(ins.first.max < it->first.max) // it->first.max > ins.first.max
          {
            insert_(std::make_pair(
              Interval<KeyType>(ins.first.max, it->first.max),
              it->second));
            this->erase(it);
            return;
          }

          ins_val_start_point = it->first.max;
          this->erase(it++);
        }
      }
      
      // MIN <= it->first.min && MIN <= it->first.max
      for(; it != end_it && it->first.max < ins.first.max; )
      {
        ValueType val = cross_fun_(it->second, ins.second);
        if(val == it->second)
        {
          if(ins_val_start_point.present())
          {
            this->insert_(std::make_pair(
              Interval<KeyType>(*ins_val_start_point, it->first.min),
              it->second));
          }

          ins_val_start_point = it->first.max;
          ++it;
        }
        else
        {
          this->erase(it++);
        }
      }

      // it->min < MAX < it->first.max
      if(it != end_it)
      {
        // split last interval
        // TODO
      }
    }
    else // it == end_it (MIN > max of all intervals)
    {
      this->insert_(ins);
    }
  }

  template<typename KeyType, typename ValueType, typename CrossFunType>
  void
  IntervalMap<KeyType, ValueType, CrossFunType>::clear()
  {
    std::map<Interval<KeyType>, ValueType>::clear();
  }

  template<typename KeyType, typename ValueType, typename CrossFunType>
  typename IntervalMap<KeyType, ValueType, CrossFunType>::const_iterator
  IntervalMap<KeyType, ValueType, CrossFunType>::
  find(const KeyType& val) const
  {
    typename std::map<Interval<KeyType>, ValueType>::
      const_iterator lit = this->upper_bound(Interval<KeyType>(val, 0));
    if(lit != this->end() && lit->min == val ||
       lit-- != this->begin() && lit->min <= val && val < lit->max)
    {
      return lit;
    }

    return this->end();
  }

  template<typename KeyType, typename ValueType, typename CrossFunType>
  size_t
  IntervalMap<KeyType, ValueType, CrossFunType>::
  size() const
  {
    return std::map<Interval<KeyType>, ValueType>::size();
  }

  template<typename KeyType, typename ValueType, typename CrossFunType>
  void
  IntervalMap<KeyType, ValueType, CrossFunType>::
  insert_(const value_type& ins)
  {
    std::map<Interval<KeyType>, ValueType>::insert(ins);
  }

  template<typename KeyType, typename ValueType, typename CrossFunType>
  template<typename ResultCrossFunType, typename RightCrossFunType>
  void
  IntervalMap<KeyType, ValueType, CrossFunType>::
  cross_(
    IntervalMap<KeyType, ValueType, RightCrossFunType>& res,
    const Interval<KeyType>& interval,
    const ValueType& value,
    const ResultCrossFunType& result_cross_fun)
    const
  {
    if(interval.min < interval.max || interval.min == interval.max)
    {
      const_iterator min_it = this->lower_bound(Interval<KeyType>(interval.min, 0));
      const_iterator max_it = this->upper_bound(Interval<KeyType>(interval.max, 0));
      const_iterator copy_begin_it = min_it;

      if(min_it != this->begin() && interval.min < (--min_it)->max)
      {
        res.insert_(std::make_pair(
          Interval<KeyType>(interval.min, std::min(min_it->max, interval.max)),
          result_cross_fun(min_it->second, value)));
      }

      if(copy_begin_it != max_it)
      {
        const_iterator copy_end_it = max_it;

        if(interval.max < (--max_it)->max)
        {
          res.insert_(std::make_pair(
            Interval<KeyType>(max_it->min, interval.max),
            result_cross_fun(max_it->second, value)));
          copy_end_it = max_it;
        }

        for(const_iterator copy_it = copy_begin_it;
          copy_it != copy_end_it; ++copy_it)
        {
          res.insert_(std::make_pair(
            copy_it->first,
            result_cross_fun(copy_it->first, value)));
        }
      }
    }
  }

  template<typename KeyType, typename ValueType, typename CrossFunType>
  template<typename ResultCrossFunType, typename RightCrossFunType>
  void
  IntervalMap<KeyType, ValueType, CrossFunType>::
  cross(
    IntervalMap<KeyType, ValueType, RightCrossFunType>& res,
    const Interval<KeyType>& interval,
    const ValueType& value,
    const ResultCrossFunType& result_cross_fun)
    const
  {
    res.clear();
    this->cross_(res, interval, value, result_cross_fun);
  }

  template<typename KeyType, typename ValueType, typename CrossFunType>
  template<typename RightCrossFunType>
  void
  IntervalMap<KeyType, ValueType, CrossFunType>::
  cross(
    const IntervalMap<KeyType, ValueType, RightCrossFunType>& right)
  {
    IntervalMap<KeyType, ValueType, CrossFunType> res;
    for(typename IntervalMap<KeyType, ValueType, RightCrossFunType>::
          const_iterator rit = right.begin();
        rit != right.end(); ++rit)
    {
      cross_(res, rit->min, rit->max, rit->second);
    }
    swap(res);
  }
}

#endif /*INTERVALMAP_HPP*/
