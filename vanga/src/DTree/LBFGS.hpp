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

#ifndef LBFSG_HPP_
#define LBFSG_HPP_

#include <cmath>

#include "Algs.hpp"
#include "FunOptimization.hpp"

namespace Vanga
{
  template<typename FunType>
  bool
  line_search(
    int n,
    double* x, // result point (out)
    double& f, // function value (in/out)
    FloatArray& g_vec, // result gradient (in/out)
    double& stp, // step (in/out)
    const double* s, // direction
    const double* xp, // start point
    const double* gp, // start point gradient
    const FunType& fun // function evaluator
    )
    throw();

  template<typename IteratorType>
  void
  lbfsg_logloss_min(
    FloatArray& yes_res,
    FloatArray& no_res,
    const std::vector<VarGroup<IteratorType> >& preds)
    throw();
}

#include "LBFGS.tpp"

#endif /*LBFSG_HPP_*/
