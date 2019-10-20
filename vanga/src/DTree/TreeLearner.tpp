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

#include <unordered_map>
#include "PredBuffer.hpp"
#include "Utils.hpp"

namespace Vanga
{
  const bool DEBUG_FEATURE_SEARCH_ = false;

  namespace
  {
    const double EPS = 0.0000001;
    const double LABEL_DOUBLE_MIN = EPS;
    const double LABEL_DOUBLE_MAX = 1 - EPS;

    const double LABEL_MIN = EPS;
    const double LABEL_MAX = 1.0 - EPS;

    const double GAIN_SHARE_PENALTY = 0.0001;
    const double GAIN_ABS_PENALTY = 0.0001;

    const bool GAIN_TRACE = false;
    const bool SELF_CHECK_ = true;

    const double DEPTH_PINALTY_STEP = 0.000001;
  }

  template<typename LearnerType>
  struct GetBestFeatureResult: public Gears::AtomicRefCountable
  {
    struct BestChoose
    {
      BestChoose()
        : gain(0.0)
      {}

      BestChoose(
        double gain_val,
        typename LearnerType::LearnTreeHolder* tree_val)
          : gain(gain_val),
            tree(Gears::add_ref(tree_val))
      {}

      double gain;
      typename LearnerType::LearnTreeHolder_var tree;
    };

    typedef std::deque<BestChoose> BestChooseArray;

    GetBestFeatureResult(
      const FeatureSet* skip_null_features,
      bool allow_negative)
      : skip_null_features_(skip_null_features),
        best_gain(allow_negative ? 1000000.0 : -EPS),
        tasks_in_progress(0)
    {}

    void
    inc()
    {
      Gears::ConditionGuard guard(lock_, cond_);
      ++tasks_in_progress;
    }

    void
    set(double gain, typename LearnerType::LearnTreeHolder* tree)
    {
      //const double DELTA_EPS = 0.0001;

      // ignore branches without modifications
      Gears::ConditionGuard guard(lock_, cond_);

      if(gain < best_gain + EPS)
      {
        if(gain <= best_gain - EPS)
        {
          best_features.clear();
        }

        best_features.push_back(BestChoose(gain, tree));
        best_gain = gain;
      }

      assert(tasks_in_progress > 0);

      if(--tasks_in_progress == 0 || tasks_in_progress % 100 == 0)
      {
        cond_.signal();
      }
    }

    void
    wait(unsigned long all_tasks)
    {
      unsigned long prev_tasks_in_progress = 0;
      Gears::ConditionGuard guard(lock_, cond_);
      while(tasks_in_progress > 0)
      {
        guard.wait();

        if(tasks_in_progress % 100 == 0 && tasks_in_progress != tasks_in_progress)
        {
          prev_tasks_in_progress = tasks_in_progress;
          std::cout << "processed " << (all_tasks - tasks_in_progress) << "/" << tasks_in_progress << " features" << std::endl;
        }
      }
    }

    bool
    get_result(BestChoose& res) const
    {
      if(!best_features.empty())
      {
        unsigned long i = Gears::safe_rand(best_features.size());
        res = best_features[i];
        return true;
      }

      return false;
    }

    const FeatureSet* skip_null_features_;
    Gears::Mutex lock_;
    Gears::Condition cond_;
    BestChooseArray best_features;
    double best_gain;
    unsigned long tasks_in_progress;

  protected:
    virtual ~GetBestFeatureResult() throw() = default;
  };

  template<typename LearnerType>
  struct GetBestFeatureParams: public Gears::AtomicRefCountable
  {
    double top_pred;
    //const typename LearnerType::FeatureRowsMap* feature_rows;
    const FeatureSet* skip_null_features;
    const typename LearnerType::BagPartArray* bags;
    unsigned long gain_check_bags;
    typename LearnerType::LearnTreeHolder* cur_tree;
    //typename LearnerType::ConstSVM_var node_svm;
    unsigned long check_depth;
    bool top_eval;
    double alpha_coef;
  };

  template<typename LearnerType, typename GainType>
  class GetBestFeatureTask: public Gears::Task
  {
  public:
    typedef Gears::IntrusivePtr<GetBestFeatureParams<LearnerType> >
      GetBestFeatureParams_var;

    typedef Gears::IntrusivePtr<GetBestFeatureResult<LearnerType> >
      GetBestFeatureResult_var;

    GetBestFeatureTask(
      GetBestFeatureResult<LearnerType>* result,
      GetBestFeatureParams<LearnerType>* params,
      unsigned long feature_id,
      typename LearnerType::SVMT* feature_svm)
      throw();

    virtual void
    execute() throw();

  protected:
    virtual
    ~GetBestFeatureTask() throw () = default;

  private:
    const GetBestFeatureResult_var result_;
    const GetBestFeatureParams_var params_;
    const unsigned long feature_id_;
    typename LearnerType::SVMT* feature_svm_;
  };

  // GetBestFeatureTask impl
  template<typename LearnerType, typename GainType>
  GetBestFeatureTask<LearnerType, GainType>::GetBestFeatureTask(
    GetBestFeatureResult<LearnerType>* result,
    GetBestFeatureParams<LearnerType>* params,
    unsigned long feature_id,
    typename LearnerType::SVMT* feature_svm)
    throw()
    : result_(Gears::add_ref(result)),
      params_(Gears::add_ref(params)),
      feature_id_(feature_id),
      feature_svm_(feature_svm)
  {
    result->inc();
  }

  template<typename LearnerType, typename GainType>
  void
  GetBestFeatureTask<LearnerType, GainType>::execute() throw()
  {
    double gain;
    typename LearnerType::LearnTreeHolder_var new_tree;
    GainType gain_calc;
    PredCollector pred_collector;

    LearnerType::check_feature_(
      gain,
      new_tree,
      pred_collector,
      gain_calc,
      params_->top_pred,
      feature_id_,
      //*(params_->feature_rows),
      //feature_svm_,
      *(params_->bags),
      params_->gain_check_bags,
      params_->cur_tree,
      //params_->node_svm,
      params_->check_depth,
      params_->alpha_coef);

    if(GAIN_TRACE)
    {
      Gears::ErrorStream ostr;
      ostr << "GAIN FOR #" << feature_id_ << ": " << gain << std::endl;
      std::cout << ostr.str() << std::endl;
    }

    result_->set(gain, new_tree);
  }

  template<typename LabelType>
  struct TreeLearner<LabelType>::LearnTreeHolder:
    public Gears::AtomicRefCountable
  {
  public:
    struct Branch
    {
      unsigned long feature_id;
      Gears::IntrusivePtr<LearnTreeHolder> yes_tree;
      Gears::IntrusivePtr<LearnTreeHolder> no_tree;
    };

    unsigned long tree_id;
    double delta_prob;

    std::vector<Branch> branches;

    BagPartArray bags;
    double delta_gain;

    LearnTreeHolder()
      : tree_id(0),
        delta_prob(0),
        delta_gain(0)
    {}

    void
    mul(double alpha)
    {
      delta_prob *= alpha;

      for(auto branch_it = branches.begin(); branch_it != branches.end(); ++branch_it)
      {
        if(branch_it->yes_tree.in())
        {
          branch_it->yes_tree->mul(alpha);
        }

        if(branch_it->no_tree.in())
        {
          branch_it->no_tree->mul(alpha);
        }
      }
    }

    void
    print(std::ostream& ostr, const char* prefix) const
    {
      ostr << prefix << "[" << tree_id << "]: delta = " << delta_prob << std::endl;
      for(auto branch_it = branches.begin(); branch_it != branches.end(); ++branch_it)
      {
        ostr << prefix << "  #" << branch_it->feature_id << std::endl;

        if(branch_it->yes_tree)
        {
          branch_it->yes_tree->print(ostr, (std::string(prefix) + "    ").c_str());
        }

        if(branch_it->no_tree)
        {
          branch_it->no_tree->print(ostr, (std::string(prefix) + "    ").c_str());
        }
      }
    }

    double
    predict(const FeatureArray& features) const throw()
    {
      double res = delta_prob;

      for(auto branch_it = branches.begin(); branch_it != branches.end(); ++branch_it)
      {
        if(std::binary_search(
             features.begin(),
             features.end(),
             branch_it->feature_id,
             FeatureLess()))
        {
          // yes tree
          res += branch_it->yes_tree ? branch_it->yes_tree->predict(features) : 0.0;
        }
        else
        {
          // no tree
          res += branch_it->no_tree ? branch_it->no_tree->predict(features) : 0.0;
        }
      }

      return res;
    }

  public:
    void
    add_branch(const Branch& branch)
    {
      for(auto branch_it = branches.begin(); branch_it != branches.end(); ++branch_it)
      {
        if(branch_it->feature_id == branch.feature_id)
        {
          assert(
            branch_it->yes_tree.in() && branch_it->no_tree.in() &&
            branch.yes_tree.in() && branch.no_tree.in());

          branch_it->yes_tree->delta_prob += branch.yes_tree->delta_prob;
          branch_it->no_tree->delta_prob += branch.no_tree->delta_prob;

          for(auto yes_branch_it = branch.yes_tree->branches.begin();
            yes_branch_it != branch.yes_tree->branches.end(); ++yes_branch_it)
          {
            branch_it->yes_tree->add_branch(*yes_branch_it);
          }

          for(auto no_branch_it = branch.no_tree->branches.begin();
            no_branch_it != branch.no_tree->branches.end(); ++no_branch_it)
          {
            branch_it->no_tree->add_branch(*no_branch_it);
          }

          return;
        }
      }

      branches.push_back(branch);
    }

  protected:
    virtual ~LearnTreeHolder() throw()
    {}
  };

