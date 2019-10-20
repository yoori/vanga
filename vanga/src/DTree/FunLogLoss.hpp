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

#ifndef FUNLOGLOSS_HPP_
#define FUNLOGLOSS_HPP_

#include "FunOptimization.hpp"
#include "FunDiscrepancy.hpp"
#include "FunSum.hpp"

namespace Vanga
{
  template<typename IteratorType>
  struct FunLogLoss
  {
  public:
    FunLogLoss(const std::vector<VarGroup<IteratorType> >& preds)
      : preds_(preds)
    {}

    // d_vars[0] is free arg (included for all groups with plus)
    double
    eval_fun_and_grad(
      FloatArray& d_grads,
      const FloatArray& d_vars)
      const
      throw();

  protected:
    const std::vector<VarGroup<IteratorType> >& preds_;
  };

  template<typename IteratorType>
  class FunDiscLogLoss:
    protected FunSum<FunLogLoss<IteratorType>, FunPostQuad>
  {
  public:
    FunDiscLogLoss(
      const std::vector<VarGroup<IteratorType> >& preds,
      double grow_after)
      throw()
      : FunSum<FunLogLoss<IteratorType>, FunPostQuad>(
          FunLogLoss<IteratorType>(preds),
          FunPostQuad(grow_after))
    {}
  };

  template<typename IteratorType>
  FunLogLoss<IteratorType>
  fun_log_loss(const std::vector<VarGroup<IteratorType> >& preds)
  {
    return FunLogLoss<IteratorType>(preds);
  }

  template<typename IteratorType>
  FunDiscLogLoss<IteratorType>
  fun_disc_log_loss(
    const std::vector<VarGroup<IteratorType> >& preds,
    double grow_after)
  {
    return FunDiscLogLoss<IteratorType>(preds, grow_after);
  }
}

namespace Vanga
{
  template<typename IteratorType>
  double
  FunLogLoss<IteratorType>::eval_fun_and_grad(
    FloatArray& d_grads,
    const FloatArray& d_vars)
    const
    throw()
  {
    const int var_number = d_vars.size() - 1;
    double fun_val = 0.0;

    std::fill(d_grads.begin(), d_grads.end(), 0.0);

    /*
    std::cerr << "eval_fun_and_grad, vars = [";
    Vanga::Algs::print(std::cerr, d_vars.begin(), d_vars.end());
    std::cerr << "]" << std::endl;
    */
    for(auto pred_it = preds_.begin(); pred_it != preds_.end(); ++pred_it)
    {
      // determine group point
      uint64_t var_mask = pred_it->vars.to_int();

      double group_x = d_vars[0];

      // eval linear combination of vars for group
      for(int var_index = 0; var_index < var_number; ++var_index)
      {
        const bool var_yes = var_mask & (static_cast<uint64_t>(1) << var_index);
        const double var_value = (var_yes ? d_vars[var_index + 1] : - d_vars[var_index + 1]);
        assert(!std::isnan(var_value));
        group_x += var_value;
      }

      // eval base grad - d_grads[0] and fun
      double grad_sum = 0.0;

      for(auto sector_pred_it = pred_it->begin;
        sector_pred_it != pred_it->end; ++sector_pred_it)
      {
        //const double exp_arg = group_x + sector_pred_it->label.pred;
        static const double LOGLOSS_EXP_MIN = -500.0;
        static const double LOGLOSS_EXP_MAX = 500.0;
        const double exp_arg = std::min(
          std::max(group_x + sector_pred_it->label.pred, LOGLOSS_EXP_MIN),
          LOGLOSS_EXP_MAX);

        const double exp = std::exp(- exp_arg);
        const double e = 1 + exp;
        const double add_grad = (1 - e * sector_pred_it->label.value) * sector_pred_it->count / e;

        assert(!std::isnan(add_grad));
        grad_sum += add_grad;

        // eval fun
        const double fun_delta = (sector_pred_it->label.value ?
          std::log(e) : (exp_arg + std::log(e))) *
          sector_pred_it->count;

        fun_val += fun_delta;

        /*
        std::cerr << "group_x = " << group_x << "(" << var_mask << "), l = " <<
          (sector_pred_it->label.value ? 1 : 0) <<
          ", count = " << sector_pred_it->count <<
          ", fun_delta = " << fun_delta <<
          ", e = " << e <<
          ", p = " << (1.0 / e) << std::endl;
        */
      }

      assert(!std::isnan(grad_sum));

      d_grads[0] += grad_sum;

      // eval variable gradients
      for(int var_index = 0; var_index < var_number; ++var_index)
      {
        /*
        double grad_sum = 0.0;

        for(auto sector_pred_it = pred_it->begin;
          sector_pred_it != pred_it->end; ++sector_pred_it)
        {
          const double exp = std::exp(- sector_pred_it->label.pred - group_x);
          const double e = 1 + exp;
          const double grad_part = (1.0 - sector_pred_it->label.value * e) / e;
          grad_sum += grad_part * sector_pred_it->count;
        }
        */

        d_grads[var_index + 1] += (
          var_mask & (static_cast<uint64_t>(1) << var_index) ? grad_sum : -grad_sum);
      }
    }

    return fun_val;
  }
}

#endif /*FUNLOGLOSS_HPP_*/
