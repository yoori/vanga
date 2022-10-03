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

#include "LogHelper.hpp"

namespace Gears
{
  namespace
  {
    const SubString SEVERITY_LABLES[] =
    {
      SubString("EMERGENCY"),
      SubString("ALERT"),
      SubString("CRITICAL"),
      SubString("ERROR"),
      SubString("WARNING"),
      SubString("NOTICE"),
      SubString("INFO"),
      SubString("DEBUG"),
      SubString("TRACE")
    };
  }

  void
  LogHelper::get_buffer_(
    std::list<Buffer_var>& res_buffers)
    noexcept
  {
    {
      SyncPolicy::WriteGuard lock(lock_);
      res_buffers.splice(buffers_.begin(), buffers_);
    }

    if(res_buffers.empty())
    {
      res_buffers.push_back(new Buffer());
    }
  }

  void
  LogHelper::prepare_buf_(
    Buffer& buf,
    const Gears::ExtendedTime* time,
    const Gears::SubString& text,
    const unsigned long* severity,
    const Gears::SubString* aspect,
    const Gears::SubString* code)
    noexcept
  {
    buf.prepare(
      text.size() +
      (aspect ? aspect->size() : 0) +
      (code ? code->size() : 0) + 1024);

    if (time)
    {
      strftime(buf.get(), buf.size(), "%a %d %b %Y", time);

      buf.advance(strlen(buf.get()));

      snprintf(
        buf.get(),
        buf.size(),
        " %02d:%02d:%02d:%06d ",
        time->tm_hour,
        time->tm_min,
        time->tm_sec,
        time->tm_usec);

      buf.advance(17);
    }

    if(code)
    {
      char* const BUFF = buf.get();

      const size_t RECORD_CODE_SIZE = code->size();
      assert(buf.size() >= RECORD_CODE_SIZE + 3);
      BUFF[0] = '[';
      ::memcpy(BUFF + 1, code->data(), RECORD_CODE_SIZE);
      BUFF[1 + RECORD_CODE_SIZE + 0] = ']';
      BUFF[1 + RECORD_CODE_SIZE + 1] = ' ';
      buf.advance(RECORD_CODE_SIZE + 3);
    }

    if(severity)
    {
      const size_t SEVERITIES =
        sizeof(SEVERITY_LABLES) / sizeof(*SEVERITY_LABLES);

      const SubString& SEVERITY =
        SEVERITY_LABLES[*severity < SEVERITIES ?
          *severity : (SEVERITIES - 1)];

      char* BUFF = buf.get();

      const size_t SEVERITY_SIZE = SEVERITY.size();
      assert(buf.size() >= SEVERITY.size() + 4 + 20);
      BUFF[0] = '[';
      memcpy(BUFF + 1, SEVERITY.data(), SEVERITY_SIZE);

      if(*severity >= SEVERITIES - 1)
      {
        buf.advance(SEVERITY_SIZE + 1);

        snprintf(
          buf.get(),
          buf.size(),
          " %lu] ",
          *severity - (SEVERITIES - 1));

        buf.advance(strlen(buf.get()));
      }
      else
      {
        BUFF[1 + SEVERITY_SIZE + 0] = ']';
        BUFF[1 + SEVERITY_SIZE + 1] = ' ';
        buf.advance(SEVERITY_SIZE + 3);
      }
    }

    if(aspect)
    {
      char* const BUFF = buf.get();
      const size_t RECORD_ASPECT_SIZE = aspect->size();
      assert(buf.size() >= RECORD_ASPECT_SIZE + 3);
      BUFF[0] = '[';
      memcpy(BUFF + 1, aspect->data(), RECORD_ASPECT_SIZE);
      BUFF[1 + RECORD_ASPECT_SIZE + 0] = ']';
      BUFF[1 + RECORD_ASPECT_SIZE + 1] = ' ';
      buf.advance(RECORD_ASPECT_SIZE + 3);
    }

    if(time || severity || aspect)
    {
      assert(buf.size() >= 2);
      buf.get()[0] = ':';
      buf.get()[1] = ' ';
      buf.advance(2);
    }

    assert(buf.size() >= text.size() + 2);
    memcpy(buf.get(), text.data(), text.size());
    buf.advance(text.size());

    buf.get()[0] = '\n';
    buf.get()[1] = '\0';
  }
}
