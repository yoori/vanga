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

#ifndef GEARS_BASIC_TIME_HPP
#define GEARS_BASIC_TIME_HPP

#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <math.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <climits>

#include "Exception.hpp"
#include "Errno.hpp"
#include "OutputMemoryStream.hpp"
#include "SubString.hpp"

OPEN_NAMESPACE(Gears)

  struct ExtendedTime;

  class Time: public timeval
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, DescriptiveException);
    DECLARE_GEARS_EXCEPTION(InvalidArgument, Exception);

    /**
     * Time zones types (either GMT or local)
     */
    enum TimeZone
    {
      TZ_GMT,
      TZ_LOCAL
    };

  public:
    /**
     * Prints out passed time value to the stream
     * @param tv Time value to print
     * @param ostr Stream for output
     */
    static void
    print(const Time& tv, std::ostream& ostr)
      /*throw (Gears::Exception)*/;

    /**
     * Gives month name by its number (0-11)
     * @param month index (0-11)
     * @return 3-letter month name
     */
    static const char*
    month(unsigned month)
      /*throw (InvalidArgument, Exception)*/;

    /**
     * Gives week day name by its index
     * @param day index (0-6, Sunday first)
     * @return 3-letter week day name
     */
    static const char*
    week_day(unsigned day)
      /*throw (InvalidArgument, Exception)*/;

    /**
     * Compares two tm dates. No TZ check is performed
     * @param t1 the first date
     * @param t2 the second date
     * @return Negative if less, zero if equal, positive if greater
     */
    static int
    compare(const tm& t1, const tm& t2) noexcept;

    /**
     * Creates Time object holding current time value
     * @return current time
     */
    static Time
    get_time_of_day() noexcept;


  public:
    /**
     * Default constructor
     */
    Time() noexcept;

    /**
     * Constructor
     * @param time provided time
     */
    explicit
    Time(const timeval& time) noexcept;

    /**
     * Constructor
     * @param time_sec provided time (seconds)
     * @param usec provided time (microseconds)
     */
    explicit
    Time(time_t time_sec, suseconds_t usec = 0) noexcept;

    /**
     * Constructor
     * Parameters are equivalent to those in set function
     * @param value time string
     * @param format format string
     */
    Time(const char* value, const char* format)
      /*throw (InvalidArgument, Exception, Gears::Exception)*/;

    /*
    explicit
    Time(double value) noexcept;
    */

    /**
     * Convertor to ExtendedTime
     * @param tz required TZ
     * @return ExtendedTime presentation based on current value and required tz
     */
    ExtendedTime
    get_time(TimeZone tz) const
      /*throw (Exception, Gears::Exception)*/;

    /**
     * Convertor to ExtendedTime
     * @return ExtendedTime GMT presentation based on current value
     */
    ExtendedTime
    get_gm_time() const /*throw (Exception, Gears::Exception)*/;

    /**
     * Convertor to ExtendedTime
     * @return ExtendedTime localtime presentation based on current value
     */
    ExtendedTime
    get_local_time() const /*throw (Exception, Gears::Exception)*/;

    /**
     * Resets current value
     * @param time_sec seconds
     * @param usec microseconds
     */
    void
    set(time_t time_sec, suseconds_t usec = 0) noexcept;

    /**
     * Parses string time representation.
     * Format is the same as in strptime().
     * @param value time string
     * @param format format string
     */
    void
    set(const char* value, const char* format)
      /*throw (InvalidArgument, Exception, Gears::Exception)*/;

    /**
     * [sign]integer.fractional
     * Returns sign
     * @return 1, 0 or -1
     */
    int
    sign() const noexcept;

    /**
     * [sign]integer.fractional
     * Returns integer part
     * @return integer part of symbolic time representation (seconds)
     */
    time_t
    integer_part() const noexcept;

    /**
     * [sign]integer.fractional
     * Returns fractional part
     * @return fractional part of symbolic time representation (microseconds)
     */
    suseconds_t
    fractional_part() const noexcept;

    /**
     * Inverts sign of the time interval
     */
    void
    invert_sign() noexcept;

    /**
     * Returns tv_sec * USEC_MAX + tv_usec
     * @return microseconds representation of time value
     */
    long long
    microseconds() const noexcept;

    /**
     * Packs current value into TIME_PACK_LEN bytes long buffer
     * @param buffer pointer to TIME_PACK_LEN bytes long buffer
     */
    void
    pack(void* buffer) const noexcept;

    /**
     * Unpacks current value from TIME_PACK_LEN bytes long buffer
     * @param buffer pointer to TIME_PACK_LEN bytes long buffer
     */
    void
    unpack(const void* buffer) noexcept;

    /**
     * Adds another time interval to the current
     * @param time time interval to add
     * @return reference to the object
     */
    Time&
    operator +=(const Time& time) noexcept;

    /**
     * Adds another time interval to the current
     * @param time time interval (seconds) to add
     * @return reference to the object
     */
    Time&
    operator +=(time_t time) noexcept;

    /**
     * Subtracts another time interval from the current
     * @param time time interval to subtract
     * @return reference to the object
     */
    Time&
    operator -=(const Time& time) noexcept;

    /**
     * Subtracts another time interval from the current
     * @param time time interval (seconds) to subtract
     * @return reference to the object
     */
    Time&
    operator -=(time_t time) noexcept;

    /**
     * Multiplies current time interval on non-negative integer multiplier
     * @param multiplier multiplier
     * @return reference to the object
     */
    Time&
    operator *=(int multiplier) noexcept;

    /**
     * Divides current time interval on non-negative integer divisor
     * @param divisor divisor
     * @return reference to the object
     */
    Time&
    operator /=(int divisor) noexcept;

    /**
     * Quicker way to call get_gm_time().format("%F %T")
     * @return formatted GM time
     */
    std::string
    gm_ft() const /*throw (Gears::Exception)*/;

    double
    as_double() const;

  public:
    static const std::size_t TIME_PACK_LEN;

    static const unsigned long TIME_LEN;

    static const suseconds_t USEC_MAX;

    static const Time ZERO;
    static const Time ONE_SECOND;
    static const Time ONE_MINUTE;
    static const Time ONE_HOUR;
    static const Time ONE_DAY;
    static const Time ONE_WEEK;

  protected:
    static const char DAYS_[][4];
    static const SubString DAYS_FULL_[];
    static const char MONTHS_[][4];
  };

  /**
   * Representation of divided time according to selected TZ
   * Microseconds granularity is provided
   * Could be converted into Time object
   */
  struct ExtendedTime : public tm
  {
  public:
    int tm_usec;
    Time::TimeZone timezone;

  public:
    typedef Time::Exception Exception;
    DECLARE_GEARS_EXCEPTION(InvalidArgument, Exception);

    /**
     * Constructor
     * @param time divided time
     * @param usec microseconds
     * @param tz TZ for supplied time
     */
    ExtendedTime(const tm& time, suseconds_t usec, Time::TimeZone tz) noexcept;
    /**
     * Constructor
     * @param sec seconds from Epoch
     * @param usec microseconds
     * @param tz TZ to convert to
     */
    ExtendedTime(time_t sec, suseconds_t usec, Time::TimeZone tz)
      /*throw (Exception, Gears::Exception)*/;

    /**
     * Constructor
     * GMT is assumed
     * @param year year (A.D.)
     * @param month month index (1-12)
     * @param day day number (1-31)
     * @param hour hours (0-23)
     * @param min minutes (0-59)
     * @param sec seconds (0-59)
     * @param usec microseconds
     */
    ExtendedTime(
      unsigned int year,
      unsigned int month,
      unsigned int day,
      unsigned int hour,
      unsigned int min,
      unsigned int sec,
      suseconds_t usec) noexcept;

    /**
     * Time convertion operator
     * @return Time object representing current value
     */
    operator Time() const
      /*throw (Exception, Gears::Exception)*/;

    /**
     * Formats time represented by this object according to fmt.
     * fmt is the same as in strftime().
     * @param fmt format string. Also %q and %%q may be used for usecs.
     * @return formatted time string
     */
    std::string
    format(const char* fmt) const
      /*throw (InvalidArgument, Exception, Gears::Exception)*/;

    /**
     * Provides time normalization (i.e. 32nd of October becomes
     * 1st of November)
     */
    void
    normalize() /*throw (Exception, Gears::Exception)*/;

    /**
     * Gives time part of the current value
     * @return time part of the current value (date fields are zeroed)
     */
    ExtendedTime
    get_time() const /*throw (Gears::Exception)*/;

    /**
     * Copies time part of supplied value
     * @param time time to copy
     */
    void
    set_time(const ExtendedTime& time) noexcept;

    /**
     * Gives date part of the current value
     * @return date part of the current value (time fields are zeroed)
     */
    ExtendedTime
    get_date() const /*throw (Gears::Exception)*/;

    /**
     * Copies date part of supplied value
     * @param time date to copy
     */
    void
    set_date(const ExtendedTime& time) noexcept;
  };

  /**
   * timegm(3) analogue
   * @param et split time stamp
   * @return seconds since epoch
   */
  time_t
  gm_to_time(const tm& et) noexcept;

  /**
   * gmtime_r(3) analogue
   * @param time seconds sunce epoch to split
   * @param et resulted split time
   */
  void
  time_to_gm(time_t time, tm& et) noexcept;

