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

#ifndef GEARS_CSV_HPP_
#define GEARS_CSV_HPP_

#include <iostream>
#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/OutputMemoryStream.hpp>
#include <Gears/Basic/SubString.hpp>

#include "AsciiStringManip.hpp"

namespace Gears
{
  class Csv
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);

  public:
    template<typename ContainerType>
    static void
    parse_line(
      ContainerType& container,
      const SubString& line)
      /*throw(Exception)*/;

    static void
    write_escaped(
      std::ostream& out,
      const SubString& value)
      noexcept;

    static void
    escape(
      std::string& res,
      const SubString& src,
      char separator = ',')
      noexcept;

  protected:
    static const Gears::Ascii::CharCategory Q_CHARS_;
    static const Gears::Ascii::CharCategory P_CHARS_;
  };
}

namespace Gears
{
  template<typename ContainerType>
  void
  Csv::parse_line(ContainerType& container, const SubString& line)
    /*throw(Exception)*/
  {
    SubString::ConstPointer pos = line.begin();
    SubString::ConstPointer pre_end = line.end();
    --pre_end;

    while(pos != line.end())
    {
      // process value
      bool quota_opened = false;

      if(pos != pre_end && *pos == '"')
      {
        quota_opened = true;
        ++pos;
        if(pos == line.end())
        {
          ErrorStream ostr;
          ostr << "unexpected end of line";
          throw Exception(ostr.str());
        }
      }

      if(quota_opened)
      {
        // find quota end
        std::string res_value;
        SubString::ConstPointer block_begin_pos = pos;
        while(true)
        {
          pos = Q_CHARS_.find_owned(block_begin_pos, line.end());
          res_value += SubString(block_begin_pos, pos).str();
          if(pos == line.end())
          {
            break;
          }
          else if(*(pos + 1) != '"')
          {
            pos += 1;
            break;
          }

          res_value += "\"";
          block_begin_pos = pos + 2;
        }

        if(pos != line.end() && *pos != ',')
        {
          ErrorStream ostr;
          ostr << "expected field separator after value close";
          throw Exception(ostr.str());
        }

        container.insert(container.end(), res_value);
      }
      else // no quota
      {
        SubString::ConstPointer end_pos = P_CHARS_.find_owned(pos, line.end());
        container.insert(container.end(), SubString(pos, end_pos).str());
        pos = end_pos;
      }

      if(pos != line.end())
      {
        ++pos;
        if(pos == line.end())
        {
          container.insert(container.end(), std::string());
        }
      }
    }
  }

  void
  Csv::write_escaped(
    std::ostream& out,
    const SubString& value)
    noexcept
  {
    std::string csv_encoded_str;
    Csv::escape(csv_encoded_str, value);
    out.write(csv_encoded_str.data(), csv_encoded_str.size());
  }
}

#endif /*GEARS_CSV_HPP_*/
