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

//#include <LogCommons/LogCommons.hpp>
//#include <LogCommons/LogCommons.ipp>

#include "DTree.hpp"
#include "SVM.hpp"

namespace Vanga
{
  const char PredictorLoader::DTREE_MODEL_HEAD[] = "dtree";
  const char PredictorLoader::UNION_MODEL_AVG_HEAD[] = "union-avg";
  const char PredictorLoader::UNION_MODEL_SUM_HEAD[] = "union-sum";
  const char PredictorLoader::UNION_MODEL_SUM_HEAD_2[] = "union";

  struct DTreeLoadHelper
  {
    struct Branch
    {
      unsigned long feature_id;
      unsigned long yes_tree_id;
      unsigned long no_tree_id;
    };

    double delta_prob;
    std::vector<Branch> branches;

    DTree_var resolved_tree;
  };

  class FastDTreeReindexFun
  {
  public:
    FastDTreeReindexFun(unsigned long table_size)
      : table_size_(table_size)
    {}

    unsigned long
    operator()(unsigned long feature_id) const throw()
    {
      return feature_id % table_size_;
    }

  protected:
    const unsigned long table_size_;
  };

  // DTree impl
  void
  DTree::save(std::ostream& ostr) const
  {
    ostr << PredictorLoader::DTREE_MODEL_HEAD << std::endl;

    ostr.setf(std::ios::fixed, std::ios::floatfield);
    ostr.precision(7);

    this->save_node_(ostr);
  }

  void
  DTree::save_node_(std::ostream& ostr) const
  {
    ostr << tree_id << '\t' << delta_prob << '\t';
    for(auto branch_it = branches_.begin(); branch_it != branches_.end(); ++branch_it)
    {
      ostr << (branch_it != branches_.begin() ? "|" : "") <<
        branch_it->feature_id << ":" <<
        (branch_it->yes_tree ? branch_it->yes_tree->tree_id : 0) << ":" <<
        (branch_it->no_tree ? branch_it->no_tree->tree_id : 0);
    }
    ostr << std::endl;

    for(auto branch_it = branches_.begin(); branch_it != branches_.end(); ++branch_it)
    {
      if(branch_it->yes_tree)
      {
        branch_it->yes_tree->save_node_(ostr);
      }

      if(branch_it->no_tree)
      {
        branch_it->no_tree->save_node_(ostr);
      }
    }
  }