  template<typename LearnerType, typename GainType>
  class LearnRevertPredictor
  {
  public:
    typedef typename LearnerType::LabelT ResultType;

    LearnRevertPredictor(const typename LearnerType::LearnTreeHolder::Branch& branch)
      : branch_(branch)
    {}

    typename LearnerType::LabelT
    operator()(const Row* row, const typename LearnerType::LabelT& label) const throw()
    {
      if(std::binary_search(
           row->features.begin(),
           row->features.end(),
           branch_.feature_id,
           FeatureLess()))
      {
        return GainType::add_delta(label, -branch_.yes_tree->predict(row->features));
      }
      else
      {
        return GainType::add_delta(label, -branch_.no_tree->predict(row->features));
      }
    }

  private:
    typename LearnerType::LearnTreeHolder::Branch branch_;
  };

  const double NULL_GAIN = 0.00003;

  // GetBestLearnTreeHolderProcessor
  template<typename LabelType, typename GainType>
  struct GetBestLearnTreeHolderProcessor
  {
    struct ContextType
    {
      ContextType(
        double base_prob_val,
        const FeatureSet& skip_null_features_val,
        double depth_pinalty_val,
        unsigned long top_element_limit_val,
        unsigned long depth_limit_val)
        throw()
        : base_prob(base_prob_val),
          skip_null_features(skip_null_features_val),
          depth_pinalty(depth_pinalty_val),
          top_element_limit(top_element_limit_val),
          depth_limit(depth_limit_val)
      {}

      const double base_prob;
      const FeatureSet& skip_null_features;

      const double depth_pinalty;
      const unsigned long top_element_limit;
      const unsigned long depth_limit;
    };

    typedef GainType GainT;

    typedef typename TreeLearner<LabelType>::LearnTreeHolder_var ResultType;

    static typename TreeLearner<LabelType>::LearnTreeHolder_var
    aggregate(
      double /*gain*/,
      typename TreeLearner<LabelType>::LearnTreeHolder* new_tree
      /*,
      unsigned long feature_id,
      double delta,
      const typename TreeLearner<LabelType, GainType>::BagPartArray&, //bags,
      typename TreeLearner<LabelType, GainType>::LearnTreeHolder* yes_arg,
      typename TreeLearner<LabelType, GainType>::LearnTreeHolder* no_arg
      */
      )
    {
      return Gears::add_ref(new_tree);
      /*
      typename TreeLearner<LabelType, GainType>::LearnTreeHolder_var union_tree =
        new typename TreeLearner<LabelType, GainType>::LearnTreeHolder();
      union_tree->delta_gain = gain;
      union_tree->delta_prob = delta; //TreeLearner<LabelType, GainType>::delta_prob(bags);

      typename TreeLearner<LabelType, GainType>::LearnTreeHolder::Branch branch;
      branch.feature_id = feature_id;
      branch.yes_tree = Gears::add_ref(yes_arg);
      branch.no_tree = Gears::add_ref(no_arg);
      union_tree->branches.push_back(branch);

      return union_tree;
      */
    }

    static typename TreeLearner<LabelType>::LearnTreeHolder_var
    null_result(
      double delta,
      const typename TreeLearner<LabelType>::BagPartArray& //bags
      )
    {
      assert(!std::isnan(delta));

      // stop node
      typename TreeLearner<LabelType>::LearnTreeHolder_var stop_tree =
        new typename TreeLearner<LabelType>::LearnTreeHolder();
      stop_tree->delta_gain = 0.0;
      stop_tree->delta_prob = delta; //TreeLearner<LabelType, GainType>::delta_prob(bags);
      //std::cerr << "stop_tree->delta_prob = " << stop_tree->delta_prob << std::endl;
      return stop_tree;
    }
  };

  void
  TreeNodeDescr::collect_feature_gains_(
    FeatureToGainMap& feature_to_gains)
    const
  {
    if(feature_id)
    {
      auto feature_it = feature_to_gains.find(feature_id);
      if(feature_it != feature_to_gains.end())
      {
        feature_it->second += delta_gain;
      }
      else
      {
        feature_to_gains.insert(std::make_pair(feature_id, delta_gain));
      }
    }

    if(yes_tree)
    {
      yes_tree->collect_feature_gains_(feature_to_gains);
    }

    if(no_tree)
    {
      no_tree->collect_feature_gains_(feature_to_gains);
    }
  }

  void
  TreeNodeDescr::gain_features(
    GainToFeatureMap& gain_to_features)
    const throw()
  {
    FeatureToGainMap feature_to_gains;
    collect_feature_gains_(feature_to_gains);

    for(auto it = feature_to_gains.begin(); it != feature_to_gains.end(); ++it)
    {
      gain_to_features.insert(std::make_pair(it->second, it->first));
    }
  }

  void
  TreeNodeDescr::features(FeatureSet& features) const
  {
    features.insert(feature_id);

    if(yes_tree)
    {
      yes_tree->features(features);
    }

    if(no_tree)
    {
      no_tree->features(features);
    }
  }

  // TreeLearner::Context
  template<typename LabelType>
  TreeLearner<LabelType>::Context::Context(const BagPartArray& bag_parts)
    : bag_parts_(bag_parts)
  {}

  template<typename LabelType>
  TreeLearner<LabelType>::Context::~Context() throw()
  {}

  template<typename LabelType>
  typename TreeLearner<LabelType>::LearnContext_var
  TreeLearner<LabelType>::Context::create_learner(
    const DTree* base_tree,
    Gears::TaskRunner* task_runner)
  {
    return new LearnContext(
      task_runner,
      bag_parts_,
      base_tree);
  }

  // TreeLearner::LearnContext
  template<typename LabelType>
  TreeLearner<LabelType>::LearnContext::LearnContext(
    Gears::TaskRunner* task_runner,
    const BagPartArray& bags,
    const DTree* base_tree)
    : task_runner_(Gears::add_ref(task_runner)),
      base_pred_(0.0),
      max_tree_id_(0),
      init_base_tree_(Gears::add_ref(base_tree)),
      init_bags_(bags)
  {}

  template<typename LabelType>
  TreeLearner<LabelType>::LearnContext::~LearnContext() throw()
  {}

  template<typename LabelType>
  template<typename GainType>
  DTree_var
  TreeLearner<LabelType>::LearnContext::train(
    unsigned long max_add_depth,
    unsigned long check_depth,
    double alpha_coef,
    FeatureSelectionStrategy feature_selection_strategy,
    bool allow_negative_gain,
    unsigned long gain_check_bags)
  {
    if(!cur_tree_.in())
    {
      cur_tree_ = fill_learn_tree_<GainType>(
        max_tree_id_,
        init_bags_,
        init_base_tree_);
    }

    // collect stop nodes &
    std::multimap<double, TreeReplace> nodes;

    fetch_nodes_<GainType>(
      nodes,
      cur_tree_,
      0.0,
      0, // cur_depth
      max_add_depth,
      check_depth,
      alpha_coef,
      feature_selection_strategy,
      allow_negative_gain,
      gain_check_bags);

    // apply node change
    if(!nodes.empty())
    {
      const TreeReplace& tree_replace = nodes.begin()->second;
      LearnTreeHolder_var old_node = tree_replace.old_tree;
      LearnTreeHolder_var new_node = tree_replace.new_tree;

      tree_replace.new_tree->mul(alpha_coef);

      //new_node->delta_prob = 0;

      /*
      std::cout << "tree_replace: old tree id = " << tree_replace.old_tree->tree_id <<
        ", new tree id = " << new_node->tree_id << std::endl <<
        "OLD TREE:" << fill_dtree_(old_node)->to_string("") << std::endl <<
        "NEW TREE:" << fill_dtree_(new_node)->to_string("") << std::endl;
      */
      adapt_learn_tree_holder_(
        new_node,
        //tree_replace.old_tree_prob_base,
        old_node->bags);

      // correct abs prob to delta prob
      old_node->tree_id = new_node->tree_id;
      old_node->delta_gain = new_node->delta_gain;
      old_node->delta_prob += new_node->delta_prob;
      //old_node->delta_prob = tree_replace.old_tree_prob_base;

      for(auto branch_it = new_node->branches.begin();
        branch_it != new_node->branches.end(); ++branch_it)
      {
        // TODO: merge equal features
        typename LearnTreeHolder::Branch branch;
        branch.feature_id = branch_it->feature_id;
        branch.yes_tree = branch_it->yes_tree;
        branch.no_tree = branch_it->no_tree;
        old_node->add_branch(branch);
      }

      old_node->bags.swap(new_node->bags);

      base_pred_ = 0.0;
    }

    return fill_dtree_(cur_tree_);
  }

