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

#include <Gears/Basic/Rand.hpp>
#include <Gears/Basic/OutputMemoryStream.hpp>
#include <Gears/Basic/SubString.hpp>
#include <Gears/Basic/StringManip.hpp>
#include <Gears/String/AsciiStringManip.hpp>
#include <Gears/String/Tokenizer.hpp>

namespace Vanga
{
  struct FirstLess
  {
    template<typename ArgType>
    bool
    operator()(const ArgType& left, const ArgType& right)
      const
    {
      return left.first < right.first;
    }
  };

  struct FirstEqual
  {
    template<typename ArgType>
    bool
    operator()(const ArgType& left, const ArgType& right)
      const
    {
      return left.first == right.first;
    }
  };

  struct PredictGroupLess
  {
    template<typename LabelType>
    bool
    operator()(const PredictGroup<LabelType>* left, const PredictGroup<LabelType>* right)
      const throw()
    {
      return left->label < right->label;
    }

    template<typename LabelType>
    bool
    operator()(const PredictGroup<LabelType>* left, const LabelType& right)
      const throw()
    {
      return left->label < right;
    }

    template<typename LabelType>
    bool
    operator()(const LabelType& left, const PredictGroup<LabelType>* right)
      const throw()
    {
      return left < right->label;
    }

    template<typename LabelType>
    bool
    operator()(
      const Gears::IntrusivePtr<PredictGroup<LabelType> >& left,
      const LabelType& right)
      const throw()
    {
      return left->label < right;
    }

    template<typename LabelType>
    bool
    operator()(const LabelType& left, const Gears::IntrusivePtr<PredictGroup<LabelType> >& right)
      const throw()
    {
      return left < right->label;
    }
  };

  namespace
  {
    class CountIterator:
      public std::iterator<std::output_iterator_tag, void, void, void, void>
    {
    public:
      CountIterator()
        : count_(0)
      {}

      template<typename ValueType>
      CountIterator&
      operator=(const ValueType& /*val*/)
      {
        return *this;
      }

      CountIterator& operator*()
      {
        return *this;
      }

      CountIterator& operator++()
      {
        ++count_;
        return *this;
      }

      CountIterator operator++(int)
      {
        ++count_;
        return *this;
      }

      unsigned long
      count() const
      {
        return count_;
      }

    private:
      unsigned long count_;
    };

    template<typename ContainerType>
    unsigned long
    binsearch_count_cross_rows(const ContainerType& left, const ContainerType& right)
    {
      // fetch left with lookup to right
      unsigned long count = 0;
      auto right_begin_it = right.begin();
      for(auto it = left.begin(); it != left.end(); ++it)
      {
        right_begin_it = std::lower_bound(right_begin_it, right.end(), *it);

        if(right_begin_it == right.end())
        {
          break;
        }
        else if(*right_begin_it == *it)
        {
          ++count;
        }
      }

      return count;
    }

    template<typename ContainerType>
    unsigned long
    count_cross_rows(const ContainerType& left, const ContainerType& right)
    {
      // left and right sorted
      size_t left_size = left.size();
      size_t right_size = right.size();

      if(right_size > left_size * 12)
      {
        // fetch left with lookup to right
        return binsearch_count_cross_rows(left, right);
      }
      else if(left_size > right_size * 12)
      {
        // fetch right with lookup to left
        return binsearch_count_cross_rows(right, left);
      }

      // fetch intersect
      CountIterator counter = std::set_intersection(
        left.begin(),
        left.end(),
        right.begin(),
        right.end(),
        CountIterator());
      return counter.count();
    }

    bool
    str_to_double(double& value, const Gears::SubString& str)
    {
      std::istringstream istr(str.str());
      istr >> value;
      return (istr.eof() && !istr.fail());
    }
  }