  Gears::IntrusivePtr<DTree>
  DTree::load(std::istream& istr, bool with_head)
  {
    unsigned long root_tree_id = 0;
    std::map<unsigned long, DTreeLoadHelper> trees;
    unsigned long line_i = 0;

    if(with_head)
    {
      std::string line;
      std::getline(istr, line);
      if(line != PredictorLoader::DTREE_MODEL_HEAD)
      {
        std::ostringstream ostr;
        ostr << "DTree::load(): invalid model type: '" << line << "'(" << line.size() << ")";
        throw Exception(ostr.str());
      }
    }

    while(!istr.eof())
    {
      std::string line;

      try
      {
        std::getline(istr, line);

        if(line.empty())
        {
          break;
        }

        if(istr.fail())
        {
          throw Exception("read failed or empty line");
        }

        Gears::CategoryTokenizer<Gears::Ascii::SepTab> tokenizer(line);
        Gears::SubString tree_id_str;
        if(!tokenizer.get_token(tree_id_str))
        {
          throw Exception("no id");
        }

        Gears::SubString delta_prob_str;
        if(!tokenizer.get_token(delta_prob_str))
        {
          throw Exception("no 'delta prob'");
        }

        // parse branches
        Gears::SubString branches_str;
        tokenizer.get_token(branches_str);

        unsigned long tree_id;
        DTreeLoadHelper dtree_load_helper;

        if(!Gears::StringManip::str_to_int(tree_id_str, tree_id))
        {
          std::ostringstream ostr;
          ostr << "invalid tree id value: '" << tree_id_str << "'";
          throw Exception(ostr.str());
        }

        {
          std::istringstream istr(delta_prob_str.str().c_str());
          istr >> dtree_load_helper.delta_prob;

          if(!istr.eof() || istr.fail())
          {
            std::ostringstream ostr;
            ostr << "invalid delta prob value: '" << delta_prob_str << "'";
            throw Exception(ostr.str());
          }
        }

        Gears::CategoryTokenizer<
          Gears::Ascii::SepBar> branches_tokenizer(branches_str);

        Gears::SubString branch_str;
        while(branches_tokenizer.get_token(branch_str))
        {
          // parse branch
          Gears::CategoryTokenizer<Gears::Ascii::SepColon> branch_tokenizer(branch_str);

          Gears::SubString feature_id_str;
          if(!branch_tokenizer.get_token(feature_id_str))
          {
            throw Exception("no 'feature id' in branch");
          }

          Gears::SubString yes_tree_id_str;
          if(!branch_tokenizer.get_token(yes_tree_id_str))
          {
            throw Exception("no 'yes tree id' in branch");
          }

          Gears::SubString no_tree_id_str;
          if(!branch_tokenizer.get_token(no_tree_id_str))
          {
            throw Exception("no 'no tree id' in branch");
          }

          DTreeLoadHelper::Branch branch;

          if(!Gears::StringManip::str_to_int(feature_id_str, branch.feature_id))
          {
            std::ostringstream ostr;
            ostr << "invalid 'feature id' value: '" << feature_id_str << "'";
            throw Exception(ostr.str());
          }

          if(!Gears::StringManip::str_to_int(yes_tree_id_str, branch.yes_tree_id))
          {
            std::ostringstream ostr;
            ostr << "invalid 'yes tree id' value: '" << yes_tree_id_str << "'";
            throw Exception(ostr.str());
          }

          if(!Gears::StringManip::str_to_int(no_tree_id_str, branch.no_tree_id))
          {
            std::ostringstream ostr;
            ostr << "invalid 'no tree id' value: '" << no_tree_id_str << "'";
            throw Exception(ostr.str());
          }

          dtree_load_helper.branches.push_back(branch);
        }

        if(root_tree_id == 0)
        {
          root_tree_id = tree_id;
        }

        trees.insert(std::make_pair(tree_id, dtree_load_helper));
      }
      catch(const Gears::Exception& ex)
      {
        std::ostringstream ostr;
        ostr << "Can't load tree (line #" << line_i << "): " << ex.what() <<
          ", line='" << line << "'";
        throw Exception(ostr.str());
      }

      ++line_i;
    }

    // create stub trees
    for(auto tree_it = trees.begin(); tree_it != trees.end(); ++tree_it)
    {
      DTree_var dtree = new DTree();
      dtree->tree_id = tree_it->first;
      dtree->delta_prob = tree_it->second.delta_prob;
      //dtree->feature_id = tree_it->second.feature_id;
      tree_it->second.resolved_tree = dtree;
    }

    // link trees
    for(auto tree_it = trees.begin(); tree_it != trees.end(); ++tree_it)
    {
      assert(tree_it->second.resolved_tree.in());

      // link branches
      for(auto branch_it = tree_it->second.branches.begin();
        branch_it != tree_it->second.branches.end(); ++branch_it)
      {
        DTree::Branch resolved_branch;
        resolved_branch.feature_id = branch_it->feature_id;

        if(branch_it->yes_tree_id)
        {
          auto yes_tree_it = trees.find(branch_it->yes_tree_id);
          if(yes_tree_it == trees.end())
          {
            std::ostringstream ostr;
            ostr << "can't resolve branch yes tree id: " << branch_it->yes_tree_id;
            throw Exception(ostr.str());
          }

          resolved_branch.yes_tree = yes_tree_it->second.resolved_tree;
        }

        if(branch_it->no_tree_id)
        {
          auto no_tree_it = trees.find(branch_it->no_tree_id);
          if(no_tree_it == trees.end())
          {
            std::ostringstream ostr;
            ostr << "can't resolve branch no tree id: " << branch_it->no_tree_id;
            throw Exception(ostr.str());
          }

          resolved_branch.no_tree = no_tree_it->second.resolved_tree;
        }

        tree_it->second.resolved_tree->branches_.push_back(resolved_branch);
      }
    }

    auto root_tree_it = trees.find(root_tree_id);
    assert(root_tree_it != trees.end());

    return root_tree_it->second.resolved_tree;
  }

  /*
  double
  DTree::predict(const FeatureArray& features) const throw()
  {
    double res = delta_prob;

    for(auto branch_it = branches_.begin(); branch_it != branches_.end(); ++branch_it)
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
  */

  std::string
  DTree::to_string(
    const char* prefix,
    const FeatureDictionary* dict,
    double base) const
    throw()
  {
    std::ostringstream ostr;
    ostr << prefix << "" << "{" << tree_id << "}";

    ostr << ": " << (delta_prob > 0 ? "+" : "") << delta_prob <<
      " = " << (base + delta_prob) <<
      "(p = " << (1.0 / (1.0 + std::exp(-(base + delta_prob)))) << ")" <<
      std::endl;

    for(auto branch_it = branches_.begin(); branch_it != branches_.end(); ++branch_it)
    {
      ostr << prefix << "+   feature #" << branch_it->feature_id;
      if(dict)
      {
        auto dict_it = dict->find(branch_it->feature_id);
        if(dict_it != dict->end())
        {
          ostr << " [" << dict_it->second << "]";
        }
      }

      ostr << std::endl;

      if(branch_it->yes_tree)
      {
        ostr << prefix << "  yes =>" << std::endl <<
          branch_it->yes_tree->to_string(
            (std::string(prefix) + "+   >   ").c_str(),
            dict,
            base);
      }

      if(branch_it->no_tree)
      {
        ostr << prefix << "  no =>" << std::endl <<
          branch_it->no_tree->to_string(
            (std::string(prefix) + "+   >   ").c_str(),
            dict,
            base);
      }
    }

    return ostr.str();
  }

