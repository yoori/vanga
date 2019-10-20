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

#ifndef GEARS_ALGS_HPP_
#define GEARS_ALGS_HPP_

#include <utility>
#include <iterator>
#include <ostream>
#include <set>
#include "OutputMemoryStream.hpp"

namespace Gears
{
  struct PairEqual
  {
    template<typename FirstType, typename SecondType>
    bool
    operator()(
      const std::pair<FirstType, SecondType>& left,
      const std::pair<FirstType, SecondType>& right) const
    {
      return left == right;
    }
  };

  struct FirstArg
  {
    template<typename FirstArgType, typename SecondArgType>
    const FirstArgType&
    operator()(const FirstArgType& left, const SecondArgType& /*right*/) const
    {
      return left;
    }
  };

  struct SecondArg
  {
    template<typename FirstArgType, typename SecondArgType>
    const SecondArgType&
    operator()(const FirstArgType& left, const SecondArgType& right) const
    {
      return right;
    }
  };

  template<typename IteratorType>
  struct IteratorRange: public std::pair<IteratorType, IteratorType>
  {
    typedef typename IteratorType::value_type value_type;
    typedef IteratorType const_iterator;

    IteratorRange() {}

    IteratorRange(const IteratorType& first, const IteratorType& second)
      : std::pair<IteratorType, IteratorType>(first, second)
    {}

    const IteratorType& begin() const
    {
      return this->first;
    }

    const IteratorType& end() const
    {
      return this->second;
    }
  };

  template<typename InsertIteratorType,
    typename FilterOpType>
  class FilterInsertIterator:
    public std::iterator<std::output_iterator_tag, void, void, void, void>
  {
  public:
    FilterInsertIterator(
      InsertIteratorType ins_it,
      FilterOpType filter_op)
      : ins_it_(ins_it), filter_op_(filter_op)
    {}

    template<typename ValueType>
    FilterInsertIterator& operator=(const ValueType& val)
    {
      if(filter_op_(val))
      {
        *(ins_it_++) = val;
      }

      return *this;
    }

    FilterInsertIterator& operator*()
    {
      return *this;
    }

    FilterInsertIterator& operator++()
    {
      return *this;
    }

    FilterInsertIterator operator++(int)
    {
      return *this;
    }

    InsertIteratorType base() const
    {
      return ins_it_;
    }

  private:
    InsertIteratorType ins_it_;
    FilterOpType filter_op_;
  };

  template<typename InsertIteratorType, typename ModifyOpType>
  class ModifyInsertIterator:
    public std::iterator<std::output_iterator_tag, void, void, void, void>
  {
  public:
    ModifyInsertIterator(
      InsertIteratorType ins_it,
      ModifyOpType modify_op)
      : ins_it_(ins_it), modify_op_(modify_op)
    {}

    template<typename ValueType>
    ModifyInsertIterator& operator=(const ValueType& val)
    {
      *(ins_it_++) = modify_op_(val);
      return *this;
    }

    ModifyInsertIterator& operator*()
    {
      return *this;
    }

    ModifyInsertIterator& operator++()
    {
      return *this;
    }

    ModifyInsertIterator& operator++(int)
    {
      return *this;
    }

    InsertIteratorType base() const
    {
      return ins_it_;
    }

  private:
    InsertIteratorType ins_it_;
    ModifyOpType modify_op_;
  };

  template<typename InsertIteratorType, typename FilterOpType>
  FilterInsertIterator<InsertIteratorType, FilterOpType>
  filter_inserter(InsertIteratorType ins_it, FilterOpType filter_op)
  {
    return FilterInsertIterator<InsertIteratorType, FilterOpType>(
      ins_it, filter_op);
  }

  template<typename InsertIteratorType, typename ModifyOpType>
  ModifyInsertIterator<InsertIteratorType, ModifyOpType>
  modify_inserter(InsertIteratorType ins_it, ModifyOpType modify_op)
  {
    return ModifyInsertIterator<InsertIteratorType, ModifyOpType>(
      ins_it, modify_op);
  }

  /*
  template<
    typename Iterator,
    typename UnaryFunction>
  class TransformIterator:
    public std::iterator<std::input_iterator_tag, void, void, void, void>
  {
  public:
    typedef decltype(std::declval<UnaryFunction>()(
      std::declval<typename Iterator::value_type>())) value_type;

    TransformIterator(
      const Iterator& iterator,
      UnaryFunction func)
      : iterator_(iterator), func_(func)
    {}

    value_type
    operator* () const
    {
      return func_(*iterator_);
    }

    TransformIterator&
    operator++()
    {
      ++iterator_;
      return *this;
    }

    bool
    operator==(const TransformIterator& arg)
    {
      return (iterator_ == arg.iterator_);
    }

    bool
    operator!= (const TransformIterator& arg)
    {
      return (iterator_ != arg.iterator_);
    }

  private:
    Iterator iterator_;
    UnaryFunction func_;
  };
  */

