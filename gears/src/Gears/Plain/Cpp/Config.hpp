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

#ifndef PLAIN_CPP_CONFIG_HPP
#define PLAIN_CPP_CONFIG_HPP

namespace Cpp
{
  const char BASE_SUFFIX[] = "_Base";
  const char PROTECTED_WRITER_SUFFIX[] = "_ProtectedWriter";
  const char DEFAULT_BUFFERS_SUFFIX[] = "_DefaultBuffers";
  const char FIELD_OFFSET_SUFFIX[] = "_OFFSET";
  const char FIXED_BUFFER_PREFIX[] = "fixed_buf_";

  const char INCLUDE_LIST[] =
    "#include <string>\n"
    "#include <Gears/Plain/Base.hpp>\n"
    "#include <Gears/Plain/String.hpp>\n"
    "#include <Gears/Plain/ConstVector.hpp>\n"
    "#include <Gears/Plain/List.hpp>\n"
    "#include <Gears/Plain/Vector.hpp>\n"
    "#include <Gears/Plain/Buffer.hpp>\n";
}

#endif /*PLAIN_CPP_CONFIG_HPP*/
