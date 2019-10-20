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

#include <iostream>
#include <vector>
#include <Gears/Basic/IntervalSet.hpp>
#include <Gears/Basic/IntervalMap.hpp>

using namespace Gears;

struct CrossFun
{
  int
  operator()(int left, int right) const
  {
    return std::max(left, right);
  }
};

typedef IntervalMap<int, int, CrossFun> TestIntervalMap;
typedef std::vector<std::pair<Interval<int>, int> > TestIntervalArray;

struct PairEqual
{
  template<
    typename LeftFirstType, typename LeftSecondType,
    typename RightFirstType, typename RightSecondType>
  bool
  operator()(
    const std::pair<LeftFirstType, LeftSecondType>& left,
    const std::pair<RightFirstType, RightSecondType>& right)
    const
  {
    return left.first == right.first && left.second == right.second;
  }
};

bool
check_(const TestIntervalMap& check, const TestIntervalArray& etalon)
{
  bool res = (
    check.size() == etalon.size() &&
    std::equal(check.begin(), check.end(), etalon.begin(), PairEqual()));

  if(!res)
  {
    std::cerr << "check failed, result:" << std::endl;
    for(TestIntervalMap::const_iterator it = check.begin();
        it != check.end(); ++it)
    {
      std::cerr << "  (" << it->first.min << ", " << it->first.max << "): " <<
        it->second << std::endl;
    }
    std::cerr << "expected:" << std::endl;
    for(TestIntervalArray::const_iterator it = etalon.begin();
        it != etalon.end(); ++it)
    {
      std::cerr << "  (" << it->first.min << ", " << it->first.max << "): " <<
        it->second << std::endl;
    }    
  }

  return res;
}

int
main()
{
  TestIntervalMap intervals;
  typedef TestIntervalMap::value_type ValueType;

  TestIntervalArray intervals_etalon;

  // check packing
  intervals.insert(ValueType(Interval<int>(0, 1), 1));
  intervals.insert(ValueType(Interval<int>(2, 4), 2));
  intervals.insert(ValueType(Interval<int>(4, 6), 2));

  intervals_etalon.push_back(std::make_pair(Interval<int>(0, 1), 1));
  intervals_etalon.push_back(std::make_pair(Interval<int>(2, 6), 2));

  if(!check_(intervals, intervals_etalon))
  {
    return -1;
  }

  // check crossing
  intervals.insert(std::make_pair(Interval<int>(3, 5), 3));

  intervals_etalon.clear();
  intervals_etalon.push_back(ValueType(Interval<int>(0, 1), 1));
  intervals_etalon.push_back(ValueType(Interval<int>(2, 3), 2));
  intervals_etalon.push_back(ValueType(Interval<int>(3, 5), 3));
  intervals_etalon.push_back(ValueType(Interval<int>(5, 6), 2));

  if(!check_(intervals, intervals_etalon))
  {
    return -1;
  }

  return 0;
}
