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

namespace Vanga
{
  template <typename PredictorType>
  PredictorSet::PredictorHolder<PredictorType>::PredictorHolder(PredictorType* predictor)
    : predictor_(Gears::add_ref(predictor))
  {
  }

  template <typename PredictorType>
  double PredictorSet::PredictorHolder<PredictorType>::predict(const FeatureArray& features) throw()
  {
    return predictor_->fpredict(features);
  }

  template <typename PredictorType>
  double PredictorSet::PredictorHolder<PredictorType>::
    predict(const FloatFeatureArray& features) throw()
  {
    return predictor_->fpredict(features);
  }

  template <typename PredictorType>
  void PredictorSet::PredictorHolder<PredictorType>::save(std::ostream& ostr) const
  {
    predictor_->save(ostr);
  }

  template <typename PredictorType>
  std::string PredictorSet::PredictorHolder<PredictorType>::
    to_string(const char* prefix, const FeatureDictionary* dict, double base) const throw()
  {
    return predictor_->to_string(prefix, dict, base);
  }

  inline PredictorSet::~PredictorSet() throw()
  {
  }

  // PredictorSet impl
  inline PredictorSet::PredictorSet()
  {
  }

  template <typename IteratorType> PredictorSet::PredictorSet(IteratorType begin, IteratorType end)
  {
    for (; begin != end; ++begin)
    {
      add(begin->first, begin->second);
    }
  }

  template <typename PredictorType> void PredictorSet::add(PredictorType* predictor, double coef)
  {
    predictors_.push_back(std::make_pair(new PredictorHolder<PredictorType>(predictor), coef));
  }

  template <typename FeatureSetType>
  double PredictorSet::fpredict(const FeatureSetType& features) const throw()
  {
    double sum = 0.0;
    for (auto it = predictors_.begin(); it != predictors_.end(); ++it)
    {
      sum += it->second * it->first->predict(features);
    }
    return sum;
  }

  void PredictorSet::save(std::ostream& ostr) const
  {
    ostr << PredictorLoader::UNION_MODEL_SUM_HEAD << std::endl;

    for (auto it = predictors_.begin(); it != predictors_.end(); ++it)
    {
      ostr << it->second << std::endl;
      //ostr << ">>>>>>>> TO PREDICTOR SAVE" << std::endl;
      it->first->save(ostr);
      //ostr << ">>>>>>>> FROM PREDICTOR SAVE" << std::endl;
      ostr << std::endl;
    }
  }

  Gears::IntrusivePtr<PredictorSet> PredictorSet::load(std::istream& istr, bool with_head)
  {
    PredictorSet_var res = new PredictorSet();

    if (with_head)
    {
      std::string line;
      std::getline(istr, line);
      if (line == PredictorLoader::UNION_MODEL_SUM_HEAD || line == PredictorLoader::
                                                                     UNION_MODEL_SUM_HEAD_2)
      {
      }
      else
      {
        std::ostringstream ostr;
        ostr << "PredictorSet::load(): invalid model type: '" << line << "'";
        throw Exception(ostr.str());
      }
    }

    std::string line;
    while (std::getline(istr, line) && !line.empty())
    {
      double coef;
      std::istringstream coef_stream(line);
      coef_stream >> coef;

      if (!coef_stream.eof() || coef_stream.fail())
      {
        std::ostringstream ostr;
        ostr << "invalid coef value: '" << line << "'";
        throw Exception(ostr.str());
      }

      PredictorType predictor_type = PredictorLoader::load_type(istr);

      if (predictor_type == PT_DTREE || predictor_type == PT_NUMERIC_DTREE)
      {
        DTree_var sub_predictor = DTree::load(istr, false, predictor_type == PT_NUMERIC_DTREE);
        res->add(sub_predictor.in(), coef);
      }
      else if (predictor_type == PT_SET)
      {
        PredictorSet_var sub_predictor = PredictorSet::load(istr, false);
        res->add(sub_predictor.in(), coef);
      }
      else
      {
        std::ostringstream ostr;
        ostr << "unknown predictor type";
        throw Exception(ostr.str());
      }
    }

    if (istr.bad() || (istr.fail() && !istr.eof()))
    {
      throw Exception("PredictorSet::load(): read failed");
    }

    return res;
  }

  std::string PredictorSet::
    to_string(const char* prefix, const FeatureDictionary* dict, double base) const throw()
  {
    std::ostringstream ostr;

    unsigned long predictor_i = 0;
    for (auto predictor_it = predictors_.begin(); predictor_it != predictors_.end();
      ++predictor_it, ++predictor_i)
    {
      ostr << prefix << "Predictor #" << predictor_i << ":" << std::endl
           << predictor_it->first->to_string((std::string(prefix) + "  ").c_str(), dict, base)
           << std::endl;
    }

    return ostr.str();
  }
}
