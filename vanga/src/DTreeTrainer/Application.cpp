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
//#include <cmath>
#include <unordered_set>

#include <Gears/Basic/Rand.hpp>
#include <Gears/Basic/AppUtils.hpp>
#include <Gears/Basic/FileManip.hpp>
#include <Gears/String/Csv.hpp>
#include <Gears/Threading/TaskRunner.hpp>

//#include <Commons/Algs.hpp>

#include <DTree/DTree.hpp>
#include <DTree/PredictorSet.hpp>
#include <DTree/LogRegPredictor.hpp>

#include <DTree/Utils.hpp>
#include <DTree/Gain.hpp>
#include <DTree/EvalMetricPrinter.hpp>
//#include <DTree/LogLossMetricEvaluator.hpp>
#include <DTree/RSquareMetricEvaluator.hpp>
#include <DTree/ActExpectedValueMetricEvaluator.hpp>
#include <DTree/PredExpectedValueMetricEvaluator.hpp>
#include <DTree/ActDiscrepancyMetricEvaluator.hpp>

#include <DTree/ActAbsLossMetricEvaluator.hpp>
#include <DTree/PredAbsLossMetricEvaluator.hpp>

#include <DTree/FastFeatureSet.hpp>

#include "Application.hpp"

//#define TRACE_OUTPUT

namespace
{
  const char USAGE[] =
    "\nUsage: \n"
    "DTreeTrainer [train|train-add|train-trees|print|predict|ensemble]\n";

  class Callback:
    public Gears::ActiveObjectCallback
  {
  public:
    void report_error(
      Gears::ActiveObjectCallback::Severity severity,
      const Gears::SubString& description,
      const Gears::SubString& error_code = Gears::SubString())
      throw()
    {
      try
      {
        std::cerr << severity << "(" << error_code << "): " <<
          description << std::endl;
      }
      catch (...) {}
    }
  };

  typedef LogRegPredictor<DTree> LogRegDTreePredictor;
  typedef Gears::IntrusivePtr<LogRegDTreePredictor> LogRegDTreePredictor_var;
}

// Application
Application_::Application_()
  throw()
{}

Application_::~Application_() throw()
{}

void
Application_::ensemble_(
  double& min_logloss,
  double& logloss1,
  double& logloss2,
  double& coef1,
  double& coef2,
  DTree* predictor1,
  DTree* predictor2,
  const SVMImpl* svm)
{
  const int div = 100;
  min_logloss = 1000000.0;
  double min_coef1 = 0;
  double min_coef2 = 0;

  for(int i = 0; i <= div; ++i)
  {
    double local_coef1 = static_cast<double>(i) / div;
    double local_coef2 = 1.0 - local_coef1;
    PredictorSet_var predictor_union = new PredictorSet();
    predictor_union->add(
      LogRegDTreePredictor_var(new LogRegPredictor<DTree>(predictor1)).in(),
      local_coef1);
    predictor_union->add(
      LogRegDTreePredictor_var(new LogRegPredictor<DTree>(predictor2)).in(),
      local_coef2);
    double local_logloss = Utils::logloss(predictor_union.in(), svm);
    if(local_logloss < min_logloss)
    {
      min_logloss = local_logloss;
      min_coef1 = local_coef1;
      min_coef2 = local_coef2;
    }

    if(i == 0)
    {
      logloss1 = local_logloss;
    }
    else if(i == div)
    {
      logloss2 = local_logloss;
    }

    //std::cerr << "c1 = " << local_coef1 << ", c2 = " << local_coef2 << ": " << local_logloss << std::endl;
  }

  coef1 = min_coef1;
  coef2 = min_coef2;
}

void
Application_::deep_print_(
  std::ostream& ostr,
  DTree* dtree,
  const char* prefix,
  const FeatureDictionary* dict,
  double base,
  const SVMImpl* svm,
  const FeatureNameDictionary* name_dict)
  const throw()
{
  Gears::IntrusivePtr<MetricPrinter<PredictedBoolLabel> > ll_metric_printer(
    new EvalMetricPrinter<PredictedBoolLabel, LogLossMetricEvaluator>("ll = ", true));
  Gears::IntrusivePtr<MetricPrinter<PredictedBoolLabel> > rsqr_metric_printer(
    new EvalMetricPrinter<PredictedBoolLabel, RSquareMetricEvaluator>("r-sqr = "));
  Gears::IntrusivePtr<MetricPrinter<PredictedBoolLabel> > aev_metric_printer(
    new EvalMetricPrinter<PredictedBoolLabel, ActExpectedValueMetricEvaluator>("aev = "));
  Gears::IntrusivePtr<MetricPrinter<PredictedBoolLabel> > pred_metric_printer(
    new EvalMetricPrinter<PredictedBoolLabel, PredExpectedValueMetricEvaluator>("pev = "));
  Gears::IntrusivePtr<MetricPrinter<PredictedBoolLabel> > disc_metric_printer(
    new EvalMetricPrinter<PredictedBoolLabel, ActDiscrepancyMetricEvaluator>("d = "));

  //Gears::IntrusivePtr<MetricPrinter<PredictedBoolLabel> > actual_absloss_metric_printer(
  //  new EvalMetricPrinter<PredictedBoolLabel, ActAbsLossMetricEvaluator>("aal = "));
  Gears::IntrusivePtr<MetricPrinter<PredictedBoolLabel> > pred_absloss_metric_printer(
    new EvalMetricPrinter<PredictedBoolLabel, PredAbsLossMetricEvaluator>("pal = "));

  //Gears::IntrusivePtr<MetricPrinter<PredictedBoolLabel> > lc_metric_printer(
  //  new EvalMetricPrinter<PredictedBoolLabel, LabelCountMetricEvaluator>("lc = "));

  Gears::IntrusivePtr<MetricPrinter<PredictedBoolLabel> > metric_printer(
    new ComposeMetricPrinter<PredictedBoolLabel>(
      ll_metric_printer,
      disc_metric_printer,
      aev_metric_printer,
      pred_metric_printer,
      //actual_absloss_metric_printer,
      pred_absloss_metric_printer));

  LogRegDTreePredictor_var log_reg_predictor = new LogRegPredictor<DTree>(dtree);
  SVMImpl_var pred_svm;

  if(svm)
  {
    pred_svm = svm->copy(
      PredictedBoolLabelLogRegAddConverter<LogRegDTreePredictor>(log_reg_predictor));
  }

  ostr << dtree->to_string_ext(
    (std::string(prefix) + "  ").c_str(),
    dict,
    base,
    pred_svm.in(),
    metric_printer.in(),
    name_dict);
  ostr << std::endl;
}

