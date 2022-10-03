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

#ifndef GEARS_LOGHELPER_HPP_
#define GEARS_LOGHELPER_HPP_

#include <memory>
#include <list>

#include <Gears/Basic/RefCountable.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>
#include <Gears/Basic/Lock.hpp>
#include <Gears/Basic/SubString.hpp>

#include "Logger.hpp"

namespace Gears
{
  class LogHelper
  {
  public:
    class Buffer: public Gears::RefCountable
    {
    public:
      Buffer() noexcept;

      const char*
      data() const noexcept;

      size_t
      data_size() const noexcept;

      char*
      get() const noexcept;

      size_t
      size() const noexcept;

      void
      advance(size_t length) noexcept;

      void
      prepare(int new_size) noexcept;

    protected:
      virtual
      ~Buffer() noexcept = default;

    private:
      std::unique_ptr<char[]> buf_;
      int size_;
      char* cur_ptr_;
      int cur_size_;
    };

    typedef Gears::IntrusivePtr<Buffer> Buffer_var;

    typedef std::list<Buffer_var> BufferList;

  protected:
    typedef Gears::SpinLock SyncPolicy;

  protected:
    void
    get_buffer_(
      BufferList& res_buffers)
      noexcept;

    static void
    prepare_buf_(
      Buffer& buf,
      const Gears::ExtendedTime* time,
      const Gears::SubString& text,
      const unsigned long* severity,
      const Gears::SubString* aspect,
      const Gears::SubString* code)
      noexcept;

  private:
    SyncPolicy lock_;
    std::list<Buffer_var> buffers_;
  };
}

namespace Gears
{
  inline
  LogHelper::Buffer::Buffer() noexcept
    : size_(0),
      cur_ptr_(0),
      cur_size_(0)
  {}

  inline const char*
  LogHelper::Buffer::data() const noexcept
  {
    return buf_.get();
  }

  inline size_t
  LogHelper::Buffer::data_size() const noexcept
  {
    return size_;
  }

  inline char*
  LogHelper::Buffer::get() const noexcept
  {
    return cur_ptr_;
  }

  inline size_t
  LogHelper::Buffer::size() const noexcept
  {
    return cur_size_;
  }

  inline void
  LogHelper::Buffer::advance(size_t length) noexcept
  {
    if(cur_size_ > 0)
    {
      cur_ptr_ += length;
      cur_size_ -= length;
    }
  }

  inline void
  LogHelper::Buffer::prepare(int new_size)
    noexcept
  {
    if(new_size > size_)
    {
      buf_.reset(new char[new_size]);
      size_ = new_size;
    }

    cur_ptr_ = buf_.get();
    cur_size_ = size_;
  }
}

#endif
