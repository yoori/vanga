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

#ifndef ACTDISCREPANCYMETRICEVALUATOR_HPP_
#define ACTDISCREPANCYMETRICEVALUATOR_HPP_

#include "Label.hpp"

namespace Vanga
{
  // ActDiscrepancyMetricEvaluator
  class ActDiscrepancyMetricEvaluator
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
    double l_sum_;
    unsigned long count_;
  };
}

namespace Vanga
{
  // ActDiscrepancyMetricEvaluator
  inline void
  ActDiscrepancyMetricEvaluator::start_metric_eval()
  {
    l_sum_ = 0;
    count_ = 0;
  }

  inline void
  ActDiscrepancyMetricEvaluator::add_metric_eval(
    const PredictedBoolLabel& label,
    unsigned long count)
  {
    l_sum_ += label.value * count;
    count_ += count;
  }

  inline double
  ActDiscrepancyMetricEvaluator::metric_result() const
  {
    const double Ml = l_sum_ / count_;
    return (l_sum_ - 2.0 * l_sum_ * Ml + Ml * Ml * count_) / count_;
  }
}

#endif /*ACTDISCREPANCYMETRICEVALUATOR_HPP_*/
