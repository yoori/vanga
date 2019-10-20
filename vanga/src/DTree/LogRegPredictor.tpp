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

#include <cmath>

namespace Vanga
{
  // LogRegPredictor
  template<typename NextPredictorType>
  LogRegPredictor<NextPredictorType>::LogRegPredictor(
    NextPredictorType* predictor) throw()
    : predictor_(Gears::add_ref(predictor))
  {}

  template<typename NextPredictorType>
  template<typename FeatureResolveFunType>
  double
  LogRegPredictor<NextPredictorType>::fpredict(
    const FeatureResolveFunType& features) const throw()
  {
    const double DOUBLE_ONE = 1.0;
    return DOUBLE_ONE / (
      DOUBLE_ONE + std::exp(predictor_ ? -predictor_->fpredict(features) : 0.0));
  }

  template<typename NextPredictorType>
  void
  LogRegPredictor<NextPredictorType>::save(std::ostream& ostr) const
  {
    predictor_->save(ostr);
  }

  template<typename NextPredictorType>
  std::string
  LogRegPredictor<NextPredictorType>::to_string(
    const char* prefix,
    const FeatureDictionary* dict,
    double base)
    const throw()
  {
    return predictor_->to_string(prefix, dict, base);
  }
}
