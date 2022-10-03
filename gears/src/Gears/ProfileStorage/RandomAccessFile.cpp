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

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

#include <Gears/Basic/OutputMemoryStream.hpp>
#include <Gears/Basic/Errno.hpp>

#include "RandomAccessFile.hpp"

namespace Gears
{
  RandomAccessFile::RandomAccessFile(
    FileController* file_controller)
    noexcept
    : file_controller_(
        file_controller ? add_ref(file_controller) :
        new PosixFileController()),
      file_handle_(-1)
  {}

  RandomAccessFile::RandomAccessFile(
    const char* file_name,
    FileController* file_controller)
    /*throw(PosixException)*/
    : file_controller_(
        file_controller ? add_ref(file_controller) :
        new PosixFileController())
  {
    open(file_name);
  }

  RandomAccessFile::~RandomAccessFile() noexcept
  {
    try
    {
      close();
    }
    catch(const Gears::Exception& ex)
    {
      std::cerr << "RandomAccessFile::~RandomAccessFile(): " << ex.what() <<
        std::endl;
    }
  }

  void
  RandomAccessFile::open(const char* file_name) /*throw(PosixException)*/
  {
    int mode = 0;
    int o_flags = O_RDONLY;

    file_handle_ = ::open64(file_name, o_flags, mode);

    if(file_handle_ < 0)
    {
      throw_errno_exception<PosixException>(
        "Can't open file '", file_name, "'");
    }
  }

  void
  RandomAccessFile::close() /*throw(PosixException)*/
  {
    if(file_handle_ != -1)
    {
      int file_handle = file_handle_;
      file_handle_ = -1;

      if(::close(file_handle) == -1)
      {
        throw_errno_exception<PosixException>(
          "Can't close file");
      }
    }
  }

  unsigned long
  RandomAccessFile::size() /*throw(PosixException)*/
  {
    struct stat64 f_stat;

    if(::fstat64(file_handle_, &f_stat))
    {
      throw_errno_exception<PosixException>(
        "Can't do fstat64");
    }

    return f_stat.st_size;
  }

  void
  RandomAccessFile::pread(
    void* buf, unsigned long read_size, unsigned long pos)
    /*throw(PosixException)*/
  {
    try
    {
      file_controller_->pread(file_handle_, buf, read_size, pos);
    }
    catch(const FileController::Exception& ex)
    {
      OutputStackStream<1024> ostr;
      ostr << "RandomAccessFile::pread(): error on file reading: " <<
        ex.what();
      throw PosixException(ostr.str());
    }
  }

  int
  RandomAccessFile::fd() const noexcept
  {
    return file_handle_;
  }
}