void
Application_::main(int& argc, char** argv)
  /*throw(Gears::Exception)*/
{
  Gears::AppUtils::CheckOption opt_help;
  Gears::AppUtils::Option<unsigned long> opt_max_features(10);
  Gears::AppUtils::Option<unsigned long> opt_max_top_element(10);
  Gears::AppUtils::Option<unsigned long> opt_depth(5);
  Gears::AppUtils::Option<unsigned long> opt_step_depth(1);
  Gears::AppUtils::Option<unsigned long> opt_check_depth(1);
  Gears::AppUtils::Option<unsigned long> opt_iterations(10);
  Gears::AppUtils::Option<unsigned long> opt_global_iterations(10);
  Gears::AppUtils::Option<unsigned long> opt_super_iterations(10);
  Gears::AppUtils::Option<unsigned long> opt_num_trees(10);
  Gears::AppUtils::Option<unsigned long> opt_threads(1);
  Gears::AppUtils::Option<unsigned long> opt_train_bags_number(10);
  Gears::AppUtils::Option<unsigned long> opt_test_bags_number(3);
  //Gears::AppUtils::Option<double> opt_add_tree_coef(1.0);
  Gears::AppUtils::Option<double> opt_alpha_coef(1.0);
  Gears::AppUtils::StringOption opt_feature_dictionary;
  Gears::AppUtils::StringOption opt_feature_name_dictionary;
  Gears::AppUtils::StringOption opt_step_model_out;
  Gears::AppUtils::StringOption opt_filter_features;
  //Gears::AppUtils::CheckOption opt_out_of_bag_validate;
  Gears::AppUtils::StringOption opt_train_strategy;
  Gears::AppUtils::CheckOption opt_anneal;
  Gears::AppUtils::CheckOption opt_allow_negative_gain;
  Gears::AppUtils::Option<unsigned long> opt_gain_check_bags_number(0);
  Gears::AppUtils::Option<double> opt_min_cover(0.0001);
  Gears::AppUtils::StringOption opt_save_best_model_file;

  Gears::AppUtils::Option<unsigned long> opt_square_diviation_choose_prob(1);
  Gears::AppUtils::Option<unsigned long> opt_logloss_choose_prob(1);

  Gears::AppUtils::Option<std::string> opt_train_pred_x;
  Gears::AppUtils::Option<std::string> opt_test_pred_x;

  // RAM ~ O(opt_max_top_element ^ opt_depth)
  // RAM ~ 100000 by default
  //
  Gears::AppUtils::Args args(-1);

  args.add(
    Gears::AppUtils::equal_name("help") ||
    Gears::AppUtils::short_name("h"),
    opt_help);
  args.add(
    Gears::AppUtils::equal_name("max-top-element") ||
    Gears::AppUtils::short_name("me"),
    opt_max_top_element);
  args.add(
    Gears::AppUtils::equal_name("depth") ||
    Gears::AppUtils::short_name("d"),
    opt_depth);
  args.add(
    Gears::AppUtils::equal_name("step-depth") ||
    Gears::AppUtils::short_name("sd"),
    opt_step_depth);
  args.add(
    Gears::AppUtils::equal_name("check-depth") ||
    Gears::AppUtils::short_name("cd"),
    opt_check_depth);
  args.add(
    Gears::AppUtils::equal_name("steps") ||
    Gears::AppUtils::short_name("n"),
    opt_iterations);
  args.add(
    Gears::AppUtils::equal_name("gsteps") ||
    Gears::AppUtils::short_name("gn"),
    opt_global_iterations);
  args.add(
    Gears::AppUtils::equal_name("ssteps") ||
    Gears::AppUtils::short_name("sn"),
    opt_super_iterations);
  args.add(
    Gears::AppUtils::equal_name("trees") ||
    Gears::AppUtils::short_name("nt"),
    opt_num_trees);
  args.add(
    Gears::AppUtils::equal_name("dict"),
    opt_feature_dictionary);
  args.add(
    Gears::AppUtils::equal_name("name-dict"),
    opt_feature_name_dictionary);
  args.add(
    Gears::AppUtils::equal_name("threads") ||
    Gears::AppUtils::short_name("t"),
    opt_threads);
  args.add(
    Gears::AppUtils::equal_name("train-bags"),
    opt_train_bags_number);
  args.add(
    Gears::AppUtils::equal_name("gain-check-bags"),
    opt_gain_check_bags_number);
  args.add(
    Gears::AppUtils::equal_name("test-bags"),
    opt_test_bags_number);
  /*
  args.add(
    Gears::AppUtils::equal_name("out-of-bag"),
    opt_out_of_bag_validate);
  */
  args.add(
    Gears::AppUtils::equal_name("step-model-out"),
    opt_step_model_out);
  args.add(
    Gears::AppUtils::equal_name("train-strategy") ||
    Gears::AppUtils::short_name("ts"),
    opt_train_strategy);
  /*
  args.add(
    Gears::AppUtils::equal_name("add-coef"),
    opt_add_tree_coef);
  */
  args.add(
    Gears::AppUtils::equal_name("filter-features"),
    opt_filter_features);
  args.add(
    Gears::AppUtils::equal_name("alpha"),
    opt_alpha_coef);
  args.add(
    Gears::AppUtils::equal_name("anneal"),
    opt_anneal);
  args.add(
    Gears::AppUtils::equal_name("negative"),
    opt_allow_negative_gain);
  args.add(
    Gears::AppUtils::equal_name("min-cover"),
    opt_min_cover);
  args.add(
    Gears::AppUtils::equal_name("save-best-model"),
    opt_save_best_model_file);

  args.add(
    Gears::AppUtils::equal_name("metric-sqd"),
    opt_square_diviation_choose_prob);
  args.add(
    Gears::AppUtils::equal_name("metric-logloss"),
    opt_logloss_choose_prob);

  args.parse(argc - 1, argv + 1);

  const Gears::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.empty() || opt_help.enabled() ||
     *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }

  auto command_it = commands.begin();
  std::string command = *command_it;
  ++command_it;

  std::unordered_set<unsigned long> filter_features;

  if(!opt_filter_features->empty())
  {
    Gears::CategoryRepeatableTokenizer<
      Gears::Ascii::SepComma> tokenizer(*opt_filter_features);
    Gears::SubString token;
    while(tokenizer.get_token(token))
    {
      uint32_t val;
      if(!Gears::StringManip::str_to_int(token, val))
      {
        Gears::ErrorStream ostr;
        ostr << "invalid list feature value '" <<
          *opt_filter_features << "'";
        throw Exception(ostr.str());
      }
      filter_features.insert(val);
    }
  }

  if(command == "train" || command == "train-add" || command == "train-trees")
  {
    if(command_it == commands.end())
    {
      std::cerr << "result file not defined" << std::endl;
      return;
    }

    const std::string result_file_path = *command_it;

    ++command_it;
    if(command_it == commands.end())
    {
      std::cerr << "train file not defined" << std::endl;
      return;
    }

    const std::string train_file_path = *command_it;

    std::ifstream train_file(train_file_path.c_str());
    if(!train_file.is_open())
    {
      std::cerr << "can't open train file: " << train_file_path << std::endl;
      return;
    }

    SVMImpl_var train_svm = SVMImpl::load(train_file);

    if(!filter_features.empty())
    {
      train_svm = train_svm->filter(filter_features);
    }

    // load test files
    std::list<SVMImpl_var> test_svms;

    for(++command_it; command_it != commands.end(); ++command_it)
    {
      const std::string test_file_path = *command_it;
      std::ifstream test_file(test_file_path.c_str());
      if(!test_file.is_open())
      {
        std::cerr << "can't open test file: " << test_file_path << std::endl;
        return;
      }

      SVMImpl_var test_svm = SVMImpl::load(test_file);

      if(!filter_features.empty())
      {
        test_svm = test_svm->filter(filter_features);
      }

      test_svms.push_back(test_svm);
    }

    if(command == "train-trees")
    {
      DTree_var tree;

      if(Gears::FileManip::file_exists(result_file_path))
      {
        std::ifstream result_file(result_file_path.c_str());
        if(result_file.fail())
        {
          Gears::ErrorStream ostr;
          ostr << "can't open model file '" << result_file_path << "'" << std::endl;
          throw Exception(ostr.str());
        }

        tree = DTree::load(result_file);

        if(!tree)
        {
          Gears::ErrorStream ostr;
          ostr << "incorrect file, it contains non dtree model";
          throw Exception(ostr.str());
        }

        const double train_logloss = Utils::log_reg_logloss(tree.in(), train_svm.in());
        const double abs_train_logloss = Utils::log_reg_absloss(tree.in(), train_svm.in());

        std::ostringstream test_ostr;
        unsigned long test_svm_i = 0;
        for(auto test_svm_it = test_svms.begin(); test_svm_it != test_svms.end();
          ++test_svm_it, ++test_svm_i)
        {
          const double ext_test_logloss = Utils::log_reg_logloss(tree.in(), test_svm_it->in());
          const double abs_ext_test_logloss = Utils::log_reg_absloss(tree.in(), test_svm_it->in());
          test_ostr <<
            ", etest-" << test_svm_i << "-ll(" << (*test_svm_it)->size() << "," <<
              (*test_svm_it)->grouped_rows.size() << ") = " << ext_test_logloss <<
            ", abs-etest-" << test_svm_i << "-ll = " << abs_ext_test_logloss;
        }

        std::cout << "loaded model: "
          "train-ll(" << train_svm->size() << "," << train_svm->grouped_rows.size() << ") = " << train_logloss <<
          ", abs-train-ll = " << abs_train_logloss <<
          test_ostr.str() <<
          ", nodes count = " << tree->node_count() <<
          "(" << Gears::Time::get_time_of_day().gm_ft() << ")" <<
          std::endl <<
          tree->to_string("  ") << std::endl;
      }
      else
      {
        std::cout << "start train (" << Gears::Time::get_time_of_day().gm_ft() << ")" <<
          std::endl;
      }

      MetricSelection metric_selection;
      metric_selection.sqd = *opt_square_diviation_choose_prob;
      metric_selection.logloss = *opt_logloss_choose_prob;

      DTree_var best_tree;
      DTree_var new_tree;
      double best_test0_logloss;

      train_dtree_set_(
        best_test0_logloss,
        best_tree,
        new_tree,
        tree,
        train_svm,
        test_svms,
        *opt_iterations,
        *opt_step_depth,
        *opt_check_depth,
        *opt_alpha_coef,
        *opt_train_bags_number,
        *opt_gain_check_bags_number,
        opt_step_model_out->c_str(),
        *opt_threads,
        opt_anneal.enabled(),
        opt_allow_negative_gain.enabled(),
        metric_selection
        );

      std::ofstream result_file(result_file_path.c_str());
      new_tree->save(result_file);

      if(!opt_save_best_model_file->empty() && best_tree.in())
      {
        std::ofstream best_result_file(opt_save_best_model_file->c_str());
        best_tree->save(best_result_file);
      }

      std::cout << "LOGLOSS: " << best_test0_logloss << std::endl;
    }
    else
    {
      assert(0);
    }
  }
  else if(command == "print")
  {
    if(command_it == commands.end())
    {
      std::cerr << "result file not defined" << std::endl;
      return;
    }

    const std::string result_file_path = *command_it;

    ++command_it;

    SVMImpl_var cover_svm;

    if(command_it != commands.end())
    {
      // cover file
      std::ifstream cover_file(command_it->c_str());
      cover_svm = SVMImpl::load(cover_file);
    }

    FeatureDictionary feature_dictionary;
    FeatureNameDictionary feature_name_dictionary;
    if(!opt_feature_dictionary->empty())
    {
      // load feature dictionary
      load_dictionary_(feature_dictionary, opt_feature_dictionary->c_str());
    }

    if(!opt_feature_name_dictionary->empty())
    {
      // load feature dictionary
      load_name_dictionary_(feature_name_dictionary, opt_feature_name_dictionary->c_str());
    }

    std::ifstream result_file(result_file_path.c_str());
    if(!result_file.is_open())
    {
      Gears::ErrorStream ostr;
      ostr << "can't open '" << result_file_path << "'";
      throw Exception(ostr.str());
    }

    DTree_var loaded_predictor = DTree::load(result_file);

    deep_print_(
      std::cout,
      loaded_predictor,
      "",
      &feature_dictionary,
      0.0,
      cover_svm,
      &feature_name_dictionary);

    /*
    std::cout << predictor_set->to_string_ext(
      "",
      &feature_dictionary,
      0.0,
      cover_svm);
    */
  }
  else if(command == "predict" || command == "predict-perf")
  {
    if(command_it == commands.end())
    {
      std::cerr << "result file not defined" << std::endl;
      return;
    }

    const std::string result_file_path = *command_it;

    ++command_it;
    if(command_it == commands.end())
    {
      std::cerr << "svm file not defined" << std::endl;
      return;
    }

    const std::string svm_file_path = *command_it;

    std::ifstream result_file(result_file_path.c_str());

    std::ifstream svm_file(svm_file_path.c_str());

    std::cout.setf(std::ios::fixed, std::ios::floatfield);
    std::cout.precision(7);

    if(command == "predict")
    {
      DTree_var tree = DTree::load(result_file);
      Gears::IntrusivePtr<LogRegPredictor<DTree> > predictor =
        new LogRegPredictor<DTree>(tree);

      FastFeatureSet fast_feature_set;

      while(!svm_file.eof())
      {
        PredictedBoolLabel label_value;
        Row_var row = SVMImpl::load_line(svm_file, label_value);
        if(!row)
        {
          break;
        }

        double predicted_value = predictor->fpredict(row->features);
        /*
        fast_feature_set.set(row->features);
        double predicted_value = predictor->fpredict(fast_feature_set);
        fast_feature_set.rollback(row->features);
        */

        std::cout << predicted_value << std::endl;
      }
    }
    else // predict-perf
    {
      DTree_var tree = DTree::load(result_file);
      //predictor = new LogRegPredictor(predictor);
      LogRegDTreePredictor_var predictor = new LogRegDTreePredictor(tree);

      std::deque<Row_var> rows;
      while(!svm_file.eof())
      {
        PredictedBoolLabel label_value;
        Row_var row = SVMImpl::load_line(svm_file, label_value);
        if(!row)
        {
          break;
        }
        rows.push_back(row);
      }

      FastFeatureSet fast_feature_set;

      Gears::Time start_time = Gears::Time::get_time_of_day();

      for(auto row_it = rows.begin(); row_it != rows.end(); ++row_it)
      {
        /*
        std::copy((*row_it)->features.begin(),
          (*row_it)->features.end(),
          std::back_inserter(feature_array));
        std::sort(feature_array.begin(), feature_array.end());
        predictor->fpredict(feature_array);
        feature_array.clear();
        */

        fast_feature_set.set((*row_it)->features);
        //tree->fpredict(fast_feature_set);
        predictor->fpredict(fast_feature_set);
        fast_feature_set.rollback((*row_it)->features);
      }

      Gears::Time end_time = Gears::Time::get_time_of_day();
      std::cout << (end_time - start_time) << std::endl;
    }
  }
  else if(command == "ensemble")
  {
    if(command_it == commands.end())
    {
      std::cerr << "predictor file(#1) not defined" << std::endl;
      return;
    }

    const std::string predictor1_file_path = *command_it;

    ++command_it;
    if(command_it == commands.end())
    {
      std::cerr << "predictor file(#2) not defined" << std::endl;
      return;
    }

    const std::string predictor2_file_path = *command_it;

    ++command_it;
    if(command_it == commands.end())
    {
      std::cerr << "svm file not defined" << std::endl;
      return;
    }

    const std::string svm_file_path = *command_it;

    std::ifstream predictor1_file(predictor1_file_path.c_str());
    DTree_var tree1 = DTree::load(predictor1_file);

    std::ifstream predictor2_file(predictor2_file_path.c_str());
    DTree_var tree2 = DTree::load(predictor2_file);

    std::ifstream svm_file(svm_file_path.c_str());
    SVMImpl_var svm = SVMImpl::load(svm_file);

    double coef1;
    double coef2;
    double min_logloss;
    double logloss1;
    double logloss2;
    ensemble_(min_logloss, logloss1, logloss2, coef1, coef2, tree1, tree2, svm);

    std::cout.setf(std::ios::fixed, std::ios::floatfield);
    std::cout.precision(7);

    std::cout << "coef1 = " << coef1 << std::endl <<
      "coef2 = " << coef2 << std::endl <<
      "logloss1 = " << logloss1 << std::endl <<
      "logloss2 = " << logloss1 << std::endl <<
      "min_logloss = " << min_logloss << std::endl
      ;
  }
  else if(command == "filter")
  {
    if(command_it == commands.end())
    {
      std::cerr << "result file not defined" << std::endl;
      return;
    }

    const std::string result_file_path = *command_it;

    ++command_it;

    SVMImpl_var cover_svm;

    if(command_it != commands.end())
    {
      // cover file
      std::ifstream cover_file(command_it->c_str());
      cover_svm = SVMImpl::load(cover_file);
    }

    DTree_var tree;

    if(Gears::FileManip::file_exists(result_file_path))
    {
      {
        std::ifstream result_file(result_file_path.c_str());
        if(result_file.fail())
        {
          Gears::ErrorStream ostr;
          ostr << "can't open model file '" << result_file_path << "'" << std::endl;
          throw Exception(ostr.str());
        }

        tree = DTree::load(result_file);
      }

      if(!tree)
      {
        Gears::ErrorStream ostr;
        ostr << "incorrect file, it contains non dtree model";
        throw Exception(ostr.str());
      }

      DTree_var modified_dtree =
        tree->filter(*opt_min_cover, cover_svm.in());

      std::ofstream result_file(result_file_path.c_str());
      modified_dtree->save(result_file);

      std::cout << "logloss = " << eval_reg_logloss_(tree, cover_svm.in()) << std::endl <<
        "res logloss = " << eval_reg_logloss_(modified_dtree, cover_svm.in()) << std::endl;
    }
  }
  else
  {
    Gears::ErrorStream ostr;
    ostr << "unknown command '" << command << "', "
      "see help for more info" << std::endl;
    throw Exception(ostr.str());
  }
}

