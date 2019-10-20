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
#include "Predictor.hpp"
#include "MetricPrinter.hpp"

namespace Vanga
{
  struct PredictorLoader
  {
    typedef Predictor::Exception Exception;

    static PredictorType
    load_type(std::istream& istr);

    static const char DTREE_MODEL_HEAD[];
    static const char UNION_MODEL_AVG_HEAD[];
    static const char UNION_MODEL_SUM_HEAD[];
    static const char UNION_MODEL_SUM_HEAD_2[];
  };

  class DTree;
  typedef Gears::IntrusivePtr<DTree> DTree_var;
  typedef Gears::IntrusivePtr<const DTree> ConstDTree_var;

  // DTree
  class DTree: public Gears::AtomicRefCountable
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, Predictor::Exception);

    friend class DTreeBranch;

    struct Branch
    {
      unsigned long feature_id;
      Gears::IntrusivePtr<DTree> yes_tree;
      Gears::IntrusivePtr<DTree> no_tree;
    };

    typedef std::vector<Branch> BranchArray;

  public:
    DTree(): delta_prob(0)
    {}

    void
    save(std::ostream& ostr) const;

    static Gears::IntrusivePtr<DTree>
    load(std::istream& istr, bool with_head = true);

    // features should be sorted
    /*
    double
    predict(const FeatureArray& features) const throw();
    */

    template<typename FeatureResolveFunType>
    double
    fpredict(const FeatureResolveFunType& fun) const throw();

    const BranchArray&
    branches() const
    {
      return branches_;
    }

    template<typename FunType>
    void
    reindex(const FunType& fun) throw();

    DTree_var
    copy() const throw();

    unsigned long
    node_count() const throw();

    template<typename LabelType>
    DTree_var
    filter(
      double min_cover,
      const SVM<LabelType>* svm) throw();

    std::string
    to_string(
      const char* prefix,
      const FeatureDictionary* dict = 0,
      double base = 0.0)
      const throw();

    template<typename LabelType>
    std::string
    to_string_ext(
      const char* prefix,
      const FeatureDictionary* dict,
      double base,
      const SVM<LabelType>* svm,
      const FeatureNameDictionary* name_dict = 0)
      const throw();

    template<typename LabelType>
    std::string
    to_string_ext(
      const char* prefix,
      const FeatureDictionary* dict,
      double base,
      const SVM<LabelType>* svm,
      const MetricPrinter<LabelType>* metric_printer,
      const FeatureNameDictionary* name_dict = 0)
      const throw();

  protected:
    virtual ~DTree() throw() {}

    void
    save_node_(std::ostream& ostr) const;

    template<typename LabelType>
    DTree_var
    filter_(
      double min_cover,
      const SVM<LabelType>* svm,
      unsigned long full_size)
      throw();

    template<typename LabelType>
    std::string
    to_string_ext_(
      const char* prefix,
      const FeatureDictionary* dict,
      double base,
      const SVM<LabelType>* svm,
      unsigned long full_size,
      const MetricPrinter<LabelType>* metric_printer,
      const FeatureNameDictionary* name_dict) const
      throw();

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
      throw();

    // features should be sorted
    double
    predict(const FeatureArray& features) const throw();

    Gears::IntrusivePtr<FastDTree>
    copy() const throw();

    void
    save(std::ostream& ostr) const;

    std::string
    to_string(
      const char* prefix,
      const FeatureDictionary* dict = 0,
      double base = 0.0)
      const throw();

    static Gears::IntrusivePtr<FastDTree>
    load(std::istream& istr, bool with_head = true);

  protected:
    typedef std::vector<uint32_t> FeatureIndexTable;

    class FastFeatureResolveFun
    {
    public:
      FastFeatureResolveFun(
        const FeatureIndexTable& eval_feature_indexes)
        throw();

      std::pair<bool, uint32_t>
      get(uint32_t) const;

    protected:
      const FeatureIndexTable& eval_feature_indexes_;
    };

  protected:
    FastDTree() throw();

    virtual
    ~FastDTree() throw() = default;

  protected:
    mutable FeatureIndexTable eval_feature_indexes_;
    DTree_var reindexed_dtree_;
  };

  typedef Gears::IntrusivePtr<FastDTree> FastDTree_var;
  */
}

#include "DTree.tpp"

#endif /*DTREE_HPP_*/
