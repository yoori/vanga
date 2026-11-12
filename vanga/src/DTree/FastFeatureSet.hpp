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
#include <type_traits>
#include <unordered_map>

#include "FloatFeatures.hpp"
#include "SVM.hpp"

namespace Vanga
{
  class FastFeatureSet
  {
  public:
    static constexpr unsigned int SIZE = 0xFFFFF;

    FastFeatureSet();

    bool check(uint32_t value);

    void set(unsigned long feature_id, uint32_t value);

    template <typename ValueType> void set(unsigned long feature_id, ValueType value);

    void set(const FeatureArray& features);

    template <typename IteratorType>
    std::enable_if_t<!std::is_arithmetic_v<IteratorType>>
    set(const IteratorType& begin_it, const IteratorType& end_it);

    void rollback(unsigned long feature_id);

    template <typename IteratorType>
    void rollback(const IteratorType& begin_it, const IteratorType& end_it);

    void rollback(const FeatureArray& features);

    void set(const FloatFeatureArray& features);

    void rollback(const FloatFeatureArray& features);

    std::pair<bool, float> get(uint32_t feature_id) const;

  protected:
    void put_(unsigned long feature_id, float value);

    std::vector<float> eval_feature_indexes_;
    std::unordered_map<unsigned long, float> overflow_;
  };

  inline FastFeatureSet::FastFeatureSet() : eval_feature_indexes_(SIZE + 1, 0.0f)
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

    for (const auto& feature : overflow_)
    {
      if (feature.second != value)
      {
        return false;
      }
    }

    return true;
  }

  inline void FastFeatureSet::put_(unsigned long feature_id, float value)
  {
    if (feature_id <= SIZE)
    {
      eval_feature_indexes_[feature_id] = value;
    }
    else if (value == 0)
    {
      overflow_.erase(feature_id);
    }
    else
    {
      overflow_[feature_id] = value;
    }
  }

  inline void FastFeatureSet::set(unsigned long feature_id, uint32_t value)
  {
    put_(feature_id, static_cast<float>(value));
  }

  template <typename ValueType> void FastFeatureSet::set(unsigned long feature_id, ValueType value)
  {
    put_(feature_id, static_cast<float>(value));
  }

  inline void FastFeatureSet::set(const FeatureArray& features)
  {
    for (auto feature_it = features.begin(); feature_it != features.end(); ++feature_it)
    {
      put_(feature_it->first, static_cast<float>(feature_it->second != 0));
    }
  }

  template <typename IteratorType>
  std::enable_if_t<!std::is_arithmetic_v<IteratorType>>
  FastFeatureSet::set(const IteratorType& begin_it, const IteratorType& end_it)
  {
    for (auto feature_it = begin_it; feature_it != end_it; ++feature_it)
    {
      if constexpr (std::is_floating_point_v<std::decay_t<decltype(feature_it->second)>>)
      {
        put_(feature_it->first, static_cast<float>(feature_it->second));
      }
      else
      {
        put_(feature_it->first, static_cast<float>(feature_it->second != 0));
      }
    }
  }

  inline void FastFeatureSet::rollback(unsigned long feature_id)
  {
    put_(feature_id, 0.0f);
  }

  template <typename IteratorType>
  void FastFeatureSet::rollback(const IteratorType& begin_it, const IteratorType& end_it)
  {
    for (auto feature_it = begin_it; feature_it != end_it; ++feature_it)
    {
      rollback(feature_it->first);
    }
  }

  inline void FastFeatureSet::rollback(const FeatureArray& features)
  {
    for (auto feature_it = features.begin(); feature_it != features.end(); ++feature_it)
    {
      rollback(feature_it->first);
    }
  }

  inline void FastFeatureSet::set(const FloatFeatureArray& features)
  {
    for (const auto& feature : features)
    {
      put_(feature.first, feature.second);
    }
  }

  inline void FastFeatureSet::rollback(const FloatFeatureArray& features)
  {
    for (const auto& feature : features)
    {
      rollback(feature.first);
    }
  }

  inline std::pair<bool, float> FastFeatureSet::get(uint32_t feature_id) const
  {
    float value = 0;
    if (feature_id <= SIZE)
    {
      value = eval_feature_indexes_[feature_id];
    }
    else
    {
      const auto it = overflow_.find(feature_id);
      if (it != overflow_.end())
      {
        value = it->second;
      }
    }
    return std::make_pair(value > 0.0000001f, value);
  }

}

#endif /*FASTFEATURESET_HPP_*/
