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

#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <cmath>
#include <vector>
#include <iostream>

#include "Algs.hpp"
#include "Predictor.hpp"
#include "FunOptimization.hpp"
#include "FunLogLoss.hpp"
#include "VecUtils.hpp"
#include "LBFGS.hpp"
#include "Label.hpp"

namespace Vanga
{
  namespace Utils
  {
    template<typename PredictorType, typename LabelType>
    double
    logloss(PredictorType* tree, const SVM<LabelType>* svm);

    template<typename PredictorType, typename LabelType>
    double
    log_reg_logloss(PredictorType* tree, const SVM<LabelType>* svm);

    template<typename PredictorType, typename LabelType>
    double
    absloss(PredictorType* tree, const SVM<LabelType>* svm);

    template<typename PredictorType, typename LabelType>
    double
    log_reg_absloss(PredictorType* tree, const SVM<LabelType>* svm);

    template<typename LabelType>
    PredArrayHolder_var
    labels(const SVM<LabelType>* svm);

    double
    avg_logloss(
      const std::vector<PredArrayHolder_var>& preds,
      const PredArrayHolder* labels);

    void
    fill_logloss(
      std::vector<std::pair<PredArrayHolder_var, PredArrayHolder_var> >& res_preds,
      const std::vector<std::pair<PredArrayHolder_var, PredArrayHolder_var> >& preds);

    template<typename FunType>
    void
    reg_grad_vars_min(
      FloatArray& yes_res,
      FloatArray& no_res,
      const FunType& fun);
  }
}

namespace Vanga
{
namespace Utils
{
  template<typename LabelType, typename MetricEvaluatorType>
  double
  eval_metric(
    const SVM<LabelType>* svm,
    const MetricEvaluatorType& metric_eval)
  {
    MetricEvaluatorType local_metric_eval(metric_eval);
    local_metric_eval.start_metric_eval();

    for(auto it = svm->grouped_rows.begin(); it != svm->grouped_rows.end(); ++it)
    {
      local_metric_eval.add_metric_eval((*it)->label, (*it)->rows.size());
    }

    return local_metric_eval.metric_result();
  }

  template<typename LabelType, typename MetricEvaluatorType>
  double
  eval_metric_per_row(
    const SVM<LabelType>* svm,
    const MetricEvaluatorType& metric_eval)
  {
    MetricEvaluatorType local_metric_eval(metric_eval);
    local_metric_eval.start_metric_eval();

    unsigned long rows = 0;

    for(auto it = svm->grouped_rows.begin(); it != svm->grouped_rows.end(); ++it)
    {
      local_metric_eval.add_metric_eval((*it)->label, (*it)->rows.size());
      rows += (*it)->rows.size();
    }

    return local_metric_eval.metric_result() / rows;
  }

  template<typename PredictorType, typename LabelType>
  double
  logloss(PredictorType* predictor, const SVM<LabelType>* svm)
  {
    double loss = 0;
    unsigned long rows = 0;
    //PredictedBoolLabelAddConverter label_conv(predictor);

    for(auto it = svm->grouped_rows.begin(); it != svm->grouped_rows.end(); ++it)
    {
      for(auto row_it = (*it)->rows.begin(); row_it != (*it)->rows.end(); ++row_it)
      {
        double pred = predictor->fpredict((*row_it)->features);

        if((*it)->label.orig())
        {
          loss -= ::log(std::max(pred, LOGLOSS_EPS));
        }
        else
        {
          loss -= ::log(1 - std::min(pred, 1 - LOGLOSS_EPS));
        }

        ++rows;
      }
    }

    return rows > 0 ? loss / rows : 0.0;
  }

