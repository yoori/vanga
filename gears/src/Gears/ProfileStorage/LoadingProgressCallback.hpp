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

#ifndef LOADINGPROGRESSCALLBACK_HPP
#define LOADINGPROGRESSCALLBACK_HPP

#include "LoadingProgressCallbackBase.hpp"

namespace Gears
{
  class LoadingProgressCallback: public LoadingProgressCallbackBase
  {
  public:
    LoadingProgressCallback(
      LoadingProgressCallbackBase* parent,
      double range) throw();

    void
    post_progress(double value) throw();

    void
    loading_is_finished() throw();

  private:
    LoadingProgressCallbackBase_var parent_;
    double range_;
    double current_value_;
    unsigned int last_reported_chunk_;
  };
}

#endif // LOADINGPROGRESSCALLBACK_HPP
