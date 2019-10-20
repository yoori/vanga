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

#ifndef VECUTILS_HPP_
#define VECUTILS_HPP_

#include <cmath>

namespace Vanga
{
  inline double
  signdiff(double l, double r)
  {
    return l * r < 0.0;
  }

  inline void
  vec_copy(
    double* target,
    const double* source,
    int n)
  {
    for(int i = 0; i < n; ++i)
    {
      target[i] = source[i];
    }
  }

  inline void
  vec_inv_copy(
    double* target,
    const double* source,
    int n)
  {
    for(int i = 0; i < n; ++i)
    {
      target[i] = -source[i];
    }
  }

  inline void
  vec_scale(double* target, double mul, int n)
  {
    for(int i = 0; i < n; ++i)
    {
      target[i] *= mul;
    }
  }

  inline void
  vec_add(double* target, const double* vec1, double mul, int n)
  {
    for(int i = 0; i < n; ++i)
    {
      target[i] += vec1[i] * mul;
    }
  }

  inline void
  vec_sub(double* target, const double* vec1, const double* vec2, int n)
  {
    for(int i = 0; i < n; ++i)
    {
      target[i] = vec1[i] - vec2[i];
    }
  }

  inline double
  vec_mul(const double* vec1, const double* vec2, int n)
  {
    double res = 0.;
    for(int i = 0; i < n; ++i)
    {
      res += vec1[i] * vec2[i];
    }

    return res;
  }

  inline double
  vec_norm(const double* vec, int n)
  {
    return std::sqrt(vec_mul(vec, vec, n));
  }
}

#endif /*VECUTILS_HPP_*/