CLOSE_NAMESPACE

// Comparance functions (for Time class)
bool
operator ==(const Gears::Time& tv1, const timeval& tv2) noexcept;

bool
operator !=(const Gears::Time& tv1, const timeval& tv2) noexcept;

bool
operator <(const Gears::Time& tv1, const timeval& tv2) noexcept;

bool
operator >(const Gears::Time& tv1, const timeval& tv2) noexcept;

bool
operator <=(const Gears::Time& tv1, const timeval& tv2) noexcept;

bool
operator >=(const Gears::Time& tv1, const timeval& tv2) noexcept;

// Arithmetics functions (for Time class)
Gears::Time
operator +(const Gears::Time& tv1, const timeval& tv2) noexcept;

Gears::Time
operator +(const Gears::Time& tv, time_t time) noexcept;

Gears::Time
operator -(const Gears::Time& tv1, const timeval& tv2) noexcept;

Gears::Time
operator -(const Gears::Time& tv, time_t time) noexcept;

Gears::Time
operator *(const Gears::Time& tv, int multiplier) noexcept;

Gears::Time
operator /(const Gears::Time& tv, int divisor) noexcept;

// Stream functions
std::ostream&
operator <<(std::ostream& ostr, const Gears::Time& time)
  /*throw (Gears::Exception)*/;

