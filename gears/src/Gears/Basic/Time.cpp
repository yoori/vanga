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

#include <algorithm>

#include "InputMemoryStream.hpp"
#include "OutputMemoryStream.hpp"
#include "Time.hpp"

// Stream functions

std::ostream&
operator <<(std::ostream& ostr, const Gears::Time& time)
  /*throw (Gears::Exception)*/
{
  char buf[256];
  snprintf(buf, sizeof(buf), "%s%lu:%.6ld (sec:usec)",
    time.sign() < 0 ? "-" : "",
    static_cast<unsigned long int>(time.integer_part()),
    static_cast<long int>(time.fractional_part()));
  return ostr << buf;
}

std::ostream&
operator <<(std::ostream& ostr, const Gears::ExtendedTime& time)
  /*throw (Gears::Exception)*/
{
  ostr.width(4);
  ostr.fill('0');
  ostr << (time.tm_year + 1900);

  ostr.width(2);
  ostr.fill('0');
  ostr << (time.tm_mon + 1);

  ostr.width(2);
  ostr.fill('0');
  ostr << time.tm_mday << ".";

  ostr.width(2);
  ostr.fill('0');
  ostr << time.tm_hour;

  ostr.width(2);
  ostr.fill('0');
  ostr << time.tm_min;

  ostr.width(2);
  ostr.fill('0');
  ostr << time.tm_sec;

  ostr.width(6);
  ostr.fill('0');
  ostr << time.tm_usec;

  return ostr;
}

std::istream&
operator >>(std::istream& istr, Gears::Time& time)
  /*throw (Gears::Time::Exception, Gears::Exception)*/
{
  std::string timestr, suffix;
  time_t sec;
  suseconds_t usec;
  char separator;

  istr >> timestr >> suffix;
  Gears::InputMemoryStream<char> parser(timestr.data(), timestr.size());
  const bool NEGATIVE = !timestr.empty() && timestr[0] == '-';
  parser >> sec >> separator >> usec;

  if (istr.bad() || parser.bad() || !parser.eof() ||
    suffix != "(sec:usec)" || usec < 0 || usec >= Gears::Time::USEC_MAX)
  {
    Gears::ErrorStream ostr;
    ostr << "operator>>(..., Time): invalid time read '" << timestr << " " << suffix << "'";
    throw Gears::Time::Exception(ostr.str());
  }

  time = NEGATIVE ? Gears::Time(-std::abs(sec) - (usec ? 1 : 0),
    usec ? Gears::Time::USEC_MAX - usec : 0) : Gears::Time(sec, usec);

  return istr;
}

std::istream&
operator >>(std::istream& istr, Gears::ExtendedTime& time)
  /*throw (Gears::ExtendedTime::Exception, Gears::Exception)*/
{
  std::string tmstr;
  istr >> tmstr;

  if (istr.bad() || istr.fail())
  {
    return istr;
  }

  unsigned int year = 0;
  unsigned int month = 0;
  unsigned int day = 0;
  unsigned int hour = 0;
  unsigned int min = 0;
  unsigned int sec = 0;
  unsigned int usec = 0;

  if (sscanf(tmstr.c_str(), "%4d%2d%2d.%2d%2d%2d%d",
    &year, &month, &day, &hour, &min, &sec, &usec) != 7 ||
    year < 1900 || month < 1 || month > 12 || day < 1 || day > 31 ||
    hour >= 24 || min >= 60 || sec > 60 || usec >= 1000000)
  {
    Gears::ErrorStream ostr;
    ostr << "operator>>(..., Time): invalid time read '" << tmstr << "'";
    throw Gears::ExtendedTime::Exception(ostr.str());
  }

  time = Gears::ExtendedTime(year, month, day, hour, min, sec, usec);

  return istr;
}

namespace Gears
{
  const std::size_t Time::TIME_PACK_LEN = 8;

  const unsigned long Time::TIME_LEN = 21;

  const suseconds_t Time::USEC_MAX = 1000000;

  const Time Time::ZERO;
  const Time Time::ONE_SECOND(1l);
  const Time Time::ONE_MINUTE(60l);
  const Time Time::ONE_HOUR(60l * 60l);
  const Time Time::ONE_DAY(24l * 60l * 60l);
  const Time Time::ONE_WEEK(7l * 24l * 60l * 60l);

  const char Time::DAYS_[][4] =
  {
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat"
  };