  template<typename PredictorType, typename LabelType>
  double
  log_reg_logloss(PredictorType* predictor, const SVM<LabelType>* svm)
  {
    const double DOUBLE_ONE = 1.0;

    double loss = 0;
    unsigned long rows = 0;
    //PredictedBoolLabelAddConverter label_conv(predictor);

    for(auto it = svm->grouped_rows.begin(); it != svm->grouped_rows.end(); ++it)
    {
      for(auto row_it = (*it)->rows.begin(); row_it != (*it)->rows.end(); ++row_it)
      {
        double label_pred = (*it)->label.pred + (
          predictor ? predictor->fpredict((*row_it)->features) : 0.0);
        double pred = DOUBLE_ONE / (DOUBLE_ONE + std::exp(-label_pred));

        if((*it)->label.orig())
        {
          loss -= ::log(std::max(pred, LOGLOSS_EPS));
        }
        else
        {
          loss -= ::log(1 - std::min(pred, 1 - LOGLOSS_EPS));
        }

        ++rows;
      }
    }

    return rows > 0 ? loss / rows : 0.0;
  }

  template<typename LabelType>
  double
  logloss_by_pred(const SVM<LabelType>* svm)
  {
    const double DOUBLE_ONE = 1.0;

    double loss = 0;
    unsigned long rows = 0;

    for(auto it = svm->grouped_rows.begin(); it != svm->grouped_rows.end(); ++it)
    {
      const double pred = DOUBLE_ONE / (DOUBLE_ONE + std::exp(-(*it)->label.pred));

      if((*it)->label.orig())
      {
        loss -= ::log(std::max(pred, LOGLOSS_EPS)) * (*it)->rows.size();
      }
      else
      {
        loss -= ::log(1 - std::min(pred, 1 - LOGLOSS_EPS)) * (*it)->rows.size();
      }

      rows += (*it)->rows.size();
    }

    return rows > 0 ? loss / rows : 0.0;
  }

  template<typename PredictorType, typename LabelType>
  double
  absloss(PredictorType* predictor, const SVM<LabelType>* svm)
  {
    double loss = 0;
    unsigned long rows = 0;

    for(auto it = svm->grouped_rows.begin(); it != svm->grouped_rows.end(); ++it)
    {
      for(auto row_it = (*it)->rows.begin(); row_it != (*it)->rows.end(); ++row_it)
      {
        double pred = predictor->fpredict((*row_it)->features);
        loss += std::fabs((*it)->label.to_float() - pred);
        ++rows;
      }
    }

    return rows > 0 ? loss / rows : 0.0;
  }

  template<typename PredictorType, typename LabelType>
  double
  log_reg_absloss(PredictorType* predictor, const SVM<LabelType>* svm)
  {
    const double DOUBLE_ONE = 1.0;

    double loss = 0;
    unsigned long rows = 0;
    //PredictedBoolLabelAddConverter label_conv(predictor);

    for(auto it = svm->grouped_rows.begin(); it != svm->grouped_rows.end(); ++it)
    {
      for(auto row_it = (*it)->rows.begin(); row_it != (*it)->rows.end(); ++row_it)
      {
        double label_pred = (*it)->label.pred + (
          predictor ? predictor->fpredict((*row_it)->features) : 0.0);
        //PredictedBoolLabel label = label_conv(*row_it, (*it)->label);
        double pred = DOUBLE_ONE / (DOUBLE_ONE + std::exp(-label_pred));
        loss += std::fabs((*it)->label.to_float() - pred);
        ++rows;
      }
    }

    return rows > 0 ? loss / rows : 0.0;
  }

  template<typename LabelType>
  PredArrayHolder_var
  labels(const SVM<LabelType>* svm)
  {
    PredArrayHolder_var res = new PredArrayHolder();
    res->values.resize(svm->size());

    unsigned long row_i = 0;
    for(auto group_it = svm->grouped_rows.begin();
      group_it != svm->grouped_rows.end(); ++group_it)
    {
      for(auto row_it = (*group_it)->rows.begin();
        row_it != (*group_it)->rows.end(); ++row_it, ++row_i)
      {
        res->values[row_i] = (*group_it)->label.orig() ? 1.0 : 0.0;
      }
    }

    return res;
  }