void
Application_::load_dictionary_(
  Vanga::FeatureDictionary& dict,
  const char* file)
{
  std::ifstream in(file);
  if(!in.is_open())
  {
    Gears::ErrorStream ostr;
    ostr << "can't open '" << file << "'";
    throw Exception(ostr.str());
  }

  std::vector<std::string> values;

  while(!in.eof())
  {
    // parse line
    std::string line;
    std::getline(in, line);
    if(line.empty())
    {
      continue;
    }

    values.resize(0);
    Gears::Csv::parse_line(values, line);

    if(values.size() != 2)
    {
      Gears::ErrorStream ostr;
      ostr << "invalid dictionary line '" << line << "'";
      throw Exception(ostr.str());
    }

    unsigned long feature_id;
    if(!Gears::StringManip::str_to_int(values[0], feature_id))
    {
      Gears::ErrorStream ostr;
      ostr << "invalid feature value '" << values[0] << "'";
      throw Exception(ostr.str());
    }
    dict[feature_id] = values[1];
  }
}

void
Application_::load_name_dictionary_(
  Vanga::FeatureNameDictionary& dict,
  const char* file)
{
  std::ifstream in(file);
  if(!in.is_open())
  {
    Gears::ErrorStream ostr;
    ostr << "can't open '" << file << "'";
    throw Exception(ostr.str());
  }

  std::vector<std::string> values;

  while(!in.eof())
  {
    // parse line
    std::string line;
    std::getline(in, line);
    if(line.empty())
    {
      continue;
    }

    values.resize(0);
    Gears::Csv::parse_line(values, line);

    if(values.size() != 2)
    {
      Gears::ErrorStream ostr;
      ostr << "invalid dictionary line '" << line << "'";
      throw Exception(ostr.str());
    }

    dict[values[0]] = values[1];
  }
}

