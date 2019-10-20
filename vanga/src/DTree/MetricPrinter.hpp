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

#ifndef METRICPRINTER_HPP_
#define METRICPRINTER_HPP_

#include <iostream>

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>

#include "Predictor.hpp"

namespace Vanga
{
  // MetricPrinter
  template<typename LabelType>
  struct MetricPrinter: public Gears::AtomicRefCountable
  {
    virtual void
    print(
      std::ostream& ostr,
      const SVM<LabelType>* svm) const = 0;

  protected:
    virtual ~MetricPrinter() throw()
    {}
  };

  // ComposeMetricPrinter
  template<typename LabelType>
  struct ComposeMetricPrinter: public MetricPrinter<LabelType>
  {
  public:
    ComposeMetricPrinter(
      MetricPrinter<LabelType>* printer1,
      MetricPrinter<LabelType>* printer2,
      MetricPrinter<LabelType>* printer3 = 0,
      MetricPrinter<LabelType>* printer4 = 0,
      MetricPrinter<LabelType>* printer5 = 0,
      MetricPrinter<LabelType>* printer6 = 0);

    virtual void
    print(
      std::ostream& ostr,
      const SVM<LabelType>* svm) const;

  protected:
    virtual ~ComposeMetricPrinter() throw()
    {}

  protected:
    std::vector<Gears::IntrusivePtr<MetricPrinter<LabelType> > > printers_;
  };
}

namespace Vanga
{
  // ComposeMetricPrinter
  template<typename LabelType>
  ComposeMetricPrinter<LabelType>::ComposeMetricPrinter(
    MetricPrinter<LabelType>* printer1,
    MetricPrinter<LabelType>* printer2,
    MetricPrinter<LabelType>* printer3,
    MetricPrinter<LabelType>* printer4,
    MetricPrinter<LabelType>* printer5,
    MetricPrinter<LabelType>* printer6)
  {
    if(printer1)
    {
      printers_.push_back(Gears::add_ref(printer1));
    }

    if(printer2)
    {
      printers_.push_back(Gears::add_ref(printer2));
    }

    if(printer3)
    {
      printers_.push_back(Gears::add_ref(printer3));
    }

    if(printer4)
    {
      printers_.push_back(Gears::add_ref(printer4));
    }

    if(printer5)
    {
      printers_.push_back(Gears::add_ref(printer5));
    }

    if(printer6)
    {
      printers_.push_back(Gears::add_ref(printer6));
    }
  }

  template<typename LabelType>
  void
  ComposeMetricPrinter<LabelType>::print(
    std::ostream& ostr,
    const SVM<LabelType>* svm) const
  {
    for(auto pit = printers_.begin(); pit != printers_.end(); ++pit)
    {
      if(pit != printers_.begin())
      {
        ostr << ", ";
      }

      (*pit)->print(ostr, svm);
    }
  }
}

#endif /*METRICPRINTER_HPP_*/