  template<typename LabelType>
  void
  TreeLearner<LabelType>::LearnContext::adapt_learn_tree_holder_(
    LearnTreeHolder* tree,
    const BagPartArray& bags)
  {
    ++max_tree_id_;
    tree->tree_id = max_tree_id_;

    for(auto branch_it = tree->branches.begin(); branch_it != tree->branches.end(); ++branch_it)
    {
      if(branch_it->yes_tree || branch_it->no_tree)
      {
        assert(branch_it->yes_tree && branch_it->no_tree);

        BagPartArray yes_bag_parts;
        BagPartArray no_bag_parts;

        for(auto bag_it = bags.begin(); bag_it != bags.end(); ++bag_it)
        {
          SVM_var yes_svm;
          SVM_var no_svm;

          div_rows_(
            yes_svm,
            no_svm,
            branch_it->feature_id,
            (*bag_it)->svm,
            (*bag_it)->bag_holder->feature_rows);

          BagPart_var yes_bag_part = new BagPart();
          yes_bag_part->bag_holder = (*bag_it)->bag_holder;
          yes_bag_part->svm = yes_svm;
          yes_bag_parts.push_back(yes_bag_part);

          BagPart_var no_bag_part = new BagPart();
          no_bag_part->bag_holder = (*bag_it)->bag_holder;
          no_bag_part->svm = no_svm;
          no_bag_parts.push_back(no_bag_part);
        }

        adapt_learn_tree_holder_(
          branch_it->yes_tree,
          yes_bag_parts);

        adapt_learn_tree_holder_(
          branch_it->no_tree,
          no_bag_parts);
      }
      else
      {
        tree->bags = bags;
      }
    }
  }

  template<typename LabelType>
  DTree_var
  TreeLearner<LabelType>::LearnContext::fill_dtree_(
    LearnTreeHolder* learn_tree_holder)
  {
    if(learn_tree_holder)
    {
      assert(!std::isnan(learn_tree_holder->delta_prob));

      DTree_var res = new DTree();
      res->tree_id = learn_tree_holder->tree_id;
      res->delta_prob = learn_tree_holder->delta_prob;

      for(auto branch_it = learn_tree_holder->branches.begin();
        branch_it != learn_tree_holder->branches.end(); ++branch_it)
      {
        if(branch_it->yes_tree || branch_it->no_tree)
        {
          DTree::Branch branch;
          branch.feature_id = branch_it->feature_id;
          branch.yes_tree = fill_dtree_(branch_it->yes_tree);
          branch.no_tree = fill_dtree_(branch_it->no_tree);
          ///*
          double avg_delta_prob = (branch.yes_tree->delta_prob + branch.no_tree->delta_prob) / 2;
          branch.yes_tree->delta_prob -= avg_delta_prob;
          branch.no_tree->delta_prob -= avg_delta_prob;
          res->delta_prob += avg_delta_prob;
          //*/
          res->branches_.push_back(branch);
        }
      }

      return res;
    }

    return DTree_var();
  }

  template<typename LabelType>
  template<typename GainType>
  void
  TreeLearner<LabelType>::LearnContext::fetch_nodes_(
    std::multimap<double, TreeReplace>& nodes,
    LearnTreeHolder* tree,
    double prob,
    unsigned long cur_depth,
    unsigned long max_add_depth,
    unsigned long check_depth,
    double alpha_coef,
    FeatureSelectionStrategy feature_selection_strategy,
    bool allow_negative_gain,
    unsigned long gain_check_bags
    )
  {
    for(auto branch_it = tree->branches.begin(); branch_it != tree->branches.end(); ++branch_it)
    {
      assert(branch_it->yes_tree && branch_it->no_tree);

      fetch_nodes_<GainType>(
        nodes,
        branch_it->yes_tree,
        0.0,
        cur_depth + 1,
        max_add_depth,
        check_depth,
        alpha_coef,
        feature_selection_strategy,
        allow_negative_gain,
        gain_check_bags);

      fetch_nodes_<GainType>(
        nodes,
        branch_it->no_tree,
        0.0,
        cur_depth + 1,
        max_add_depth,
        check_depth,
        alpha_coef,
        feature_selection_strategy,
        allow_negative_gain,
        gain_check_bags);
    }

    // search remove candidates

    if(false)
    {
      LearnTreeHolder_var rem_tree;
      double best_rem_gain = 0.0;
      PredCollector pred_collector;
      GainType gain_calc;

      unsigned long branch_i = 0;
      for(auto branch_it = tree->branches.begin(); branch_it != tree->branches.end(); ++branch_it, ++branch_i)
      {
        double rem_delta;
        double delta_gain = eval_remove_gain_on_bags_(
          rem_delta,
          pred_collector,
          gain_calc,
          base_pred_,
          tree->bags,
          *branch_it);

        /*
        std::cerr << "remove delta = " << delta_gain << std::endl;
        */
        if(delta_gain < -EPS && delta_gain < best_rem_gain)
        {
          LearnTreeHolder_var rem_tree = new LearnTreeHolder(*tree);
          rem_tree->delta_gain = delta_gain;
          rem_tree->delta_prob += rem_delta;
          rem_tree->branches.erase(rem_tree->branches.begin() + branch_i);
        }
      }

      if(rem_tree && rem_tree->delta_gain < -EPS)
      {
        TreeReplace rem_tree_replace;
        rem_tree_replace.old_tree = Gears::add_ref(tree);
        rem_tree_replace.new_tree = rem_tree;
        nodes.insert(std::make_pair(rem_tree->delta_gain, rem_tree_replace));
      }
    }

    // search add candidates
    typename GetBestLearnTreeHolderProcessor<LabelType, GainType>::ContextType
      gain_search_context(
        prob,
        FeatureSet(), // skip_null_features
        0.0, // pinalty
        10, // top_element_limit
        max_add_depth
        );

    /*
    std::cout << "to best_dig_: tree_id = " << tree->tree_id <<
      ", delta_prob = " << tree->delta_prob <<
      ", max_add_depth = " << max_add_depth <<
      std::endl;
    */

    if(DEBUG_FEATURE_SEARCH_)
    {
      std::cerr << std::string(10 - check_depth * 2, ' ') <<
        "to best_dig_ for tree #" << tree->tree_id << std::endl;
    }

    FeatureSet skip_null_features;
    for(auto branch_it = tree->branches.begin(); branch_it != tree->branches.end(); ++branch_it)
    {
      skip_null_features.insert(branch_it->feature_id);
    }

    LearnTreeHolder_var add_tree = best_dig_(
      task_runner_,
      gain_search_context,
      GetBestLearnTreeHolderProcessor<LabelType, GainType>(),
      0.0, // base_pred
      base_pred_,
      skip_null_features,
      tree->bags,
      gain_check_bags,
      tree,
      max_add_depth,
      check_depth,
      alpha_coef,
      allow_negative_gain);

    add_tree->delta_gain += DEPTH_PINALTY_STEP * cur_depth; // depth pinalty

    if(DEBUG_FEATURE_SEARCH_)
    {
      std::cerr << std::string(10 - check_depth * 2, ' ') <<
        "from best_dig_ for tree #" << tree->tree_id <<
        ": delta_gain = " << (add_tree ? add_tree->delta_gain : 0.0) <<
        std::endl;
    }

    /*
    std::cout << "from best_dig_: tree_id = " << tree->tree_id <<
      ", delta_gain = " << (delta_tree ? delta_tree->delta_gain : 0.0) <<
      std::endl <<
      (delta_tree ? fill_dtree_(delta_tree)->to_string("") : std::string()) << 
      std::endl;
    */

    if(add_tree && add_tree->delta_gain < -EPS)
    {
      TreeReplace tree_replace;
      tree_replace.old_tree = Gears::add_ref(tree);
      tree_replace.new_tree = add_tree;
      nodes.insert(std::make_pair(add_tree->delta_gain, tree_replace));
    }
  }

