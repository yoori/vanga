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

#ifndef PREDABSLOSSMETRICEVALUATOR_HPP_
#define PREDABSLOSSMETRICEVALUATOR_HPP_

#include "Label.hpp"

namespace Vanga
{
  // PredAbsLossMetricEvaluator
  class PredAbsLossMetricEvaluator
  {
  public:
    void
    start_metric_eval();

    void
    add_metric_eval(
      const PredictedBoolLabel& label,
      unsigned long count);

    double
    metric_result() const;

  protected:
    double sum_;
    unsigned long count_;
  };
}

namespace Vanga
{
  // PredAbsLossMetricEvaluator
  inline void
  PredAbsLossMetricEvaluator::start_metric_eval()
  {
    sum_ = 0;
    count_ = 0;
  }

  inline void
  PredAbsLossMetricEvaluator::add_metric_eval(
    const PredictedBoolLabel& label,
    unsigned long count)
  {
    sum_ += std::abs(label.pred - label.value);
    count_ += count;
  }

  inline double
  PredAbsLossMetricEvaluator::metric_result() const
  {
    if(count_ > 0)
    {
      return sum_ / count_;
    }

    return 0;
  }
}

#endif /*PREDABSLOSSMETRICEVALUATOR_HPP_*/
