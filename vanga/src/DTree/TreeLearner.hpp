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

#ifndef TREELEARNER_HPP_
#define TREELEARNER_HPP_

#include <map>
#include <set>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>
#include <Gears/Basic/Time.hpp>
#include <Gears/Threading/TaskRunner.hpp>

#include "SVM.hpp"
#include "DTree.hpp"
#include "Gain.hpp"

namespace Vanga
{
  typedef std::set<unsigned long> FeatureSet;
  typedef std::vector<unsigned long> OrderedFeatureArray;

  // TreeNodeDescr
  struct TreeNodeDescr: public Gears::AtomicRefCountable
  {
  public:
    typedef std::map<double, double> AvgCoverMap;
    typedef std::multimap<double, unsigned long> GainToFeatureMap;

  public:
    TreeNodeDescr()
      : delta_prob(0.0)
    {}

    unsigned long feature_id;
    double delta_prob;
    double delta_gain;
    Gears::IntrusivePtr<TreeNodeDescr> yes_tree;
    Gears::IntrusivePtr<TreeNodeDescr> no_tree;

  public:
    // return avg => cover mapping
    void
    avg_covers(AvgCoverMap& avg_covers, unsigned long all_rows) const throw();

    void
    gain_features(GainToFeatureMap& gain_to_features) const throw();

    Gears::IntrusivePtr<TreeNodeDescr>
    sub_tree(
      double min_avg,
      unsigned long min_node_cover = 0)
      const throw();

    /*
    std::string
    to_string(
      const char* prefix,
      const FeatureDictionary* dict = 0,
      const unsigned long* all_rows = 0)
      const throw();

    std::string
    to_xml(
      const char* prefix,
      const FeatureDictionary* dict = 0,
      const unsigned long* all_rows = 0)
      const throw();
    */

    void
    features(FeatureSet& features) const;

  protected:
    typedef std::map<unsigned long, double> FeatureToGainMap;

  protected:
    void
    collect_avg_covers_(
      AvgCoverMap& avg_covers,
      unsigned long all_rows) const;

    void
    collect_feature_gains_(
      FeatureToGainMap& feature_to_gains)
      const;
  };

  typedef Gears::IntrusivePtr<TreeNodeDescr> TreeNodeDescr_var;

  // TreeLearner
  template<typename LabelType>
  class TreeLearner
  {
  public:
    typedef SVM<LabelType> SVMT;
    typedef LabelType LabelT;
    //typedef GainType GainT;

    typedef Gears::IntrusivePtr<const SVM<LabelType> > ConstSVM_var;

    typedef Gears::IntrusivePtr<SVM<LabelType> > SVM_var;

    typedef std::vector<SVM_var> SVMArray;

    typedef std::deque<Row*> RowPtrArray;

    typedef std::vector<unsigned long> FeatureIdArray;

    typedef std::unordered_map<unsigned long, SVM_var> FeatureRowsMap;

    class LearnTreeHolder;
    typedef Gears::IntrusivePtr<LearnTreeHolder> LearnTreeHolder_var;

    struct GainTreeNodeDescrKey
    {
      GainTreeNodeDescrKey(double gain_val, TreeNodeDescr* tree_node_val);

      bool
      operator<(const GainTreeNodeDescrKey& right) const;

      double gain;
      TreeNodeDescr_var tree_node;
    };

    typedef std::multiset<GainTreeNodeDescrKey> GainToTreeNodeDescrMap;

    // BagHolder
    struct BagHolder: public Gears::AtomicRefCountable
    {
      FeatureIdArray features;
      FeatureRowsMap feature_rows;
      SVM_var bag;

    protected:
      virtual ~BagHolder() throw() {}
    };

    typedef Gears::IntrusivePtr<BagHolder> BagHolder_var;
    typedef std::vector<BagHolder_var> BagHolderArray;

    // BagPart
    struct BagPart: public Gears::AtomicRefCountable
    {
      BagHolder_var bag_holder;
      SVM_var svm;

    protected:
      virtual ~BagPart() throw() {}
    };

    typedef Gears::IntrusivePtr<BagPart> BagPart_var;
    typedef std::vector<BagPart_var> BagPartArray;

    // LearnContext
    class LearnContext: public Gears::AtomicRefCountable
    {
      friend class TreeLearner;

    public:
      enum FeatureSelectionStrategy
      {
        FSS_BEST,
        FSS_TOP3_RANDOM
      };