  inline void
  fill_logloss(
    std::vector<std::pair<PredArrayHolder_var, PredArrayHolder_var> >& res_preds,
    const std::vector<std::pair<PredArrayHolder_var, PredArrayHolder_var> >& preds)
  {
    for(auto pred_it = preds.begin(); pred_it != preds.end(); ++pred_it)
    {
      PredArrayHolder_var res_first = new PredArrayHolder();
      res_first->values.resize(pred_it->first->values.size());

      auto res_it = res_first->values.begin();

      for(auto labeled_it = pred_it->first->values.begin();
        labeled_it != pred_it->first->values.end();
        ++labeled_it, ++res_it)
      {
        *res_it = - ::log(std::max(*labeled_it, LOGLOSS_EPS));
      }

      PredArrayHolder_var res_second = new PredArrayHolder();
      res_second->values.resize(pred_it->second->values.size());

      res_it = res_second->values.begin();

      for(auto unlabeled_it = pred_it->second->values.begin();
        unlabeled_it != pred_it->second->values.end();
        ++unlabeled_it, ++res_it)
      {
        *res_it = - ::log(1 - std::min(*unlabeled_it, 1 - LOGLOSS_EPS));
      }

      res_preds.push_back(std::make_pair(res_first, res_second));
    }
  }

  inline double
  avg_logloss(
    const std::vector<PredArrayHolder_var>& preds,
    const PredArrayHolder* labels)
  {
    double loss = 0;

    for(size_t i = 0; i < labels->values.size(); ++i)
    {
      double pred = 0.0;
      for(auto pred_it = preds.begin(); pred_it != preds.end(); ++pred_it)
      {
        pred += (*pred_it)->values[i];
      }
      pred /= preds.size();

      loss -= (1.0 - labels->values[i]) * ::log(1 - std::min(pred, 1 - LOGLOSS_EPS)) +
        labels->values[i] * ::log(std::max(pred, LOGLOSS_EPS));
    }

    return loss;
  }

  template<typename ContainerType>
  double
  eval_r(
    const ContainerType& exp_array,
    unsigned long unlabeled_sum,
    double point)
  {
    double f_val = -static_cast<double>(unlabeled_sum);

    for(auto exp_it = exp_array.begin(); exp_it != exp_array.end(); ++exp_it)
    {
      double r = exp_it->first * std::exp(point);
      double divider = 1.0 + r;
      f_val -= static_cast<double>(exp_it->second) * r / (divider * divider);
    }

    return f_val;
  }

  template<typename ContainerType>
  double
  eval_r_derivative(
    const ContainerType& exp_array,
    unsigned long unlabeled_sum,
    double point)
  {
    double f_val = static_cast<double>(unlabeled_sum);

    for(auto exp_it = exp_array.begin(); exp_it != exp_array.end(); ++exp_it)
    {
      double r = std::exp(exp_it->first + point);
      double divider = 1.0 + r;
      f_val -= static_cast<double>(exp_it->second) / divider;
    }

    return f_val;
  }

  void
  reduce_grad_for_step(FloatArray& grads)
  {
    double max_grad = std::fabs(grads[0]);
    unsigned long max_grad_index = 0;

    for(unsigned long var_index = 1; var_index < grads.size(); ++var_index)
    {
      const double abs_grad = std::fabs(grads[var_index]);
      if(abs_grad > max_grad)
      {
        max_grad = abs_grad;
        max_grad_index = var_index;
      }
    }

    for(unsigned long var_index = 0; var_index < grads.size(); ++var_index)
    {
      if(var_index != max_grad_index)
      {
        grads[var_index] = 0.0;
      }
    }
  }

