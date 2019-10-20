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

#ifndef LABEL_HPP_
#define LABEL_HPP_

#include <Gears/Basic/SubString.hpp>
#include "Predictor.hpp"

namespace Vanga
{
  DECLARE_GEARS_EXCEPTION(LabelException, Gears::DescriptiveException);

  struct BoolLabel
  {
    bool value;

    BoolLabel();

    void
    load(const Gears::SubString& str);

    void
    save(std::ostream& ostr) const;

    bool
    orig() const;

    double
    to_float() const;

    bool
    operator<(const BoolLabel& right) const;

    BoolLabel
    operator+(const BoolLabel& right) const throw();

    void
    print(std::ostream& ostr) const throw();
  };

  struct PredictedBoolLabel: public BoolLabel
  {
    double pred;

    PredictedBoolLabel();

    PredictedBoolLabel(bool v, double pred_val)
      : pred(pred_val)
    {
      value = v;
    }

    void
    load(const Gears::SubString& str);

    PredictedBoolLabel
    operator+(const PredictedBoolLabel& right) const throw();

    bool
    operator<(const PredictedBoolLabel& right) const;

    void
    print(std::ostream& ostr) const throw();
  };

  // PredictedFloatLabel
  struct PredictedFloatLabel
  {
    double value;
    double pred;

    PredictedFloatLabel();

    PredictedFloatLabel(double v, double pred_val)
      : value(v),
        pred(pred_val)
    {}

    double
    orig() const;

    void
    load(const Gears::SubString& str);

    PredictedFloatLabel
    operator+(const PredictedFloatLabel& right) const throw();

    bool
    operator<(const PredictedFloatLabel& right) const;

    void
    print(std::ostream& ostr) const throw();
  };

  // PredictedBoolLabelDTreeAddConverter
  template<typename PredictorType>
  struct PredictedBoolLabelAddConverter
  {
  public:
    typedef PredictedBoolLabel ResultType;

  public:
    PredictedBoolLabelAddConverter(PredictorType* predictor)
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

  struct PredictedBoolLabelAnnealer
  {
  public:
    typedef PredictedBoolLabel ResultType;

  public:
    PredictedBoolLabelAnnealer()
    {}

    PredictedBoolLabel
    operator()(const Row*, const PredictedBoolLabel& label) const
    {
      PredictedBoolLabel converted_label = label;
      unsigned long val = Gears::safe_rand(10);
      double res_val = 0;

      if(val > 4)
      {
        res_val = (val - 4) * 0.2;
      }

      if(converted_label.orig())
      {
        converted_label.pred -= res_val;
      }
      else
      {
        converted_label.pred += res_val;
      }

      return converted_label;
    }
  };

  /*
  struct LabelType: public Generics::SimpleDecimal<uint64_t, 18, 8>
  {
    LabelType() {}

    LabelType(int val)
      : Generics::SimpleDecimal<uint64_t, 18, 8>(val < 0, std::abs(val), 0)
    {}

    LabelType(const Generics::SimpleDecimal<uint64_t, 18, 8>& init)
      : Generics::SimpleDecimal<uint64_t, 18, 8>(init)
    {}

    LabelType(const Gears::SubString& str)
      : Generics::SimpleDecimal<uint64_t, 18, 8>(str)
    {}

    LabelType
    operator*(int right) const
    {
      return LabelType::mul(
        *this,
        Generics::SimpleDecimal<uint64_t, 18, 8>(right < 0, std::abs(right), 0),
        Generics::DMR_FLOOR);
    }
  };

  inline LabelType
  str_to_label(const Gears::SubString& str)
  {
    return LabelType(str);
  }

  inline double
  label_to_double(LabelType label)
  {
    double res;
    label.to_floating(res);
    return res;
  }
  */
}

namespace Vanga
{
  // BoolLabel impl
  inline
  BoolLabel::BoolLabel()
    : value(false)
  {}