  template<typename LabelType>
  template<typename GainType>
  typename TreeLearner<LabelType>::LearnTreeHolder_var
  TreeLearner<LabelType>::LearnContext::fill_learn_tree_(
    unsigned long& max_tree_id,
    const BagPartArray& bags,
    const DTree* tree)
  {
    LearnTreeHolder_var res = new LearnTreeHolder();

    if(tree)
    {
      assert(!std::isnan(tree->delta_prob));

      res->tree_id = tree->tree_id;
      res->delta_prob = tree->delta_prob;

      for(auto branch_it = tree->branches_.begin();
        branch_it != tree->branches_.end(); ++branch_it)
      {
        typename LearnTreeHolder::Branch branch;
        branch.feature_id = branch_it->feature_id;

        BagPartArray yes_bag_parts;
        BagPartArray no_bag_parts;

        TreeLearner::div_bags_(yes_bag_parts, no_bag_parts, bags, branch_it->feature_id);

        branch.yes_tree = fill_learn_tree_<GainType>(
          max_tree_id,
          yes_bag_parts,
          branch_it->yes_tree);

        branch.no_tree = fill_learn_tree_<GainType>(
          max_tree_id,
          no_bag_parts,
          branch_it->no_tree);

        res->branches.push_back(branch);
      }
    }
    else
    {
      // init root node
      unsigned long bag_i = Gears::safe_rand(bags.size());
      const BagPart& bag_part = *bags[bag_i];

      res->tree_id = 1;

      GainType gain_calc;
      PredCollector pred_collector;

      double gain = eval_init_delta_(
        res->delta_prob,
        pred_collector,
        gain_calc,
        0.0,
        bag_part.svm,
        LabelType(),
        true // eval delta
        );
      (void)gain;

      {
        Gears::ErrorStream ostr;
        ostr << "INIT DELTA: delta_prob = " << res->delta_prob <<
          ", gain = " << gain <<
          std::endl;
        std::cout << ostr.str() << std::endl;
      }

      base_pred_ = res->delta_prob;
    }

    res->bags = bags;
    max_tree_id = std::max(max_tree_id, res->tree_id);

    //std::cerr << "delta_prob(" << res->tree_id << "): " << res->delta_prob << std::endl;

    return res;
  }

  template<typename LabelType>
  void
  TreeLearner<LabelType>::div_by_tree_(
    std::vector<std::pair<SVM_var, double> >& svms,
    LearnTreeHolder* tree,
    const FeatureRowsMap& feature_rows)
    throw()
  {
    if(tree)
    {
      for(auto svm_it = svms.begin(); svm_it != svms.end(); ++svm_it)
      {
        svm_it->second += tree->delta_prob;
      }

      for(auto branch_it = tree->branches.begin();
        branch_it != tree->branches.end(); ++branch_it)
      {
        std::vector<std::pair<SVM_var, double> > yes_svms;
        std::vector<std::pair<SVM_var, double> > no_svms;

        for(auto svm_it = svms.begin(); svm_it != svms.end(); ++svm_it)
        {
          auto feature_it = feature_rows.find(branch_it->feature_id);
          if(feature_it != feature_rows.end())
          {
            SVM_var yes_svm;
            SVM_var no_svm;

            SVM<LabelType>::cross(yes_svm, no_svm, svm_it->first, feature_it->second);

            yes_svms.push_back(
              std::make_pair(
                yes_svm,
                svm_it->second /*+ (branch_it->yes_tree ? branch_it->yes_tree->delta_prob : 0.0)*/));

            no_svms.push_back(
              std::make_pair(
                no_svm,
                svm_it->second /*+ (branch_it->no_tree ? branch_it->no_tree->delta_prob : 0.0)*/));
          }
          else
          {
            no_svms.push_back(
              std::make_pair(
                svm_it->first,
                svm_it->second /*+ (branch_it->no_tree ? branch_it->no_tree->delta_prob : 0.0)*/));
          }
        }

        if(branch_it->yes_tree)
        {
          div_by_tree_(yes_svms, branch_it->yes_tree, feature_rows);
        }

        if(branch_it->no_tree)
        {
          div_by_tree_(no_svms, branch_it->no_tree, feature_rows);
        }

        std::copy(no_svms.begin(), no_svms.end(), std::back_inserter(yes_svms));
        svms.swap(yes_svms);
      }
    }
  }

  template<typename LabelType>
  template<typename GainType>
  double
  TreeLearner<LabelType>::eval_feature_gain_by_delta_(
    GainType& gain_calc,
    double add_delta,
    LearnTreeHolder* new_tree,
    SVM<LabelType>* node_svm,
    const FeatureRowsMap& feature_rows)
    throw()
  {
    double old_metric;

    {
      // eval current metric
      gain_calc.start_metric_eval();

      for(auto node_group_it = node_svm->grouped_rows.begin();
        node_group_it != node_svm->grouped_rows.end();
        ++node_group_it)
      {
        gain_calc.add_metric_eval(
          (*node_group_it)->label,
          (*node_group_it)->rows.size());
      }

      old_metric = gain_calc.metric_result();
    }

    double new_metric;

    {
      std::vector<std::pair<SVM_var, double> > div_svms;
      div_svms.push_back(std::make_pair(Gears::add_ref(node_svm), 0.0));

      div_by_tree_(
        div_svms,
        new_tree,
        feature_rows);

      /*
      if(!new_tree->branches.empty())
      {
        std::cerr << "F #" << new_tree->branches.begin()->feature_id <<
          ": yes delta = " << (
            new_tree->branches.begin()->yes_tree ? new_tree->branches.begin()->yes_tree->delta_prob : 0.0) <<
          ", no delta = " << (
            new_tree->branches.begin()->no_tree ? new_tree->branches.begin()->no_tree->delta_prob : 0.0) <<
          std::endl;
      }
      else
      {
        std::cerr << "F: empty branches" <<
          std::endl;
      }

      std::cerr << "DIV TREES(" << div_svms.size() << "):";
      for(auto i = div_svms.begin(); i != div_svms.end(); ++i)
      {
        std::cerr << " " << i->second;
      }
      std::cerr << std::endl;
      */

      // for each group div it by tree with label
      // div rows by new tree and fetch
      gain_calc.start_metric_eval();

      for(auto cross_group_it = div_svms.begin();
          cross_group_it != div_svms.end(); ++cross_group_it)
      {
        for(auto node_group_it = cross_group_it->first->grouped_rows.begin();
          node_group_it != cross_group_it->first->grouped_rows.end();
          ++node_group_it)
        {
          gain_calc.add_metric_eval(
            GainType::add_delta((*node_group_it)->label, cross_group_it->second + add_delta),
            (*node_group_it)->rows.size());
        }
      }

      new_metric = gain_calc.metric_result();

      /*
      // eval new metric
      gain_calc.start_metric_eval();

      for(auto node_group_it = node_svm->grouped_rows.begin();
        node_group_it != node_svm->grouped_rows.end();
        ++node_group_it)
      {
        double cur_pred = 0;
        unsigned long cur_count = 0;

        for(auto row_it = (*node_group_it)->rows.begin();
          row_it != (*node_group_it)->rows.end(); ++row_it)
        {
          const double pred = new_tree->predict((*row_it)->features);
          if(std::abs(cur_pred - pred) > EPS) // cur_pred != pred
          {
            if(cur_count > 0)
            {
              gain_calc.add_metric_eval(
                GainT::add_delta((*node_group_it)->label, cur_pred + add_delta),
                cur_count);
            }

            cur_pred = pred;
            cur_count = 1;
          }
          else
          {
            ++cur_count;
          }
        }

        if(cur_count > 0)
        {
          gain_calc.add_metric_eval(
            GainT::add_delta((*node_group_it)->label, cur_pred + add_delta),
            cur_count);
        }
      }
      */
    }

    //double new_metric = gain_calc.metric_result();

    /*
    std::cout << "TMP DEBUG eval_feature_gain_by_delta_: old_metric = " <<
      old_metric << ", new_metric = " << new_metric << std::endl;
    */
    return new_metric - old_metric;
  }

  template<typename LabelType>
  template<typename GainType>
  double
  TreeLearner<LabelType>::eval_init_delta_(
    double& delta,
    PredCollector& pred_collector,
    GainType& gain_calc,
    double top_pred,
    const SVM<LabelType>* node_svm,
    const LabelType& /*add_label*/,
    bool /*eval_delta*/)
    throw()
  {
    pred_collector.start_delta_eval(1, top_pred);

    for(auto node_group_it = node_svm->grouped_rows.begin();
      node_group_it != node_svm->grouped_rows.end(); ++node_group_it)
    {
      pred_collector.add_delta_eval(0, (*node_group_it)->label, (*node_group_it)->rows.size());
    }

    pred_collector.fin_delta_eval();

    //std::cout << "top_pred: " << top_pred << std::endl;
    FloatArray yes_deltas(1, top_pred);
    FloatArray deltas(1, top_pred);
    gain_calc.delta_result(yes_deltas, deltas, pred_collector);
    delta = deltas[0];

    gain_calc.start_metric_eval();

    for(auto node_group_it = node_svm->grouped_rows.begin();
      node_group_it != node_svm->grouped_rows.end(); ++node_group_it)
    {
      gain_calc.add_metric_eval(
        GainType::add_delta((*node_group_it)->label, delta),
        (*node_group_it)->rows.size());
    }

    assert(!std::isnan(delta));

    return gain_calc.metric_result();
  }

