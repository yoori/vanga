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
#include <random>
#include <vector>
#include <Gears/Basic/IntervalSet.hpp>
#include <Gears/Basic/IntervalMap.hpp>

using namespace Gears;

struct CrossFun
{
  int operator()(int left, int right) const;
};

int CrossFun::operator()(int left, int right) const
{
  return std::max(left, right);
}

using TestIntervalMap = IntervalMap<int, int, CrossFun>;
using TestIntervalArray = std::vector<std::pair<Interval<int>, int>>;

struct PairEqual
{
  template <typename LeftFirstType, typename LeftSecondType, typename RightFirstType,
    typename RightSecondType>
  bool operator()(const std::pair<LeftFirstType, LeftSecondType>& left,
    const std::pair<RightFirstType, RightSecondType>& right) const;
};

template <typename LeftFirstType, typename LeftSecondType, typename RightFirstType,
  typename RightSecondType>
bool PairEqual::operator()(const std::pair<LeftFirstType, LeftSecondType>& left,
  const std::pair<RightFirstType, RightSecondType>& right) const
{
  return left.first == right.first && left.second == right.second;
}

bool check_(const TestIntervalMap& check, const TestIntervalArray& etalon)
{
  bool res = (check.size() == etalon.size() &&
              std::equal(check.begin(), check.end(), etalon.begin(), PairEqual()));

  if (!res)
  {
    std::cerr << "check failed, result:" << std::endl;
    for (TestIntervalMap::const_iterator it = check.begin(); it != check.end(); ++it)
    {
      std::cerr << "  (" << it->first.min << ", " << it->first.max << "): " << it->second
                << std::endl;
    }
    std::cerr << "expected:" << std::endl;
    for (TestIntervalArray::const_iterator it = etalon.begin(); it != etalon.end(); ++it)
    {
      std::cerr << "  (" << it->first.min << ", " << it->first.max << "): " << it->second
                << std::endl;
    }
  }

  return res;
}

int main()
{
  TestIntervalMap intervals;
  using ValueType = TestIntervalMap::value_type;

  TestIntervalArray intervals_etalon;

  // check packing
  intervals.insert(ValueType(Interval<int>(0, 1), 1));
  intervals.insert(ValueType(Interval<int>(2, 4), 2));
  intervals.insert(ValueType(Interval<int>(4, 6), 2));

  intervals_etalon.push_back(std::make_pair(Interval<int>(0, 1), 1));
  intervals_etalon.push_back(std::make_pair(Interval<int>(2, 6), 2));

  if (!check_(intervals, intervals_etalon))
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

  if (!check_(intervals, intervals_etalon))
  {
    return -1;
  }

  // Check splitting, gaps, containment and both insertion orders against
  // a pointwise oracle, including normalization of equal adjacent values.
  std::mt19937 random(42);
  intervals.clear();
  std::vector<int> expected(40, 0);
  for (unsigned int step = 0; step < 2000; ++step)
  {
    if (step % 20 == 0)
    {
      intervals.clear();
      std::fill(expected.begin(), expected.end(), 0);
    }
    int left = random() % 41;
    int right = random() % 41;
    if (right < left)
    {
      std::swap(left, right);
    }
    int value = 1 + random() % 9;
    intervals.insert(ValueType(Interval<int>(left, right), value));
    for (int point = left; point < right; ++point)
    {
      expected[point] = std::max(expected[point], value);
    }

    intervals_etalon.clear();
    for (int begin = 0; begin < 40;)
    {
      int end = begin + 1;
      while (end < 40 && expected[end] == expected[begin])
      {
        ++end;
      }

      if (expected[begin] != 0)
      {
        intervals_etalon.push_back(ValueType(Interval<int>(begin, end), expected[begin]));
      }
      begin = end;
    }

    if (!check_(intervals, intervals_etalon))
    {
      std::cerr << "insertion step " << step << std::endl;
      return -1;
    }
    for (int point = 0; point <= 40; ++point)
    {
      auto found = intervals.find(point);
      int actual = found != intervals.end() ? found->second : 0;
      if (actual != (point < 40 ? expected[point] : 0))
      {
        std::cerr << "incorrect lookup at " << point << std::endl;
        return -1;
      }
    }
  }

  return 0;
}