std::ostream&
operator <<(std::ostream& ostr, const Gears::ExtendedTime& time)
  /*throw (Gears::Exception)*/;

std::istream&
operator >>(std::istream& istr, Gears::Time& time)
  /*throw (Gears::Time::Exception, Gears::Exception)*/;

std::istream&
operator >>(std::istream& istr, Gears::ExtendedTime& time)
  /*throw (Gears::ExtendedTime::Exception, Gears::Exception)*/;

//
// Inlines
//

OPEN_NAMESPACE(Gears)

  //
  // ExtendedTime class
  //

  inline
  ExtendedTime::ExtendedTime(const tm& time, suseconds_t usec,
    Time::TimeZone tz) noexcept
    : tm(time),
      tm_usec(usec),
      timezone(tz)
  {}

  inline
  ExtendedTime::ExtendedTime(
    unsigned int year,
    unsigned int month,
    unsigned int day,
    unsigned int hour,
    unsigned int min,
    unsigned int sec,
    suseconds_t usec) noexcept
  {
    tm_year = year - 1900;
    tm_mon = month - 1;
    tm_mday = day;
    tm_hour = hour;
    tm_min = min;
    tm_sec = sec;
    tm_usec = usec;
    timezone = Time::TZ_GMT;

    time_to_gm(gm_to_time(*this), *this);
  }

  inline
  ExtendedTime::operator Time() const
    /*throw (Exception, Gears::Exception)*/
  {
    time_t sec = 0;
    switch (timezone)
    {
    case Time::TZ_LOCAL:
      {
        tm tmp = *this;
        sec = ::mktime(&tmp);
      }
      break;

    case Time::TZ_GMT:
      sec = gm_to_time(*this);
      break;
    }

    return Time(sec, tm_usec);
  }

  inline void
  ExtendedTime::normalize() /*throw (Exception, Gears::Exception)*/
  {
    const time_t invalid = static_cast<time_t>(-1);
    time_t res = invalid;
    switch (timezone)
    {
    case Time::TZ_GMT:
      time_to_gm(gm_to_time(*this), *this);
      res = 0;
      break;
    case Time::TZ_LOCAL:
      res = mktime(this);
      break;
    default:
      break;
    }

    if (res == invalid)
    {
      throw Exception("ExtendedTime::normalize(): can't normalize.");
    }
  }

  inline ExtendedTime
  ExtendedTime::get_time() const /*throw (Gears::Exception)*/
  {
    ExtendedTime res(*this);
    res.tm_mday = 0;
    res.tm_mon = 0;
    res.tm_wday = 0;
    res.tm_yday = 0;
    res.tm_year = 0;
    return res;
  }

  inline void
  ExtendedTime::set_time(const ExtendedTime& time) noexcept
  {
    tm_hour = time.tm_hour;
    tm_min = time.tm_min;
    tm_sec = time.tm_sec;
    tm_usec = time.tm_usec;
  }

  inline ExtendedTime
  ExtendedTime::get_date() const /*throw (Gears::Exception)*/
  {
    ExtendedTime res(*this);
    res.tm_hour = 0;
    res.tm_min = 0;
    res.tm_sec = 0;
    res.tm_usec = 0;
    return res;
  }

  inline void
  ExtendedTime::set_date(const ExtendedTime& time) noexcept
  {
    tm_mday = time.tm_mday;
    tm_mon = time.tm_mon;
    tm_year = time.tm_year;
  }

  //
  // Time class
  //

  inline Time
  Time::get_time_of_day() noexcept
  {
    Time time;
    gettimeofday(&time, NULL);
    return time;
  }

  inline
  Time::Time() noexcept
  {
    tv_sec = 0;
    tv_usec = 0;
  }

  inline
  Time::Time(const timeval& time) noexcept
    : timeval(time)
  {}

  inline
  Time::Time(time_t time_sec, suseconds_t usec) noexcept
  {
    tv_sec = time_sec;
    tv_usec = usec;
  }

  inline void
  Time::set(time_t time_sec, suseconds_t usec) noexcept
  {
    tv_sec = time_sec;
    tv_usec = usec;
  }

  inline
  Time::Time(const char* value, const char* format)
    /*throw (InvalidArgument, Exception, Gears::Exception)*/
  {
    set(value, format);
  }

  /*
  inline
  Time::Time(double value) noexcept
  {
    double floor_value = ::floor(value);
    tv_sec = static_cast<int>(floor_value);
    tv_usec = static_cast<int>((value - floor_value) * USEC_MAX);
  }
  */

  inline
  void
  Time::print(const Time& time, std::ostream& ostr)
    /*throw (Gears::Exception)*/
  {
    ostr << Time(time);
  }

  inline int
  Time::compare(const tm& t1, const tm& t2) noexcept
  {
    int diff = t1.tm_year - t2.tm_year;

    if (diff == 0)
    {
      diff = t1.tm_mon - t2.tm_mon;
    }

    if (diff == 0)
    {
      diff = t1.tm_mday - t2.tm_mday;
    }

    if (diff == 0)
    {
      diff = t1.tm_hour - t2.tm_hour;
    }

    if (diff == 0)
    {
      diff = t1.tm_min - t2.tm_min;
    }

    if (diff == 0)
    {
      diff = t1.tm_sec - t2.tm_sec;
    }

    return diff;
  }

  inline const char*
  Time::month(unsigned month) /*throw (InvalidArgument, Exception)*/
  {
    if (month > 11)
    {
      ErrorStream ostr;
      ostr << "Time::month(...): invalid month specified '" << month << "'";
      throw InvalidArgument(ostr.str());
    }

    return MONTHS_[month];
  }

  inline const char*
  Time::week_day(unsigned day) /*throw (InvalidArgument, Exception)*/
  {
    if (day > 6)
    {
      ErrorStream ostr;
      ostr << "Time::week_day(...): invalid day specified '" << day << "'";
      throw InvalidArgument(ostr.str());
    }

    return DAYS_[day];
  }

  inline ExtendedTime
  Time::get_time(TimeZone tz) const
    /*throw (Exception, Gears::Exception)*/
  {
    return ExtendedTime(tv_sec, tv_usec, tz);
  }

  inline ExtendedTime
  Time::get_gm_time() const /*throw (Exception, Gears::Exception)*/
  {
    return ExtendedTime(tv_sec, tv_usec, TZ_GMT);
  }

  inline ExtendedTime
  Time::get_local_time() const /*throw (Exception, Gears::Exception)*/
  {
    return ExtendedTime(tv_sec, tv_usec, TZ_LOCAL);
  }

  inline int
  Time::sign() const noexcept
  {
    return tv_sec > 0 ? 1 : tv_sec ? -1 : 0;
  }

  inline time_t
  Time::integer_part() const noexcept
  {
    return tv_sec >= 0 ? tv_sec : tv_usec ? -tv_sec - 1 : -tv_sec;
  }

  inline suseconds_t
  Time::fractional_part() const noexcept
  {
    return tv_sec >= 0 || !tv_usec ? tv_usec : USEC_MAX - tv_usec;
  }

  inline void
  Time::invert_sign() noexcept
  {
    if (tv_usec)
    {
      tv_sec = -tv_sec - 1;
      tv_usec = USEC_MAX - tv_usec;
    }
    else
    {
      tv_sec = -tv_sec;
    }
  }

  inline long long
  Time::microseconds() const noexcept
  {
    return tv_sec * static_cast<long long>(USEC_MAX) + tv_usec;
  }

  inline void
  Time::set(const char* value, const char* format)
    /*throw (InvalidArgument, Exception, Gears::Exception)*/
  {
    if (value == 0 || format == 0)
    {
      ErrorStream ostr;
      ostr << "Time::set(): one of the arguments is NULL.";
      throw InvalidArgument(ostr.str());
    }

    tm time;
    time_to_gm(0, time);

    if (::strptime(value, format, &time) == 0)
    {
      ErrorStream ostr;
      ostr << "Time::set(): can't parse string '" << value <<
        "' according to format '" << format << "'";
      throw Exception(ostr.str());
    }
    *this = ExtendedTime(time, 0, TZ_GMT);
  }

  inline void
  Time::pack(void* buffer) const noexcept
  {
    int32_t* buf = static_cast<int32_t*>(buffer);
    buf[0] = static_cast<int32_t>(tv_sec);
    buf[1] = static_cast<int32_t>(tv_usec);
  }

  inline void
  Time::unpack(const void* buffer) noexcept
  {
    const int32_t* buf = static_cast<const int32_t*>(buffer);
    set(static_cast<time_t>(buf[0]), static_cast<suseconds_t>(buf[1]));
  }

  inline Time&
  Time::operator +=(const Time& time) noexcept
  {
    tv_sec += time.tv_sec;
    tv_usec += time.tv_usec;
    if (tv_usec >= USEC_MAX)
    {
      tv_sec++;
      tv_usec -= USEC_MAX;
    }
    return *this;
  }

  inline Time&
  Time::operator +=(time_t time) noexcept
  {
    tv_sec += time;
    return *this;
  }

  inline Time&
  Time::operator -=(const Time& time) noexcept
  {
    tv_sec -= time.tv_sec;
    if (tv_usec < time.tv_usec)
    {
      tv_sec--;
      tv_usec += USEC_MAX - time.tv_usec;
    }
    else
    {
      tv_usec -= time.tv_usec;
    }
    return *this;
  }

  inline Time&
  Time::operator -=(time_t time) noexcept
  {
    tv_sec -= time;
    return *this;
  }

  inline Time&
  Time::operator *=(int multiplier) noexcept
  {
    const bool INVERT_SIGN = tv_sec < 0;
    const bool NEG_MULTIPLIER = multiplier < 0;

    if (INVERT_SIGN)
    {
      invert_sign();
    }
    if (NEG_MULTIPLIER)
    {
      multiplier = -multiplier;
    }

    if (multiplier < INT_MAX / USEC_MAX)
    {
      time_t usec = static_cast<time_t>(tv_usec) * multiplier;
      set(tv_sec * multiplier + usec / USEC_MAX,
        static_cast<suseconds_t>(usec % USEC_MAX));
    }
    else
    {
      long long usec = static_cast<long long>(tv_usec) * multiplier;
      set(static_cast<time_t>(tv_sec * multiplier + usec / USEC_MAX),
        static_cast<suseconds_t>(usec % USEC_MAX));
    }

    if (INVERT_SIGN != NEG_MULTIPLIER)
    {
      invert_sign();
    }

    return *this;
  }

  inline Time&
  Time::operator /=(int divisor) noexcept
  {
    const bool INVERT_SIGN = tv_sec < 0;
    const bool NEG_DIVISOR = divisor < 0;

    if (INVERT_SIGN)
    {
      invert_sign();
    }
    if (NEG_DIVISOR)
    {
      divisor = -divisor;
    }

    time_t sec = tv_sec / divisor;
    time_t sec_left = tv_sec - sec * divisor;

    if (sec_left < INT_MAX / USEC_MAX)
    {
      set(sec, static_cast<suseconds_t>((sec_left * USEC_MAX + tv_usec) /
        divisor));
    }
    else
    {
      set(sec, static_cast<suseconds_t>((static_cast<long long>(sec_left) *
        USEC_MAX + tv_usec) / divisor));
    }

    if (INVERT_SIGN != NEG_DIVISOR)
    {
      invert_sign();
    }

    return *this;
  }

  inline std::string
  Time::gm_ft() const /*throw (Gears::Exception)*/
  {
    return get_gm_time().format("%F %T");
  }

  inline double
  Time::as_double() const
  {
    return static_cast<double>(fractional_part()) / USEC_MAX + integer_part();
  }

