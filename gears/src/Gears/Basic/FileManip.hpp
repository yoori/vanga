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

#ifndef GEARS_FILEMANIP_HPP_
#define GEARS_FILEMANIP_HPP_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>

#include "Exception.hpp"
#include "Errno.hpp"
#include "SubString.hpp"

namespace Gears
{
  namespace FileManip
  {
    DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);

    bool
    dir_exists(const Gears::SubString& path) throw();

    bool
    file_exists(const Gears::SubString& path) throw();

    void
    rename(const Gears::SubString& src,
      const Gears::SubString& dst,
      bool ignore_non_existing)
      throw(Gears::Exception);

    void
    split_path(
      const char* path_name,
      std::string* path,
      std::string* name,
      bool strip_slash = true)
      throw();
  }
}

namespace Gears
{
namespace FileManip
{
  inline bool
  dir_exists(const Gears::SubString& path) throw()
  {
    struct stat info;

    if(::stat(path.str().c_str(), &info) != 0)
    {
      return false;
    }
    else if(info.st_mode & S_IFDIR)
    {
      return true;
    }

    return false;
  }

  inline bool
  file_exists(const Gears::SubString& path) throw()
  {
    struct stat info;

    if(::stat(path.str().c_str(), &info) != 0)
    {
      return false;
    }
    else if(info.st_mode & S_IFREG)
    {
      return true;
    }

    return false;
  }

  inline void
  rename(const Gears::SubString& src,
    const Gears::SubString& dst,
    bool ignore_non_existing)
    throw(Gears::Exception)
  {
    static const char* FUN = "rename()";

    if(std::rename(src.str().c_str(), dst.str().c_str()))
    {
      if(!ignore_non_existing || errno != ENOENT)
      {
        Gears::throw_errno_exception<Exception>(
          FUN,
          ": failed to rename file '",
          src.str().c_str(),
          "' to '",
          dst.str().c_str(),
          "'");
      }
    }
  }

  inline void
  split_path(
    const char* path_name,
    std::string* path,
    std::string* name,
    bool strip_slash)
    throw()
  {
    std::string path_pattern_s(path_name);
    std::string::size_type pos = path_pattern_s.find_last_of('/');
    if (pos != std::string::npos && pos != path_pattern_s.size() - 1)
    {
      if(path)
      {
        path->assign(path_pattern_s, 0, strip_slash ? pos : pos +  1);
      }

      if(name)
      {
        name->assign(
          path_pattern_s,
          pos + 1,
          path_pattern_s.size() - pos - 1);
      }
    }
    else
    {
      if(path)
      {
        path->swap(path_pattern_s);
      }

      if(name)
      {
        name->clear();
      }
    }
  }
}
}


#endif /*COMMONS_FILEMANIP_HPP_*/
