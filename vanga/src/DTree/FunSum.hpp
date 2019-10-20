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

#ifndef FUNSUM_HPP_
#define FUNSUM_HPP_

#include <cmath>

namespace Vanga
{
  template<typename Fun1Type, typename Fun2Type>
  struct FunSum
  {
  public:
    FunSum(const Fun1Type& fun1, const Fun2Type& fun2)
      : fun1_(fun1),
        fun2_(fun2)
    {}

    double
    eval_fun_and_grad(
      FloatArray& d_grads,
      const FloatArray& d_vars)
      const
      throw();

  protected:
    Fun1Type fun1_;
    Fun2Type fun2_;
    mutable FloatArray grads2_;
  };
}

namespace Vanga
{
  template<typename Fun1Type, typename Fun2Type>
  double
  FunSum<Fun1Type, Fun2Type>::eval_fun_and_grad(
    FloatArray& grads,
    const FloatArray& point)
    const
    throw()
  {
    grads2_.resize(point.size());
    double f1 = fun1_.eval_fun_and_grad(grads, point);
    double f2 = fun2_.eval_fun_and_grad(grads2_, point);
    auto grad_it = grads.begin();
    for(auto grad2_it = grads2_.begin(); grad2_it != grads2_.end();
      ++grad_it, ++grad2_it)
    {
      *grad_it += *grad2_it;
    }

    return f1 + f2;
  }
}

#endif /*FUNSUM_HPP_*/
