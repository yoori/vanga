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

#ifndef LOGLOSSMETRICEVALUATOR_HPP_
#define LOGLOSSMETRICEVALUATOR_HPP_

#include <DTree/Label.hpp>

namespace Vanga
{
  // LogLossMetricEvaluator
  class LogLossMetricEvaluator
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
  LogLossMetricEvaluator::start_metric_eval()
  {
    result_metric_ = 0.0;
  }

  inline void
  LogLossMetricEvaluator::add_metric_eval(
    const PredictedBoolLabel& label,
    unsigned long count)
  {
    //const double exp = std::exp(- label.pred);
    const double exp = std::exp(- std::min(std::max(label.pred, LOGLOSS_EXP_MIN), LOGLOSS_EXP_MAX));

    /*
    std::cout << "add_metric_eval: exp = " << exp << ", count = " << count << ", ";
    label.print(std::cout);
    std::cout << std::endl;
    */
    double metric_delta;

    if(label.value)
    {
      metric_delta = (std::log(1.0 / (1.0 + exp)));
      //std::cout << "add_metric_eval: l1 = " << (1.0 / (1.0 + exp)) << std::endl;
    }
    else
    {
      metric_delta = (std::log(1 - 1.0 / (1.0 + exp)));
      //std::cout << "add_metric_eval: l2 = " << (1 - 1.0 / (1.0 + exp)) << std::endl;
    }

    assert(std::abs(metric_delta) < 10000000.0);

    /*
    std::cout << "add_metric_eval: metric_delta = " << metric_delta << std::endl;
    std::cout << "add_metric_eval: metric_delta * count = " << (metric_delta * count) << std::endl;
    */
    result_metric_ -= metric_delta * count;
  }

  inline double
  LogLossMetricEvaluator::metric_result() const
  {
    return result_metric_;
  }
}

#endif