  // TreeLearner
  template<typename LabelType>
  template<typename ProcessorType>
  typename ProcessorType::ResultType
  TreeLearner<LabelType>::best_dig_(
    Gears::TaskRunner* task_runner,
    const typename ProcessorType::ContextType& /*context*/,
    const ProcessorType& processor,
    double base_pred, //
    double cur_delta, // current check delta
    const FeatureSet& skip_null_features,
    const BagPartArray& bags,
    unsigned long gain_check_bags,
    LearnTreeHolder* cur_tree,
    unsigned long max_depth,
    unsigned long check_depth,
    double alpha_coef,
    bool allow_negative_gain)
    throw()
  {
    // select bag randomly
    //unsigned long best_feature_id = 0;
    double best_gain = 0.0;
    LearnTreeHolder_var best_new_tree;
    /*
    double best_no_delta = 0.0;
    double best_yes_delta = 0.0;
    */

    if(max_depth > 0 &&
      get_best_feature_<typename ProcessorType::GainT>(
        best_gain,
        best_new_tree,
        task_runner,
        base_pred + cur_delta,
        //bag_part.bag_holder->feature_rows,
        skip_null_features,
        bags,
        gain_check_bags,
        cur_tree,
        //bag_part.svm,
        check_depth,
        true,
        alpha_coef,
        allow_negative_gain
        ))
    {
      return processor.aggregate(
        best_gain,
        best_new_tree);
      
      // divide rows to yes/no arrays
      /*
      BagPartArray yes_bag_parts;
      BagPartArray no_bag_parts;

      TreeLearner::div_bags_(yes_bag_parts, no_bag_parts, bags, best_feature_id);

      auto yes_res = max_depth > 1 ?
        best_dig_(
          task_runner,
          context,
          processor,
          base_pred, // base delta
          cur_delta + best_yes_delta, // cur delta
          FeatureSet(), // skip_null_features
          yes_bag_parts,
          nullptr, // cur tree
          max_depth - 1,
          check_depth,
          alpha_coef) :
        processor.null_result(best_yes_delta, yes_bag_parts);

      auto no_res = max_depth > 1 ?
        best_dig_(
          task_runner,
          context,
          processor,
          base_pred, // base delta
          cur_delta + best_no_delta, // cur delta
          FeatureSet(), // skip_null_features
          no_bag_parts,
          nullptr, // cur tree
          max_depth - 1,
          check_depth,
          alpha_coef) :
        processor.null_result(best_no_delta, no_bag_parts);

      return processor.aggregate(
        best_feature_id,
        best_gain,
        cur_delta,
        bags,
        yes_res,
        no_res);
      */
    }

    return processor.null_result(cur_delta, bags);
  }

  template<typename LabelType>
  template<typename GainType>
  bool
  TreeLearner<LabelType>::get_best_feature_(
    double& best_gain,
    LearnTreeHolder_var& new_tree,
    Gears::TaskRunner* task_runner,
    double top_pred,
    const FeatureSet& skip_null_features,
    const BagPartArray& bags,
    unsigned long gain_check_bags,
    LearnTreeHolder* cur_tree,
    unsigned long check_depth,
    bool top_eval,
    double alpha_coef,
    bool allow_negative_gain)
    throw()
  {
    // process sub tree
    unsigned long bag_i = Gears::safe_rand(bags.size());
    const BagPart& bag_part = *bags[bag_i];
    const SVM<LabelType>* node_svm = bag_part.svm;
    const FeatureRowsMap& feature_rows = bag_part.bag_holder->feature_rows;

    if(node_svm->grouped_rows.empty())
    {
      return false;
    }

    typedef TreeLearner<LabelType> ThisType;

    if(check_depth == 0)
    {
      // stop tracing
      /*
      if(DEBUG_FEATURE_SEARCH_)
      {
        std::cerr << std::string(10 - check_depth * 2, ' ') <<
          "from get_best_feature_: check_depth = " << check_depth <<
          ", best_feature_id = " << best_feature_id <<
          ", best_gain = " << best_gain <<
          ", delta = " << init_delta << std::endl;
      }
      */

      return false;
    }

    // find best features (by gain)
    unsigned long feature_i = 1;

    Gears::IntrusivePtr<GetBestFeatureResult<ThisType> > result =
      new GetBestFeatureResult<ThisType>(
        &skip_null_features,
        allow_negative_gain);

    Gears::IntrusivePtr<GetBestFeatureParams<ThisType> >
      params = new GetBestFeatureParams<ThisType>();
    params->top_pred = top_pred/* + init_delta*/;
    //params->feature_rows = &feature_rows;
    params->skip_null_features = &skip_null_features;
    //params->node_svm = Gears::add_ref(node_svm);
    params->bags = &bags;
    params->gain_check_bags = gain_check_bags;
    params->check_depth = check_depth;
    params->top_eval = top_eval;
    params->alpha_coef = alpha_coef;

    // check add features
    for(auto feature_it = feature_rows.begin();
      feature_it != feature_rows.end();
      ++feature_it, ++feature_i)
    {
      if(task_runner)
      {
        Gears::Task_var task = new GetBestFeatureTask<ThisType, GainType>(
          result,
          params,
          feature_it->first,
          feature_it->second);

        task_runner->enqueue_task(task);
      }
      else
      {
        PredCollector pred_collector;
        GainType gain_calc;

        double gain;
        LearnTreeHolder_var new_tree;

        result->inc();

        check_feature_(
          gain,
          new_tree,
          pred_collector,
          gain_calc,
          params->top_pred,
          feature_it->first,
          //feature_rows,
          //feature_it->second,
          bags,
          gain_check_bags,
          cur_tree,
          //node_svm,
          check_depth,
          alpha_coef);

        if(GAIN_TRACE)
        {
          Gears::ErrorStream ostr;
          ostr << "GAIN FOR #" << feature_it->first << ": " << gain << std::endl;
          std::cout << ostr.str() << std::endl;
        }

        //gain += (-gain * GAIN_SHARE_PENALTY) + GAIN_ABS_PENALTY;

        result->set(gain, new_tree);
      }
    }

    // check exchange trees
    if(cur_tree)
    {
      for(auto branch_it = cur_tree->branches.begin();
        branch_it != cur_tree->branches.end(); ++branch_it)
      {
      }
    }

    result->wait(feature_rows.size());

    typename GetBestFeatureResult<ThisType>::BestChoose best_choose;
    if(result->get_result(best_choose))
    {
      /*
      std::cout << "best_choose for tree #" <<
        (cur_tree ? cur_tree->tree_id : 0) <<
        ": gain = " << best_choose.gain << ", tree : " << std::endl;

      if(best_choose.tree)
      {
        best_choose.tree->print(std::cout, " ");
      }
      else
      {
        std::cout << "null";
      }
      std::cout << std::endl;
      */

      best_gain = best_choose.gain;
      new_tree = best_choose.tree;
    }

    /*
    if(best_gain < NULL_GAIN && best_gain > -NULL_GAIN) // ~ 0
    {
      new_tree = nullptr;
    }
    else
    {
      // correct best_no_delta, best_yes_delta for alpha coef
    }
    */

    if(DEBUG_FEATURE_SEARCH_)
    {
      std::cerr << std::string(10 - check_depth * 2, ' ') <<
        "from get_best_feature_: check_depth = " << check_depth <<
        //", best_feature_id = " << best_feature_id <<
        ", best_gain = " << best_gain <<
        //", delta = " << init_delta <<
        std::endl;

      if(new_tree)
      {
        new_tree->print(std::cerr, "    ");
      }
    }

    return new_tree; //best_feature_id;
  }

  template<typename LabelType>
  template<typename GainType>
  void
  TreeLearner<LabelType>::check_feature_(
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
    unsigned long /*check_depth*/,
    double /*alpha_coef*/)
    throw()
  {
    double sub_gain = eval_feature_gain_on_bags_(
      new_tree,
      pred_collector,
      gain_calc,
      top_pred,
      feature_id,
      bags,
      cur_tree,
      gain_check_bags
      );

    res_gain = sub_gain; //std::min(sub_gain + GAIN_ABS_PENALTY, 0.0);

    //std::cout << "check_feature_(#" << feature_id << "): res_gain = " << res_gain << std::endl;

    ///*
    if(DEBUG_FEATURE_SEARCH_)
    {
      std::cerr << std::string(10, ' ') <<
        "from check_feature_ #" << feature_id << ": gain = " << res_gain <<
        //", no_delta = " << res_no_delta <<
        //", yes_delta = " << res_yes_delta <<
        std::endl;
      if(new_tree)
      {
        new_tree->print(std::cerr, "  ");
      }
    }
    //*/
  }

  template<typename LabelType>
  struct SVMIterate
  {
    unsigned int feature_index;
    const SVM<LabelType>* svm;
    typename SVM<LabelType>::PredictGroupArray::const_iterator it;
  };

  struct MaskPred
  {
    MaskPred(): count(0) 
    {}

    unsigned long count;
    double pred;
  };

