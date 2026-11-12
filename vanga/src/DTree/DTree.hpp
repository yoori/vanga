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

#ifndef DTREE_HPP_
#define DTREE_HPP_

#include <iostream>
#include <map>
#include <vector>

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/Time.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>

#include "Label.hpp"
#include "SVM.hpp"
#include "FloatFeatures.hpp"
#include "Predictor.hpp"
#include "MetricPrinter.hpp"

namespace Vanga
{
  struct PredictorLoader
  {
    using Exception = Predictor::Exception;

    static PredictorType load_type(std::istream& istr);

    static const char DTREE_MODEL_HEAD[];
    static const char NUMERIC_DTREE_MODEL_HEAD[];
    static const char UNION_MODEL_AVG_HEAD[];
    static const char UNION_MODEL_SUM_HEAD[];
    static const char UNION_MODEL_SUM_HEAD_2[];
  };

  class DTree;
  using DTree_var = Gears::IntrusivePtr<DTree>;
  using ConstDTree_var = Gears::IntrusivePtr<const DTree>;

  // DTree
  class DTree : public Gears::AtomicRefCountable
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, Predictor::Exception);

    friend class DTreeBranch;
    friend class NumericTreeLearner;

    struct Branch
    {
      unsigned long feature_id;
      Gears::IntrusivePtr<DTree> yes_tree;
      Gears::IntrusivePtr<DTree> no_tree;
      bool numeric = false;
      float border = 0;
      bool nan_go_right = false;
    };

    using BranchArray = std::vector<Branch>;

  public:
    DTree();

    // Import scalar CatBoost oblivious trees with FloatFeature splits.
    // CatBoost flat feature index N maps to Vanga/LibSVM feature ID N + 1.
    // Predict using FloatFeatureArray to retain numeric values (including NaN).
    explicit DTree(std::istream& catboost_json);
    explicit DTree(const std::string& catboost_json);

    bool uses_numeric_features() const noexcept;

    void save(std::ostream& ostr) const;

    static Gears::IntrusivePtr<DTree>
    load(std::istream& istr, bool with_head = true, bool numeric_format = false);

    // features should be sorted
    /*
    double
    predict(const FeatureArray& features) const noexcept;
    */

    template <typename FeatureResolveFunType>
    double fpredict(const FeatureResolveFunType& fun) const noexcept;

    const BranchArray& branches() const;

    template <typename FunType> void reindex(const FunType& fun) noexcept;

    DTree_var copy() const noexcept;

    unsigned long node_count() const noexcept;

    template <typename LabelType>
    DTree_var filter(double min_cover, const SVM<LabelType>* svm) noexcept;

    std::string to_string(const char* prefix, const FeatureDictionary* dict = 0,
      double base = 0.0) const noexcept;

    template <typename LabelType>
    std::string to_string_ext(const char* prefix, const FeatureDictionary* dict, double base,
      const SVM<LabelType>* svm, const FeatureNameDictionary* name_dict = 0) const noexcept;

    template <typename LabelType>
    std::string to_string_ext(const char* prefix, const FeatureDictionary* dict, double base,
      const SVM<LabelType>* svm, const MetricPrinter<LabelType>* metric_printer,
      const FeatureNameDictionary* name_dict = 0) const noexcept;

  protected:
    virtual ~DTree() noexcept;

    void save_node_(std::ostream& ostr) const;

    void load_catboost_(std::istream& istr);

    bool numeric_model_ = false;

    template <typename LabelType>
    DTree_var filter_(double min_cover, const SVM<LabelType>* svm, unsigned long full_size)
      noexcept;

    template <typename LabelType>
    std::string to_string_ext_(const char* prefix, const FeatureDictionary* dict, double base,
      const SVM<LabelType>* svm, unsigned long full_size,
      const MetricPrinter<LabelType>* metric_printer, const FeatureNameDictionary* name_dict) const
      noexcept;

  public:
    unsigned long tree_id;
    double delta_prob;
    BranchArray branches_;

    /*
    unsigned long tree_id;
    unsigned long feature_id;
    IntrusivePtr<DTree> yes_tree;
    IntrusivePtr<DTree> no_tree;
    */
  };

  // DTree with reduced feature indexes
  // thread unsafe - can't be used in few threads conrurrently
  /*
  class FastDTree: public Predictor
  {
  public:
    FastDTree(DTree* tree, unsigned long table_size = 0xFFFF)
      noexcept;

    // features should be sorted
    double
    predict(const FeatureArray& features) const noexcept;

    Gears::IntrusivePtr<FastDTree>
    copy() const noexcept;

    void
    save(std::ostream& ostr) const;

    std::string
    to_string(
      const char* prefix,
      const FeatureDictionary* dict = 0,
      double base = 0.0)
      const noexcept;

    static Gears::IntrusivePtr<FastDTree>
    load(std::istream& istr, bool with_head = true);

  protected:
    typedef std::vector<uint32_t> FeatureIndexTable;

    class FastFeatureResolveFun
    {
    public:
      FastFeatureResolveFun(
        const FeatureIndexTable& eval_feature_indexes)
        noexcept;

      std::pair<bool, uint32_t>
      get(uint32_t) const;

    protected:
      const FeatureIndexTable& eval_feature_indexes_;
    };

  protected:
    FastDTree() noexcept;

    virtual
    ~FastDTree() noexcept = default;

  protected:
    mutable FeatureIndexTable eval_feature_indexes_;
    DTree_var reindexed_dtree_;
  };

  typedef Gears::IntrusivePtr<FastDTree> FastDTree_var;
  */
}

#include "DTree.tpp"

#endif /*DTREE_HPP_*/
