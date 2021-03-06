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

#ifndef ACTEXPECTEDVALUEMETRICEVALUATOR_HPP_
#define ACTEXPECTEDVALUEMETRICEVALUATOR_HPP_

#include "Label.hpp"

namespace Vanga
{
  // LabelCountMetricEvaluator
  class LabelCountMetricEvaluator
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
    double label_count_;
  };

  // ActExpectedValueMetricEvaluator
  class ActExpectedValueMetricEvaluator
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
    double p_sum_;
    unsigned long count_;
  };
}

namespace Vanga
{
  // LabelCountMetricEvaluator
  inline void
  LabelCountMetricEvaluator::start_metric_eval()
  {
    label_count_ = 0;
  }

  inline void
  LabelCountMetricEvaluator::add_metric_eval(
    const PredictedBoolLabel& label,
    unsigned long count)
  {
    label_count_ += label.value * count;
  }

  inline double
  LabelCountMetricEvaluator::metric_result() const
  {
    return label_count_;
  }

  // ActExpectedValueMetricEvaluator
  inline void
  ActExpectedValueMetricEvaluator::start_metric_eval()
  {
    p_sum_ = 0;
    count_ = 0;
  }

  inline void
  ActExpectedValueMetricEvaluator::add_metric_eval(
    const PredictedBoolLabel& label,
    unsigned long count)
  {
    p_sum_ += label.value * count;
    count_ += count;
  }

  inline double
  ActExpectedValueMetricEvaluator::metric_result() const
  {
    return count_ > 0 ? (p_sum_ / count_) : 0.0;
  }
}

#endif /*ACTEXPECTEDVALUEMETRICEVALUATOR_HPP_*/
