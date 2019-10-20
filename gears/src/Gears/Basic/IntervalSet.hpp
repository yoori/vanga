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

#ifndef INTERVALSET_HPP
#define INTERVALSET_HPP

#include <limits>
#include <set>

namespace Gears
{
  // Interval
  template<typename ValueType>
  struct Interval
  {
    Interval(unsigned long min_val,
      unsigned long max_val);

    bool
    operator<(const Interval& right) const;

    bool
    operator==(const Interval& right) const;

    ValueType min, max;
  };

  // IntervalSet
  template<typename ValueType>
  struct IntervalSet: public std::set<Interval<ValueType> >
  {
    void
    normalize(
      const ValueType& min = std::numeric_limits<ValueType>::min(),
      const ValueType& max = std::numeric_limits<ValueType>::max())
      throw();

    bool
    contains(const ValueType& val) const throw();

    void
    cross(IntervalSet<ValueType>& res,
      const ValueType& min,
      const ValueType& max)
      const throw();

    void
    cross(const IntervalSet<ValueType>& res)
      throw();
  };
}

namespace Gears
{
  // Interval
  template<typename ValueType>
  Interval<ValueType>::Interval(unsigned long min_val,
    unsigned long max_val)
    : min(min_val), max(max_val)
  {}

  template<typename ValueType>
  bool
  Interval<ValueType>::operator<(const Interval& right) const
  {
    return min < right.min || (
      min == right.min && max < right.max);
  }

  template<typename ValueType>
  bool
  Interval<ValueType>::operator==(const Interval& right) const
  {
    return min == right.min && max == right.max;
  }

  // IntervalSet
  template<typename ValueType>
  void
  IntervalSet<ValueType>::normalize(
    const ValueType& min,
    const ValueType& max)
    throw()
  {
    std::set<Interval<ValueType> > ints;

    ValueType pmax = min;
    for(typename std::set<Interval<ValueType> >::iterator it =
          this->begin(); it != this->end(); ++it)
    {
      ValueType rmin = std::max(pmax, it->min);
      ValueType rmax = std::min(it->max, max);
      if(rmin <= rmax)
      {
        if(rmin == pmax && !ints.empty())
        {
          // union intervals
          ValueType pmin = ints.rbegin()->min;
          ints.erase(--ints.end());
          ints.insert(Interval<ValueType>(pmin, rmax));
        }
        else
        {
          ints.insert(Interval<ValueType>(rmin, rmax));
        }

        pmax = rmax;
      }
    }

    this->swap(ints);
  }

  template<typename ValueType>
  bool
  IntervalSet<ValueType>::contains(const ValueType& val) const throw()
  {
    typename std::set<Interval<ValueType> >::const_iterator lit =
      this->upper_bound(Interval<ValueType>(val, 0));
    return lit != this->end() && lit->min == val ||
      lit-- != this->begin() && lit->min <= val && val < lit->max;
  }

  template<typename ValueType>
  void
  IntervalSet<ValueType>::cross(
    IntervalSet<ValueType>& res,
    const ValueType& min,
    const ValueType& max)
    const
    throw()
  {
    typedef typename std::set<Interval<ValueType> >::const_iterator
      ConstIterator;

    if(min < max || min == max)
    {
      ConstIterator min_it = this->lower_bound(Interval<ValueType>(min, 0));
      ConstIterator max_it = this->upper_bound(Interval<ValueType>(max, 0));
      ConstIterator copy_begin_it = min_it;

      if(min_it != this->begin() && min < (--min_it)->max)
      {
        res.insert(Interval<ValueType>(
          min,
          std::min(min_it->max, max)));
      }

      if(copy_begin_it != max_it)
      {
        ConstIterator copy_end_it = max_it;

        if(max < (--max_it)->max)
        {
          res.insert(Interval<ValueType>(max_it->min, max));
          copy_end_it = max_it;
        }

        std::copy(copy_begin_it,
          copy_end_it,
          std::inserter(res, res.begin()));
      }
    }
  }

  template<typename ValueType>
  void
  IntervalSet<ValueType>::cross(const IntervalSet<ValueType>& right)
    throw()
  {
    IntervalSet<ValueType> res;
    for(typename IntervalSet<ValueType>::const_iterator rit =
          right.begin();
        rit != right.end(); ++rit)
    {
      cross(res, rit->min, rit->max);
    }
    swap(res);
  }
}

#endif /*INTERVALSET_HPP*/
