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

#ifndef FUNSQUAREDIVIATIONLOSS_HPP_
#define FUNSQUAREDIVIATIONLOSS_HPP_

#include <DTree/FunOptimization.hpp>
#include <DTree/FunDiscrepancy.hpp>
#include <DTree/FunSum.hpp>

namespace Vanga
{
  template<typename IteratorType>
  struct FunSquareDiviationLoss
  {
  public:
    FunSquareDiviationLoss(const std::vector<VarGroup<IteratorType> >& preds)
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
  FunSquareDiviationLoss<IteratorType>
  fun_square_diviation_loss(const std::vector<VarGroup<IteratorType> >& preds)
  {
    return FunSquareDiviationLoss<IteratorType>(preds);
  }
}

namespace Vanga
{
  template<typename IteratorType>
  double
  FunSquareDiviationLoss<IteratorType>::eval_fun_and_grad(
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
    //
    // e = exp(-exp_arg)
    //
    // d D / d x = 2 / count (
    //   SUM(- var_yes * (exp / e^2 * (L - 1/e))) +
    //   SUM(var_yes (exp / e^2)) * SUM(L - 1/e) / count
    //
    // B1 = - SUM(var_yes * (exp / e^2 * (L - 1 / e)))
    // B2 = SUM(var_yes * (exp / e^2))
    // B3 = SUM(L - 1 / e)
    //
    // dD/dx = 2/count * (B1 + B2 * B3  / count)
    //
    /*
    double B1 = 0;
    double B2 = 0;
    double B3 = 0;
    */
    unsigned long count = 0;

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
        group_x += var_value;
      }

      // eval base grad - d_grads[0] and fun
      double grad_sum = 0.0;

      for(auto sector_pred_it = pred_it->begin;
        sector_pred_it != pred_it->end; ++sector_pred_it)
      {
        const double exp_arg = group_x + sector_pred_it->label.pred;
        const double exp = std::exp(- exp_arg);
        const double e = 1 + exp;

        /*
        grad_sum += 2 * exp * (1 - e * sector_pred_it->label.value) *
          sector_pred_it->count / e / e;
        */

        grad_sum += 2 * (1 / e - sector_pred_it->label.value) * (1 - 1 / e) *
          sector_pred_it->count;

        // eval fun
        const double fun_delta = (sector_pred_it->label.value - 1 / e) *
          (sector_pred_it->label.value - 1 / e) *
          sector_pred_it->count;

        fun_val += fun_delta;

        count += sector_pred_it->count;
      }

      if(grad_sum > 1000000000.0)
      {
        std::cout << "grad_sum = " << grad_sum << std::endl;
        assert(0);
      }

      d_grads[0] += grad_sum;

      // eval variable gradients
      for(int var_index = 0; var_index < var_number; ++var_index)
      {
        d_grads[var_index + 1] += (
          var_mask & (static_cast<uint64_t>(1) << var_index) ? grad_sum : -grad_sum);
      }
    }

    for(auto grad_it = d_grads.begin(); grad_it != d_grads.end(); ++grad_it)
    {
      if(*grad_it > 1000000000.0)
      {
        std::cout << "*grad_it = " << *grad_it << std::endl;
        assert(0);
      }
    }

    return fun_val;
  }
}

#endif /*FUNSQUAREDIVIATIONLOSS_HPP_*/