  // SVM impl
  template<typename LabelType>
  void
  SVM<LabelType>::dump() const
  {
    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      for(auto row_it = (*group_it)->rows.begin(); row_it != (*group_it)->rows.end(); ++row_it)
      {
        std::cout << "ROW(" << (*group_it)->label.to_float() << ")" << std::endl;
      }
    }
  }

  template<typename LabelType>
  void
  SVM<LabelType>::save(std::ostream& out) const
    /*throw(Exception)*/
  {
    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      for(auto row_it = (*group_it)->rows.begin(); row_it != (*group_it)->rows.end(); ++row_it)
      {
        save_line(out, *row_it, row_it->label);
      }
    }
  }

  template<typename LabelType>
  void
  SVM<LabelType>::save_line_(
    std::ostream& out,
    const LabelType& label_value,
    const FeatureArray& features)
  {
    label_value.save(out);
    for(auto feature_it = features.begin(); feature_it != features.end(); ++feature_it)
    {
      out << ' ' << feature_it->first << ':' << feature_it->second;
    }
    out << std::endl;
  }

  template<typename LabelType>
  void
  SVM<LabelType>::save_line(
    std::ostream& out,
    const Row* row,
    const LabelType& label_value)
  {
    save_line_(out, label_value, row->features);
  }

  template<typename LabelType>
  Row_var
  SVM<LabelType>::load_line(
    std::istream& in,
    LabelType& label_value)
  {
    FeatureArray features;
    return load_line_(in, label_value, features);
  }

  template<typename LabelType>
  Gears::IntrusivePtr<SVM<LabelType> >
  SVM<LabelType>::copy() const throw()
  {
    Gears::IntrusivePtr<SVM<LabelType> > res = new SVM<LabelType>();

    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      res->grouped_rows.push_back(new PredictGroup<LabelType>(**group_it));
    }

    return res;
  }

  template<typename LabelType>
  template<typename LabelAdapterType>
  Gears::IntrusivePtr<SVM<typename LabelAdapterType::ResultType> >
  SVM<LabelType>::copy_pred(const LabelAdapterType& label_adapter) const throw()
  {
    Gears::IntrusivePtr<SVM<typename LabelAdapterType::ResultType> > res =
      new SVM<typename LabelAdapterType::ResultType>();

    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      for(auto row_it = (*group_it)->rows.begin(); row_it != (*group_it)->rows.end(); ++row_it)
      {
        res->add_row(*row_it, label_adapter(*row_it, (*group_it)->label));
      }
    }

    res->sort_();

    return res;
  }

  template<typename LabelType>
  template<typename LabelAdapterType>
  Gears::IntrusivePtr<SVM<typename LabelAdapterType::ResultType> >
  SVM<LabelType>::copy(const LabelAdapterType& label_adapter) const throw()
  {
    return this->copy_pred(label_adapter);
  }

  template<typename LabelType>
  void
  SVM<LabelType>::cross(
    Gears::IntrusivePtr<SVM<LabelType> >& cross_svm,
    Gears::IntrusivePtr<SVM<LabelType> >& diff_svm,
    const SVM<LabelType>* left_svm,
    const SVM<LabelType>* right_svm)
    throw()
  {
    cross_svm = new SVM<LabelType>();
    diff_svm = new SVM<LabelType>();

    cross_svm->grouped_rows.reserve(left_svm->grouped_rows.size());
    diff_svm->grouped_rows.reserve(left_svm->grouped_rows.size());

    auto left_group_it = left_svm->grouped_rows.begin();
    auto right_group_it = right_svm->grouped_rows.begin();

    while(left_group_it != left_svm->grouped_rows.end() &&
      right_group_it != right_svm->grouped_rows.end())
    {
      if((*right_group_it)->label < (*left_group_it)->label)
      {
        // non cross rows
        ++right_group_it;
      }
      else if((*left_group_it)->label < (*right_group_it)->label)
      {
        //diff_svm->grouped_rows.push_back(new PredictGroup<LabelType>(**left_group_it));
        diff_svm->grouped_rows.push_back(*left_group_it);

        ++left_group_it;
      }
      else // (*left_group_it)->label == (*right_group_it)->label
      {
        if((*left_group_it) == (*right_group_it))
        {
          // equal groups : cross is full
          cross_svm->grouped_rows.push_back(*left_group_it);
        }
        else
        {
          PredictGroup_var cross_group = new PredictGroup<LabelType>();
          cross_group->label = (*left_group_it)->label;

          cross_group->rows.reserve(std::min((*left_group_it)->rows.size(), (*right_group_it)->rows.size()));

          std::set_intersection(
            (*left_group_it)->rows.begin(),
            (*left_group_it)->rows.end(),
            (*right_group_it)->rows.begin(),
            (*right_group_it)->rows.end(),
            std::back_inserter(cross_group->rows));

          if(!cross_group->rows.empty())
          {
            cross_svm->grouped_rows.push_back(cross_group);
          }

          PredictGroup_var diff_group;

          if(!cross_group->rows.empty())
          {
            diff_group = new PredictGroup<LabelType>();
            diff_group->label = (*left_group_it)->label;

            diff_group->rows.reserve((*left_group_it)->rows.size() - cross_group->rows.size());

            std::set_difference(
              (*left_group_it)->rows.begin(),
              (*left_group_it)->rows.end(),
              (*right_group_it)->rows.begin(),
              (*right_group_it)->rows.end(),
              std::back_inserter(diff_group->rows));
          }
          else
          {
            diff_group = (*left_group_it);
          }

          if(!diff_group->rows.empty())
          {
            diff_svm->grouped_rows.push_back(diff_group);
          }
        }

        ++left_group_it;
        ++right_group_it;
      }
    }

    // process tail
    while(left_group_it != left_svm->grouped_rows.end())
    {
      diff_svm->grouped_rows.push_back(*left_group_it);
      ++left_group_it;
    }
  }

  template<typename LabelType>
  void
  SVM<LabelType>::cross_count(
    unsigned long& cross_count,
    unsigned long& left_diff_count,
    unsigned long& right_diff_count,
    const SVM<LabelType>* left_svm,
    const SVM<LabelType>* right_svm)
    throw()
  {
    cross_count = 0;
    left_diff_count = 0;
    right_diff_count = 0;

    auto left_group_it = left_svm->grouped_rows.begin();
    auto right_group_it = right_svm->grouped_rows.begin();

    while(left_group_it != left_svm->grouped_rows.end() &&
      right_group_it != right_svm->grouped_rows.end())
    {
      if((*right_group_it)->label < (*left_group_it)->label)
      {
        right_diff_count += (*right_group_it)->rows.size();

        ++right_group_it;
      }
      else if((*left_group_it)->label < (*right_group_it)->label)
      {
        left_diff_count += (*left_group_it)->rows.size();

        ++left_group_it;
      }
      else // (*left_group_it)->label == (*right_group_it)->label
      {
        if((*left_group_it) == (*right_group_it))
        {
          // equal groups : cross is full
          cross_count += (*left_group_it)->rows.size();
        }
        else
        {
          CountIterator counter = std::set_intersection(
            (*left_group_it)->rows.begin(),
            (*left_group_it)->rows.end(),
            (*right_group_it)->rows.begin(),
            (*right_group_it)->rows.end(),
            CountIterator());

          cross_count += counter.count();

          CountIterator left_counter = std::set_difference(
            (*left_group_it)->rows.begin(),
            (*left_group_it)->rows.end(),
            (*right_group_it)->rows.begin(),
            (*right_group_it)->rows.end(),
            CountIterator());

          left_diff_count += left_counter.count();

          CountIterator right_counter = std::set_difference(
            (*right_group_it)->rows.begin(),
            (*right_group_it)->rows.end(),
            (*left_group_it)->rows.begin(),
            (*left_group_it)->rows.end(),
            CountIterator());

          right_diff_count += right_counter.count();
        }

        ++left_group_it;
        ++right_group_it;
      }
    }

    // process tails
    while(left_group_it != left_svm->grouped_rows.end())
    {
      left_diff_count += (*left_group_it)->rows.size();
      ++left_group_it;
    }

    while(right_group_it != right_svm->grouped_rows.end())
    {
      right_diff_count += (*right_group_it)->rows.size();
      ++right_group_it;
    }
  }

  template<typename LabelType>
  Gears::IntrusivePtr<SVM<LabelType> >
  SVM<LabelType>::part(unsigned long res_size)
    const throw()
  {
    Gears::IntrusivePtr<SVM<LabelType> > res = new SVM<LabelType>();
    const unsigned long cur_size = size();

    if(cur_size > 0)
    {
      for(unsigned long i = 0; i < res_size; ++i)
      {
        unsigned long pos = Gears::safe_rand(cur_size);
        LabelType label_value;
        Row_var row = get_row_(label_value, pos);
        res->add_row(row, label_value);
      }
    }

    res->sort_();

    return res;
  }

  template<typename LabelType>
  void
  SVM<LabelType>::portions_div(
    std::vector<Gears::IntrusivePtr<SVM<LabelType> > >& res,
    unsigned long portions_num)
    const throw()
  {
    for(unsigned long i = 0; i < portions_num; ++i)
    {
      res.push_back(new SVM<LabelType>());
    }

    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      for(auto row_it = (*group_it)->rows.begin(); row_it != (*group_it)->rows.end(); ++row_it)
      {
        unsigned long portion_i = Gears::safe_rand(portions_num);
        res[portion_i]->add_row(*row_it, (*group_it)->label);
      }
    }
  }

  template<typename LabelType>
  std::pair<Gears::IntrusivePtr<SVM<LabelType> >, Gears::IntrusivePtr<SVM<LabelType> > >
  SVM<LabelType>::div(unsigned long res_size)
    const throw()
  {
    Gears::IntrusivePtr<SVM<LabelType> > res_first = new SVM<LabelType>();
    Gears::IntrusivePtr<SVM<LabelType> > res_second = new SVM<LabelType>();

    unsigned long cur_size = this->size();

    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      for(auto row_it = (*group_it)->rows.begin(); row_it != (*group_it)->rows.end(); ++row_it)
      {
        unsigned long pos = Gears::safe_rand(cur_size);
        if(pos < res_size)
        {
          res_first->add_row(*row_it, (*group_it)->label);
        }
        else
        {
          res_second->add_row(*row_it, (*group_it)->label);
        }
      }
    }

    res_first->sort_();
    res_second->sort_();

    return std::make_pair(res_first, res_second);
  }

  template<typename LabelType>
  Gears::IntrusivePtr<SVM<LabelType> >
  SVM<LabelType>::by_feature(unsigned long feature_id, bool yes)
    const
  {
    Gears::IntrusivePtr<SVM<LabelType> > res = new SVM<LabelType>();

    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      for(auto row_it = (*group_it)->rows.begin(); row_it != (*group_it)->rows.end(); ++row_it)
      {
        bool found = std::binary_search(
          (*row_it)->features.begin(),
          (*row_it)->features.end(),
          std::pair<uint32_t, uint32_t>(feature_id, 0),
          FirstLess());

        if(yes && found)
        {
          res->add_row(*row_it, (*group_it)->label);
        }
        else if(!yes && !found)
        {
          res->add_row(*row_it, (*group_it)->label);
        }
      }
    }

    res->sort_();

    return res;    
  }

  template<typename LabelType>
  double
  SVM<LabelType>::label_float_sum() const throw()
  {
    double res = 0.0;
    
    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      res += (*group_it)->label.to_float() * (*group_it)->rows.size();
    }

    return res;
  }

  template<typename LabelType>
  unsigned long
  SVM<LabelType>::size() const throw()
  {
    unsigned long res_size = 0;
    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      res_size += (*group_it)->rows.size();
    }
    return res_size;
  }

  template<typename LabelType>
  Gears::IntrusivePtr<SVM<LabelType> >
  SVM<LabelType>::load(std::istream& in, unsigned long lines)
    /*throw(Exception)*/
  {
    unsigned long line_i = 0;
    Gears::IntrusivePtr<SVM<LabelType> > svm(new SVM<LabelType>());
    FeatureArray features;
    features.reserve(10240);

    while(!in.eof() && (lines == 0 || line_i < lines))
    {
      LabelType label;
      Row_var new_row = load_line_(in, label, features);

      if(new_row)
      {
        svm->add_row(new_row, label);
      }

      ++line_i;

      if(line_i % 100000 == 0)
      {
        std::cerr << "loaded " << line_i << " lines" << std::endl;
      }
    }

    std::cerr << "loading finished (" << line_i << " lines)" << std::endl;

    svm->sort_();

    std::cerr << "rows sorted" << std::endl;

    //std::cout << "GROUPS: " << svm->grouped_rows.size() << std::endl;
    //svm->dump();
    //std::cout << "^^^^^^^^^^^^^^^^^^" << std::endl;

    return svm;
  }

  template<typename LabelType>
  typename SVM<LabelType>::PredictGroup_var
  SVM<LabelType>::add_row(Row* row, const LabelType& label)
    throw()
  {
    // find group
    auto found_it = std::lower_bound(
      grouped_rows.begin(),
      grouped_rows.end(),
      label,
      PredictGroupLess());

    if(found_it == grouped_rows.end() || label < (*found_it)->label)
    {
      PredictGroup_var new_group = new PredictGroup<LabelType>();
      new_group->label = label;
      found_it = grouped_rows.insert(found_it, new_group);
    }

    (*found_it)->rows.push_back(Gears::add_ref(row));

    return *found_it;
  }

  template<typename LabelType>
  void
  SVM<LabelType>::print_labels(std::ostream& ostr)
    throw()
  {
    for(auto row_it = grouped_rows.begin(); row_it != grouped_rows.end(); ++row_it)
    {
      //ostr << "label = ";
      (*row_it)->label.print(ostr);
      ostr << ": " << (*row_it)->rows.size() << std::endl;
    }
  }

  template<typename LabelType>
  template<typename ContainerType>
  Gears::IntrusivePtr<SVM<LabelType> >
  SVM<LabelType>::filter(const ContainerType& filter_features)
    const
  {
    Gears::IntrusivePtr<SVM<LabelType> > res_svm = new SVM<LabelType>();

    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      PredictGroup_var new_group = new PredictGroup<LabelType>();
      new_group->label = (*group_it)->label;

      for(auto row_it = (*group_it)->rows.begin(); row_it != (*group_it)->rows.end(); ++row_it)
      {
        Row_var row = new Row();
        FeatureArray filtered_features;

        for(auto it = row->features.begin(); it != row->features.end(); ++it)
        {
          if(filter_features.find(it->first) != filter_features.end())
          {
            filtered_features.push_back(*it);
          }
        }

        row->features.swap(filtered_features);
        new_group->rows.push_back(row);
      }

      res_svm->grouped_rows.push_back(new_group);
    }

    return res_svm;
  }

  template<typename LabelType>
  bool
  SVM<LabelType>::nocheck_load_line_(
    std::istream& in,
    LabelType& label_value,
    FeatureArray& features)
  {
    features.clear();

    std::string line;
    std::getline(in, line);
    if(line.empty())
    {
      return false;
    }

    Gears::CategoryRepeatableTokenizer<
      Gears::Ascii::SepSpace> tokenizer(line);
    Gears::SubString token;
    bool label = true;

    while(tokenizer.get_token(token))
    {
      if(label)
      {
        label = false;
        label_value.load(token.str());
      }
      else
      {
        Gears::SubString::SizeType pos = token.find(':');
        Gears::SubString feature_value_str = token.substr(0, pos);
        unsigned long feature_value = 0;
        if(!Gears::StringManip::str_to_int(feature_value_str, feature_value))
        {
          Gears::ErrorStream ostr;
          ostr << "can't parse feature '" << feature_value_str << "'";
          throw Exception(ostr);
        }

        unsigned long value = 1;
        if(pos != Gears::SubString::NPOS)
        {
          Gears::SubString value_str = token.substr(pos + 1);
          double dvalue;
          if(!str_to_double(dvalue, value_str))
          {
            Gears::ErrorStream ostr;
            ostr << "can't parse feature value '" << value_str << "'";
            throw Exception(ostr);
          }
          value = (dvalue > 0.0000001 ? 1 : 0);
        }

        features.push_back(std::make_pair(feature_value, value));
      }
    }

    return true;
  }

  template<typename LabelType>
  Row_var
  SVM<LabelType>::load_line_(
    std::istream& in,
    LabelType& label_value,
    FeatureArray& features)
  {
    features.clear();

    std::string line;
    std::getline(in, line);
    if(line.empty())
    {
      return 0;
    }

    Gears::CategoryRepeatableTokenizer<
      Gears::Ascii::SepSpace> tokenizer(line);
    Gears::SubString token;
    bool label = true;

    while(tokenizer.get_token(token))
    {
      if(label)
      {
        label = false;
        label_value.load(token.str());
        //std::cerr << "label_value = " << label_value << std::endl;

        /*
        if(!Gears::StringManip::str_to_int(token, label_value))
        {
          Gears::ErrorStream ostr;
          ostr << "can't parse label '" << token << "'";
          throw Exception(ostr);
        }
        */
      }
      else
      {
        Gears::SubString::SizeType pos = token.find(':');
        Gears::SubString feature_value_str = token.substr(0, pos);
        unsigned long feature_value = 0;
        if(!Gears::StringManip::str_to_int(feature_value_str, feature_value))
        {
          std::ostringstream ostr;
          ostr << "can't parse feature '" << feature_value_str.str() << "'";
          throw Exception(ostr.str());
        }

        unsigned long value = 1;
        if(pos != Gears::SubString::NPOS)
        {
          Gears::SubString value_str = token.substr(pos + 1);
          double dvalue;
          if(!str_to_double(dvalue, value_str))
          {
            std::ostringstream ostr;
            ostr << "can't parse feature value '" << value_str << "'";
            throw Exception(ostr.str());
          }
          value = (dvalue > 0.0000001 ? 1 : 0);
        }

        features.push_back(std::make_pair(feature_value, value));
      }
    }

    std::sort(features.begin(), features.end(), FirstLess());
    auto erase_begin_it = std::unique(features.begin(), features.end(), FirstEqual());
    features.erase(erase_begin_it, features.end());

    // create Row
    Row_var new_row(new Row());
    new_row->features.swap(features);
    return new_row;
  }

  template<typename LabelType>
  Row_var
  SVM<LabelType>::get_row_(LabelType& label, unsigned long pos)
    const throw()
  {
    for(auto group_it = grouped_rows.begin();
      group_it != grouped_rows.end(); ++group_it)
    {
      if(pos < (*group_it)->rows.size())
      {
        label = (*group_it)->label;
        return (*group_it)->rows[pos];
      }

      pos -= (*group_it)->rows.size();
    }

    return Row_var();
  }

  template<typename LabelType>
  void
  SVM<LabelType>::sort_() throw()
  {
    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      assert(!(*group_it)->rows.empty());
      std::sort((*group_it)->rows.begin(), (*group_it)->rows.end());
    }
  }
}
