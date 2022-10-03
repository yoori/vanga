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

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <Gears/Basic/Errno.hpp>

#include "FileReader.hpp"

namespace Gears
{
  FileReader::FileReader(
    int fd,
    bool exclusive,
    unsigned long buffer_size,
    FileController* file_controller)
    /*throw(Exception)*/
    : exclusive_(exclusive),
      buffer_size_(buffer_size),
      direct_read_min_size_(buffer_size / 2),
      seek_min_size_(16 * 4096),
      file_controller_(
        file_controller ? add_ref(file_controller) :
        new PosixFileController()),
      fd_(fd),
      fd_own_(false),
      fd_pos_(0),
      eof_(false),
      mem_buf_pos_(0)
  {
    struct stat64 f_stat;

    if(::fstat64(fd_, &f_stat))
    {
      throw_errno_exception<Exception>(
        "Can't do fstat64");
    }

    file_size_ = f_stat.st_size;

    mem_buf_ = file_controller_->create_buffer();
  }

  FileReader::FileReader(
    const char* file_name,
    unsigned long buffer_size,
    bool disable_caching,
    FileController* file_controller)
    /*throw(Exception)*/
    : exclusive_(true),
      buffer_size_(buffer_size),
      direct_read_min_size_(
        disable_caching ?
        // don't allow direct read if caching disabled, because result buffer must be aligned
        std::numeric_limits<unsigned long>::max() :
        buffer_size / 2),
      seek_min_size_(16 * 4096),
      file_controller_(
        file_controller ? add_ref(file_controller) :
        new PosixFileController()),
      fd_own_(true),
      fd_pos_(0),
      eof_(false),
      mem_buf_pos_(0)
  {
    fd_ = ::open64(
      file_name,
      O_RDONLY,
      //disable_caching ? O_DIRECT : 0 // temporary disabled - need implement buffers alignment
      0
      );

    if(fd_ < 0)
    {
      throw_errno_exception<Exception>(
        "Can't open file '", file_name, "'");
    }

    struct stat64 f_stat;

    if(::fstat64(fd_, &f_stat))
    {
      throw_errno_exception<Exception>(
        "Can't do fstat64");
    }

    file_size_ = f_stat.st_size;

    mem_buf_ = file_controller_->create_buffer();
  }

  FileReader::~FileReader() noexcept
  {
    if(fd_own_ && fd_ >= 0)
    {
      ::close(fd_);
    }
  }

  unsigned long
  FileReader::read(void* read_buf, unsigned long read_size)
    /*throw(Exception)*/
  {
    if (eof_)
    {
      throw Exception("FileReader::read: EOF achieved");
    }

    unsigned long cur_read_size = read_size;
    unsigned char* cur_read_buf = static_cast<unsigned char*>(read_buf);

    // fill that already loaded into buffer
    if(mem_buf_->membuf().size() - mem_buf_pos_ > 0)
    {
      unsigned long filled_size = std::min(
        mem_buf_->membuf().size() - mem_buf_pos_,
        cur_read_size);

      ::memcpy(
        cur_read_buf,
        static_cast<unsigned char*>(mem_buf_->membuf().data()) + mem_buf_pos_,
        filled_size);

      cur_read_buf += filled_size;
      cur_read_size -= filled_size;
      mem_buf_pos_ += filled_size;
    }

    assert(cur_read_size == 0 || mem_buf_pos_ == mem_buf_->membuf().size());

    // fill part that isn't present in buffer
    while(cur_read_size)
    {
      if(cur_read_size > direct_read_min_size_)
      {
        // read directly into result buffer
        unsigned long read_op_size = read_(cur_read_buf, cur_read_size);

        if(read_op_size == 0)
        {
          // eof
          return cur_read_buf -
            static_cast<unsigned char*>(read_buf);
        }

        cur_read_buf += read_op_size;
        cur_read_size -= read_op_size;
      }
      else
      {
        // small size read, do it into buffer
        unsigned long read_op_size = read_mem_buf_();

        if(read_op_size == 0)
        {
          // eof
          return cur_read_buf -
            static_cast<unsigned char*>(read_buf);
        }

        if(read_op_size < cur_read_size)
        {
          // buffer will be used fully
          ::memcpy(cur_read_buf, mem_buf_->membuf().data(), read_op_size);
          cur_read_buf += read_op_size;
          mem_buf_pos_ = 0;
          cur_read_size -= read_op_size;
        }
        else
        {
          ::memcpy(cur_read_buf,
            static_cast<unsigned char*>(mem_buf_->membuf().data()),
            cur_read_size);
          cur_read_buf += cur_read_size;
          mem_buf_pos_ = cur_read_size;
          cur_read_size = 0;
        }
      }
    } // while(cur_read_size)

    return read_size;
  }

  unsigned long
  FileReader::pos()
    const noexcept
  {
    return fd_pos_ + mem_buf_pos_ - mem_buf_->membuf().size();
  }

  unsigned long
  FileReader::file_size() const noexcept
  {
    return file_size_;
  }

  bool
  FileReader::eof()
    const noexcept
  {
    return fd_pos_ == file_size_ &&
      mem_buf_pos_ == mem_buf_->membuf().size();
  }

