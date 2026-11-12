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

#ifndef PREDICTOR_SVMUTIL_APPLICATION_HPP_
#define PREDICTOR_SVMUTIL_APPLICATION_HPP_

#include <map>
#include <set>
#include <string>
#include <vector>
#include <deque>

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/Singleton.hpp>
#include <Gears/Basic/Time.hpp>

#include <DTree/TreeLearner.hpp>
#include <DTree/Label.hpp>

using namespace Vanga;

class Application_
{
public:
  Application_() noexcept;

  virtual ~Application_() noexcept;

  void main(int& argc, char** argv) /*throw(Gears::Exception)*/;

protected:
  DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);

  using SVMImpl = SVM<BoolLabel>;

  using SVMImpl_var = Gears::IntrusivePtr<SVMImpl>;

  using FeatureIdSet = std::unordered_set<unsigned long>;

  struct CountFeatureKey
  {
    CountFeatureKey();

    CountFeatureKey(unsigned long count_val, unsigned long feature_id_val);

    bool operator<(const CountFeatureKey& right) const;

    const unsigned long count;
    const unsigned long feature_id;
  };

  using CountFeatureSet = std::set<CountFeatureKey>;

protected:
  // load only counters
  void load_feature_counters_(CountFeatureSet& feature_counters, const char* file_path);

  void correlate_(FeatureIdSet& checked_features, FeatureIdSet& skip_features,
    bool& some_feature_unloaded, const char* file_path, const FeatureIdSet& untouch_features,
    const FeatureIdSet& ignore_features, unsigned long max_features, double max_corr_coef,
    unsigned long min_occur_coef);
};

inline Application_::CountFeatureKey::CountFeatureKey() : count(0), feature_id(0)
{
}

inline Application_::CountFeatureKey::
  CountFeatureKey(unsigned long count_val, unsigned long feature_id_val)
  : count(count_val), feature_id(feature_id_val)
{
}

inline bool Application_::CountFeatureKey::operator<(const CountFeatureKey& right) const
{
  return count < right.count || (count == right.count && feature_id < right.feature_id);
}

using Application = Gears::Singleton<Application_>;

#endif /*PREDICTOR_SVMUTIL_APPLICATION_HPP_*/
