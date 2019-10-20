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

#ifndef FILELEVEL_HPP
#define FILELEVEL_HPP

#include <deque>

#include "BaseLevel.hpp"
#include "ReadMemLevel.hpp"
#include "RandomAccessFile.hpp"
#include "FileReader.hpp"
#include "FileWriter.hpp"
#include "LoadingProgressCallbackBase.hpp"

namespace Gears
{
  template<typename KeyType, typename KeySerializerType>
  class ReadFileLevel: public ReadBaseLevel<KeyType>
  {
  public:
    DECLARE_GEARS_EXCEPTION(Interrupted, typename ReadBaseLevel<KeyType>::Exception);

    class KeyIteratorImpl:
      public ReadBaseLevel<KeyType>::KeyIterator
    {
    public:
      KeyIteratorImpl(
        const ReadFileLevel<KeyType, KeySerializerType>* read_mem_level)
        throw();

      virtual bool
      get_next(
        KeyType& key,
        ProfileOperation& operation,
        Time& access_time)
        throw();

    private:
      const Gears::IntrusivePtr<
        const ReadFileLevel<KeyType, KeySerializerType> > read_file_level_;
      typename ReadFileLevel<KeyType, KeySerializerType>::
        ProfileRefs::const_iterator profiles_it_;
    };

    class IteratorImpl:
      public ReadBaseLevel<KeyType>::Iterator
    {
    public:
      IteratorImpl(
        const ReadFileLevel<KeyType, KeySerializerType>* read_mem_level,
        unsigned long read_buffer_size,
        bool disable_caching,
        FileController* file_controller)
        throw(typename ReadBaseLevel<KeyType>::Exception);

      virtual bool
      get_next(
        KeyType& key,
        ProfileOperation& operation,
        Time& access_time)
        throw(typename ReadBaseLevel<KeyType>::Exception);

      virtual ConstSmartMemBuf_var
      get_profile()
        throw (typename ReadBaseLevel<KeyType>::Exception);

    private:
      const Gears::IntrusivePtr<
        const ReadFileLevel<KeyType, KeySerializerType> > read_file_level_;
      FileReader file_reader_;
      MemBuf key_buf_;
      bool inited_;
      bool profile_inited_;
      bool end_reached_;
      ConstSmartMemBuf_var profile_;
    };

  public:
    // load index from file
    ReadFileLevel(
      const char* index_file_name,
      const char* file_name,
      unsigned long read_buf_size,
      bool disable_caching_on_fetch,
      FileController* file_controller = 0,
      LoadingProgressCallbackBase* progress_checker_parent = 0)
      throw(typename ReadBaseLevel<KeyType>::Exception);

    // save data into file
    ReadFileLevel(
      const char* index_file_name,
      const char* file_name,
      typename ReadBaseLevel<KeyType>::Iterator* src_it,
      unsigned long write_buf_size,
      bool disable_caching_on_fetch,
      volatile sig_atomic_t* interrupter,
      FileController* file_controller = 0)
      throw(Interrupted, typename ReadBaseLevel<KeyType>::Exception);

    virtual CheckProfileResult
    check_profile(const KeyType& key) const
      throw(typename ReadBaseLevel<KeyType>::Exception);

    virtual GetProfileResult
    get_profile(const KeyType& key) const
      throw(typename ReadBaseLevel<KeyType>::Exception);

    virtual typename ReadBaseLevel<KeyType>::KeyIterator_var
    get_key_iterator() const
      throw();

    virtual typename ReadBaseLevel<KeyType>::Iterator_var
    get_iterator(unsigned long read_buffer_size) const
      throw(typename ReadBaseLevel<KeyType>::Exception);

    virtual unsigned long
    size() const
      throw();

    virtual unsigned long
    area_size() const
      throw();

    virtual unsigned long
    merge_free_size() const
      throw();

    virtual Time
    min_access_time() const
      throw();

    void
    rename_files(
      const char* new_index_file_name,
      const char* new_body_file_name)
      throw(typename ReadBaseLevel<KeyType>::Exception);

  protected:
    // pos, size eval in runtime by result file size
    struct ProfileRef
    {
      unsigned long area_size() const
      {
        return sizeof(uint32_t) +
          sizeof(uint64_t) +
          sizeof(uint32_t) +
          sizeof(uint64_t) +
          (operation != PO_ERASE ? size : 0);
      }

      KeyType key;

      ProfileOperation operation;
      uint64_t pos;
      uint32_t size;
      // TODO: eval max size and pos in runtime at saving and keep in file head (
      //  in much cases can be used uint16_t or uint32_t)
      uint32_t access_time; // keep with seconds precision (short variant time < 2106 year)
      // TODO: for exclude 2106 limitation - keep offset relative to minimum time in file
    };

    class ProfileRefsComparator
    {
    public:
      bool
      operator() (
        const ProfileRef& ref,
        const KeyType& key) const
        throw ()
      {
        return (ref.key < key);
      }

      bool
      operator() (
        const KeyType& key,
        const ProfileRef& ref) const
        throw ()
      {
        return (key < ref.key);
      }
    };

    typedef std::deque<ProfileRef> ProfileRefs;

  protected:
    virtual ~ReadFileLevel() throw()
    {}

    void
    read_index_profile_ref_(
      ProfileRef& profile_ref,
      FileReader& reader) const
      throw(FileReader::Exception);

    void
    write_index_profile_ref_(
      FileWriter& writer,
      const ProfileRef& profile_ref) const
      throw(FileWriter::Exception);

    static void
    read_body_head_(
      FileReader& reader,
      unsigned long& version,
      unsigned long& id)
      throw(FileReader::Exception);

    static void
    skip_body_head_(FileReader& reader)
      throw(FileReader::Exception);

    bool
    read_body_key_(
      FileReader& reader,
      MemBuf& key_buf,
      KeyType& key,
      ProfileOperation& operation,
      uint32_t& access_time)
      const throw(
        Gears::Exception,
        FileReader::Exception,
        typename ReadBaseLevel<KeyType>::Exception);

    ConstSmartMemBuf_var
    read_body_profile_(FileReader& reader)
      const throw(Gears::Exception, FileReader::Exception);

    void
    skip_body_profile_(FileReader& reader)
      const throw(FileReader::Exception);

    uint32_t
    read_key_(
      FileReader& reader,
      MemBuf& key_buf,
      KeyType& key)
      const throw(
        Gears::Exception,
        FileReader::Exception,
        typename ReadBaseLevel<KeyType>::Exception);

    unsigned long
    write_key_(
      FileWriter& writer,
      MemBuf& key_buf,
      const KeyType& key)
      const throw(
        Gears::Exception,
        FileWriter::Exception,
        typename ReadBaseLevel<KeyType>::Exception);

    void
    write_null_key_(FileWriter& writer)
      const throw(FileWriter::Exception);

  private:
    const KeySerializerType key_serializer_;
    const bool disable_caching_on_fetch_;
    std::string index_file_name_;
    std::string body_file_name_;
    const FileController_var file_controller_;
    mutable std::unique_ptr<RandomAccessFile> file_;

    /// sorted
    ProfileRefs profiles_;
    unsigned long size_;
    unsigned long area_size_;
    unsigned long merge_free_size_;
    Time min_access_time_;
  };
}

#include "FileLevel.tpp"

#endif /*FILELEVEL_HPP*/