void
Application_::init_bags_(
  SVMImplArray& bags,
  SVMImpl_var& test_svm,
  SVMImpl_var& train_svm,
  SVMImpl* ext_train_svm,
  unsigned long train_bags)
{
  static const Fold FOLDS[] = {
    //{ 1.0, 10 },
    //{ 0.9, 5 },
    // annealing
    //{ 0.8, 5 },
    { 0.5, 2 },
    { 0.3, 2 }
    //{ 0.1, 1 }
  };

  (void)FOLDS;

  //static const Fold FOLDS[] = { { 0.0, 3 }, { 1.0, 3 }, { 0.9, 10 }, { 0.5, 10 }, { 0.3, 10 }, { 0.1, 10 }, { 0.05, 10 } };

  /*
  if(out_of_bag_validate)
  {
    std::pair<SVMImpl_var, SVMImpl_var> sets = ext_train_svm->div(ext_train_svm->size() * 9 / 10);
    train_svm = sets.first;
    test_svm = sets.second;
  }
  else
  {
    train_svm = Gears::add_ref(ext_train_svm);
    test_svm = Gears::add_ref(ext_test_svm);
  }
  */

  train_svm = Gears::add_ref(ext_train_svm);
  test_svm = SVMImpl_var();

  bags.clear();

  if(train_bags > 0)
  {
    div_bags_(bags, train_bags, train_svm);
    //std::cerr << "bags: " << bags.size() << std::endl;
    //fill_bags_(bags, FOLDS, train_bags, train_svm);
  }
  else
  {
    bags.push_back(train_svm->copy());
  }
}