  const SubString Time::DAYS_FULL_[] =
  {
    SubString("Sunday", 6),
    SubString("Monday", 6),
    SubString("Tuesday", 7),
    SubString("Wednesday", 9),
    SubString("Thursday", 8),
    SubString("Friday", 6),
    SubString("Saturday", 8)
  };

  const char Time::MONTHS_[][4] =
  {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
  };

  static const int DAYS[2][12] =
  {
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 }
  };

  time_t
  gm_to_time(const tm& et) noexcept
  {
    const long YEARS = et.tm_year - 70;
    return ((YEARS * 365) + (YEARS + 1) / 4 +
      DAYS[(YEARS & 3) == 2][et.tm_mon] + et.tm_mday - 1) * 86400 +
      et.tm_hour * 3600 + et.tm_min * 60 + et.tm_sec;
  }

  void
  time_to_gm(time_t time, tm& et) noexcept
  {
    memset(&et, 0, sizeof(et));
    et.tm_sec = time % 60;
    time /= 60;
    et.tm_min = time % 60;
    time /= 60;
    et.tm_hour = time % 24;
    time /= 24;
    et.tm_wday = (time + 4) % 7;
    int years = static_cast<int>(time / (4 * 365 + 1) * 4);
    time %= 4 * 365 + 1;
    long leap = 0;
    if (time >= 365)
    {
      if (time >= 365 * 2)
      {
        if (time >= 365 * 3 + 1)
        {
          years += 3;
          time -= 365 * 3 + 1;
        }
        else
        {
          years += 2;
          time -= 365 * 2;
          leap = 1;
        }
      }
      else
      {
        years++;
        time -= 365;
      }
    }
    et.tm_year = years + 70;
    et.tm_yday = static_cast<int>(time);
    const int* const CDAYS(DAYS[leap]);
    const int* const MONTH(
      std::lower_bound(CDAYS + 1, CDAYS + 12, ++time) - 1);
    et.tm_mon = static_cast<int>(MONTH - CDAYS);
    et.tm_mday = static_cast<int>(time - *MONTH);
  }

  ExtendedTime::ExtendedTime(time_t sec, suseconds_t usec, Time::TimeZone tz)
    /*throw (Exception, Gears::Exception)*/
  {
    switch (tz)
    {
    case Time::TZ_GMT:
      time_to_gm(sec, *this);
      break;

    case Time::TZ_LOCAL:
      if(localtime_r(&sec, this) == 0)
      {
        int error_code = errno;
        char seconds[16];
        snprintf(seconds, sizeof(seconds), "%lu",
          static_cast<unsigned long>(sec));
        throw_errno_exception<Exception>(error_code,
          "ExtendedTime::ExtendedTime()",
					"localtime_r(", seconds, ") failed");
      }
      break;

    default:
      ErrorStream ostr;
      ostr << "ExtendedTime::ExtendedTime(): invalid TZ type";
      throw Exception(ostr.str());
    }

    tm_usec = usec;
    timezone = tz;
  }

  std::string
  ExtendedTime::format(const char* fmt) const
    /*throw (InvalidArgument, Exception, Gears::Exception)*/
  {
	  static const char* FUN = "ExtendedTime::format()";

    if (fmt == 0)
    {
      ErrorStream ostr;
      ostr << FUN << ": format argument is NULL";
      throw InvalidArgument(ostr.str());
    }

    const unsigned int STR_LEN = 256;
    char str[STR_LEN];
    size_t length = ::strftime(str, STR_LEN, fmt, this);

    if (length == 0)
    {
      ErrorStream ostr;
      ostr << FUN << ": can't format time according to format '" << fmt << "'";
      throw Exception(ostr.str());
    }

    if (char* cur = strstr(str, "%q"))
    {
      char usec[7];
      snprintf(usec, sizeof(usec), "%06lu",
        static_cast<unsigned long>(tm_usec));

      do
      {
        if (length >= STR_LEN - 4)
        {
          ErrorStream ostr;
          ostr << FUN <<
            ": can't format time according to format '" << fmt << "'";
          throw Exception(ostr.str());
        }

        if (cur[2])
        {
          memmove(cur + 6, cur + 2, length - (cur - str) - 1);
          memcpy(cur, usec, 6);
        }
        else
        {
          memcpy(cur, usec, 7);
          break;
        }
        length += 4;
      }
      while ((cur = strstr(cur + 6, "%q")));
    }

    return str;
  }

}

