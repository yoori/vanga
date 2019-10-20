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

#include <list>
#include <vector>
#include <iterator>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <cmath>
#include <unordered_set>

#include <DTree/Utils.hpp>
#include <DTree/Gain.hpp>

struct Holder
{
  /*
  Holder(double x_val, unsigned long label_count_val, unsigned long count_val)
    : x(x_val),
      label_count(label_count_val),
      count(count_val)
  {}
  */
  double x;
  unsigned long label_count;
  unsigned long count;
};

using namespace Vanga;

struct TestLabel
{
  TestLabel()
    : value(false), pred(0.0)
  {}

  TestLabel(bool value_val, double pred_val)
    : value(value_val),
      pred(pred_val)
  {}

  bool value;
  double pred;
};

struct TestValue
{
  TestValue()
    : count(0)
  {}

  TestValue(bool value_val, double pred_val, unsigned long count_val)
    : label(value_val, pred_val),
      count(count_val)
  {}

  TestLabel label;
  unsigned long count;
};

void
grad_logloss_min_synt_test()
{
  // SYNT

  /*
  {
    // f24379
    PredCollector pred_collector;
    pred_collector.start_delta_eval(1, 0.0);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(false, 0), 9952 - 5740);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(true, 0), 5740);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(false, 0), 48 - 43);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(true, 0), 43);
    pred_collector.fin_delta_eval();

    std::vector<double> yes_res(1, 0.0);
    std::vector<double> no_res(1, 0.0);
    LogLossDeltaEvaluator eval;
    eval.delta_result(yes_res, no_res, pred_collector);

    LogLossMetricEvaluator metric_eval;
    metric_eval.start_metric_eval();
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), no_res[0]),
      9952 - 5740);
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(true, 0), no_res[0]),
      5740);
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), yes_res[0]),
      48 - 43);
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(true, 0), yes_res[0]),
      43);
    double ll = metric_eval.metric_result();

    std::cout << "synt(f24379): no[0] = " << no_res[0] <<
      ", yes[0] = " << yes_res[0] <<
      ", ll = " << ll <<
      std::endl;
  }

  {
    // f7439
    PredCollector pred_collector;
    pred_collector.start_delta_eval(1, 0.0);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(false, 0), 5789 - 5721);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(true, 0), 5721);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(false, 0), 4211 - 62);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(true, 0), 62);
    pred_collector.fin_delta_eval();

    std::vector<double> yes_res(1, 0.0);
    std::vector<double> no_res(1, 0.0);
    LogLossDeltaEvaluator eval;
    eval.delta_result(yes_res, no_res, pred_collector);

    LogLossMetricEvaluator metric_eval;
    metric_eval.start_metric_eval();
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), no_res[0]),
      5789 - 5721);
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(true, 0), no_res[0]),
      5721);
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), yes_res[0]),
      4211 - 62);
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(true, 0), yes_res[0]),
      62);
    double ll = metric_eval.metric_result();

    std::cout << "synt(f7439): no[0] = " << no_res[0] <<
      ", yes[0] = " << yes_res[0] <<
      ", ll = " << ll <<
      std::endl;
  }
  */

  {
    //const double PRED = -0.2876821;

    // f3
    // before split
    double old_ll;

    {
      LogLossMetricEvaluator metric_eval;
      metric_eval.start_metric_eval();
      metric_eval.add_metric_eval(PredictedBoolLabel(false, -18.6102705), 2400);
      metric_eval.add_metric_eval(PredictedBoolLabel(false, -2.7712915), 3200);
      metric_eval.add_metric_eval(PredictedBoolLabel(true, -2.7712915), 200);
      metric_eval.add_metric_eval(PredictedBoolLabel(true, 7.7062343), 2200);
      old_ll = metric_eval.metric_result();
    }

    // after split
    std::vector<double> yes_res(3, 0.0);
    std::vector<double> no_res(3, 0.0);
    double ll;

    {
      PredCollector pred_collector;
      pred_collector.start_delta_eval(3, 0.0);
      pred_collector.add_delta_eval(3, PredictedBoolLabel(false, -18.6102705), 2400);
      pred_collector.add_delta_eval(4, PredictedBoolLabel(false, -2.7712915), 3200);
      pred_collector.add_delta_eval(4, PredictedBoolLabel(true, -2.7712915), 200);
      pred_collector.add_delta_eval(0, PredictedBoolLabel(true, 7.7062343), 2200);
      pred_collector.fin_delta_eval();

      LogLossDeltaEvaluator eval;
      eval.delta_result(yes_res, no_res, pred_collector);

      LogLossMetricEvaluator metric_eval;
      metric_eval.start_metric_eval();
      metric_eval.add_metric_eval(
        PredictedLogLossGain::add_delta(PredictedBoolLabel(false, -18.6102705), yes_res[0] + yes_res[1] + no_res[2]), 2400);
      metric_eval.add_metric_eval(
        PredictedLogLossGain::add_delta(PredictedBoolLabel(false, -2.7712915), no_res[0] + no_res[1] + yes_res[2]), 3200);
      metric_eval.add_metric_eval(
        PredictedLogLossGain::add_delta(PredictedBoolLabel(true, -2.7712915), no_res[0] + no_res[1] + yes_res[2]), 200);
      metric_eval.add_metric_eval(
        PredictedLogLossGain::add_delta(PredictedBoolLabel(true, 7.7062343), no_res[0] + no_res[1] + no_res[2]), 2200);
      ll = metric_eval.metric_result();
    }

    std::cout << "synt(f3): " << std::endl <<
      "  no[0] = " << no_res[0] << ", yes[0] = " << yes_res[0] << std::endl <<
      "  no[1] = " << no_res[1] << ", yes[1] = " << yes_res[1] << std::endl <<
      "  no[2] = " << no_res[2] << ", yes[2] = " << yes_res[2] << std::endl <<
      "  old_ll = " << old_ll <<
      ", ll = " << ll <<
      std::endl;
  }
}