void
Application_::prepare_bags_(
  TreeLearner<PredictedBoolLabel>::Context_var& context,
  SVMImplArray& bags,
  DTree* predictor,
  bool anneal)
{
  std::cout << "to prepare bags" << std::endl;

  for(auto bag_it = bags.begin(); bag_it != bags.end(); ++bag_it)
  {
    *bag_it = (*bag_it)->copy(PredictedBoolLabelAddConverter<DTree>(predictor));

    if(anneal)
    {
      *bag_it = (*bag_it)->copy(PredictedBoolLabelAnnealer());
    }
  }

  context = TreeLearner<PredictedBoolLabel>::create_context(bags);

  /*
  std::cout << "bags prepared (" << bags.size() << "): ";
  for(auto bag_it = bags.begin(); bag_it != bags.end(); ++bag_it)
  {
    std::cout << (bag_it != bags.begin() ? ", " : "") << (*bag_it)->size();
  }
  std::cout << std::endl;

  for(auto bag_it = bags.begin(); bag_it != bags.end(); ++bag_it)
  {
    std::cout << "BAG #" << (bag_it - bags.begin()) << std::endl;
    (*bag_it)->print_labels(std::cout);
  }
  */
}

struct TreeModify
{
  TreeModify()
    : index(-2),
      logloss(100000.0),
      gain(-100000)
  {}

  TreeModify(
    int index_val,
    DTree* new_dtree_val,
    double logloss_val,
    double gain_val)
    : index(index_val),
      new_dtree(Gears::add_ref(new_dtree_val)),
      logloss(logloss_val),
      gain(gain_val)
  {}

  int index;
  DTree_var new_dtree;
  double logloss;
  double gain;
};

void
apply_dtree_set_mod(
  std::vector<DTree_var>& cur_dtrees,
  const TreeModify& tree_modify)
{
  if(tree_modify.index < 0)
  {
    cur_dtrees.push_back(tree_modify.new_dtree);
  }
  else
  {
    cur_dtrees[tree_modify.index] = tree_modify.new_dtree;
  }
}

