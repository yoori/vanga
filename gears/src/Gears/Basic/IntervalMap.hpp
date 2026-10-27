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

#include <algorithm>
#include <iterator>
#include <limits>
#include <set>
#include <map>
#include <vector>

#include "Optional.hpp"
#include "IntervalSet.hpp"

namespace Gears
{
  // IntervalSet
  // KeyType must have operator<
  // ValueType must have operator==
  //
  template <typename KeyType, typename ValueType, typename CrossFunType>
  class IntervalMap : protected std::map<Interval<KeyType>, ValueType>
  {
  public:
    using const_iterator = typename std::map<Interval<KeyType>, ValueType>::const_iterator;
    using value_type = typename std::map<Interval<KeyType>, ValueType>::value_type;

  public:
    IntervalMap(const CrossFunType& cross_fun = CrossFunType());

    const_iterator begin() const;

    const_iterator end() const;

    const_iterator find(const KeyType& val) const;

    size_t size() const;

    void insert(const value_type& val);

    void clear();

    template <typename ResultCrossFunType, typename RightCrossFunType>
    void
    cross(IntervalMap<KeyType, ValueType, RightCrossFunType>& res, const Interval<KeyType>& min,
      const ValueType& value, const ResultCrossFunType& result_cross_fun) const;

    template <typename RightCrossFunType>
    void cross(const IntervalMap<KeyType, ValueType, RightCrossFunType>& res);

  protected:
    void insert_(const value_type& val);

    template <typename ResultCrossFunType, typename RightCrossFunType>
    void cross_(IntervalMap<KeyType, ValueType, RightCrossFunType>& res,
      const Interval<KeyType>& interval, const ValueType& value,
      const ResultCrossFunType& result_cross_fun) const;

  private:
    CrossFunType cross_fun_;
  };
}

namespace Gears
{
  // IntervalMap
  template <typename KeyType, typename ValueType, typename CrossFunType>
  IntervalMap<KeyType, ValueType, CrossFunType>::IntervalMap(const CrossFunType& cross_fun)
    : cross_fun_(cross_fun)
  {
  }

  template <typename KeyType, typename ValueType, typename CrossFunType>
  typename IntervalMap<KeyType, ValueType, CrossFunType>::const_iterator
  IntervalMap<KeyType, ValueType, CrossFunType>::begin() const
  {
    return std::map<Interval<KeyType>, ValueType>::begin();
  }

  template <typename KeyType, typename ValueType, typename CrossFunType>
  typename IntervalMap<KeyType, ValueType, CrossFunType>::const_iterator
  IntervalMap<KeyType, ValueType, CrossFunType>::end() const
  {
    return std::map<Interval<KeyType>, ValueType>::end();
  }

  template <typename KeyType, typename ValueType, typename CrossFunType>
  void IntervalMap<KeyType, ValueType, CrossFunType>::insert(const value_type& ins)
  {
    if (!(ins.first.min < ins.first.max))
    {
      return;
    }

    using Map = std::map<Interval<KeyType>, ValueType>;
    auto first = this->lower_bound(Interval<KeyType>(ins.first.min, ins.first.min));
    if (first != this->begin())
    {
      auto prev = std::prev(first);
      if (ins.first.min < prev->first.max)
      {
        first = prev;
      }
    }

    // Stage the split intervals before erasing the overlapping entries.
    std::vector<std::pair<Interval<KeyType>, ValueType>> parts;
    KeyType cursor = ins.first.min;
    auto last = first;
    for (; last != this->end() && last->first.min < ins.first.max; ++last)
    {
      const auto& old = last->first;
      if (old.min < ins.first.min)
      {
        parts.push_back(std::make_pair(Interval<KeyType>(old.min, ins.first.min), last->second));
      }

      if (cursor < old.min)
      {
        parts.push_back(std::make_pair(Interval<KeyType>(cursor, old.min), ins.second));
      }

      const KeyType cross_min = std::max(old.min, ins.first.min);
      const KeyType cross_max = std::min(old.max, ins.first.max);
      parts.push_back(std::
          make_pair(Interval<KeyType>(cross_min, cross_max), cross_fun_(last->second, ins.second)));
      cursor = cross_max;

      if (ins.first.max < old.max)
      {
        parts.push_back(std::make_pair(Interval<KeyType>(ins.first.max, old.max), last->second));
      }
    }

    if (cursor < ins.first.max)
    {
      parts.push_back(std::make_pair(Interval<KeyType>(cursor, ins.first.max), ins.second));
    }

    this->erase(first, last);
    for (const auto& part : parts)
    {
      auto it = Map::insert(part).first;
      if (it != this->begin())
      {
        auto prev = std::prev(it);
        if (prev->first.max == it->first.min && prev->second == it->second)
        {
          const Interval<KeyType> merged(prev->first.min, it->first.max);
          const ValueType value = it->second;
          this->erase(prev);
          this->erase(it);
          it = Map::insert(std::make_pair(merged, value)).first;
        }
      }
      auto next = std::next(it);
      if (next != this->end() && it->first.max == next->first.min && it->second == next->second)
      {
        const Interval<KeyType> merged(it->first.min, next->first.max);
        const ValueType value = it->second;
        this->erase(next);
        this->erase(it);
        Map::insert(std::make_pair(merged, value));
      }
    }
  }