  struct FeatureDelta
  {
    bool
    operator<(const FeatureDelta& right) const
    {
      return feature_id < right.feature_id;
    };

    unsigned long feature_id;
    double no_delta;
    double yes_delta;
  };

  typedef std::vector<FeatureDelta> FeatureDeltaArray;

  template<typename LabelType>
  template<typename GainType>
  double
  TreeLearner<LabelType>::eval_feature_gain_(
    LearnTreeHolder_var& min_new_tree,
    PredCollector& pred_collector,
    GainType& gain_calc,
    double add_delta,
    const LearnTreeHolder* cur_tree,
    unsigned long add_feature_id,
    //const OrderedFeatureArray& features,
    const FeatureRowsMap& feature_rows,
    const SVM<LabelType>* node_svm)
    throw()
  {
    assert(!std::isnan(add_delta));

    const double DELTA_REDUCE = 0.8;
    const double EMPTY_DELTA = 10;

    FeatureDeltaArray features;

    {
      FeatureDelta feature_delta;
      feature_delta.feature_id = add_feature_id;
      feature_delta.no_delta = 0.0;
      feature_delta.yes_delta = 0.0;
      features.push_back(feature_delta);
    }

    if(cur_tree)
    {
      double new_feature_delta = 0;

      for(auto branch_it = cur_tree->branches.begin();
        branch_it != cur_tree->branches.end(); ++branch_it)
      {
        FeatureDelta feature;
        feature.feature_id = branch_it->feature_id;
        /*
        feature.no_delta = 0.0;
        feature.yes_delta = 0.0;
        */

        const double no_delta = branch_it->no_tree.in() ?
          branch_it->no_tree->delta_prob : 0.0; 
        const double yes_delta = branch_it->yes_tree.in() ?
          branch_it->yes_tree->delta_prob : 0.0;
       
        feature.no_delta = no_delta * DELTA_REDUCE;
        feature.yes_delta = yes_delta * DELTA_REDUCE;

        new_feature_delta += std::abs(yes_delta - no_delta) / 2;

        features.push_back(feature);
      }

      if(cur_tree->branches.size() > 0)
      {
        new_feature_delta /= cur_tree->branches.size();
      }

      features.begin()->no_delta = - new_feature_delta + add_delta;
      features.begin()->yes_delta = new_feature_delta + add_delta;
    }
    else
    {
      features.begin()->no_delta = - EMPTY_DELTA + add_delta;
      features.begin()->yes_delta = EMPTY_DELTA + add_delta;
    }

    std::sort(features.begin(), features.end());

    // p = SUM(label) / <rows number>
    // eval p
    // fetch feature rows & fetch node rows
    double old_metric;

    {
      // eval current metric
      gain_calc.start_metric_eval();

      for(auto node_group_it = node_svm->grouped_rows.begin();
        node_group_it != node_svm->grouped_rows.end();
        ++node_group_it)
      {
        gain_calc.add_metric_eval(
          (*node_group_it)->label,
          (*node_group_it)->rows.size());
      }

      old_metric = gain_calc.metric_result();
    }

    pred_collector.start_delta_eval(features.size(), add_delta);

    {
      std::vector<SVMIterate<LabelType> > feature_svm_poses;
      unsigned int feature_index = 0;
      for(auto feature_it = features.begin();
        feature_it != features.end(); ++feature_it, ++feature_index)
      {
        auto feature_rows_it = feature_rows.find(feature_it->feature_id);
        if(feature_rows_it != feature_rows.end())
        {
          SVMIterate<LabelType> svm_it;
          svm_it.feature_index = feature_index;
          svm_it.svm = feature_rows_it->second;
          svm_it.it = feature_rows_it->second->grouped_rows.begin();
          feature_svm_poses.push_back(svm_it);
        }
      }

      for(auto node_group_it = node_svm->grouped_rows.begin();
        node_group_it != node_svm->grouped_rows.end();
        ++node_group_it)
      {
        Gears::IntrusivePtr<BufferPtr<uint64_t> > row_masks =
          BufferProvider<uint64_t>::instance().get();

        row_masks->buf().resize((*node_group_it)->rows.size(), 0);

        for(auto feature_pos_it = feature_svm_poses.begin();
          feature_pos_it != feature_svm_poses.end();
          ++feature_pos_it)
        {
          const uint64_t feature_mask =
            static_cast<uint64_t>(1) << feature_pos_it->feature_index;

          while(feature_pos_it->it != feature_pos_it->svm->grouped_rows.end() &&
            (*feature_pos_it->it)->label < (*node_group_it)->label)
          {
            ++(feature_pos_it->it);
          }

          if(feature_pos_it->it != feature_pos_it->svm->grouped_rows.end() &&
             !((*node_group_it)->label < (*feature_pos_it->it)->label))
          {
            // node_group_it->label == (*feature_pos_it)->it->label
            // cross rows in this group for up bit in masks
            const RowArray& cur_feature_rows = (*feature_pos_it->it)->rows;

            auto row_it = (*node_group_it)->rows.begin();
            auto row_end_it = (*node_group_it)->rows.end();
            auto mask_it = row_masks->buf().begin();
            auto feature_row_it = cur_feature_rows.begin();

            while(row_it != row_end_it &&
              feature_row_it != cur_feature_rows.end())
            {
              if(*row_it < *feature_row_it)
              {
                ++row_it;
                ++mask_it;
              }
              else if(*feature_row_it < *row_it)
              {
                ++feature_row_it;
              }
              else
              {
                *mask_it = *mask_it | feature_mask;
                ++row_it;
                ++mask_it;
                ++feature_row_it;
              }
            }
          }
        }

        Gears::IntrusivePtr<BufferPtr<unsigned long> > mask_counts =
          BufferProvider<uint64_t>::instance().get();

        auto& mask_buf = row_masks->buf();
        auto& mask_count_buf = mask_counts->buf();
        mask_count_buf.resize(static_cast<uint64_t>(1) << features.size(), 0);

        for(auto mask_it = mask_buf.begin(); mask_it != mask_buf.end(); ++mask_it)
        {
          mask_count_buf[*mask_it] += 1;
        }

        uint64_t mask_i = 0;
        for(auto mask_count_it = mask_count_buf.begin();
          mask_count_it != mask_count_buf.end();
          ++mask_count_it, ++mask_i)
        {
          if(*mask_count_it > 0)
          {
            pred_collector.add_delta_eval(
              mask_i,
              (*node_group_it)->label,
              *mask_count_it);
          }
        }
      }
    }

    pred_collector.fin_delta_eval();

    // eval new metrics
    /*
    FloatArray yes_deltas(features.size(), 0.0); //add_delta);
    FloatArray no_deltas(features.size(), 0.0); //add_delta);
    */

    // use add_delta as start point of search
    //FloatArray yes_deltas(features.size(), add_delta);
    //FloatArray no_deltas(features.size(), add_delta);

    double min_gain = 100000000.0;

    for(int mode_i = 0; mode_i < 3; ++mode_i)
    {
      // use old delta values for new point search
      FloatArray yes_deltas(features.size(), 0);
      FloatArray no_deltas(features.size(), 0);

      if(mode_i > 0)
      {
        // noise start point
        const double MAX_NOISE = 8.0; // 1 / (1 + e(-8)) = 0.99966
        for(unsigned long i = 0; i < features.size(); ++i)
        {
          yes_deltas[i] += MAX_NOISE * (Gears::safe_rand(200000) - 100000) / 200000;
          no_deltas[i] += MAX_NOISE * (Gears::safe_rand(200000) - 100000) / 200000;
        }
      }

      {
        auto no_it = no_deltas.begin();
        auto yes_it = yes_deltas.begin();
        for(auto feature_it = features.begin(); feature_it != features.end();
          ++feature_it, ++yes_it, ++no_it)
        {
          assert(!std::isnan(feature_it->yes_delta));
          assert(!std::isnan(feature_it->no_delta));

          *yes_it = feature_it->yes_delta;
          *no_it = feature_it->no_delta;
        }
      }

      gain_calc.delta_result(yes_deltas, no_deltas, pred_collector);

      /*
      for(auto delta_it = yes_deltas.begin(); delta_it != yes_deltas.end(); ++delta_it)
      {
        *delta_it += add_delta;
      }

      for(auto delta_it = no_deltas.begin(); delta_it != no_deltas.end(); ++delta_it)
      {
        *delta_it += add_delta;
      }
      */

      Gears::IntrusivePtr<BufferPtr<MaskPred> > mask_preds =
        BufferProvider<MaskPred>::instance().get();

      gain_calc.start_metric_eval();

      {
        std::vector<SVMIterate<LabelType> > feature_svm_poses;
        unsigned int feature_index = 0;
        for(auto feature_it = features.begin();
          feature_it != features.end(); ++feature_it, ++feature_index)
        {
          auto feature_rows_it = feature_rows.find(feature_it->feature_id);
          if(feature_rows_it != feature_rows.end())
          {
            SVMIterate<LabelType> svm_it;
            svm_it.feature_index = feature_index;
            svm_it.svm = feature_rows_it->second;
            svm_it.it = feature_rows_it->second->grouped_rows.begin();
            feature_svm_poses.push_back(svm_it);
          }
        }

        for(auto node_group_it = node_svm->grouped_rows.begin();
          node_group_it != node_svm->grouped_rows.end();
          ++node_group_it)
        {
          Gears::IntrusivePtr<BufferPtr<uint64_t> > row_masks =
            BufferProvider<uint64_t>::instance().get();

          auto& mask_buf = row_masks->buf();
          mask_buf.resize((*node_group_it)->rows.size(), 0);

          for(auto feature_pos_it = feature_svm_poses.begin();
            feature_pos_it != feature_svm_poses.end();
            ++feature_pos_it)
          {
            const uint64_t feature_mask =
              static_cast<uint64_t>(1) << feature_pos_it->feature_index;

            while(feature_pos_it->it != feature_pos_it->svm->grouped_rows.end() &&
              (*feature_pos_it->it)->label < (*node_group_it)->label)
            {
              ++(feature_pos_it->it);
            }

            if(feature_pos_it->it != feature_pos_it->svm->grouped_rows.end() &&
               !((*node_group_it)->label < (*feature_pos_it->it)->label))
            {
              // node_group_it->label == (*feature_pos_it)->it->label
              // cross rows in this group for up bit in masks
              const RowArray& cur_feature_rows = (*feature_pos_it->it)->rows;

              auto row_it = (*node_group_it)->rows.begin();
              auto mask_it = mask_buf.begin();
              auto feature_row_it = cur_feature_rows.begin();

              while(row_it != (*node_group_it)->rows.end() &&
                feature_row_it != cur_feature_rows.end())
              {
                if(*row_it < *feature_row_it)
                {
                  ++row_it;
                  ++mask_it;
                }
                else if(*feature_row_it < *row_it)
                {
                  ++feature_row_it;
                }
                else
                {
                  *mask_it = *mask_it | feature_mask;
                  ++row_it;
                  ++mask_it;
                  ++feature_row_it;
                }
              }
            }
          }

          Gears::IntrusivePtr<BufferPtr<MaskPred> > mask_preds =
            BufferProvider<MaskPred>::instance().get();

          auto& mask_pred_buf = mask_preds->buf();
          mask_pred_buf.resize(static_cast<uint64_t>(1) << features.size());

          for(auto mask_it = mask_buf.begin(); mask_it != mask_buf.end(); ++mask_it)
          {
            if(mask_pred_buf[*mask_it].count == 0)
            {
              // eval pred
              double cur_pred = add_delta;
              uint64_t mask = *mask_it;
              for(unsigned long feature_index = 0;
                feature_index < features.size(); ++feature_index)
              {
                if(mask & 1)
                {
                  cur_pred += yes_deltas[feature_index];
                }
                else
                {
                  cur_pred += no_deltas[feature_index];
                }

                mask = mask >> 1;
              }

              mask_pred_buf[*mask_it].pred = cur_pred;
            }

            mask_pred_buf[*mask_it].count += 1;
          }

          uint64_t mask_i = 0;
          for(auto mask_pred_it = mask_pred_buf.begin(); mask_pred_it != mask_pred_buf.end();
            ++mask_pred_it, ++mask_i)
          {
            if(mask_pred_it->count > 0)
            {
              gain_calc.add_metric_eval(
                GainType::add_delta((*node_group_it)->label, mask_pred_it->pred),
                mask_pred_it->count);
            }
          }
        }
      }

      const double new_metric = gain_calc.metric_result();
      const double result_gain = new_metric - old_metric;

      // assert
      if(GAIN_TRACE && result_gain > 0.001)
      {
        std::cout << std::endl << "GAIN : " << old_metric << " => " << new_metric << std::endl;

        auto feature_it = features.begin();
        auto no_delta_it = no_deltas.begin();
        for(auto yes_delta_it = yes_deltas.begin(); yes_delta_it != yes_deltas.end();
          ++yes_delta_it, ++no_delta_it, ++feature_it)
        {
          std::cout << "F #" << feature_it->feature_id <<
            ": no = " << *no_delta_it <<
            ", yes = " << *yes_delta_it << std::endl;
        }

        //assert(0);
      }

      // create dtree by deltas
      LearnTreeHolder_var new_tree = new LearnTreeHolder();
      new_tree->delta_gain = result_gain;

      // first no delta contains free argument : move it to second element if need drop it
      assert(!features.empty());

      auto yes_delta_it = yes_deltas.begin();
      auto no_delta_it = no_deltas.begin();
      for(auto feature_it = features.begin();
        feature_it != features.end();
        ++feature_it, ++yes_delta_it, ++no_delta_it)
      {
        const double free_arg = (*yes_delta_it + *no_delta_it) / 2;

        /*
        if(std::abs(*yes_delta_it - *no_delta_it) >= 0.01)
        {
          // add branch
          typename LearnTreeHolder::Branch new_branch;
          new_branch.feature_id = feature_it->feature_id;
          new_branch.yes_tree = new LearnTreeHolder();
          new_branch.yes_tree->delta_prob = *yes_delta_it;
          new_branch.no_tree = new LearnTreeHolder();
          new_branch.no_tree->delta_prob = *no_delta_it;
          new_tree->add_branch(new_branch);
        }
        else
        {
          new_tree->delta_prob += free_arg;
        }
        */

        const double delta = *yes_delta_it - free_arg;

        if(std::abs(delta) >= 0.01)
        {
          assert(!std::isnan(delta));

          // add branch
          typename LearnTreeHolder::Branch new_branch;
          new_branch.feature_id = feature_it->feature_id;
          new_branch.yes_tree = new LearnTreeHolder();
          new_branch.yes_tree->delta_prob = delta;
          new_branch.no_tree = new LearnTreeHolder();
          new_branch.no_tree->delta_prob = -delta;
          new_tree->add_branch(new_branch);
        }

        new_tree->delta_prob += free_arg;
      }

      if(result_gain < min_gain)
      {
        min_gain = result_gain;
        min_new_tree.swap(new_tree);
      }
    }

    return min_gain;
  }