      template<typename GainType>
      DTree_var
      train(
        unsigned long max_add_depth,
        unsigned long check_depth,
        double alpha = 1.0,
        FeatureSelectionStrategy feature_selection_strategy = FSS_BEST,
        bool allow_negative_gain = false,
        unsigned long gain_check_bags = 0);

    protected:
      struct TreeReplace
      {
        LearnTreeHolder_var old_tree;
        LearnTreeHolder_var new_tree;
      };

      struct DigCacheKey
      {
        DigCacheKey(unsigned long tree_id_val, unsigned long bag_id_val)
          : tree_id(tree_id_val),
            bag_id(bag_id_val)
        {}

        unsigned long tree_id;
        unsigned long bag_id;
      };

      struct DigCache
      {
        unsigned long best_feature_id;
        double best_gain;
      };

      typedef std::map<DigCacheKey, DigCache> DigCacheMap;

    protected:
      LearnContext(
        Gears::TaskRunner* task_runner,
        const BagPartArray& bags,
        const DTree* tree);

      virtual ~LearnContext() throw();

      template<typename GainType>
      LearnTreeHolder_var
      fill_learn_tree_(
        unsigned long& max_tree_id,
        const BagPartArray& bags,
        const DTree* tree);

      template<typename GainType>
      void
      fetch_nodes_(
        std::multimap<double, TreeReplace>& nodes,
        LearnTreeHolder* tree,
        double prob,
        unsigned long cur_depth,
        unsigned long max_add_depth,
        unsigned long check_depth,
        double alpha_coef,
        FeatureSelectionStrategy feature_selection_strategy,
        bool allow_negative_gain,
        unsigned long gain_check_bags);

      DTree_var
      fill_dtree_(LearnTreeHolder* learn_tree_holder);

      void
      adapt_learn_tree_holder_(
        LearnTreeHolder* tree,
        const BagPartArray& bags);

    protected:
      Gears::TaskRunner_var task_runner_;

      BagHolderArray bags;

      LearnTreeHolder_var cur_tree_;
      double base_pred_;
      unsigned long max_tree_id_;

      ConstDTree_var init_base_tree_;
      BagPartArray init_bags_;

      DigCacheMap dig_cache_;
    };

    typedef Gears::IntrusivePtr<LearnContext>
      LearnContext_var;

    // Context
    class Context: public Gears::AtomicRefCountable
    {
      friend class TreeLearner;

    public:
      LearnContext_var
      create_learner(
        const DTree* base_tree,
        Gears::TaskRunner* task_runner = 0);

    protected:
      Context(const BagPartArray& bag_parts);

      virtual ~Context() throw();

    protected:
      BagPartArray bag_parts_;
    };

    typedef Gears::IntrusivePtr<Context> Context_var;

  public:
    static Context_var
    create_context(const SVMArray& svm_array);

    static double
    eval_gain(
      unsigned long yes_value_labeled,
      unsigned long yes_value_unlabeled,
      unsigned long labeled,
      unsigned long unlabeled);

    /*
    static double
    delta_prob(const BagPartArray& bags);
    */

    static void
    revert_bags_pred_(
      BagPartArray& res_bag_parts,
      const BagPartArray& bag_parts,
      const typename LearnTreeHolder::Branch& branch);

    static void
    div_bags_(
      BagPartArray& yes_bag_parts,
      BagPartArray& no_bag_parts,
      const BagPartArray& bag_parts,
      unsigned long feature_id);

    static void
    div_rows_(
      SVM_var& yes_svm,
      SVM_var& no_svm,
      unsigned long feature_id,
      const SVM<LabelType>* svm,
      const FeatureRowsMap& feature_rows);

    static void
    fill_feature_rows(
      FeatureIdArray& features,
      FeatureRowsMap& feature_rows,
      const SVM<LabelType>& svm)
      throw();

    template<typename GainType>
    static void
    check_feature_(
      double& res_gain,
      LearnTreeHolder_var& new_tree,
      PredCollector& pred_collector,
      GainType& gain_calc,
      double top_pred,
      unsigned long feature_id,
      //const FeatureRowsMap& feature_rows,
      //const SVM<LabelType>* feature_svm,
      const BagPartArray& bags,
      unsigned long gain_check_bags,
      LearnTreeHolder* cur_tree,
      //const SVM<LabelType>* node_svm,
      unsigned long check_depth,
      double alpha_coef)
      throw();