  template <typename KeyType, typename ValueType, typename CrossFunType>
  void IntervalMap<KeyType, ValueType, CrossFunType>::clear()
  {
    std::map<Interval<KeyType>, ValueType>::clear();
  }

  template <typename KeyType, typename ValueType, typename CrossFunType>
  typename IntervalMap<KeyType, ValueType, CrossFunType>::const_iterator
  IntervalMap<KeyType, ValueType, CrossFunType>::find(const KeyType& val) const
  {
    auto it = this->lower_bound(Interval<KeyType>(val, val));
    if (it != this->end() && it->first.min == val && val < it->first.max)
    {
      return it;
    }

    if (it != this->begin())
    {
      --it;
      if (!(val < it->first.min) && val < it->first.max)
      {
        return it;
      }
    }

    return this->end();
  }

  template <typename KeyType, typename ValueType, typename CrossFunType>
  size_t IntervalMap<KeyType, ValueType, CrossFunType>::size() const
  {
    return std::map<Interval<KeyType>, ValueType>::size();
  }

  template <typename KeyType, typename ValueType, typename CrossFunType>
  void IntervalMap<KeyType, ValueType, CrossFunType>::insert_(const value_type& ins)
  {
    std::map<Interval<KeyType>, ValueType>::insert(ins);
  }

  template <typename KeyType, typename ValueType, typename CrossFunType>
  template <typename ResultCrossFunType, typename RightCrossFunType>
  void IntervalMap<KeyType, ValueType, CrossFunType>::
    cross_(IntervalMap<KeyType, ValueType, RightCrossFunType>& res,
      const Interval<KeyType>& interval, const ValueType& value,
      const ResultCrossFunType& result_cross_fun) const
  {
    if (interval.min < interval.max || interval.min == interval.max)
    {
      const_iterator min_it = this->lower_bound(Interval<KeyType>(interval.min, 0));
      const_iterator max_it = this->upper_bound(Interval<KeyType>(interval.max, 0));
      const_iterator copy_begin_it = min_it;

      if (min_it != this->begin() && interval.min < (--min_it)->max)
      {
        res.insert_(std::
            make_pair(Interval<KeyType>(interval.min, std::min(min_it->max, interval.max)),
              result_cross_fun(min_it->second, value)));
      }

      if (copy_begin_it != max_it)
      {
        const_iterator copy_end_it = max_it;

        if (interval.max < (--max_it)->max)
        {
          res.insert_(std::make_pair(Interval<KeyType>(max_it->min, interval.max),
            result_cross_fun(max_it->second, value)));
          copy_end_it = max_it;
        }

        for (const_iterator copy_it = copy_begin_it; copy_it != copy_end_it; ++copy_it)
        {
          res.insert_(std::make_pair(copy_it->first, result_cross_fun(copy_it->first, value)));
        }
      }
    }
  }

  template <typename KeyType, typename ValueType, typename CrossFunType>
  template <typename ResultCrossFunType, typename RightCrossFunType>
  void IntervalMap<KeyType, ValueType, CrossFunType>::
    cross(IntervalMap<KeyType, ValueType, RightCrossFunType>& res,
      const Interval<KeyType>& interval, const ValueType& value,
      const ResultCrossFunType& result_cross_fun) const
  {
    res.clear();
    this->cross_(res, interval, value, result_cross_fun);
  }

  template <typename KeyType, typename ValueType, typename CrossFunType>
  template <typename RightCrossFunType>
  void IntervalMap<KeyType, ValueType, CrossFunType>::
    cross(const IntervalMap<KeyType, ValueType, RightCrossFunType>& right)
  {
    IntervalMap<KeyType, ValueType, CrossFunType> res;
    for (typename IntervalMap<KeyType, ValueType, RightCrossFunType>::
           const_iterator rit = right.begin();
      rit != right.end(); ++rit)
    {
      cross_(res, rit->min, rit->max, rit->second);
    }
    swap(res);
  }
}

#endif /*INTERVALMAP_HPP*/