double
Application_::eval_reg_logloss_(
  DTree* predictor,
  const SVMImpl* svm)
{
  LogRegDTreePredictor_var log_reg_predictor = new LogRegPredictor<DTree>(predictor);
  return Utils::logloss(log_reg_predictor.in(), svm);
}

void
Application_::train_dtree_set_(
  double& best_test0_logloss,
  DTree_var& best_dtree,
  DTree_var& new_dtree,
  const DTree* prev_dtree,
  SVMImpl* ext_train_svm,
  const std::list<SVMImpl_var>& ext_test_svms,
  unsigned long max_global_iterations,
  unsigned long step_depth,
  unsigned long check_depth,
  double alpha_coef,
  unsigned long train_bags,
  unsigned long gain_check_bags,
  const char* opt_step_model_out,
  unsigned long threads,
  bool anneal,
  bool allow_negative_gain,
  const MetricSelection& metric_selection)
  throw()
{
  //const bool USE_NEGATIVE_GAIN = true;

  Gears::ActiveObjectCallback_var callback(new Callback());

  Gears::TaskRunner_var task_runner = threads > 1 ?
    new Gears::TaskRunner(
      callback,
      threads,
      10*1024*1024 // stack size
      ) :
    0;

  if(task_runner)
  {
    task_runner->activate_object();
  }

  // prepare bags
  TreeLearner<PredictedBoolLabel>::Context_var context;
  SVMImplArray bags;
  SVMImpl_var test_svm;
  SVMImpl_var train_svm;

  // train
  //double prev_logloss = 1000000.0;
  DTree_var cur_dtree = prev_dtree ? prev_dtree->copy() : DTree_var();
  best_test0_logloss = 1000000.0;
  best_dtree = cur_dtree ? cur_dtree->copy() : DTree_var();

  {
    // print init state
    DTree_var tree = cur_dtree ? cur_dtree : DTree_var(new DTree());

    std::ostringstream test_ostr;
    unsigned long test_svm_i = 0;
    for(auto test_svm_it = ext_test_svms.begin(); test_svm_it != ext_test_svms.end();
      ++test_svm_it, ++test_svm_i)
    {
      const double ext_test_logloss = Utils::log_reg_logloss(tree.in(), test_svm_it->in());
      const double abs_ext_test_logloss = Utils::log_reg_absloss(tree.in(), test_svm_it->in());
      test_ostr <<
        ", etest-" << test_svm_i << "-ll(" << (*test_svm_it)->size() << "," <<
          (*test_svm_it)->grouped_rows.size() << ") = " << ext_test_logloss <<
        ", abs-etest-" << test_svm_i << "-ll = " << abs_ext_test_logloss;
    }

    const double train_logloss = Utils::log_reg_logloss(tree.in(), ext_train_svm);

    std::cout << "[INIT]: "
      "train-ll(" << ext_train_svm->size() << "," << ext_train_svm->grouped_rows.size() << ") = " << train_logloss <<
        test_ostr.str() <<
      "(" << Gears::Time::get_time_of_day().gm_ft() << ")" <<
      std::endl;
  }
  
  for(unsigned long gi = 0; gi < max_global_iterations; ++gi)
  {
    bags.clear();

    SVMImpl_var add_test_svm;

    init_bags_(
      bags,
      add_test_svm,
      train_svm,
      ext_train_svm,
      train_bags);

    //double base_test_logloss = eval_reg_logloss_(cur_dtree, test_svm);
    prepare_bags_(context, bags, cur_dtree, anneal);

    // try extend existing trees
    // TODO: SVM for node
    std::vector<TreeModify> modifications;

    DTree_var modified_dtree = cur_dtree ? cur_dtree->copy() : DTree_var();

    //const double cur_logloss = ;
    std::string selected_metric;

    train_on_bags_(
      modified_dtree,
      selected_metric,
      context,
      train_svm,
      ext_test_svms,
      gain_check_bags,
      1, // max_iterations
      step_depth,
      check_depth,
      alpha_coef,
      false,
      allow_negative_gain,
      task_runner,
      metric_selection);

    //const double gain = base_test_logloss - cur_logloss;
    cur_dtree = modified_dtree;

    //std::cout << "Modify: ll = " << cur_logloss << ", gain = " << gain << std::endl;

    //double test_logloss;

    {
      // eval step result
      if(opt_step_model_out[0])
      {
        std::ostringstream step_file_name_ostr;
        step_file_name_ostr << opt_step_model_out << gi << ".dtf";
        std::ofstream step_file(step_file_name_ostr.str());
        cur_dtree->save(step_file);
      }

      bool optimized = false;
      std::ostringstream test_ostr;
      unsigned long test_svm_i = 0;
      for(auto test_svm_it = ext_test_svms.begin(); test_svm_it != ext_test_svms.end();
        ++test_svm_it, ++test_svm_i)
      {
        const double ext_test_logloss = Utils::log_reg_logloss(cur_dtree.in(), test_svm_it->in());
        const double abs_ext_test_logloss = Utils::log_reg_absloss(cur_dtree.in(), test_svm_it->in());
        test_ostr <<
          ", etest-" << test_svm_i << "-ll(" << (*test_svm_it)->size() << ") = " << ext_test_logloss <<
          ", abs-etest-" << test_svm_i << "-ll = " << abs_ext_test_logloss;

        if(test_svm_i == 0)
        {
          if(ext_test_logloss < best_test0_logloss)
          {
            best_dtree = cur_dtree->copy();
            best_test0_logloss = ext_test_logloss;
            optimized = true;
          }
        }
      }

      const double train_logloss = Utils::log_reg_logloss(cur_dtree.in(), train_svm.in());

      std::cout << "[" << gi << "]: " <<
        "train-ll(" << train_svm->size() << "," << train_svm->grouped_rows.size() << ") = " << train_logloss <<
        test_ostr.str() <<
        ", nodes count = " << cur_dtree->node_count() <<
        ", metric = " << selected_metric <<
        " (" << Gears::Time::get_time_of_day().gm_ft() << ")" << (optimized ? " *" : "") <<
        std::endl <<
        cur_dtree->to_string("  ") << std::endl;
    }

    //prev_logloss = test_logloss;
  }

  if(task_runner)
  {
    task_runner->deactivate_object();
    task_runner->wait_object();
  }

  new_dtree = cur_dtree;
}

