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

#ifndef PREDICTOR_HPP_
#define PREDICTOR_HPP_

#include <cstddef>
#include <iostream>
#include <map>
#include <vector>
#include <deque>

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>

#include "SVM.hpp"

namespace Vanga
{
  enum PredictorType
  {
    PT_NONE = 1,
    PT_DTREE,
    PT_SET,
    PT_NUMERIC_DTREE
  };

  using FeatureNameDictionary = std::map<std::string, std::string>;

  using FeatureDictionary = std::map<unsigned long, std::string>;

  struct PredArrayHolder : public Gears::AtomicRefCountable
  {
    using PredArray = std::deque<double>;

    PredArray values;

  protected:
    virtual ~PredArrayHolder() noexcept;
  };

  inline PredArrayHolder::~PredArrayHolder() noexcept
  {
  }

  using PredArrayHolder_var = Gears::IntrusivePtr<PredArrayHolder>;

  namespace Predictor
  {
    DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);
  }

  // Predictor
  /*
  class Predictor: public virtual Gears::AtomicRefCountable
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);

  public:
    virtual double
    predict(const FeatureArray& features) const noexcept = 0;

    PredArrayHolder_var
    predict(const RowArray& rows) const noexcept;

    template<typename LabelType>
    PredArrayHolder_var
    predict(const SVM<LabelType>* svm) const noexcept;

    virtual void
    save(std::ostream& ostr) const = 0;

    virtual std::string
    to_string(
      const char* prefix,
      const FeatureDictionary* dict = 0,
      double base = 0.0)
      const noexcept = 0;

  protected:
    virtual ~Predictor() noexcept {}
  };

  typedef Gears::IntrusivePtr<Predictor> Predictor_var;
  */
}

#include "Predictor.tpp"

#endif /*PREDICTOR_HPP_*/