void
grad_logloss_min_synt2_test()
{
  {
    //const double PRED = -0.2876821;

    // f3
    // before split
    double old_ll;

    {
      LogLossMetricEvaluator metric_eval;
      metric_eval.start_metric_eval();
      metric_eval.add_metric_eval(PredictedBoolLabel(false, 4.0466179), 281);
      metric_eval.add_metric_eval(PredictedBoolLabel(false, 4.0466179), 10);
      metric_eval.add_metric_eval(PredictedBoolLabel(true, 4.0466179), 15978);
      metric_eval.add_metric_eval(PredictedBoolLabel(true, 4.0466179), 2237);
      old_ll = metric_eval.metric_result();
    }

    // after split
    std::vector<double> yes_res(1, 0.0);
    std::vector<double> no_res(1, 0.0);
    double ll;

    {
      PredCollector pred_collector;
      pred_collector.start_delta_eval(1, 0.0);
      pred_collector.add_delta_eval(0, PredictedBoolLabel(false, 4.0466179), 281);
      pred_collector.add_delta_eval(1, PredictedBoolLabel(false, 4.0466179), 10);
      pred_collector.add_delta_eval(0, PredictedBoolLabel(true, 4.0466179), 15978);
      pred_collector.add_delta_eval(1, PredictedBoolLabel(true, 4.0466179), 2237);
      pred_collector.fin_delta_eval();

      LogLossDeltaEvaluator eval;
      eval.delta_result(yes_res, no_res, pred_collector);

      LogLossMetricEvaluator metric_eval;
      metric_eval.start_metric_eval();
      metric_eval.add_metric_eval(
        PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 4.0466179), no_res[0]), 281);
      metric_eval.add_metric_eval(
        PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 4.0466179), yes_res[0]), 10);
      metric_eval.add_metric_eval(
        PredictedLogLossGain::add_delta(PredictedBoolLabel(true, 4.0466179), no_res[0]), 15978);
      metric_eval.add_metric_eval(
        PredictedLogLossGain::add_delta(PredictedBoolLabel(true, 4.0466179), yes_res[0]), 2237);
      ll = metric_eval.metric_result();
    }

    std::cout << "synt: " << std::endl <<
      "  no[0] = " << no_res[0] << ", yes[0] = " << yes_res[0] << std::endl <<
      "  old_ll = " << old_ll <<
      ", ll = " << ll <<
      std::endl;
  }
}