CLOSE_NAMESPACE

namespace TimeHelper
{
  inline
  Gears::Time
  mul(const timeval& tv, int multiplier) noexcept
  {
    return Gears::Time(tv.tv_sec * multiplier +
      static_cast<time_t>(tv.tv_usec) * multiplier /
        Gears::Time::USEC_MAX,
      static_cast<suseconds_t>(static_cast<time_t>(tv.tv_usec) *
        multiplier % Gears::Time::USEC_MAX));
  }

  inline
  Gears::Time
  div(const timeval& tv, int divisor) noexcept
  {
    return Gears::Time(tv.tv_sec / divisor,
      static_cast<suseconds_t>((
        (tv.tv_sec - tv.tv_sec / divisor * divisor) *
          Gears::Time::USEC_MAX + tv.tv_usec) / divisor));
  }
}

//
// Global functions
//

// Comparance functions

inline
bool
operator ==(const Gears::Time& tv1, const timeval& tv2) noexcept
{
  return tv1.tv_sec == tv2.tv_sec && tv1.tv_usec == tv2.tv_usec;
}

inline
bool
operator !=(const Gears::Time& tv1, const timeval& tv2) noexcept
{
  return tv1.tv_sec != tv2.tv_sec || tv1.tv_usec != tv2.tv_usec;
}

