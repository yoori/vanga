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

#ifndef FILEWRITER_HPP
#define FILEWRITER_HPP

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/MemBuf.hpp>

#include "FileController.hpp"

namespace Gears
{
  class FileWriter
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);

    FileWriter(
      const char* file,
      unsigned long buffer_size,
      bool append = false,
      bool disable_caching = false,
      FileController* file_controller = 0)
      /*throw(Exception)*/;

    FileWriter(
      int fd,
      unsigned long buffer_size,
      FileController* file_controller = 0)
      /*throw(Exception)*/;

    ~FileWriter() noexcept;

    // result can be less then read_size only if reached eof
    void
    write(const void* val, unsigned long read_size)
      /*throw(Exception)*/;

    void
    flush() /*throw(Exception)*/;

    unsigned long
    size() const noexcept;

    void
    close() /*throw(Exception)*/;

  private:
    void
    write_(const void* val, unsigned long read_size)
      /*throw(Exception)*/;

  private:
    const unsigned long buffer_size_;
    const unsigned long direct_write_min_size_;
    FileController_var file_controller_;

    bool fd_own_;
    int fd_;
    unsigned long fd_pos_;

    SmartMemBuf_var mem_buf_;
  };
}

#endif /*FILEWRITER_HPP*/
