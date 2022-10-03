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

#ifndef SVM_HPP_
#define SVM_HPP_

#include <vector>
#include <deque>
#include <algorithm>
#include <iostream>

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>

namespace Vanga
{
  struct FeatureLess
  {
    bool
    operator()(unsigned long left, const std::pair<unsigned long, unsigned long>& right)
      const
    {
      return left < right.first;
    }

    bool
    operator()(const std::pair<unsigned long, unsigned long>& left, unsigned long right)
      const
    {
      return left.first < right;
    }

    bool
    operator()(
      const std::pair<unsigned long, unsigned long>& left,
      const std::pair<unsigned long, unsigned long>& right)
      const
    {
      return left.first < right.first;
    }
  };

  struct FeatureArray: public std::vector<std::pair<uint32_t, uint32_t> >
  {
    std::pair<bool, uint32_t>
    get(uint32_t feature_id) const
    {
      if(std::binary_search(
        this->begin(),
        this->end(),
        feature_id,
        FeatureLess()))
      {
        return std::make_pair(true, 1);
      }
      else
      {
        return std::make_pair(false, 0);
      }
    }
  };
  
  //typedef std::vector<std::pair<uint32_t, uint32_t> > FeatureArray;

  struct Row: public Gears::AtomicRefCountable
  {
    FeatureArray features;

  protected:
    virtual ~Row() throw() {}
  };

  typedef Gears::IntrusivePtr<Row> Row_var;

  //typedef std::deque<Row_var> RowArray;
  typedef std::vector<Row_var> RowArray;

  template<typename LabelType>
  struct PredictGroup: public Gears::AtomicRefCountable
  {
    LabelType label;
    RowArray rows;

  protected:
    virtual ~PredictGroup() throw() = default;
  };

  // SVM
  template<typename LabelType>
  struct SVM: public Gears::AtomicRefCountable
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);

    typedef Gears::IntrusivePtr<PredictGroup<LabelType> >
      PredictGroup_var;

    typedef std::vector<PredictGroup_var> PredictGroupArray;

  public:
    PredictGroupArray grouped_rows;

  public:
    void
    dump() const;

    Gears::IntrusivePtr<SVM<LabelType> >
    copy() const throw();

    template<typename LabelAdapterType>
    Gears::IntrusivePtr<SVM<typename LabelAdapterType::ResultType> >
    copy(const LabelAdapterType& label_adapter) const throw();

    template<typename LabelAdapterType>
    Gears::IntrusivePtr<SVM<typename LabelAdapterType::ResultType> >
    copy_pred(const LabelAdapterType& label_adapter) const throw();

    static void
    cross(
      Gears::IntrusivePtr<SVM>& cross_svm,
      Gears::IntrusivePtr<SVM>& diff_svm,
      const SVM* left_svm,
      const SVM* right_svm)
      throw();

    static void
    cross_count(
      unsigned long& cross_count,
      unsigned long& left_diff_count,
      unsigned long& right_diff_count,
      const SVM<LabelType>* left_svm,
      const SVM<LabelType>* right_svm)
      throw();

    Gears::IntrusivePtr<SVM<LabelType> >
    part(unsigned long res_size) const throw();

    std::pair<Gears::IntrusivePtr<SVM<LabelType> >, Gears::IntrusivePtr<SVM<LabelType> > >
    div(unsigned long res_size) const throw();

    void
    portions_div(
      std::vector<Gears::IntrusivePtr<SVM<LabelType> > >& res,
      unsigned long portions_num)
      const throw();

    PredictGroup_var
    add_row(Row* row, const LabelType& label)
      throw();

    double
    label_float_sum() const throw();

    unsigned long
    size() const throw();

    void
    print_labels(std::ostream& ostr)
      throw();

    static Gears::IntrusivePtr<SVM<LabelType> >
    load(std::istream& in, unsigned long lines = 0)
      /*throw(Exception)*/;

    static Row_var
    load_line(
      std::istream& in,
      LabelType& label_value);

    void
    save(std::ostream& out) const
      /*throw(Exception)*/;

    static void
    save_line(
      std::ostream& out,
      const Row* row,
      const LabelType& label_value);

    Gears::IntrusivePtr<SVM<LabelType> >
    by_feature(unsigned long feature_id, bool yes)
      const;

    template<typename ContainerType>
    Gears::IntrusivePtr<SVM<LabelType> >
    filter(const ContainerType& features)
      const;

  public:
    void
    sort_() throw();

    static void
    save_line_(
      std::ostream& out,
      const LabelType& label_value,
      const FeatureArray& features);

    static bool
    nocheck_load_line_(
      std::istream& in,
      LabelType& label_value,
      FeatureArray& features);

    static Row_var
    load_line_(
      std::istream& in,
      LabelType& label_value,
      FeatureArray& features);

  protected:
    Row_var
    get_row_(LabelType& label_value, unsigned long pos)
      const throw();

  protected:
    virtual ~SVM() throw() {}
  };

  /*
  typedef ReferenceCounting::ConstPtr<SVM> CSVM_var;
  typedef Gears::IntrusivePtr<SVM> SVM_var;

  typedef std::vector<SVM_var> SVMArray;
  */
}

#include "SVM.tpp"

#endif /*SVM_HPP_*/