  unsigned long
  FileReader::skip(unsigned long skip_size)
    /*throw(Exception)*/
  {
    /*
    std::cout << "skip begin: fd_pos_ = " << fd_pos_ <<
      ", mem_buf_pos_ = " << mem_buf_pos_ <<
      ", skip_size = " << skip_size << std::endl;
    */
    unsigned long cur_skip_size = skip_size;

    while(cur_skip_size)
    {
      /*
      std::cout << "skip step: fd_pos_ = " << fd_pos_ <<
        ", mem_buf_pos_ = " << mem_buf_pos_ <<
        ", cur_skip_size = " << cur_skip_size <<
        ", mem_buf_->size() = " << mem_buf_->size() << std::endl;
      */
      if(mem_buf_->membuf().size() - mem_buf_pos_ > 0)
      {
        /*
        std::cout << "skip on membuf: fd_pos_ = " << fd_pos_ <<
          ", mem_buf_pos_ = " << mem_buf_pos_ <<
          ", mem_buf_->size() = " << mem_buf_->size() << std::endl;
        */
        // skip on mem_buf_
        if(mem_buf_->membuf().size() - mem_buf_pos_ >= cur_skip_size)
        {
          mem_buf_pos_ += cur_skip_size;
          /*
          std::cout << "skip return(1): fd_pos_ = " << fd_pos_ <<
            ", mem_buf_pos_ = " << mem_buf_pos_ << std::endl;
          */
          return skip_size;
        }
        else
        {
          cur_skip_size -= mem_buf_->membuf().size() - mem_buf_pos_;
          //fd_pos_ += mem_buf_->size() - mem_buf_pos_;
          mem_buf_->membuf().resize(0);
          mem_buf_pos_ = 0;
        }
      }

      if(cur_skip_size)
      {
        if(cur_skip_size < seek_min_size_)
        {
          /*
          std::cout << "skip small on membuf: fd_pos_ = " << fd_pos_ <<
            ", cur_skip_size = " << cur_skip_size <<
            ", mem_buf_pos_ = " << mem_buf_pos_ <<
            ", mem_buf_->size() = " << mem_buf_->size() << std::endl;
          */
          // small skip, read buf and go to skip on mem_buf_
          unsigned long read_op_size = read_mem_buf_();

          if(read_op_size == 0)
          {
            /*
            std::cout << "skip return(2): fd_pos_ = " << fd_pos_ <<
              ", mem_buf_pos_ = " << mem_buf_pos_ << std::endl;
            */
            return skip_size - cur_skip_size;
          }
        }
        else
        {
          /*
          std::cout << "skip by seek: fd_pos_ = " << fd_pos_ <<
            ", mem_buf_pos_ = " << mem_buf_pos_ <<
            ", mem_buf_->size() = " << mem_buf_->size() << std::endl;
          */
          off_t seek_res = 0;
          unsigned long res_skip_size;

          if(file_size_ < fd_pos_ + cur_skip_size)
          {
            if(exclusive_)
            {
              seek_res = ::lseek(fd_, 0, SEEK_END);
            }
            res_skip_size = (skip_size - cur_skip_size) +
              file_size_ - fd_pos_;

            fd_pos_ = file_size_;
          }
          else
          {
            if(exclusive_)
            {
              seek_res = ::lseek(fd_, cur_skip_size, SEEK_CUR);
            }
            res_skip_size = skip_size;

            fd_pos_ += cur_skip_size;
          }

          if(seek_res < 0)
          {
            OutputStackStream<1024> ostr;
            ostr << "FileReader::seek(): error on file seeking.";
            throw_errno_exception<Exception>(ostr.str());
          }

          /*
          std::cout << "skip return(3): fd_pos_ = " << fd_pos_ <<
            ", mem_buf_pos_ = " << mem_buf_pos_ << std::endl;
          */
          return res_skip_size;
        }
      }
    } // while(cur_skip_size)

    /*
    std::cout << "skip return(4): skip_size = " << skip_size <<
      ", fd_pos_ = " << fd_pos_ <<
      ", mem_buf_pos_ = " << mem_buf_pos_ << std::endl;
    */

    return skip_size;
  }

  unsigned long
  FileReader::read_(void* buf, unsigned long read_size)
    /*throw(Exception)*/
  {
    assert(!eof_);

    ssize_t res;

    try
    {
      if(exclusive_)
      {
        res = file_controller_->read(fd_, buf, read_size);
      }
      else
      {
        res = file_controller_->pread(fd_, buf, read_size, fd_pos_);
      }
    }
    catch(const FileController::Exception& ex)
    {
      ErrorStream ostr;
      ostr << "FileReader::read_(): error on file reading: " <<
        ex.what();
      throw Exception(ostr.str());
    }

    if(res == 0)
    {
      eof_ = true;
    }

    fd_pos_ += res;
    return static_cast<unsigned long>(res);
  }

  unsigned long
  FileReader::read_mem_buf_()
    /*throw(Exception)*/
  {
    if(mem_buf_->membuf().capacity() == 0)
    {
      mem_buf_->membuf().alloc(buffer_size_);
    }

    unsigned long read_op_size = read_(
      static_cast<unsigned char*>(mem_buf_->membuf().data()),
      buffer_size_);

    mem_buf_->membuf().resize(read_op_size);
    mem_buf_pos_ = 0;
    return read_op_size;
  }
}
