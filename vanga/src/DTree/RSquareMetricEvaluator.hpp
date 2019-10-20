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

#ifndef RSQUAREMETRICEVALUATOR_HPP_
#define RSQUAREMETRICEVALUATOR_HPP_

#include "Label.hpp"

namespace Vanga
{
  // RSquareMetricEvaluator
  class RSquareMetricEvaluator
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
    double ss_res_;
    double l_sum_;
    double p_sqr_sum_;
    double p_sum_;
    unsigned long count_;
  };
}

namespace Vanga
{
  // RSquareMetricEvaluator
  inline void
  RSquareMetricEvaluator::start_metric_eval()
  {
    ss_res_ = 0;
    l_sum_ = 0;
    p_sum_ = 0;
    p_sqr_sum_ = 0;
    count_ = 0;
  }

  inline void
  RSquareMetricEvaluator::add_metric_eval(
    const PredictedBoolLabel& label,
    unsigned long count)
  {
    const double exp = std::exp(- std::min(std::max(label.pred, LOGLOSS_EXP_MIN), LOGLOSS_EXP_MAX));
    const double p = 1.0 / (1.0 + exp);

    ss_res_ += (label.value - p) * (label.value - p) * count;
    p_sqr_sum_ += p * p;
    p_sum_ += p;
    l_sum_ += label.value * count;
    count_ += count;
  }

  inline double
  RSquareMetricEvaluator::metric_result() const
  {
    if(l_sum_ < 0.00001)
    {
      return ss_res_ > 0.00001 ? 1 : 0;
    }
    else if(count_ > 0)
    {
      const double Ml = l_sum_ / count_;
      const double ss_reg = p_sqr_sum_ - 2.0 * p_sum_ * Ml + Ml * Ml * count_;
      assert(ss_reg >= 0.0);
      assert(ss_res_ >= 0.0);
      const double res = ss_reg / (ss_reg + ss_res_);
      //const double ss_tot = l_sum_ - 2.0 * l_sum_ * Ml + Ml * Ml * count_;
      //const double res = ss_res_ / (ss_res_ + ss_tot);
      assert(res < 1000000.0);
      return res;
    }

    return -1;
  }
}

#endif /*RSQUAREMETRICEVALUATOR_HPP_*/