  DTree_var
  DTree::copy() const throw()
  {
    DTree_var res = new DTree();
    res->tree_id = tree_id;
    res->delta_prob = delta_prob;
    for(auto branch_it = branches_.begin(); branch_it != branches_.end(); ++branch_it)
    {
      Branch branch;
      branch.feature_id = branch_it->feature_id;
      branch.yes_tree = branch_it->yes_tree ? branch_it->yes_tree->copy() : DTree_var();
      branch.no_tree = branch_it->no_tree ? branch_it->no_tree->copy() : DTree_var();
      res->branches_.push_back(branch);
    }

    return res;
  }

  unsigned long
  DTree::node_count() const throw()
  {
    unsigned long res = 1;
    for(auto branch_it = branches_.begin(); branch_it != branches_.end(); ++branch_it)
    {
      res += (branch_it->yes_tree ? branch_it->yes_tree->node_count() : 0) +
        (branch_it->no_tree ? branch_it->no_tree->node_count() : 0);
    }
    
    return res;
  }

  /*
  FastDTree::FastFeatureResolveFun::FastFeatureResolveFun(
    const FeatureIndexTable& eval_feature_indexes)
    throw()
    : eval_feature_indexes_(eval_feature_indexes)
  {}

  inline
  std::pair<bool, uint32_t>
  FastDTree::FastFeatureResolveFun::get(uint32_t feature_id) const
  {
    return std::make_pair(eval_feature_indexes_[feature_id], 1);
  }

  FastDTree::FastDTree(DTree* tree, unsigned long table_size)
    throw()
  {
    eval_feature_indexes_.resize(table_size, 0);
    reindexed_dtree_ = tree->copy();
    FastDTreeReindexFun reindex_fun(table_size);
    reindexed_dtree_->reindex(reindex_fun);
  }

  FastDTree::FastDTree() throw()
  {}

  double
  FastDTree::predict(const FeatureArray& features) const throw()
  {
    // set features in table
    //uint32_t eval_feature_indexes_size = eval_feature_indexes_.size();

    for(auto feature_it = features.begin(); feature_it != features.end(); ++feature_it)
    {
      eval_feature_indexes_[feature_it->first & 0xFFFF] = feature_it->second;
    }

    FastFeatureResolveFun feature_fun(eval_feature_indexes_);
    double res = reindexed_dtree_->fpredict(feature_fun);

    // rollback features in table
    for(auto feature_it = features.begin(); feature_it != features.end(); ++feature_it)
    {
      eval_feature_indexes_[feature_it->first & 0xFFFF] = 0;
    }

    return res;
  }

  FastDTree_var
  FastDTree::copy() const throw()
  {
    FastDTree_var res = new FastDTree();
    res->eval_feature_indexes_ = eval_feature_indexes_;
    res->reindexed_dtree_ = reindexed_dtree_;
    return res;
  }

  void
  FastDTree::save(std::ostream& ostr) const
  {
    reindexed_dtree_->save(ostr);
  }

  std::string
  FastDTree::to_string(
    const char* prefix,
    const FeatureDictionary* dict,
    double base)
    const throw()
  {
    return reindexed_dtree_->to_string(prefix, dict, base);
  }
  */

  // PredictorLoader
  PredictorType
  PredictorLoader::load_type(std::istream& istr)
  {
    std::string line;
    std::getline(istr, line);

    if(line == DTREE_MODEL_HEAD)
    {
      return PT_DTREE;
    }
    else if(line == UNION_MODEL_SUM_HEAD || line == UNION_MODEL_SUM_HEAD_2)
    {
      return PT_SET;
    }

    return PT_NONE;
  }

  /*
  DTree_var
  PredictorLoader::load_dtree(std::istream& istr)
  {
  }
  
  Predictor_var
  PredictorLoader::load(std::istream& istr)
  {
    PredictorType predictor_type = load_type(istr);

    if(predictor_type == PT_DTREE)
    {
      DTree::load(
    }
    else if(predictor_type == PT_SET)
    {
    }

    if(line == DTREE_MODEL_HEAD)
    {
      return DTree::load(istr, false);
    }
    else if(line == UNION_MODEL_SUM_HEAD || line == UNION_MODEL_SUM_HEAD_2)
    {
      return PredictorSet::load(istr, false);
    }
    else
    {
      std::ostringstream ostr;
      ostr << "PredictorLoader::load(): invalid model type: '" << line << "'";
      throw Exception(ostr.str());
    }
  }
  */
}