  template<typename LabelType>
  template<typename GainType>
  double
  TreeLearner<LabelType>::eval_remove_gain_on_bags_(
    double& delta,
    PredCollector& pred_collector,
    GainType& gain_calc,
    double add_delta,
    const BagPartArray& bags,
    const typename LearnTreeHolder::Branch& branch)
    throw()
  {
    unsigned long base_bag_i = Gears::safe_rand(bags.size());
    const BagPart& base_bag_part = *bags[base_bag_i];

    double unused_delta;
    double base_gain = eval_init_delta_(
      unused_delta,
      pred_collector,
      gain_calc,
      add_delta,
      base_bag_part.svm,
      LabelType(),
      false);

    SVM_var rem_svm = base_bag_part.svm->copy_pred(
      LearnRevertPredictor<TreeLearner<LabelType>, GainType>(branch));

    double rem_gain = eval_init_delta_(
      delta,
      pred_collector,
      gain_calc,
      add_delta,
      rem_svm,
      LabelType(),
      true);

    if(bags.size() == 1)
    {
      return rem_gain - base_gain;
    }

    double sum_gain = 0;
    unsigned long bag_i = 0;

    for(auto bag_it = bags.begin(); bag_it != bags.end(); ++bag_it, ++bag_i)
    {
      base_gain = eval_init_delta_(
        unused_delta,
        pred_collector,
        gain_calc,
        add_delta,
        (*bag_it)->svm,
        LabelType(),
        false);

      rem_svm = (*bag_it)->svm->copy_pred(
        LearnRevertPredictor<TreeLearner<LabelType>, GainType>(branch));

      rem_gain = eval_init_delta_(
        unused_delta,
        pred_collector,
        gain_calc,
        add_delta + delta,
        rem_svm,
        LabelType(),
        true);

      sum_gain += (rem_gain - base_gain);
    }

    return sum_gain / (bags.size() - 1);
  }