void
grad_logloss_eval_test()
{
  {
    //const double PRED = -0.2876821;

    /*
    {
      PredCollector pred_collector;
      pred_collector.start_delta_eval(1, 0.0);
      pred_collector.add_delta_eval(0, PredictedBoolLabel(false, 4.0466179), 281);
      pred_collector.add_delta_eval(1, PredictedBoolLabel(false, 4.0466179), 10);
      pred_collector.add_delta_eval(0, PredictedBoolLabel(true, 4.0466179), 15978);
      pred_collector.add_delta_eval(1, PredictedBoolLabel(true, 4.0466179), 2237);
      pred_collector.fin_delta_eval();

      FloatArray grads(2, 0.0);
      FloatArray vars;
      vars.push_back(566.003);
      vars.push_back(635.442);

      auto fun_ll = Vanga::fun_log_loss(pred_collector.groups());
      double f = fun_ll.eval_fun_and_grad(grads, vars);
      std::cout << "ll([";
      Algs::print(std::cout, vars.begin(), vars.end());
      std::cout << "]) = " << f << ", grads = [";
      Algs::print(std::cout, grads.begin(), grads.end());
      std::cout << "]" << std::endl;
    }
    */

    {
      PredCollector pred_collector;
      pred_collector.start_delta_eval(1, 0.0);
      pred_collector.add_delta_eval(0, PredictedBoolLabel(false, 0), 281);
      pred_collector.add_delta_eval(1, PredictedBoolLabel(false, 0), 10);
      pred_collector.add_delta_eval(0, PredictedBoolLabel(true, 0), 15978);
      pred_collector.add_delta_eval(1, PredictedBoolLabel(true, 0), 2237);
      pred_collector.fin_delta_eval();

      FloatArray grads(2, 0.0);
      FloatArray vars;
      vars.push_back(566.003);
      vars.push_back(635.442);

      auto fun_ll = Vanga::fun_log_loss(pred_collector.groups());
      double f = fun_ll.eval_fun_and_grad(grads, vars);
      std::cout << "ll([";
      Algs::print(std::cout, vars.begin(), vars.end());
      std::cout << "]) = " << f << ", grads = [";
      Algs::print(std::cout, grads.begin(), grads.end());
      std::cout << "]" << std::endl;
    }

    {
      PredCollector pred_collector;
      pred_collector.start_delta_eval(1, 0.0);
      pred_collector.add_delta_eval(0, PredictedBoolLabel(false, 0), 281);
      pred_collector.add_delta_eval(1, PredictedBoolLabel(false, 0), 10);
      pred_collector.add_delta_eval(0, PredictedBoolLabel(true, 0), 15978);
      pred_collector.add_delta_eval(1, PredictedBoolLabel(true, 0), 2237);
      pred_collector.fin_delta_eval();

      FloatArray grads(2, 0.0);
      FloatArray vars;
      vars.push_back(4.7355);
      vars.push_back(-0.6775);

      auto fun_ll = Vanga::fun_log_loss(pred_collector.groups());
      double f = fun_ll.eval_fun_and_grad(grads, vars);
      std::cout << "ll([";
      Algs::print(std::cout, vars.begin(), vars.end());
      std::cout << "]) = " << f << ", grads = [";
      Algs::print(std::cout, grads.begin(), grads.end());
      std::cout << "]" << std::endl;
    }
  }
}

