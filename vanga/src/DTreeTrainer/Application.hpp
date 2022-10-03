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

#ifndef UTILS_DTREETRAINER_APPLICATION_HPP_
#define UTILS_DTREETRAINER_APPLICATION_HPP_

#include <map>
#include <string>
#include <vector>

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>
#include <Gears/Basic/Singleton.hpp>
#include <Gears/Basic/Time.hpp>

#include <DTree/TreeLearner.hpp>
#include <DTree/Label.hpp>

using namespace Vanga;

class Application_
{
public:
  enum TrainMode
  {
    AT_BEST_BY_GAIN,
    AT_BEST_BY_GAIN_AND_ADD,
    AT_ADD_ONLY,
    AT_NOT_ADD,
    AT_ALL,
    AT_ALL_NOT_ADD
  };

  struct MetricSelection
  {
    unsigned long sqd;
    unsigned long logloss;    
  };

  DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);

  typedef PredictedLogLossGain UseLogLossGain;
  //typedef LogLossGain UseLogLossGain;

  struct Fold
  {
    double portion;
    unsigned long weight;
  };

  typedef std::vector<Fold> FoldArray;

  struct DTreeProp: public Gears::AtomicRefCountable
  {
    typedef std::map<std::string, std::string> PropMap;
    PropMap props;

    std::string
    to_string() const
    {
      std::ostringstream ss;
      for(auto it = props.begin(); it != props.end(); ++it)
      {
        ss << (it != props.begin() ? "," : "") << it->first << "=" << it->second;
      }
      return ss.str();
    }

  protected:
    virtual ~DTreeProp() throw() {}
  };

  typedef Gears::IntrusivePtr<DTreeProp> DTreeProp_var;

  typedef SVM<PredictedBoolLabel> SVMImpl;

  typedef Gears::IntrusivePtr<SVMImpl> SVMImpl_var;

  typedef std::vector<SVMImpl_var> SVMImplArray;

public:
  Application_() throw();

  virtual
  ~Application_() throw();

  void
  main(int& argc, char** argv)
    /*throw(Gears::Exception)*/;

protected:
  /*
   * Set => Train + Test
   *   Set of DTree:
   *     Train => SubTrain + Noise + SubTest
   *     SubTrain => SubTrainBag's
   */
  void
  train_dtree_set_(
    double& best_test0_loss,
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
    throw();

  double
  train_on_bags_(
    DTree_var& res_tree,
    std::string& selected_metric,
    TreeLearner<PredictedBoolLabel>::Context* context,
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
    const MetricSelection& metric_selection);

  void
  init_bags_(
    SVMImplArray& bags,
    SVMImpl_var& test_svm,
    SVMImpl_var& train_svm,
    SVMImpl* ext_train_svm,
    unsigned long train_bags);

  void
  prepare_bags_(
    TreeLearner<PredictedBoolLabel>::Context_var& context,
    SVMImplArray& bags,
    DTree* predictor,
    bool anneal);

  void
  select_best_forest_(
    std::vector<unsigned long>& indexes,
    const std::vector<PredArrayHolder_var>& preds,
    const SVMImpl* svm,
    unsigned long max_trees);

  void
  div_bags_(
    SVMImplArray& bags,
    unsigned long bag_number,
    SVMImpl* svm);

  template<int FOLD_SIZE>
  void
  fill_bags_(
    SVMImplArray& bags,
    const Fold (&folds)[FOLD_SIZE],
    unsigned long bag_number,
    SVMImpl* svm);

  static DTreeProp_var
  fill_dtree_prop_(
    unsigned long step_depth,
    unsigned long best_iteration,
    const SVMImplArray& bags,
    unsigned long train_size);

  void
  load_dictionary_(
    Vanga::FeatureDictionary& dict,
    const char* file);

  void
  load_name_dictionary_(
    Vanga::FeatureNameDictionary& dict,
    const char* file);

  void
  deep_print_(
    std::ostream& ostr,
    DTree* predictor,
    const char* prefix,
    const FeatureDictionary* dict,
    double base,
    const SVMImpl* svm,
    const FeatureNameDictionary* name_dict)
    const throw();

  void
  ensemble_(
    double& min_logloss,
    double& logloss1,
    double& logloss2,
    double& coef1,
    double& coef2,
    DTree* predictor1,
    DTree* predictor2,
    const SVMImpl* svm);

  double
  eval_reg_logloss_(DTree* dtree, const SVMImpl* svm);
};

typedef Gears::Singleton<Application_> Application;

#endif /*UTILS_DTREETRAINER_APPLICATION_HPP_*/
