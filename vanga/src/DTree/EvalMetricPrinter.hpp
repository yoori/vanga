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

#ifndef EVALMETRICPRINTER_HPP_
#define EVALMETRICPRINTER_HPP_

#include <iostream>

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>

#include "Predictor.hpp"
#include "MetricPrinter.hpp"
#include "Utils.hpp"

namespace Vanga
{
  // EvalMetricPrinter
  template<typename LabelType, typename MetricEvaluatorType>
  struct EvalMetricPrinter: public MetricPrinter<LabelType>
  {
  public:
    EvalMetricPrinter(const char* prefix, bool per_item = false);

    virtual void
    print(
      std::ostream& ostr,
      const SVM<LabelType>* svm) const;

  protected:
    virtual ~EvalMetricPrinter() throw()
    {}

  protected:
    const std::string prefix_;
    const bool per_item_;
  };
}

namespace Vanga
{
  // EvalMetricPrinter
  template<typename LabelType, typename MetricEvaluatorType>
  EvalMetricPrinter<LabelType, MetricEvaluatorType>::EvalMetricPrinter(
    const char* prefix,
    bool per_item)
    : prefix_(prefix),
      per_item_(per_item)
  {}

  template<typename LabelType, typename MetricEvaluatorType>
  void
  EvalMetricPrinter<LabelType, MetricEvaluatorType>::print(
    std::ostream& ostr,
    const SVM<LabelType>* svm) const
  {
    const double metric_val = per_item_ ?
      Utils::eval_metric_per_row(svm, MetricEvaluatorType()) :
      Utils::eval_metric(svm, MetricEvaluatorType());
    std::ostringstream ss;
    ss.setf(std::ios::fixed, std::ios::floatfield);
    ss.precision(5);
    ss << metric_val;
    ostr << prefix_ << ss.str();
  }
}

#endif /*EVALMETRICPRINTER_HPP_*/