  template<typename FunType>
  void
  reg_grad_vars_min(
    FloatArray& yes_res,
    FloatArray& no_res,
    const FunType& fun)
  {
    // find min of RegLogLoss(x, lambda) = LogLoss(x) + lambda^2 * ||x||, where x = (yes_res, no_res)

    const bool USE_LINESEARCH = true;
    const bool DEBUG = false;
    //const double DISCREPANCY_COEF = 1;
    //const double D_POW = 1;
    //double lambda = 1.0; //100;
    //const bool USE_DISCREPANCY = false;
    const double X_MIN = LOGLOSS_EXP_MIN;
    const double X_MAX = LOGLOSS_EXP_MAX;

    const int var_number = yes_res.size();

    assert(var_number > 0 && static_cast<int>(no_res.size()) == var_number);

    if(DEBUG)
    {
      std::cerr << "reg_grad_vars_logloss_min started" << std::endl;
    }

    double coef = 10.0;

    std::vector<double> d_vars(var_number + 1);
    d_vars[0] = 0.0;
    for(int var_index = 0; var_index < var_number; ++var_index)
    {
      double avg = (no_res[var_index] + yes_res[var_index]) / 2;
      d_vars[0] += avg;
      d_vars[var_index + 1] = yes_res[var_index] - avg;
    }

    /*
    std::cout << "SOLVE: START, d_base = " << d_base <<
      ", d_vars = [";
    Vanga::Algs::print(std::cout, d_vars.begin(), d_vars.end());
    std::cout << "]" << std::endl;
    */

    std::vector<double> d_grads(var_number + 1);
    bool stepped = !USE_LINESEARCH;
    unsigned long iteration = 0;
    const unsigned long MAX_ITERATIONS = 100;
    double pred_min = X_MIN;
    double pred_max = X_MAX;

    /*
    for(auto pred_it = preds.begin(); pred_it != preds.end(); ++pred_it)
    {
      for(auto sector_pred_it = pred_it->begin;
        sector_pred_it != pred_it->end; ++sector_pred_it)
      {
        pred_min = std::min(pred_min, -sector_pred_it->label.pred + X_MIN);
        pred_max = std::max(pred_max, -sector_pred_it->label.pred + X_MAX);
      }
    }
    */

    if(DEBUG)
    {
      std::cerr << "START [";
      Vanga::Algs::print(std::cerr, d_vars.begin(), d_vars.end());
      std::cerr << "]: pred_min = " << pred_min <<
        ", pred_max = " << pred_max <<
        std::endl;
    }

    FloatArray direction(d_vars.size(), 0.0);
    FloatArray new_vars(d_vars.size(), 0.0);
    FloatArray new_grads(d_grads.size(), 0.0);

    FloatArray diff_vars(d_vars.size(), 0.0);

    double new_f = fun.eval_fun_and_grad(d_grads, d_vars);
    double cur_f = new_f;

    do
    {
      if(DEBUG)
      {
        std::cerr << "START STEP [";
        Vanga::Algs::print(std::cerr, d_vars.begin(), d_vars.end());
        std::cerr << "]: f = " << cur_f << ", stepped = " << stepped <<
          ", pred_min = " << pred_min <<
          ", pred_max = " << pred_max <<
          std::endl;
      }

      // update graident for strategy
      if(stepped)
      {
        if(DEBUG)
        {
          std::cerr << "ORIG GRAD : [";
          Vanga::Algs::print(std::cerr, d_grads.begin(), d_grads.end());
          std::cerr << "]" << std::endl;
        }

        reduce_grad_for_step(d_grads);
      }

      // eval grad quad and update point
      double grad_quad_sum = 0.0;

      if(!USE_LINESEARCH)
      {
        new_vars = d_vars;
        new_grads = d_grads;

        for(int var_index = 1; var_index < var_number + 1; ++var_index)
        {
          // bound grad for pred_min, pred_max

          new_grads[var_index] = std::max(new_grads[var_index], new_vars[var_index] - pred_max);
          new_grads[var_index] = std::min(new_grads[var_index], new_vars[var_index] - pred_min);

          assert(std::abs(new_grads[var_index]) < 10000000.0 && !isnan(new_grads[var_index]));

          grad_quad_sum += new_grads[var_index] * new_grads[var_index];

          new_grads[var_index] = coef * new_grads[var_index];

          // eval new point and correct for bounds
          new_vars[var_index] -= new_grads[var_index];
          new_vars[var_index] = std::min(std::max(new_vars[var_index], pred_min), pred_max);
        }

        new_grads[0] = std::max(new_grads[0], new_vars[0] - pred_max);
        new_grads[0] = std::min(new_grads[0], new_vars[0] - pred_min);

        assert(std::abs(new_grads[0]) < 10000000.0 && !isnan(new_grads[0]));

        grad_quad_sum += new_grads[0] * new_grads[0];
        new_grads[0] = coef * new_grads[0];

        new_vars[0] -= new_grads[0];
        new_vars[0] = std::min(std::max(new_vars[0], pred_min), pred_max);

        std::fill(new_grads.begin(), new_grads.end(), 0.0);
        new_f = fun.eval_fun_and_grad(new_grads, new_vars);

        const double eps = 0.9;
        if(new_f > cur_f - eps * coef * grad_quad_sum)
        {
          coef *= 0.5;
        }
      }
      else
      {
        vec_inv_copy(&direction[0], &d_grads[0], d_grads.size());
        grad_quad_sum = vec_norm(&d_grads[0], d_grads.size());
        if(grad_quad_sum > 0.0000001)
        {
          coef = 1.0 / std::sqrt(grad_quad_sum);

          /*
          std::cerr << "to line_search: vars = [";
          Vanga::Algs::print(std::cerr, d_vars.begin(), d_vars.end());
          std::cerr << "], grad = [";
          Vanga::Algs::print(std::cerr, d_grads.begin(), d_grads.end());
          std::cerr << "], dir = [";
          Vanga::Algs::print(std::cerr, direction.begin(), direction.end());
          std::cerr << "], coef = " << coef << std::endl;
          */
          new_vars = d_vars;
          new_grads = d_grads;

          // line search
          bool line_search_res = line_search<FunType>(
            d_vars.size(),
            new_vars, // new vars
            new_f,
            new_grads, // new grads
            coef,
            &direction[0], // direction
            &d_vars[0], // start point
            fun);

          /*
          std::cerr << "from line_search: vars = [";
          Vanga::Algs::print(std::cerr, d_vars.begin(), d_vars.end());
          std::cerr << "], grad = [";
          Vanga::Algs::print(std::cerr, d_grads.begin(), d_grads.end());
          std::cerr << "], dir = [";
          Vanga::Algs::print(std::cerr, direction.begin(), direction.end());
          std::cerr << "], coef = " << coef << std::endl << std::endl;

          std::cerr << "line_search_res = " << line_search_res << std::endl;
          */
          (void)line_search_res;
        }

        grad_quad_sum = vec_norm(&new_grads[0], new_grads.size());
        //coef = 10.0;
      }

      if(DEBUG)
      {
        std::cerr << "step: vars = [";
        Vanga::Algs::print(std::cerr, d_vars.begin(), d_vars.end());
        std::cerr << "], grad = [";
        Vanga::Algs::print(std::cerr, d_grads.begin(), d_grads.end());
        std::cerr << "], coef = " << coef <<
          ": new f = " << new_f <<
          ", prev f = " << cur_f << std::endl;
      }

      if(DEBUG)
      {
        std::cerr << "FIN STEP: f = " << new_f << ", coef = " << coef <<
          ", grad_quad_sum = " << grad_quad_sum << std::endl;
      }

      // check stop criterias
      vec_sub(&diff_vars[0], &d_vars[0], &new_vars[0], d_vars.size());

      const double diff = std::abs(new_f - cur_f);
      const double var_diff = vec_mul(&diff_vars[0], &diff_vars[0], diff_vars.size());

      if((diff < 0.000001 && grad_quad_sum < 0.001 && var_diff < 0.001) ||
         (stepped && (diff < 0.000001 ||
           grad_quad_sum < 0.001 || var_diff < 0.001 || iteration >= MAX_ITERATIONS / 2)))
      {
        if(stepped)
        {
          stepped = false;

          if(DEBUG)
          {
            std::cerr << "STEPPED off" << std::endl;
            coef = 1.0;
          }
        }
        else
        {
          break;
        }
      }

      d_vars = new_vars;
      d_grads = new_grads;
      cur_f = new_f;

      ++iteration;
    }
    while(iteration < MAX_ITERATIONS);

    // normalize result
    for(int var_index = 1; var_index < var_number + 1; ++var_index)
    {
      if(std::fabs(d_vars[var_index]) > 0.001)
      {
        yes_res[var_index - 1] = d_vars[var_index];
        no_res[var_index - 1] = -d_vars[var_index];
      }
      else
      {
        yes_res[var_index - 1] = 0;
        no_res[var_index - 1] = 0;
      }
    }

    yes_res[0] += d_vars[0];
    no_res[0] += d_vars[0];

    if(DEBUG)
    {
      std::cerr << "reg_grad_vars_logloss_min finished: iterations = " << iteration << std::endl;
    }
  }
}
}

#endif /*UTILS_HPP_*/