double
Application_::train_on_bags_(
  DTree_var& res_tree,
  std::string& selected_metric,
  TreeLearner<PredictedBoolLabel>::Context* ext_context,
  SVMImpl* train_svm,
  const std::list<SVMImpl_var>& test_svms,
  unsigned long gain_check_bags,
  unsigned long max_iterations,
  unsigned long step_depth,
  unsigned long check_depth,
  double alpha_coef,
  bool print_trace,
  bool allow_negative_gain,
  Gears::TaskRunner* task_runner,
  const MetricSelection& metric_selection)
{
  TreeLearner<PredictedBoolLabel>::Context_var context =
    Gears::add_ref(ext_context);

  TreeLearner<PredictedBoolLabel>::LearnContext_var learn_context =
    context->create_learner(res_tree, task_runner);

  DTree_var cur_dtree = res_tree ? res_tree->copy() : DTree_var(); // = res_tree;

  std::cout.setf(std::ios::fixed, std::ios::floatfield);
  std::cout.precision(7);

  unsigned long best_iter_i = 0;
  double best_test_logloss = 1000000.0;
  DTree_var best_dtree;

  //train_svm->dump();
  //std::cout << "==============" << std::endl;

  // select metric
  unsigned long metric_i = Gears::safe_rand(
    metric_selection.sqd + metric_selection.logloss);

  if(metric_i < metric_selection.sqd)
  {
    selected_metric = "sq";
  }
  else
  {
    selected_metric = "ll";
  }

  for(unsigned long iter_i = 0; iter_i < max_iterations; ++iter_i)
  {
    if(metric_i < metric_selection.sqd)
    {
      cur_dtree = learn_context->train<PredictedSquareDiviationGain>(
        step_depth,
        check_depth,
        alpha_coef,
        TreeLearner<PredictedBoolLabel>::LearnContext::FSS_BEST,
        allow_negative_gain,
        gain_check_bags);
    }
    else
    {
      cur_dtree = learn_context->train<PredictedLogLossGain>(
        step_depth,
        check_depth,
        alpha_coef,
        TreeLearner<PredictedBoolLabel>::LearnContext::FSS_BEST,
        allow_negative_gain,
        gain_check_bags);
    }

    std::vector<DTree_var> prev_dtrees;

    const double train_logloss = Utils::log_reg_logloss(res_tree.in(), train_svm);
    double primary_test_logloss = 0.0;

    std::ostringstream test_ostr;
    unsigned long test_svm_i = 0;
    for(auto test_svm_it = test_svms.begin(); test_svm_it != test_svms.end();
      ++test_svm_it, ++test_svm_i)
    {
      const double ext_test_logloss = Utils::log_reg_logloss(res_tree.in(), test_svm_it->in());
      const double abs_ext_test_logloss = Utils::log_reg_absloss(res_tree.in(), test_svm_it->in());
      test_ostr <<
        ", etest-" << test_svm_i << "-ll(" << (*test_svm_it)->size() << "," <<
          (*test_svm_it)->grouped_rows.size() << ") = " << ext_test_logloss <<
        ", abs-etest-" << test_svm_i << "-ll = " << abs_ext_test_logloss <<
        ", best-etest-0-ll = " << best_test_logloss <<
        " // " << selected_metric;

      if(test_svm_i == 0)
      {
        primary_test_logloss = ext_test_logloss;
      }
    }

    if(primary_test_logloss < best_test_logloss)
    {
      best_test_logloss = primary_test_logloss;
      best_iter_i = iter_i;
      best_dtree = cur_dtree;
    }

    if(print_trace)
    {
      std::cout << "[" << iter_i << "]: "
        "train-loss = " << train_logloss <<
        test_ostr.str() <<
        std::endl;
    }

    //std::cout << cur_dtree->to_string("", 0) << std::endl;
  }

  (void)best_iter_i;

  if(print_trace)
  {
    std::cout << best_dtree->to_string("", 0) << std::endl;
  }

  res_tree = best_dtree;

  return best_test_logloss;
}

Application_::DTreeProp_var
Application_::fill_dtree_prop_(
  unsigned long step_depth,
  unsigned long best_iteration,
  const SVMImplArray& bags,
  unsigned long train_size)
{
  DTreeProp_var res = new DTreeProp();

  {
    std::ostringstream ss;
    ss << step_depth;
    res->props["sd"] = ss.str();
  }

  {
    std::ostringstream ss;
    ss << best_iteration;
    res->props["iter"] = ss.str();
  }

  {
    std::ostringstream ss;
    for(auto bag_it = bags.begin(); bag_it != bags.end(); ++bag_it)
    {
      if(bag_it != bags.begin())
      {
        ss << ",";
      }
      ss << (static_cast<double>((*bag_it)->size()) / train_size);
    }
    res->props["bags"] = ss.str();
  }
  
  return res;
}

void
Application_::div_bags_(
  SVMImplArray& bags,
  unsigned long bag_number,
  SVMImpl* svm)
{
  svm->portions_div(bags, bag_number);

  std::cout << "bags filled: " << bag_number << std::endl;
}

