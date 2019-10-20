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

#ifndef LOGREGPREDICTOR_HPP_
#define LOGREGPREDICTOR_HPP_

#include <Gears/Basic/AtomicRefCountable.hpp>

namespace Vanga
{
  // LogRegPredictor
  template<typename NextPredictorType>
  class LogRegPredictor: public virtual Gears::AtomicRefCountable
  {
  public:
    LogRegPredictor(NextPredictorType* predictor) throw();

    template<typename FeatureResolveFunType>
    double
    fpredict(const FeatureResolveFunType& fun) const throw();

    virtual void
    save(std::ostream& ostr) const;

    virtual std::string
    to_string(
      const char* prefix,
      const FeatureDictionary* dict = 0,
      double base = 0.0)
      const throw();

  protected:
    virtual
    ~LogRegPredictor() throw() {}

  protected:
    Gears::IntrusivePtr<NextPredictorType> predictor_;
  };

  // PredictedBoolLabelLogRegAddConverter
  template<typename PredictorType>
  struct PredictedBoolLabelLogRegAddConverter
  {
  public:
    typedef PredictedBoolLabel ResultType;

  public:
    PredictedBoolLabelLogRegAddConverter(PredictorType* predictor)
      : predictor_(Gears::add_ref(predictor))
    {}

    PredictedBoolLabel
    operator()(const Row* row, const PredictedBoolLabel& label) const
    {
      PredictedBoolLabel converted_label = label;

      if(predictor_)
      {
        converted_label.pred = label.pred + predictor_->fpredict(row->features);
      }
      else
      {
        converted_label.pred = label.pred;
      }

      return converted_label;
    }

  protected:
    Gears::IntrusivePtr<PredictorType> predictor_;
  };
}

#include "LogRegPredictor.tpp"

#endif /*LOGREGPREDICTOR_HPP_*/
