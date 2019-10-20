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

#ifndef NEWGAIN_HPP_
#define NEWGAIN_HPP_

#include "Label.hpp"
#include "Utils.hpp"
#include "LBFGS.hpp"
#include "PredBuffer.hpp"

#include "Metrics/LogLossMetricEvaluator.hpp"

#include "Metrics/FunSquareDiviationLoss.hpp"
#include "Metrics/SquareDiviationMetricEvaluator.hpp"

//#define DEBUG_SOLVE_

namespace Vanga
{
  // PredCollector
  class PredCollector
  {
  public:
    struct Pred
    {
      Pred(): count(0)
      {}

      Pred(const PredictedBoolLabel& label_val, unsigned long count_val)
        : label(label_val),
          count(count_val)
      {}

      PredictedBoolLabel label;
      unsigned long count;
    };

    typedef std::vector<Gears::IntrusivePtr<BufferPtr<Pred> > >
      CollectGroupArray;

    typedef std::vector<VarGroup<Buffer<Pred>::const_iterator> >
      GroupArray;

  public:
    void
    start_delta_eval(
      unsigned long vars_number,
      double add_delta);

    void
    add_delta_eval(
      const VarIndexArray& var_indexes,
      const PredictedBoolLabel& label,
      unsigned long count);

    void
    fin_delta_eval();

    const GroupArray&
    groups() const;

  protected:
    double add_delta_;
    CollectGroupArray collect_groups_;
    GroupArray groups_;
  };

  // LogLossDeltaEvaluator
  class LogLossDeltaEvaluator
  {
  public:
    void
    delta_result(
      FloatArray& yes_res,
      FloatArray& no_res,
      const PredCollector& pred_collector)
      const;
  };

  // SquareDiviationDeltaEvaluator
  class SquareDiviationDeltaEvaluator
  {
  public:
    void
    delta_result(
      FloatArray& yes_res,
      FloatArray& no_res,
      const PredCollector& pred_collector)
      const;
  };

  struct DefaultPinaltyStrategy
  {
    double
    operator()(double gain) const
    {
      return gain;
    }
  };

  // PredictedLogLossGain
  struct PredictedLogLossGain:
    public LogLossDeltaEvaluator,
    public LogLossMetricEvaluator
  {
    static PredictedBoolLabel
    add_delta(const PredictedBoolLabel& val, double delta);
  };

  // PredictedSquareDiviationGain
  struct PredictedSquareDiviationGain:
    public SquareDiviationDeltaEvaluator,
    public SquareDiviationMetricEvaluator
  {
    static PredictedBoolLabel
    add_delta(const PredictedBoolLabel& val, double delta);
  };
}

namespace Vanga
{
  const bool DEBUG_COLL = false; //true;

  // PredCollector
  inline void
  PredCollector::start_delta_eval(
    unsigned long vars_number,
    double add_delta)
  {
    if(DEBUG_COLL)
    {
      std::cout << "pred_collector.start_delta_eval: "
        "add_delta = " << add_delta <<
        ", vars_number = " << vars_number <<
        std::endl;
    }

    collect_groups_.clear();
    collect_groups_.resize(1 << vars_number);

    groups_.clear();

    add_delta_ = add_delta;
  }

  inline void
  PredCollector::add_delta_eval(
    const VarIndexArray& var_indexes,
    const PredictedBoolLabel& label,
    unsigned long count)
  {
    if(DEBUG_COLL)
    {
      std::cout << "pred_collector.add_delta_eval(" << var_indexes.to_int() << "): ";
      label.print(std::cout);
      std::cout << ", count = " << count << std::endl;
    }

    uint64_t group_i = var_indexes.to_int();

    assert(group_i < collect_groups_.size());
    Gears::IntrusivePtr<BufferPtr<Pred> >& group = collect_groups_[group_i];

    if(!group.in())
    {
      group = BufferProvider<Pred>::instance().get();
    }

    Buffer<Pred>& buf = group->buf();
    buf.push_back(Pred(
      PredictedBoolLabel(
        label.value,
        label.pred + add_delta_),
      count));
  }

  void
  PredCollector::fin_delta_eval()
  {
    if(DEBUG_COLL)
    {
      std::cout << "pred_collector.fin_delta_eval" << std::endl;
    }
    
    uint64_t index = 0;
    for(auto buf_it = collect_groups_.begin();
      buf_it != collect_groups_.end(); ++buf_it, ++index)
    {
      if(buf_it->in())
      {
        groups_.push_back(VarGroup<Buffer<Pred>::const_iterator>(
          VarIndexArray(index),
          (*buf_it)->buf().begin(),
          (*buf_it)->buf().end()));
      }
    }
  }

  const PredCollector::GroupArray&
  PredCollector::groups() const
  {
    return groups_;
  }

  // LogLossDeltaEvaluator
  inline void
  LogLossDeltaEvaluator::delta_result(
    FloatArray& yes_res,
    FloatArray& no_res,
    const PredCollector& pred_collector)
    const
  {
    const auto& groups = pred_collector.groups();

    for(auto it = yes_res.begin(); it != yes_res.end(); ++it)
    {
      assert(!std::isnan(*it));
    }

    for(auto it = no_res.begin(); it != no_res.end(); ++it)
    {
      assert(!std::isnan(*it));
    }

    Utils::reg_grad_vars_min(
      yes_res,
      no_res,
      fun_log_loss(groups));

    /*
    std::copy(start_point.begin(), start_point.end(), std::back_inserter(res));

    std::vector<std::pair<Buffer<Pred>::const_iterator, Buffer<Pred>::const_iterator> > values;
    for(auto buf_it = groups_.begin(); buf_it != groups_.end(); ++buf_it)
    {
      //std::cout << "buf: " << (*buf_it)->buf().size() << std::endl;
      values.push_back(std::make_pair((*buf_it)->buf().begin(), (*buf_it)->buf().end()));
    }
    Utils::grad_logloss_min(res, values);
    */
  }

  PredictedBoolLabel
  PredictedLogLossGain::add_delta(
    const PredictedBoolLabel& val,
    double delta)
  {
    PredictedBoolLabel res;
    res.value = val.orig();
    res.pred = val.pred + delta;
    return res;
  }

  // SquareDiviationDeltaEvaluator
  inline void
  SquareDiviationDeltaEvaluator::delta_result(
    FloatArray& yes_res,
    FloatArray& no_res,
    const PredCollector& pred_collector)
    const
  {
    const auto& groups = pred_collector.groups();

    Utils::reg_grad_vars_min(
      yes_res,
      no_res,
      fun_square_diviation_loss(groups));
  }

  PredictedBoolLabel
  PredictedSquareDiviationGain::add_delta(
    const PredictedBoolLabel& val,
    double delta)
  {
    PredictedBoolLabel res;
    res.value = val.orig();
    res.pred = val.pred + delta;
    return res;
  }
}

#endif /*NEWGAIN_HPP_*/
