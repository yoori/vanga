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

#ifndef FUNDISCREPANCY_HPP_
#define FUNDISCREPANCY_HPP_

#include <cmath>

namespace Vanga
{
  struct FunPostQuad
  {
  public:
    FunPostQuad(double grow_after)
      : grow_after_(grow_after),
        grow_after_quad_(grow_after * grow_after)
    {}

    double
    eval_fun_and_grad(
      FloatArray& d_grads,
      const FloatArray& d_vars)
      const
      throw();

  protected:
    const double grow_after_;
    const double grow_after_quad_;
  };
}

namespace Vanga
{
  inline double
  FunPostQuad::eval_fun_and_grad(
    FloatArray& grads,
    const FloatArray& point)
    const
    throw()
  {
    double norm = 0.0;
    for(auto var_it = point.begin(); var_it != point.end(); ++var_it)
    {
      norm += *var_it * (*var_it);
    }

    if(norm > grow_after_quad_)
    {
      // 2*v*(sqrt(norm) - C) / sqrt(norm)
      const double norm_sqrt = std::sqrt(norm);
      const double mul = 2 * (norm_sqrt - grow_after_) / norm_sqrt;

      auto grad_it = grads.begin();
      for(auto var_it = point.begin(); var_it != point.end(); ++var_it, ++grad_it)
      {
        *grad_it = *var_it * mul;
      }

      return (norm_sqrt - grow_after_) * (norm_sqrt - grow_after_);
    }
    else
    {
      std::fill(grads.begin(), grads.end(), 0.0);
      return 0.0;
    }
  }
}

#endif /*FUNDISCREPANCY_HPP_*/
