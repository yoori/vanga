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

#include "Csv.hpp"

namespace Gears
{
  const Ascii::CharCategory
  Csv::Q_CHARS_("\"");

  const Ascii::CharCategory
  Csv::P_CHARS_(",");

  const Ascii::CharCategory
  C_NON_CSV(",\"\n\r");

  const Ascii::CharCategory
  PC_NON_CSV(";\"\n\r");

  void
  Csv::escape(
    std::string& res,
    const SubString& src,
    char separator)
    throw()
  {
    // fields that contain separator, double-quotes,
    //     or line-breaks must be quoted.
    // a quote within a field must be escaped with
    //    an additional quote immediately preceding the literal quote.
    const char non_csv_chars[] = {separator , '"', '\n', '\r', 0};

    const Ascii::CharCategory& non_csv =
      separator == ',' ? C_NON_CSV :
      separator == ';' ? PC_NON_CSV :
      Ascii::CharCategory(
        static_cast<const char*>(non_csv_chars));

    if(!non_csv.find_owned(src.begin(), src.end()))
    {
      res = src.str();
    }
    else
    {
      std::string dest;
      dest.reserve(1024);
      dest.push_back('"');

      SubString::ConstPointer prev_ptr = src.begin();
      SubString::ConstPointer ptr = src.begin();
      for(; ptr != src.end(); ++ptr)
      {
        if (*ptr == '"')
        {
          dest.append(prev_ptr, ptr);
          dest.append(2, '"');
          prev_ptr = ptr + 1;
        }
      }

      if(prev_ptr != src.end())
      {
        dest.append(prev_ptr, src.end());
      }

      dest.push_back('"');
      res.swap(dest);
    }
  }
}
