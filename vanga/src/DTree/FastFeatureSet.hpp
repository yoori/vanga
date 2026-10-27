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

#ifndef FASTFEATURESET_HPP_
#define FASTFEATURESET_HPP_

#include <cstddef>
#include <vector>

namespace Vanga
{
  class FastFeatureSet
  {
  public:
    static constexpr unsigned int SIZE = 0xFFFFF;

    FastFeatureSet();

    bool check(uint32_t value);

    void set(unsigned long feature_id, uint32_t value);

    void set(const FeatureArray& features);

    template <typename IteratorType>
    void set(const IteratorType& begin_it, const IteratorType& end_it);

    void rollback(unsigned long feature_id);

    template <typename IteratorType>
    void rollback(const IteratorType& begin_it, const IteratorType& end_it);

    void rollback(const FeatureArray& features);

    std::pair<bool, uint32_t> get(uint32_t feature_id) const;

  protected:
    std::vector<uint32_t> eval_feature_indexes_;
  };

  inline FastFeatureSet::FastFeatureSet() : eval_feature_indexes_(SIZE + 1, 0)
  {
  }

  inline bool FastFeatureSet::check(uint32_t value)
  {
    for (auto feature_it = eval_feature_indexes_.begin(); feature_it != eval_feature_indexes_.end();
      ++feature_it)
    {
      if (*feature_it != value)
      {
        return false;
      }
    }

    return true;
  }

  inline void FastFeatureSet::set(unsigned long feature_id, uint32_t value)
  {
    eval_feature_indexes_[feature_id & SIZE] = value;
  }

  inline void FastFeatureSet::set(const FeatureArray& features)
  {
    for (auto feature_it = features.begin(); feature_it != features.end(); ++feature_it)
    {
      eval_feature_indexes_[feature_it->first & SIZE] = (feature_it->second != 0);
    }
  }

  template <typename IteratorType>
  void FastFeatureSet::set(const IteratorType& begin_it, const IteratorType& end_it)
  {
    for (auto feature_it = begin_it; feature_it != end_it; ++feature_it)
    {
      eval_feature_indexes_[feature_it->first & SIZE] = (feature_it->second != 0);
    }
  }

  inline void FastFeatureSet::rollback(unsigned long feature_id)
  {
    eval_feature_indexes_[feature_id & SIZE] = 0;
  }

  template <typename IteratorType>
  void FastFeatureSet::rollback(const IteratorType& begin_it, const IteratorType& end_it)
  {
    for (auto feature_it = begin_it; feature_it != end_it; ++feature_it)
    {
      eval_feature_indexes_[feature_it->first & SIZE] = 0;
    }
  }

  inline void FastFeatureSet::rollback(const FeatureArray& features)
  {
    for (auto feature_it = features.begin(); feature_it != features.end(); ++feature_it)
    {
      eval_feature_indexes_[feature_it->first & SIZE] = 0;
    }
  }

  inline std::pair<bool, uint32_t> FastFeatureSet::get(uint32_t feature_id) const
  {
    if (eval_feature_indexes_[feature_id & SIZE])
    {
      return std::make_pair(true, 1);
    }
    else
    {
      return std::make_pair(false, 0);
    }
  }

}

#endif /*FASTFEATURESET_HPP_*/