inline
bool
operator <(const Gears::Time& tv1, const timeval& tv2) noexcept
{
  return tv1.tv_sec < tv2.tv_sec ||
    (tv1.tv_sec == tv2.tv_sec && tv1.tv_usec < tv2.tv_usec);
}

inline
bool
operator >(const Gears::Time& tv1, const timeval& tv2) noexcept
{
  return tv1.tv_sec > tv2.tv_sec ||
    (tv1.tv_sec == tv2.tv_sec && tv1.tv_usec > tv2.tv_usec);
}

inline
bool
operator <=(const Gears::Time& tv1, const timeval& tv2) noexcept
{
  return tv1.tv_sec < tv2.tv_sec ||
    (tv1.tv_sec == tv2.tv_sec && tv1.tv_usec <= tv2.tv_usec);
}

inline
bool
operator >=(const Gears::Time& tv1, const timeval& tv2) noexcept
{
  return tv1.tv_sec > tv2.tv_sec ||
    (tv1.tv_sec == tv2.tv_sec && tv1.tv_usec >= tv2.tv_usec);
}

// Arithmetic functions

inline
Gears::Time
operator -(const Gears::Time& tv) noexcept
{
  return tv.tv_usec ? Gears::Time(-tv.tv_sec - 1,
    Gears::Time::USEC_MAX - tv.tv_usec) : Gears::Time(-tv.tv_sec, 0);
}

