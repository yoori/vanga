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

#ifndef METRIC_HPP_
#define METRIC_HPP_

#include "Label.hpp"

namespace Vanga
{
  struct LogLoss
  {
    void
    add(const BoolLabel& label, unsigned long count);

    void
    add(const PredictedBoolLabel& label, unsigned long count);

    double logloss_sum;
    unsigned long count;
  };
}

namespace Vanga
{
  void
  LogLoss::add(const BoolLabel& label, double pred, unsigned long count_val)
  {
    static const double EPS = 0.0000001;

    if(label.value)
    {
      logloss_sum += std::log(std::max(pred, EPS)) * count;
    }
    else
    {
      logloss_sum += std::log(1 - std::min(pred, 1 - EPS)) * count;
    }

    count += count_val;
  }

  void
  LogLoss::add(const PredictedBoolLabel& label, double pred, unsigned long count)
  {
  }

  double
  LogLoss::result() const
  {
    return logloss_sum / count;
  }
}
    
#endif /*METRIC_HPP_*/
