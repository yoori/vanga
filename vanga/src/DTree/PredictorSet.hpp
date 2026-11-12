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

#ifndef PREDICTORSET_HPP_
#define PREDICTORSET_HPP_

#include "FloatFeatures.hpp"

namespace Vanga
{
  // PredictorSet
  class PredictorSet : public Gears::AtomicRefCountable
  {
  public:
    using Exception = Predictor::Exception;

  public:
    PredictorSet();

    template <typename IteratorType> PredictorSet(IteratorType begin, IteratorType end);

    template <typename PredictorType> void add(PredictorType* tree, double coef = 1.0);

    template <typename FeatureSetType>
    double fpredict(const FeatureSetType& features) const noexcept;

    void save(std::ostream& ostr) const;

    std::string to_string(const char* prefix, const FeatureDictionary* dict = 0,
      double base = 0.0) const noexcept;

    static Gears::IntrusivePtr<PredictorSet> load(std::istream& istr, bool with_head = true);

    /*
    const std::vector<std::pair<Predictor_var, double> >&
    predictors() const;
    */
  protected:
    struct PredictorDelegate : public Gears::AtomicRefCountable
    {
      virtual double predict(const FeatureArray&) noexcept = 0;

      virtual double predict(const FloatFeatureArray&) noexcept = 0;

      virtual void save(std::ostream& ostr) const = 0;

      virtual std::string
      to_string(const char* prefix, const FeatureDictionary* dict, double base) const noexcept = 0;
    };

    using PredictorDelegate_var = Gears::IntrusivePtr<PredictorDelegate>;

    template <typename PredictorType> class PredictorHolder : public PredictorDelegate
    {
    public:
      PredictorHolder(PredictorType* predictor);

      virtual double predict(const FeatureArray& features) noexcept;

      virtual double predict(const FloatFeatureArray& features) noexcept;

      void save(std::ostream& ostr) const;

      std::string
      to_string(const char* prefix, const FeatureDictionary* dict, double base) const noexcept;

    protected:
      Gears::IntrusivePtr<PredictorType> predictor_;
    };

  protected:
    virtual ~PredictorSet() noexcept;

  protected:
    std::vector<std::pair<PredictorDelegate_var, double>> predictors_;
  };

  using PredictorSet_var = Gears::IntrusivePtr<PredictorSet>;
}

#include "PredictorSet.tpp"

#endif /*PREDICTORSET_HPP_*/
