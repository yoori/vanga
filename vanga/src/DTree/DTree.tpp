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

#include <sstream>
#include <cmath>

namespace Vanga
{
  // DTree impl
  template<typename LabelType>
  std::string
  DTree::to_string_ext_(
    const char* prefix,
    const FeatureDictionary* dict,
    double base,
    const SVM<LabelType>* svm,
    unsigned long full_size,
    const MetricPrinter<LabelType>* metric_printer,
    const FeatureNameDictionary* name_dict) const
    throw()
  {
    std::ostringstream ostr;
    ostr << prefix << "{" << tree_id << "}";

    ostr << ": " << (delta_prob > 0 ? "+" : "") << delta_prob <<
      " = " << (base + delta_prob);

    if(svm)
    {
      ostr << "(cover = " << (static_cast<double>(svm->size()) * 100.0 / full_size) << "%)";
      if(metric_printer)
      {
        ostr << ": ";
        metric_printer->print(ostr, svm);
      }
    }

    ostr << std::endl;

    for(auto branch_it = branches_.begin(); branch_it != branches_.end(); ++branch_it)
    {
      ostr << prefix << "+  feature #" << branch_it->feature_id;

      if(dict)
      {
        auto dict_it = dict->find(branch_it->feature_id);

        if(dict_it != dict->end())
        {
          ostr << " [" << dict_it->second;
          if(name_dict)
          {
            auto name_it = name_dict->find(dict_it->second);
            if(name_it != name_dict->end())
            {
              ostr << "," << name_it->second;
            }
          }
          ostr << "]";
        }
      }

      ostr << ":";

      if(svm)
      {
        ostr << " cover = " << (static_cast<double>(svm->size()) * 100.0 / full_size) << "%";
      }

      ostr << std::endl;

      if(branch_it->yes_tree)
      {
        Gears::IntrusivePtr<SVM<LabelType> > yes_svm =
          svm ?
          svm->by_feature(branch_it->feature_id, true) :
          Gears::IntrusivePtr<SVM<LabelType> >();

        ostr << prefix << "+    yes =>" << std::endl <<
          branch_it->yes_tree->to_string_ext_(
            (std::string(prefix) + ">    >    ").c_str(),
            dict,
            base + delta_prob,
            yes_svm.in(),
            full_size,
            metric_printer,
            name_dict);
      }

      if(branch_it->no_tree)
      {
        Gears::IntrusivePtr<SVM<LabelType> > no_svm =
          svm ?
          svm->by_feature(branch_it->feature_id, false) :
          Gears::IntrusivePtr<SVM<LabelType> >();

        ostr << prefix << "+    no =>" << std::endl <<
          branch_it->no_tree->to_string_ext_(
            (std::string(prefix) + ">    >    ").c_str(),
            dict,
            base + delta_prob,
            no_svm.in(),
            full_size,
            metric_printer,
            name_dict);
      }
    }

    return ostr.str();
  }

  template<typename FunType>
  void
  DTree::reindex(const FunType& fun)
    throw()
  {
    for(auto branch_it = branches_.begin(); branch_it != branches_.end(); ++branch_it)
    {
      branch_it->feature_id = fun(branch_it->feature_id);

      if(branch_it->yes_tree)
      {
        branch_it->yes_tree->reindex(fun);
      }

      if(branch_it->no_tree)
      {
        branch_it->no_tree->reindex(fun);
      }
    }
  }

  template<typename LabelType>
  std::string
  DTree::to_string_ext(
    const char* prefix,
    const FeatureDictionary* dict,
    double base,
    const SVM<LabelType>* svm,
    const FeatureNameDictionary* name_dict)
    const throw()
  {
    return to_string_ext_<LabelType>(
      prefix,
      dict,
      base,
      svm,
      svm ? svm->size() : 0,
      0,
      name_dict);
  }

  template<typename LabelType>
  std::string
  DTree::to_string_ext(
    const char* prefix,
    const FeatureDictionary* dict,
    double base,
    const SVM<LabelType>* svm,
    const MetricPrinter<LabelType>* metric_printer,
    const FeatureNameDictionary* name_dict)
    const throw()
  {
    return to_string_ext_(
      prefix,
      dict,
      base,
      svm,
      svm ? svm->size() : 0,
      metric_printer,
      name_dict);
  }

  template<typename LabelType>
  DTree_var
  DTree::filter(
    double min_cover,
    const SVM<LabelType>* svm) throw()
  {
    return filter_(min_cover, svm, svm ? svm->size() : 0);
  }

  template<typename LabelType>
  DTree_var
  DTree::filter_(
    double min_cover,
    const SVM<LabelType>* svm,
    unsigned long full_size) throw()
  {
    const double cover = static_cast<double>(svm->size()) / full_size;

    if(cover < min_cover)
    {
      return DTree_var();
    }

    BranchArray res_branches;

    for(auto branch_it = branches_.begin(); branch_it != branches_.end(); ++branch_it)
    {
      Branch branch(*branch_it);

      if(branch.yes_tree)
      {
        Gears::IntrusivePtr<SVM<LabelType> > yes_svm =
          svm ?
          svm->by_feature(branch_it->feature_id, true) :
          Gears::IntrusivePtr<SVM<LabelType> >();

        branch.yes_tree = branch.yes_tree->filter_(min_cover, yes_svm.in(), full_size);
      }

      if(branch.no_tree)
      {
        Gears::IntrusivePtr<SVM<LabelType> > no_svm =
          svm ?
          svm->by_feature(branch_it->feature_id, false) :
          Gears::IntrusivePtr<SVM<LabelType> >();

        branch.no_tree = branch.no_tree->filter_(min_cover, no_svm.in(), full_size);
      }

      res_branches.push_back(branch);
    }

    DTree_var res(new DTree(*this));
    res->branches_.swap(res_branches);
    return res;
  }

  template<typename FeatureSetType>
  double
  DTree::fpredict(const FeatureSetType& feature_set) const throw()
  {
    double res = delta_prob;

    for(auto branch_it = branches_.begin(); branch_it != branches_.end(); ++branch_it)
    {
      if(feature_set.get(branch_it->feature_id).first)
      {
        // yes tree
        res += branch_it->yes_tree ? branch_it->yes_tree->fpredict(feature_set) : 0.0;
      }
      else
      {
        // no tree
        res += branch_it->no_tree ? branch_it->no_tree->fpredict(feature_set) : 0.0;
      }
    }

    return res;
  }
}
