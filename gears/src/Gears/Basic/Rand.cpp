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

#include "Lock.hpp"

#include "ISAAC.hpp"
#include "MT19937.hpp"

namespace Gears
{
  namespace
  {
    StaticInitializedMutex mutex;
    ISAAC generator;
  }

  const size_t MT19937::STATE_SIZE;
  const uint32_t MT19937::RAND_MAXIMUM;

  const uint32_t ISAAC::RAND_MAXIMUM;
  const size_t ISAAC::SIZE;

  uint32_t
  safe_rand() throw ()
  {
    StaticInitializedMutex::WriteGuard lock(mutex);
    return generator.rand() >> 1;
  }
}
