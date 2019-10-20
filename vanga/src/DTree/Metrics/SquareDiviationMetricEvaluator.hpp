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

#ifndef SQUAREDIVIATIONMETRICEVALUATOR_HPP_
#define SQUAREDIVIATIONMETRICEVALUATOR_HPP_

#include <DTree/Label.hpp>

namespace Vanga
{
  // SquareDiviationMetricEvaluator
  class SquareDiviationMetricEvaluator
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
    double result_metric_;
  };
}

namespace Vanga
{
  // LogLossMetricEvaluator
  inline void
  SquareDiviationMetricEvaluator::start_metric_eval()
  {
    result_metric_ = 0.0;
  }

  inline void
  SquareDiviationMetricEvaluator::add_metric_eval(
    const PredictedBoolLabel& label,
    unsigned long count)
  {
    const double exp = std::exp(- std::min(std::max(label.pred, LOGLOSS_EXP_MIN), LOGLOSS_EXP_MAX));

    const double p = 1.0 / (1.0 + exp);
    double metric_delta;

    if(label.value)
    {
      metric_delta = (1.0 - p) * (1.0 - p);
    }
    else
    {
      metric_delta = p * p;
    }

    assert(std::abs(metric_delta) < 10000000.0);

    result_metric_ += metric_delta * count;
  }

  inline double
  SquareDiviationMetricEvaluator::metric_result() const
  {
    return result_metric_;
  }
}

#endif