void
grad_logloss_min_test()
{
  typedef std::vector<TestValue>::iterator TestValueIterator;

  {
    // =>0.25
    PredCollector pred_collector;
    pred_collector.start_delta_eval(1, 0.0);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(false, 0), 6);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(true, 0), 2);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(false, 0), 9);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(true, 0), 1);
    pred_collector.fin_delta_eval();

    std::vector<double> yes_res(1, 0.0);
    std::vector<double> no_res(1, 0.0);
    LogLossDeltaEvaluator eval;
    eval.delta_result(yes_res, no_res, pred_collector);

    std::cout << "grad result(yes=>0.1, no=>0.25): "
      "yes_delta = " << yes_res[0] <<
      ", no_delta = " << no_res[0] << std::endl;
  }

  ///*
  {
    // AND
    PredCollector pred_collector;
    pred_collector.start_delta_eval(2, 0.0);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(false, 0), 10);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(false, 0), 10);
    pred_collector.add_delta_eval(2, PredictedBoolLabel(false, 0), 10);
    pred_collector.add_delta_eval(3, PredictedBoolLabel(true, 0), 10);
    pred_collector.fin_delta_eval();

    std::vector<double> yes_res(2, 0.0);
    std::vector<double> no_res(2, 0.0);
    LogLossDeltaEvaluator eval;
    eval.delta_result(yes_res, no_res, pred_collector);

    std::cout << "grad result(AND): "
      "no[0] = " << no_res[0] <<
      ", yes[0] = " << yes_res[0] <<
      ", no[1] = " << no_res[1] <<
      ", yes[1] = " << yes_res[1] << std::endl;
  }

  {
    // OR
    PredCollector pred_collector;
    pred_collector.start_delta_eval(2, 0.0);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(false, 0), 10);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(true, 0), 10);
    pred_collector.add_delta_eval(2, PredictedBoolLabel(true, 0), 10);
    pred_collector.add_delta_eval(3, PredictedBoolLabel(true, 0), 10);
    pred_collector.fin_delta_eval();

    std::vector<double> yes_res(2, 0.0);
    std::vector<double> no_res(2, 0.0);
    LogLossDeltaEvaluator eval;
    eval.delta_result(yes_res, no_res, pred_collector);

    std::cout << "grad result(OR): "
      "no[0] = " << no_res[0] <<
      ", yes[0] = " << yes_res[0] <<
      ", no[1] = " << no_res[1] <<
      ", yes[1] = " << yes_res[1] << std::endl;
  }

  {
    // XOR
    PredCollector pred_collector;
    pred_collector.start_delta_eval(2, 0.0);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(false, 0), 10);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(true, 0), 10);
    pred_collector.add_delta_eval(2, PredictedBoolLabel(true, 0), 10);
    pred_collector.add_delta_eval(3, PredictedBoolLabel(false, 0), 10);
    pred_collector.fin_delta_eval();

    std::vector<double> yes_res(2, 0.0);
    std::vector<double> no_res(2, 0.0);
    LogLossDeltaEvaluator eval;
    eval.delta_result(yes_res, no_res, pred_collector);

    std::cout << "grad result(XOR): "
      "no[0] = " << no_res[0] <<
      ", yes[0] = " << yes_res[0] <<
      ", no[1] = " << no_res[1] <<
      ", yes[1] = " << yes_res[1] << std::endl;
  }

  {
    // AND (1&2),3,4
    PredCollector pred_collector;
    pred_collector.start_delta_eval(4, 0.0);
    pred_collector.add_delta_eval(2, PredictedBoolLabel(false, 0), 1); // 2
    pred_collector.add_delta_eval(5, PredictedBoolLabel(false, 0), 1); // 1 & 3
    pred_collector.add_delta_eval(6, PredictedBoolLabel(false, 0), 1); // 2 & 4
    pred_collector.add_delta_eval(9, PredictedBoolLabel(false, 0), 1); // 1 & 4
    pred_collector.add_delta_eval(10, PredictedBoolLabel(false, 0), 1); // 2 & 4
    pred_collector.add_delta_eval(13, PredictedBoolLabel(false, 0), 1); // 1 & 3 & 4
    pred_collector.add_delta_eval(14, PredictedBoolLabel(false, 0), 1); // 2 & 3 & 4
    pred_collector.add_delta_eval(15, PredictedBoolLabel(true, 0), 2); // 1 & 2 & 3 & 4
    pred_collector.fin_delta_eval();

    std::vector<double> yes_res(4, 0.0);
    std::vector<double> no_res(4, 0.0);
    LogLossDeltaEvaluator eval;
    eval.delta_result(yes_res, no_res, pred_collector);

    LogLossMetricEvaluator metric_eval;
    metric_eval.start_metric_eval();
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), no_res[0] + yes_res[1] + no_res[2] + no_res[3]), 1); // 2: 2
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), yes_res[0] + no_res[1] + yes_res[2] + no_res[3]), 1); // 5: 1 & 3
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), no_res[0] + yes_res[1] + yes_res[2] + no_res[3]), 1); // 6: 2 & 3
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), yes_res[0] + no_res[1] + no_res[2] + yes_res[3]), 1); // 9: 1 & 4
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), no_res[0] + yes_res[1] + no_res[2] + yes_res[3]), 1); // 10: 2 & 4
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), yes_res[0] + no_res[1] + yes_res[2] + yes_res[3]), 1); // 13: 1 & 3 & 4
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), no_res[0] + yes_res[1] + yes_res[2] + yes_res[3]), 1); // 14: 2 & 3 & 4
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(true, 0), yes_res[0] + yes_res[1] + yes_res[2] + yes_res[3]), 2); // 15: 1 & 2 & 3 & 4
    double ll = metric_eval.metric_result();

    metric_eval.start_metric_eval();
    metric_eval.add_metric_eval(PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), -100), 1); // 2: 2
    metric_eval.add_metric_eval(PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), -100), 1); // 5: 1 & 3
    metric_eval.add_metric_eval(PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), -100), 1); // 6: 2 & 3
    metric_eval.add_metric_eval(PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), -100), 1); // 9: 1 & 4
    metric_eval.add_metric_eval(PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), -100), 1); // 10: 2 & 4
    metric_eval.add_metric_eval(PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), -100), 1); // 13: 1 & 3 & 4
    metric_eval.add_metric_eval(PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), -100), 1); // 14: 2 & 3 & 4
    metric_eval.add_metric_eval(PredictedLogLossGain::add_delta(PredictedBoolLabel(true, 0), 100), 2); // 15: 1 & 2 & 3 & 4
    double ll_ideal = metric_eval.metric_result();

    std::cout << "grad result(1&2,3,4): "
      "no[0] = " << no_res[0] <<
      ", yes[0] = " << yes_res[0] <<
      ", no[1] = " << no_res[1] <<
      ", yes[1] = " << yes_res[1] <<
      ", no[2] = " << no_res[2] <<
      ", yes[2] = " << yes_res[2] <<
      ", no[3] = " << no_res[3] <<
      ", yes[3] = " << yes_res[3] <<
      ", ll = " << ll <<
      ", ll_ideal = " << ll_ideal <<
      std::endl << std::endl;
  }

  {
    // 1
    PredCollector pred_collector;
    pred_collector.start_delta_eval(2, 0.0);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(true, 0), 10);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(false, 0), 10);
    pred_collector.add_delta_eval(2, PredictedBoolLabel(true, 0), 10);
    pred_collector.add_delta_eval(3, PredictedBoolLabel(false, 0), 10);
    pred_collector.fin_delta_eval();

    std::vector<double> yes_res(2, 0.0);
    std::vector<double> no_res(2, 0.0);
    LogLossDeltaEvaluator eval;
    eval.delta_result(yes_res, no_res, pred_collector);

    std::cout << "grad result(1; 2 no affect): "
      "no[0] = " << no_res[0] <<
      ", yes[0] = " << yes_res[0] <<
      ", no[1] = " << no_res[1] <<
      ", yes[1] = " << yes_res[1] << std::endl << std::endl;
  }
  //*/
}

