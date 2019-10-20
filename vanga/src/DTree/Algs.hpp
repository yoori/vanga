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

#ifndef VANGA_ALGS_HPP_
#define VANGA_ALGS_HPP_

#include <iterator>
#include <ostream>

namespace Vanga
{
  namespace Algs
  {
    template<typename IteratorType>
    std::ostream&
    print(std::ostream& out,
      IteratorType it_begin, IteratorType it_end, const char* delim = ", ")
    {
      for(IteratorType it = it_begin; it != it_end; ++it)
      {
        if(it != it_begin)
        {
          out << delim;
        }
        
        out << *it;
      }
      
      return out;
    }
  }
}

#endif /*VANGA_ALGS_HPP_*/
