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

#ifndef FUNOPTIMIZATION_HPP_
#define FUNOPTIMIZATION_HPP_

#include <stdint.h>
#include <vector>

namespace Vanga
{
  const double LOGLOSS_EPS = 0.0000001;
  const double LOGLOSS_EXP_MIN = -10.0;
  const double LOGLOSS_EXP_MAX = 10.0;

  typedef std::vector<double> FloatArray;

  struct VarIndexArray
  {
    VarIndexArray(uint64_t mask_val = 0)
      : mask_(mask_val)
    {}

    uint64_t
    to_int() const
    {
      return mask_;
    }

  protected:
    uint64_t mask_;
  };

  //
  template<typename IteratorType>
  struct VarGroup
  {
    VarGroup(const VarIndexArray& vars_val, IteratorType begin_val, IteratorType end_val)
      : vars(vars_val),
        begin(begin_val),
        end(end_val)
    {}

    VarIndexArray vars;
    IteratorType begin;
    IteratorType end;
  };
}

#endif /*FUNOPTIMIZATION_HPP_*/