  protected:
    // ProcessorType
    //   ContextType
    //   ResultType
    //   aggregate(const ResultType& yes_res, const ResultType& no_res)
    //   null_result()
    //
    template<typename ProcessorType>
    static typename ProcessorType::ResultType
    best_dig_(
      Gears::TaskRunner* task_runner,
      const typename ProcessorType::ContextType& context,
      const ProcessorType& processor,
      double base_pred,
      double cur_delta,
      const FeatureSet& skip_null_features,
      const BagPartArray& bags,
      unsigned long gain_check_bags,
      LearnTreeHolder* cur_tree,
      unsigned long max_depth,
      unsigned long check_depth,
      double alpha_coef,
      bool allow_negative_gain)
      throw();

    template<typename GainType>
    static bool
    get_best_feature_(
      //unsigned long& best_feature_id,
      double& best_gain,
      LearnTreeHolder_var& new_tree,
      /*
      double& best_no_delta,
      double& best_yes_delta,
      */
      Gears::TaskRunner* task_runner,
      double top_pred,
      //const FeatureRowsMap& feature_rows,
      const FeatureSet& skip_features,
      const BagPartArray& bags,
      unsigned long gain_check_bags,
      LearnTreeHolder* cur_tree,
      //const SVM<LabelType>* node_svm,
      unsigned long check_depth,
      bool top_eval,
      double alpha_coef,
      bool allow_negative_gain)
      throw();

    template<typename GainType>
    static double
    eval_feature_gain_(
      LearnTreeHolder_var& new_tree,
      PredCollector& pred_collector,
      GainType& gain_calc,
      double add_delta,
      const LearnTreeHolder* cur_tree,
      unsigned long add_feature_id,
      //const OrderedFeatureArray& features,
      const FeatureRowsMap& feature_rows,
      const SVM<LabelType>* node_svm)
      throw();

    template<typename GainType>
    static double
    eval_remove_gain_on_bags_(
      double& delta,
      PredCollector& pred_collector,
      GainType& gain_calc,
      double add_delta,
      const BagPartArray& bags,
      const typename LearnTreeHolder::Branch& branch)
      throw();

    template<typename GainType>
    static double
    eval_feature_gain_on_bags_(
      LearnTreeHolder_var& new_tree,
      PredCollector& pred_collector,
      GainType& gain_calc,
      double add_delta,
      unsigned long feature_id,
      const BagPartArray& bags,
      LearnTreeHolder* cur_tree,
      unsigned long gain_check_bags)
      throw();

    template<typename GainType>
    static double
    eval_init_delta_(
      double& delta,
      PredCollector& pred_collector,
      GainType& gain_calc,
      double top_pred,
      const SVM<LabelType>* node_svm,
      const LabelType& add_label,
      bool eval_delta)
      throw();

    template<typename GainType>
    static double
    eval_feature_gain_by_delta_(
      GainType& gain_calc,
      double add_delta,
      LearnTreeHolder* new_tree,
      SVM<LabelType>* node_svm,
      const FeatureRowsMap& feature_rows)
      throw();

    static void
    div_by_tree_(
      std::vector<std::pair<SVM_var, double> >& svms,
      LearnTreeHolder* tree,
      const FeatureRowsMap& feature_rows)
      throw();

    static void
    div_group_(
      typename SVM<LabelType>::PredictGroup_var& yes_group,
      typename SVM<LabelType>::PredictGroup_var& no_group,
      unsigned long feature_id,
      const PredictGroup<LabelType>* div_group,
      const FeatureRowsMap& feature_rows);
  };
}

namespace Vanga
{
  template<typename LabelType>
  TreeLearner<LabelType>::GainTreeNodeDescrKey::GainTreeNodeDescrKey(
    double gain_val,
    TreeNodeDescr* tree_node_val)
    : gain(gain_val),
      tree_node(Gears::add_ref(tree_node_val))
  {}

  template<typename LabelType>
  bool
  TreeLearner<LabelType>::GainTreeNodeDescrKey::operator<(
    const GainTreeNodeDescrKey& right) const
  {
    return gain < right.gain ||
      (gain == right.gain && tree_node < right.tree_node);
  }
}

#include "TreeLearner.tpp"

#endif /*TREELEARNER_HPP_*/