template<int FOLD_SIZE>
void
Application_::fill_bags_(
  SVMImplArray& bags,
  const Fold (&folds)[FOLD_SIZE],
  unsigned long bag_number,
  SVMImpl* svm)
{
  unsigned long fold_sum = 0;
  for(unsigned long i = 0; i < FOLD_SIZE; ++i)
  {
    fold_sum += folds[i].weight;
  }

  std::cout << "to fill " << bag_number << " bags" << std::endl;

  for(unsigned long bug_i = 0; bug_i < bag_number; ++bug_i)
  {
    // select folds randomly
    unsigned long fold_index = 0;
    unsigned long c_fold = Gears::safe_rand(fold_sum);
    unsigned long cur_fold_sum = 0;

    for(unsigned long i = 0; i < FOLD_SIZE; ++i)
    {
      cur_fold_sum += folds[i].weight;
      if(c_fold < cur_fold_sum)
      {
        fold_index = i;
        break;
      }
    }

    unsigned long part_size = static_cast<unsigned long>(folds[fold_index].portion * svm->size());

    SVMImpl_var part_svm;

    /*
    for(auto row_it = svm->grouped_rows.begin(); row_it != svm->grouped_rows.end(); ++row_it)
    {
      std::cout << "X>> label = " << (*row_it)->label.orig() <<
        ", pred = " << (*row_it)->label.pred <<
        ": " << (*row_it)->rows.size() << std::endl;
    }
    */

    if(part_size > 0)
    {
      part_svm = svm->part(part_size);
    }
    else
    {
      part_svm = Gears::add_ref(svm);
    }

    /*
    for(auto row_it = part_svm->grouped_rows.begin(); row_it != part_svm->grouped_rows.end(); ++row_it)
    {
      std::cout << "P>> label = " << (*row_it)->label.orig() <<
        ", pred = " << (*row_it)->label.pred <<
        ": " << (*row_it)->rows.size() << std::endl;
    }

    std::cout << std::endl;
    */

    bags.push_back(part_svm);
  }

  std::cout << "bags filled" << std::endl;
}

/*
void
Application_::fill_bags_(
  SVMImplArray& bags,
  const FoldArray& folds,
  unsigned long bag_number,
  SVMImpl* svm)
{
  unsigned long fold_sum = 0;
  for(auto fold_it = folds.begin(); fold_it != folds.end(); ++fold_it)
  {
    fold_sum += fold_it->weight;
  }

  for(unsigned long bug_i = 0; bug_i < bag_number; ++bug_i)
  {
    // select folds randomly
    unsigned long fold_index = 0;
    unsigned long c_fold = Gears::safe_rand(fold_sum);
    unsigned long cur_fold_sum = 0;

    for(auto fold_it = folds.begin(); fold_it != folds.end(); ++fold_it)
    {
      cur_fold_sum += fold_it->weight;
      if(c_fold < cur_fold_sum)
      {
        fold_index = fold_it - folds.begin();
        break;
      }
    }

    unsigned long part_size = static_cast<unsigned long>(folds[fold_index].portion * svm->size());

    SVMImpl_var part_svm;

    if(part_size > 0)
    {
      part_svm = svm->part(part_size);
    }
    else
    {
      part_svm = Gears::add_ref(svm);
    }

    bags.push_back(part_svm);
  }
}
*/

/*
void
Application_::construct_best_forest_(
  std::vector<std::pair<DTree_var, DTreeProp_var> >& res_trees,
  const std::vector<std::pair<DTree_var, DTreeProp_var> >& check_trees,
  unsigned long max_trees,
  const SVMImpl* svm)
  throw()
{
  std::vector<PredArrayHolder_var> preds;

  for(auto it = check_trees.begin(); it != check_trees.end(); ++it)
  {
    preds.push_back(it->first->Predictor::predict(svm));
  }

  std::vector<unsigned long> result_indexes;
  select_best_forest_(
    result_indexes,
    preds,
    svm,
    max_trees);

  std::vector<std::pair<DTree_var, DTreeProp_var> > new_trees;
  for(auto tree_index_it = result_indexes.begin();
    tree_index_it != result_indexes.end();
    ++tree_index_it)
  {
    new_trees.push_back(check_trees[*tree_index_it]);
  }

  res_trees.swap(new_trees);
}
*/
 
void
Application_::select_best_forest_(
  std::vector<unsigned long>& indexes,
  const std::vector<PredArrayHolder_var>& preds,
  const SVMImpl* svm,
  unsigned long max_trees)
{
  //std::vector<std::pair<PredArrayHolder_var, PredArrayHolder_var> > loss_preds;
  //TreeLearner::fill_logloss(loss_preds, preds);

  //uint64_t m = 0xFFFFFFFFFFFFFFFF;
  indexes.clear();

  uint64_t maskmax = 0xFFFFFFFFFFFFFFFF >> (64 - preds.size());
  std::vector<PredArrayHolder_var> variant;
  variant.reserve(preds.size());

  uint64_t min_mask = 0;
  double min_logloss = 1000000.0;
  unsigned long min_trees_num = 100;

  PredArrayHolder_var labels = Utils::labels(svm);

  //std::cout << "maxmask=" << maskmax << std::endl;

  for(uint64_t mask = 1; mask <= maskmax; ++mask)
  {
    // fill variant
    variant.clear();

    auto pred_it = preds.begin();
    unsigned long trees_num = 0;
    for(unsigned long i = 0; pred_it != preds.end(); ++i, ++pred_it)
    {
      if((i > 0 ? mask >> i : mask) & 0x1)
      {
        ++trees_num;
        variant.push_back(*pred_it);
      }
    }

    /*
    std::cout << "mask=" << mask <<
      ", trees_num = " << trees_num <<
      ", max_trees = " << max_trees << std::endl;
    */

    if(trees_num >= max_trees)
    {
      continue;
    }

    double avg_logloss = Utils::avg_logloss(variant, labels);
    //std::cout << "mask=" << mask << ", avg_logloss = " << avg_logloss << std::endl;

    if(avg_logloss < min_logloss ||
       (std::abs(avg_logloss - min_logloss) < 0.000001 && trees_num < min_trees_num))
    {
      min_logloss = avg_logloss;
      min_mask = mask;
      min_trees_num = trees_num;
    }
  }

  auto pred_it = preds.begin();
  for(unsigned long i = 0; pred_it != preds.end(); ++i, ++pred_it)
  {
    if((i > 0 ? min_mask >> i : min_mask) & 0x1)
    {
      indexes.push_back(i);
    }
  }
}

// main
int
main(int argc, char** argv)
{
  Application_* app = 0;

  try
  {
    app = &Application::instance();
  }
  catch (...)
  {
    std::cerr << "main(): Critical: Got exception while "
      "creating application object.\n";
    return -1;
  }

  assert(app);

  try
  {
    app->main(argc, argv);
  }
  catch(const Gears::Exception& ex)
  {
    std::cerr << "Caught Gears::Exception: " << ex.what() << std::endl;
    return -1;
  }

  return 0;
}