  template<typename LabelType>
  template<typename GainType>
  double
  TreeLearner<LabelType>::eval_feature_gain_on_bags_(
    LearnTreeHolder_var& new_tree,
    PredCollector& pred_collector,
    GainType& gain_calc,
    double add_delta,
    unsigned long feature_id,
    const BagPartArray& bags, // bags already labeled by cur_tree ?
    LearnTreeHolder* cur_tree,
    unsigned long gain_check_bags)
    throw()
  {
    // eval feature gain for optimal node direct childs delta search
    // if new node with feature_id added
    //
    unsigned long base_bag_i = Gears::safe_rand(bags.size());
    const BagPart& base_bag_part = *bags[base_bag_i];

    auto feature_it = base_bag_part.bag_holder->feature_rows.find(feature_id);
    if(feature_it == base_bag_part.bag_holder->feature_rows.end())
    {
      return 0.0;
    }

    OrderedFeatureArray eval_features;

    eval_features.push_back(feature_id);
    if(cur_tree)
    {
      for(auto branch_it = cur_tree->branches.begin();
        branch_it != cur_tree->branches.end(); ++branch_it)
      {
        eval_features.push_back(branch_it->feature_id);
      }
    }

    std::sort(eval_features.begin(), eval_features.end());

    double base_gain = eval_feature_gain_(
      new_tree,
      pred_collector,
      gain_calc,
      add_delta,
      cur_tree,
      feature_id,
      //eval_features,
      base_bag_part.bag_holder->feature_rows,
      base_bag_part.svm);

    //std::cerr << "base_gain = " << base_gain << std::endl;
    if(bags.size() == 1)
    {
      return base_gain;
    }

    BagPartArray check_bags_holder;
    const BagPartArray* check_bags = &bags;

    if(gain_check_bags > 0)
    {
      std::vector<BagPart*> choose_bags;
      choose_bags.reserve(bags.size());
      for(auto b_it = bags.begin(); b_it != bags.end(); ++b_it)
      {
        choose_bags.push_back(*b_it);
      }

      for(unsigned long i = 0; i < gain_check_bags && !choose_bags.empty(); ++i)
      {
        unsigned long check_bag_i = Gears::safe_rand(choose_bags.size());
        check_bags_holder.push_back(Gears::add_ref(choose_bags[check_bag_i]));
        choose_bags.erase(choose_bags.begin() + check_bag_i);
      }

      check_bags = &check_bags_holder;
    }

    double sum_gain = 0;
    unsigned long bag_i = 0;

    for(auto bag_it = check_bags->begin(); bag_it != check_bags->end(); ++bag_it, ++bag_i)
    {
      if(bag_i != base_bag_i)
      {
        feature_it = (*bag_it)->bag_holder->feature_rows.find(feature_id);
        if(feature_it != (*bag_it)->bag_holder->feature_rows.end())
        {
          const double local_gain = eval_feature_gain_by_delta_(
            gain_calc,
            add_delta,
            new_tree,
            (*bag_it)->svm,
            (*bag_it)->bag_holder->feature_rows);

          //std::cerr << "GAIN bag #" << bag_i << " = " << local_gain << std::endl;

          sum_gain += local_gain;
        }
      }
      else if(SELF_CHECK_)
      {
        const double local_gain = eval_feature_gain_by_delta_(
          gain_calc,
          add_delta,
          new_tree,
          (*bag_it)->svm,
          (*bag_it)->bag_holder->feature_rows);

        //std::cerr << "GAIN CHECK, base_gain = " << base_gain << ", local_gain = " << local_gain << std::endl;
        assert(std::abs(local_gain - base_gain) < 0.01);
        (void)local_gain;
      }
    }

    const double gain_on_bags = sum_gain / (bags.size() - 1);
    return gain_on_bags;
  }

  template<typename LabelType>
  void
  TreeLearner<LabelType>::fill_feature_rows(
    FeatureIdArray& features,
    FeatureRowsMap& feature_rows,
    const SVM<LabelType>& svm)
    throw()
  {
    std::deque<unsigned long> features_queue;

    unsigned long row_i = 0;
    for(auto group_it = svm.grouped_rows.begin(); group_it != svm.grouped_rows.end(); ++group_it)
    {
      std::unordered_map<
        unsigned long,
        typename SVM<LabelType>::PredictGroup_var> groups;

      for(auto row_it = (*group_it)->rows.begin();
        row_it != (*group_it)->rows.end(); ++row_it, ++row_i)
      {
        for(auto feature_it = (*row_it)->features.begin();
          feature_it != (*row_it)->features.end(); ++feature_it)
        {
          auto ins_group_it = groups.find(feature_it->first);
          if(ins_group_it == groups.end())
          {
            SVM_var& fr = feature_rows[feature_it->first];
            if(!fr.in())
            {
              fr = new SVM<LabelType>();
              features_queue.push_back(feature_it->first);
            }

            groups.insert(std::make_pair(
              feature_it->first,
              fr->add_row(*row_it, (*group_it)->label)));
          }
          else
          {
            ins_group_it->second->rows.push_back(*row_it);
          }
        }

        if(row_i > 0 && row_i % 100000 == 0)
        {
          std::cerr << row_i << " rows processed" << std::endl;
        }
      }
    }

    features.reserve(features_queue.size());
    std::copy(features_queue.begin(), features_queue.end(), std::back_inserter(features));
  }

  template<typename LabelType>
  typename TreeLearner<LabelType>::Context_var
  TreeLearner<LabelType>::create_context(const SVMArray& svm_array)
  {
    BagPartArray bag_parts;
    for(auto svm_it = svm_array.begin(); svm_it != svm_array.end(); ++svm_it)
    {
      BagHolder_var new_bag_holder = new BagHolder();
      new_bag_holder->bag = *svm_it;
      TreeLearner::fill_feature_rows(
        new_bag_holder->features,
        new_bag_holder->feature_rows,
        **svm_it);

      BagPart_var new_bag_part = new BagPart();
      new_bag_part->bag_holder = new_bag_holder;
      new_bag_part->svm = (*svm_it)->copy();
      bag_parts.push_back(new_bag_part);
    }

    return new Context(bag_parts);
  }

  /*
  template<typename LabelType, typename GainType>
  void
  TreeLearner<LabelType, GainType>::revert_bags_pred_(
    BagPartArray& res_bag_parts,
    const BagPartArray& bag_parts,
    const typename LearnTreeHolder::Branch& branch)
  {
    for(auto bag_it = bag_parts.begin(); bag_it != bag_parts.end(); ++bag_it)
    {
      SVM_var res_svm = (*bag_it)->copy(LearnRevertPredictor(branch));

      BagPart_var bag_part = new BagPart();
      bag_part->bag_holder = (*bag_it)->bag_holder;
      bag_part->svm = res_svm;
      res_bag_parts.push_back(bag_part);
    }
  }
  */

  template<typename LabelType>
  void
  TreeLearner<LabelType>::div_bags_(
    BagPartArray& yes_bag_parts,
    BagPartArray& no_bag_parts,
    const BagPartArray& bag_parts,
    unsigned long feature_id)
  {
    for(auto bag_it = bag_parts.begin(); bag_it != bag_parts.end(); ++bag_it)
    {
      SVM_var yes_svm;
      SVM_var no_svm;
      
      div_rows_(
        yes_svm,
        no_svm,
        feature_id,
        (*bag_it)->svm,
        (*bag_it)->bag_holder->feature_rows);

      BagPart_var yes_bag_part = new BagPart();
      yes_bag_part->bag_holder = (*bag_it)->bag_holder;
      yes_bag_part->svm = yes_svm;
      yes_bag_parts.push_back(yes_bag_part);

      BagPart_var no_bag_part = new BagPart();
      no_bag_part->bag_holder = (*bag_it)->bag_holder;
      no_bag_part->svm = no_svm;
      no_bag_parts.push_back(no_bag_part);
    }
  }

  template<typename LabelType>
  void
  TreeLearner<LabelType>::div_rows_(
    SVM_var& yes_svm,
    SVM_var& no_svm,
    unsigned long feature_id,
    const SVM<LabelType>* div_svm,
    const FeatureRowsMap& feature_rows)
  {
    auto feature_it = feature_rows.find(feature_id);
    if(feature_it != feature_rows.end())
    {
      const SVM<LabelType>* feature_svm = feature_it->second;
      SVM<LabelType>::cross(yes_svm, no_svm, div_svm, feature_svm);
      assert(yes_svm->size() + no_svm->size() == div_svm->size());
    }
    else
    {
      yes_svm = new SVM<LabelType>();
      no_svm = div_svm->copy();
    }
  }

  template<typename LabelType>
  void
  TreeLearner<LabelType>::div_group_(
    typename SVM<LabelType>::PredictGroup_var& yes_group,
    typename SVM<LabelType>::PredictGroup_var& no_group,
    unsigned long feature_id,
    const PredictGroup<LabelType>* div_group,
    const FeatureRowsMap& feature_rows)
  {
    auto feature_it = feature_rows.find(feature_id);
    if(feature_it != feature_rows.end())
    {
      yes_group = new PredictGroup<LabelType>();
      no_group = new PredictGroup<LabelType>();

      std::set_intersection(
        div_group->rows.begin(),
        div_group->rows.end(),
        feature_it->second.all_rows.begin(),
        feature_it->second.all_rows.end(),
        std::back_inserter(yes_group->rows));

      std::set_difference(
        div_group->rows.begin(),
        div_group->rows.end(),
        feature_it->second.all_rows.begin(),
        feature_it->second.all_rows.end(),
        std::back_inserter(no_group->rows));
    }
    else
    {
      yes_group = SVM<LabelType>::PredictGroup_var();
      no_group = Gears::add_ref(div_group);
    }
  }
}
