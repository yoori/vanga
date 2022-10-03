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

#ifndef RANDOMACCESSFILE_HPP
#define RANDOMACCESSFILE_HPP

// We support large files, use mmap64 instead mmap, etc
#ifndef PS_NOT_USE_LARGEFILES

#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#endif

#include <sys/types.h>

#include <Gears/Basic/Exception.hpp>
#include "FileController.hpp"

/**
 * Wrappers over low-level system functions
 */
namespace Gears
{
  /**
   * Handle work with file, that contain user data. Allow open, close, resize,
   * map part of file to memory and unmap chunk of file from memory
   */
  class RandomAccessFile
  {
  public:
    DECLARE_GEARS_EXCEPTION(PosixException, Gears::DescriptiveException);

  public:
    RandomAccessFile(FileController* file_controller = 0)
      noexcept;

    /**
     * This constructor open file_name with specified flag
     */
    RandomAccessFile(
      const char* file_name,
      FileController* file_controller = 0)
      /*throw(PosixException)*/;

    /**
     * Destructor close file
     */
    ~RandomAccessFile() noexcept;

    void
    open(const char* file_name) /*throw(PosixException)*/;

    void
    close() /*throw(PosixException)*/;

    /**
     * @return information about a file - its size.
     */
    unsigned long
    size() /*throw(PosixException)*/;

    void
    pread(void* buf, unsigned long read_size, unsigned long pos)
      /*throw(PosixException)*/;

    int
    fd() const noexcept;

  protected:
    FileController_var file_controller_;
    /// Descriptor for file opening
    int file_handle_;
  };
}

#endif // RANDOMACCESSFILE_HPP