inline
Gears::Time
abs(const Gears::Time& tv) noexcept
{
  return tv.tv_sec < 0 ? -Gears::Time(tv) : Gears::Time(tv);
}

inline
Gears::Time
operator +(const Gears::Time& tv1, const timeval& tv2) noexcept
{
  return tv1.tv_usec + tv2.tv_usec >= Gears::Time::USEC_MAX ?
    Gears::Time(tv1.tv_sec + tv2.tv_sec + 1,
      tv1.tv_usec + tv2.tv_usec - Gears::Time::USEC_MAX) :
    Gears::Time(tv1.tv_sec + tv2.tv_sec, tv1.tv_usec + tv2.tv_usec);
}

inline
Gears::Time
operator +(const Gears::Time& tv, time_t time) noexcept
{
  return Gears::Time(tv.tv_sec + time, tv.tv_usec);
}

inline
Gears::Time
operator -(const Gears::Time& tv1, const timeval& tv2) noexcept
{
  return tv1.tv_usec < tv2.tv_usec ?
    Gears::Time(tv1.tv_sec - tv2.tv_sec - 1,
      Gears::Time::USEC_MAX + tv1.tv_usec - tv2.tv_usec) :
    Gears::Time(tv1.tv_sec - tv2.tv_sec, tv1.tv_usec - tv2.tv_usec);
}

inline
Gears::Time
operator -(const Gears::Time& tv, time_t time) noexcept
{
  return Gears::Time(tv.tv_sec - time, tv.tv_usec);
}

inline
Gears::Time
operator *(const Gears::Time& tv, int multiplier) noexcept
{
  return (tv.tv_sec < 0) == (multiplier < 0) ?
    TimeHelper::mul(abs(tv), std::abs(multiplier)) :
    -TimeHelper::mul(abs(tv), std::abs(multiplier));
}

inline
Gears::Time
operator /(const Gears::Time& tv, int divisor) noexcept
{
  return (tv.tv_sec < 0) == (divisor < 0) ?
    TimeHelper::div(abs(tv), std::abs(divisor)) :
    -TimeHelper::div(abs(tv), std::abs(divisor));
}

#endif