  template<
    typename FirstInputIteratorType,
    typename SecondInputIteratorType,
    typename OutputIteratorType,
    typename LessOpType,
    typename MergeOpType>
  OutputIteratorType
  merge_unique(
    FirstInputIteratorType first_it, FirstInputIteratorType first_end,
    SecondInputIteratorType second_it, SecondInputIteratorType second_end,
    OutputIteratorType output_it,
    LessOpType less_op,
    MergeOpType merge_op)
  {
    while(first_it != first_end && second_it != second_end)
    {
      if(less_op(*first_it, *second_it))
      {
        *(output_it++) = *first_it;
        ++first_it;
      }
      else if(less_op(*second_it, *first_it))
      {
        *(output_it++) = *second_it;
        ++second_it;
      }
      else
      {
        *(output_it++) = merge_op(*first_it, *second_it);
        ++first_it;
        ++second_it;
      }
    }

    return std::copy(first_it, first_end,
      std::copy(second_it, second_end, output_it));
  }

  template<
    typename FirstInputIteratorType,
    typename SecondInputIteratorType,
    typename OutputIteratorType,
    typename LessOpType>
  OutputIteratorType
  merge_unique(
    FirstInputIteratorType first_it, FirstInputIteratorType first_end,
    SecondInputIteratorType second_it, SecondInputIteratorType second_end,
    OutputIteratorType output_it,
    LessOpType less_op)
  {
    return merge_unique(
      first_it, first_end,
      second_it, second_end,
      output_it,
      less_op,
      FirstArg());
  }

  template<
    typename FirstInputIteratorType,
    typename SecondInputIteratorType,
    typename OutputIteratorType>
  OutputIteratorType
  merge_unique(
    FirstInputIteratorType first_it, FirstInputIteratorType first_end,
    SecondInputIteratorType second_it, SecondInputIteratorType second_end,
    OutputIteratorType output_it)
  {
    return merge_unique(
      first_it, first_end,
      second_it, second_end,
      output_it,
      std::less<typename std::iterator_traits<FirstInputIteratorType>::
        value_type>(),
      FirstArg());
  }

  template<
    typename FirstInputIteratorType,
    typename SecondInputIteratorType,
    typename OutputIteratorType,
    typename LessOpType,
    typename CrossOpType>
  OutputIteratorType
  cross(
    FirstInputIteratorType first_it, FirstInputIteratorType first_end,
    SecondInputIteratorType second_it, SecondInputIteratorType second_end,
    OutputIteratorType output_it,
    LessOpType less_op, CrossOpType cross_op)
  {
    while(first_it != first_end && second_it != second_end)
    {
      if(less_op(*first_it, *second_it))
      {
        ++first_it;
      }
      else if(less_op(*second_it, *first_it))
      {
        ++second_it;
      }
      else
      {
        *(output_it++) = cross_op(*first_it, *second_it);
        ++first_it;
        ++second_it;
      }
    }

    return output_it;
  }

  template<typename RangeIteratorType, typename LessOpType>
  struct _RangeLess
  {
    _RangeLess() {}
    _RangeLess(const LessOpType& less_val): less_op(less_val) {}

    bool operator()(
      const RangeIteratorType& left, const RangeIteratorType& right) const
    {
      return less_op(*(left.first), *(right.first));
    }

    LessOpType less_op;
  };

  template<
    typename RangeInputInteratorType,
    typename OutputIteratorType,
    typename LessOpType>
  OutputIteratorType custom_merge_n(
    RangeInputInteratorType range_begin,
    RangeInputInteratorType range_end,
    OutputIteratorType output_it,
    LessOpType less_op) throw (Gears::Exception)
  {
    typedef typename std::iterator_traits<
      RangeInputInteratorType>::value_type::const_iterator SubIterator;
    typedef IteratorRange<SubIterator> ItRange;
    typedef std::multiset<ItRange, _RangeLess<ItRange, LessOpType> > RangeSet;
    RangeSet range_set(less_op);

    /* init range set */
    for(; range_begin != range_end; ++range_begin)
    {
      if(range_begin->begin() != range_begin->end())
      {
        range_set.insert(ItRange(range_begin->begin(), range_begin->end()));
      }
    }

    /* merge loop */
    while(!range_set.empty())
    {
      typename RangeSet::iterator cur_it = range_set.begin();
      *output_it++ = *cur_it->first;

      /* update range set */
      ItRange cur_range = *cur_it;
      range_set.erase(cur_it++);
      if(++(cur_range.first) != cur_range.second)
      {
        range_set.insert(cur_range);
      }
    }

    return output_it;
  }

  template<typename IteratorType>
  IteratorRange<IteratorType>
  iterator_range(IteratorType begin, IteratorType end)
  {
    return IteratorRange<IteratorType>(begin, end);
  }

  template<typename IteratorType>
  std::ostream&
  print(std::ostream& out,
    IteratorType it_begin, IteratorType it_end, const char* delim = ", ")
  {
    for(IteratorType it = it_begin; it != it_end; ++it)
    {
      if(it != it_begin)
      {
        out << delim;
      }

      out << *it;
    }

    return out;
  }
}

#endif