void
grad_dispersion_min_test()
{
  typedef std::vector<TestValue>::iterator TestValueIterator;

  {
    // =>0.25
    PredCollector pred_collector;
    pred_collector.start_delta_eval(1, 0.0);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(false, 0), 6);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(true, 0), 2);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(false, 0), 9);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(true, 0), 1);
    pred_collector.fin_delta_eval();

    std::vector<double> yes_res(1, 0.0);
    std::vector<double> no_res(1, 0.0);
    LogLossDeltaEvaluator eval;
    eval.delta_result(yes_res, no_res, pred_collector);

    std::cout << "grad result(yes=>0.1, no=>0.25): "
      "yes_delta = " << yes_res[0] <<
      ", no_delta = " << no_res[0] << std::endl;
  }

  ///*
  {
    // AND
    PredCollector pred_collector;
    pred_collector.start_delta_eval(2, 0.0);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(false, 0), 10);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(false, 0), 10);
    pred_collector.add_delta_eval(2, PredictedBoolLabel(false, 0), 10);
    pred_collector.add_delta_eval(3, PredictedBoolLabel(true, 0), 10);
    pred_collector.fin_delta_eval();

    std::vector<double> yes_res(2, 0.0);
    std::vector<double> no_res(2, 0.0);
    LogLossDeltaEvaluator eval;
    eval.delta_result(yes_res, no_res, pred_collector);

    std::cout << "grad result(AND): "
      "no[0] = " << no_res[0] <<
      ", yes[0] = " << yes_res[0] <<
      ", no[1] = " << no_res[1] <<
      ", yes[1] = " << yes_res[1] << std::endl;
  }

  {
    // OR
    PredCollector pred_collector;
    pred_collector.start_delta_eval(2, 0.0);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(false, 0), 10);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(true, 0), 10);
    pred_collector.add_delta_eval(2, PredictedBoolLabel(true, 0), 10);
    pred_collector.add_delta_eval(3, PredictedBoolLabel(true, 0), 10);
    pred_collector.fin_delta_eval();

    std::vector<double> yes_res(2, 0.0);
    std::vector<double> no_res(2, 0.0);
    LogLossDeltaEvaluator eval;
    eval.delta_result(yes_res, no_res, pred_collector);

    std::cout << "grad result(OR): "
      "no[0] = " << no_res[0] <<
      ", yes[0] = " << yes_res[0] <<
      ", no[1] = " << no_res[1] <<
      ", yes[1] = " << yes_res[1] << std::endl;
  }

  {
    // XOR
    PredCollector pred_collector;
    pred_collector.start_delta_eval(2, 0.0);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(false, 0), 10);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(true, 0), 10);
    pred_collector.add_delta_eval(2, PredictedBoolLabel(true, 0), 10);
    pred_collector.add_delta_eval(3, PredictedBoolLabel(false, 0), 10);
    pred_collector.fin_delta_eval();

    std::vector<double> yes_res(2, 0.0);
    std::vector<double> no_res(2, 0.0);
    LogLossDeltaEvaluator eval;
    eval.delta_result(yes_res, no_res, pred_collector);

    std::cout << "grad result(XOR): "
      "no[0] = " << no_res[0] <<
      ", yes[0] = " << yes_res[0] <<
      ", no[1] = " << no_res[1] <<
      ", yes[1] = " << yes_res[1] << std::endl;
  }

  {
    // AND (1&2),3,4
    PredCollector pred_collector;
    pred_collector.start_delta_eval(4, 0.0);
    pred_collector.add_delta_eval(2, PredictedBoolLabel(false, 0), 1); // 2
    pred_collector.add_delta_eval(5, PredictedBoolLabel(false, 0), 1); // 1 & 3
    pred_collector.add_delta_eval(6, PredictedBoolLabel(false, 0), 1); // 2 & 4
    pred_collector.add_delta_eval(9, PredictedBoolLabel(false, 0), 1); // 1 & 4
    pred_collector.add_delta_eval(10, PredictedBoolLabel(false, 0), 1); // 2 & 4
    pred_collector.add_delta_eval(13, PredictedBoolLabel(false, 0), 1); // 1 & 3 & 4
    pred_collector.add_delta_eval(14, PredictedBoolLabel(false, 0), 1); // 2 & 3 & 4
    pred_collector.add_delta_eval(15, PredictedBoolLabel(true, 0), 2); // 1 & 2 & 3 & 4
    pred_collector.fin_delta_eval();

    std::vector<double> yes_res(4, 0.0);
    std::vector<double> no_res(4, 0.0);
    LogLossDeltaEvaluator eval;
    eval.delta_result(yes_res, no_res, pred_collector);

    LogLossMetricEvaluator metric_eval;
    metric_eval.start_metric_eval();
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), no_res[0] + yes_res[1] + no_res[2] + no_res[3]), 1); // 2: 2
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), yes_res[0] + no_res[1] + yes_res[2] + no_res[3]), 1); // 5: 1 & 3
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), no_res[0] + yes_res[1] + yes_res[2] + no_res[3]), 1); // 6: 2 & 3
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), yes_res[0] + no_res[1] + no_res[2] + yes_res[3]), 1); // 9: 1 & 4
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), no_res[0] + yes_res[1] + no_res[2] + yes_res[3]), 1); // 10: 2 & 4
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), yes_res[0] + no_res[1] + yes_res[2] + yes_res[3]), 1); // 13: 1 & 3 & 4
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), no_res[0] + yes_res[1] + yes_res[2] + yes_res[3]), 1); // 14: 2 & 3 & 4
    metric_eval.add_metric_eval(
      PredictedLogLossGain::add_delta(PredictedBoolLabel(true, 0), yes_res[0] + yes_res[1] + yes_res[2] + yes_res[3]), 2); // 15: 1 & 2 & 3 & 4
    double ll = metric_eval.metric_result();

    metric_eval.start_metric_eval();
    metric_eval.add_metric_eval(PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), -100), 1); // 2: 2
    metric_eval.add_metric_eval(PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), -100), 1); // 5: 1 & 3
    metric_eval.add_metric_eval(PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), -100), 1); // 6: 2 & 3
    metric_eval.add_metric_eval(PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), -100), 1); // 9: 1 & 4
    metric_eval.add_metric_eval(PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), -100), 1); // 10: 2 & 4
    metric_eval.add_metric_eval(PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), -100), 1); // 13: 1 & 3 & 4
    metric_eval.add_metric_eval(PredictedLogLossGain::add_delta(PredictedBoolLabel(false, 0), -100), 1); // 14: 2 & 3 & 4
    metric_eval.add_metric_eval(PredictedLogLossGain::add_delta(PredictedBoolLabel(true, 0), 100), 2); // 15: 1 & 2 & 3 & 4
    double ll_ideal = metric_eval.metric_result();

    std::cout << "grad result(1&2,3,4): "
      "no[0] = " << no_res[0] <<
      ", yes[0] = " << yes_res[0] <<
      ", no[1] = " << no_res[1] <<
      ", yes[1] = " << yes_res[1] <<
      ", no[2] = " << no_res[2] <<
      ", yes[2] = " << yes_res[2] <<
      ", no[3] = " << no_res[3] <<
      ", yes[3] = " << yes_res[3] <<
      ", ll = " << ll <<
      ", ll_ideal = " << ll_ideal <<
      std::endl << std::endl;
  }

  {
    // 1
    PredCollector pred_collector;
    pred_collector.start_delta_eval(2, 0.0);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(true, 0), 10);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(false, 0), 10);
    pred_collector.add_delta_eval(2, PredictedBoolLabel(true, 0), 10);
    pred_collector.add_delta_eval(3, PredictedBoolLabel(false, 0), 10);
    pred_collector.fin_delta_eval();

    std::vector<double> yes_res(2, 0.0);
    std::vector<double> no_res(2, 0.0);
    LogLossDeltaEvaluator eval;
    eval.delta_result(yes_res, no_res, pred_collector);

    std::cout << "grad result(1; 2 no affect): "
      "no[0] = " << no_res[0] <<
      ", yes[0] = " << yes_res[0] <<
      ", no[1] = " << no_res[1] <<
      ", yes[1] = " << yes_res[1] << std::endl << std::endl;
  }
  //*/
}

