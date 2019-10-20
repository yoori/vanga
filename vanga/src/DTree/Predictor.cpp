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

#include "Predictor.hpp"
#include "DTree.hpp"

namespace Vanga
{
  // Predictor
  /*
  PredArrayHolder_var
  Predictor::predict(const RowArray& rows) const throw()
  {
    PredArrayHolder_var res = new PredArrayHolder();
    res->values.resize(rows.size());

    unsigned long i = 0;
    for(auto it = rows.begin(); it != rows.end(); ++it, ++i)
    {
      res->values[i] = predict((*it)->features);
    }

    return res;
  }
  */
}