  inline void
  BoolLabel::load(const Gears::SubString& str)
  {
    static const char* FUN = "BoolLabel::load()";

    if(str.length() != 1)
    {
      Gears::ErrorStream ostr;
      ostr << FUN << ": can't parse label '" << str << "'";
      throw LabelException(ostr.str());
    }
    else if(str[0] == '1')
    {
      value = true;
    }
    else if(str[0] == '0')
    {
      value = false;
    }
    else
    {
      Gears::ErrorStream ostr;
      ostr << FUN << ": can't parse label '" << str << "'";
      throw LabelException(ostr.str());
    }
  }

  inline void
  BoolLabel::save(std::ostream& ostr) const
  {
    ostr << (value ? "1" : "0");
  }

  inline bool
  BoolLabel::orig() const
  {
    return value;
  }

  inline double
  BoolLabel::to_float() const
  {
    return value ? 1.0 : 0.0;
  }

  inline bool
  BoolLabel::operator<(const BoolLabel& right) const
  {
    return !this->value && right.value;
  }

  inline BoolLabel
  BoolLabel::operator+(const BoolLabel& right) const throw()
  {
    BoolLabel res;
    res.value = value || right.value;
    return res;
  }

  inline void
  BoolLabel::print(std::ostream& ostr) const throw()
  {
    ostr << orig();
  }

  // PredictedBoolLabel impl
  inline
  PredictedBoolLabel::PredictedBoolLabel()
    : pred(0.0)
  {}

  inline void
  PredictedBoolLabel::load(const Gears::SubString& str)
  {
    static const char* FUN = "PredictedBoolLabel::load()";

    Gears::SubString::SizeType pred_pos = str.find(',');
    if(pred_pos == Gears::SubString::NPOS)
    {
      BoolLabel::load(str);
      pred = 0.0;
    }
    else
    {
      BoolLabel::load(str.substr(0, pred_pos));
      Gears::SubString pred_part_str = str.substr(pred_pos + 1);
      std::istringstream istr(pred_part_str.str());
      istr >> pred;
      if(!istr.eof() || istr.fail())
      {
        Gears::ErrorStream ostr;
        ostr << FUN << ": can't parse label prediced part '" << pred_part_str << "'";
        throw LabelException(ostr.str());
      }
    }
  }

  inline PredictedBoolLabel
  PredictedBoolLabel::operator+(const PredictedBoolLabel& right) const throw()
  {
    PredictedBoolLabel res;
    res.value = value || right.value;
    res.pred += right.pred;
    return res;
  }

  inline bool
  PredictedBoolLabel::operator<(const PredictedBoolLabel& right) const
  {
    if(!value)
    {
      return right.value || pred < right.pred;
    }

    return right.value && pred < right.pred;
  }

  inline void
  PredictedBoolLabel::print(std::ostream& ostr) const throw()
  {
    ostr << "label = " << orig() <<
      ", pred = " << pred;
  }

  // PredictedFloatLabel impl
  inline
  PredictedFloatLabel::PredictedFloatLabel()
    : value(0.0),
      pred(0.0)
  {}

  inline double
  PredictedFloatLabel::orig() const
  {
    return value;
  }

  inline void
  PredictedFloatLabel::load(const Gears::SubString& str)
  {
    value = ::atof(str.str().c_str());
    pred = 0.0;
  }

  inline PredictedFloatLabel
  PredictedFloatLabel::operator+(const PredictedFloatLabel& right) const throw()
  {
    PredictedFloatLabel res(*this);
    res.value += right.value;
    res.pred += right.pred;
    return res;
  }

  inline bool
  PredictedFloatLabel::operator<(const PredictedFloatLabel& right) const
  {
    return value < right.value || (
      value == right.value && pred < right.pred);
  }

  inline void
  PredictedFloatLabel::print(std::ostream& ostr) const throw()
  {
    ostr << "label = " << orig() <<
      ", pred = " << pred;
  }
}

#endif /*LABEL_HPP_*/