void
grad_sqr_min_test()
{
  typedef std::vector<TestValue>::iterator TestValueIterator;

  {
    // =>0.25
    PredCollector pred_collector;
    pred_collector.start_delta_eval(1, 0.0);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(false, 0), 6);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(true, 0), 2);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(false, 0), 9);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(true, 0), 1);
    pred_collector.fin_delta_eval();

    std::vector<double> yes_res(1, 0.0);
    std::vector<double> no_res(1, 0.0);
    SquareDiviationDeltaEvaluator eval;
    eval.delta_result(yes_res, no_res, pred_collector);

    std::cout << "grad result(yes=>0.1, no=>0.25): "
      "yes_delta = " << yes_res[0] <<
      ", no_delta = " << no_res[0] << std::endl;
  }
}

void
grad_sqr_min_test2()
{
  typedef std::vector<TestValue>::iterator TestValueIterator;

  {
    // =>0.25
    PredCollector pred_collector;
    pred_collector.start_delta_eval(1, 0.0);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(false, -0.7179448), 6);
    pred_collector.add_delta_eval(0, PredictedBoolLabel(true, -0.7179448), 2);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(false, -0.7179448), 9);
    pred_collector.add_delta_eval(1, PredictedBoolLabel(true, -0.7179448), 1);
    pred_collector.fin_delta_eval();

    std::vector<double> yes_res(1, 0.0);
    std::vector<double> no_res(1, 0.0);
    SquareDiviationDeltaEvaluator eval;
    eval.delta_result(yes_res, no_res, pred_collector);

    std::cout << "grad result(yes=>0.1, no=>0.25): "
      "yes_delta = " << yes_res[0] <<
      ", no_delta = " << no_res[0] << std::endl;
  }
}

// main
int
main(int, char**)
{
  std::cout << "start testing logloss" << std::endl;

  grad_logloss_min_test();
  /*
  grad_logloss_min_synt_test();
  grad_logloss_min_synt2_test();
  */
  //grad_logloss_eval_test();

  std::cout << "start testing square" << std::endl;
  grad_sqr_min_test();
  grad_sqr_min_test2();

  return 0;
}


