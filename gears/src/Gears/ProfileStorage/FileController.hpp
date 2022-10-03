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

#ifndef FILECONTROLLER_HPP_
#define FILECONTROLLER_HPP_

#include <unordered_map>
#include <Gears/Basic/AtomicRefCountable.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>
#include <Gears/Basic/Time.hpp>
#include <Gears/Basic/MemBuf.hpp>
#include <Gears/Basic/Lock.hpp>
#include <Gears/Basic/AtomicCounter.hpp>

namespace Gears
{
  class FileController: public virtual AtomicRefCountable
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, DescriptiveException);

  public:
    virtual SmartMemBuf_var
    create_buffer() const noexcept = 0;

    virtual int
    open(const char* file_name, int flags, mode_t mode)
      /*throw(Exception)*/ = 0;

    virtual void
    close(int fd) /*throw(Exception)*/ = 0;

    virtual ssize_t
    pread(int fd, void* val, unsigned long read_size, unsigned long fd_pos)
      /*throw(Exception)*/ = 0;

    virtual ssize_t
    read(int fd, void* val, unsigned long read_size)
      /*throw(Exception)*/ = 0;

    virtual ssize_t
    write(int fd, const void* val, unsigned long write_size)
      /*throw(Exception)*/ = 0;

  protected:
    virtual
    ~FileController() noexcept = default;
  };

  typedef IntrusivePtr<FileController>
    FileController_var;

  class PosixFileController: public FileController
  {
  public:
    using FileController::Exception;

    class Stat: public AtomicRefCountable
    {
      friend class PosixFileController;

    protected:
      virtual void
      add_read_time_(
        const Time& start,
        const Time& stop,
        unsigned long size) noexcept = 0;

      virtual void
      add_write_time_(
        const Time& start,
        const Time& stop,
        unsigned long size) noexcept = 0;

      virtual
      ~Stat() noexcept = default;
    };

    typedef IntrusivePtr<Stat> Stat_var;

  public:
    PosixFileController(
      Stat* pread_stat = 0,
      uint64_t min_free_space = 0,
      unsigned long free_space_check_size_period = 0)
      noexcept;

    virtual SmartMemBuf_var
    create_buffer() const noexcept;

    virtual int
    open(const char* file_name, int flags, mode_t mode)
      /*throw(Exception)*/;

    virtual void
    close(int fd) /*throw(Exception)*/;

    virtual ssize_t
    pread(int fd_, void* val, unsigned long read_size, unsigned long fd_pos)
      /*throw(Exception)*/;

    virtual ssize_t
    read(int fd_, void* val, unsigned long read_size)
      /*throw(Exception)*/;

    virtual ssize_t
    write(int fd_, const void* val, unsigned long write_size)
      /*throw(Exception)*/;

  protected:
    virtual ~PosixFileController() noexcept = default;

    ssize_t
    pread_(int fd_, void* val, unsigned long read_size, unsigned long fd_pos)
      /*throw(Exception)*/;

    ssize_t
    read_(int fd_, void* val, unsigned long read_size)
      /*throw(Exception)*/;

    ssize_t
    write_(int fd_, const void* val, unsigned long write_size)
      /*throw(Exception)*/;

    bool
    control_devices_() const noexcept;

  protected:
    typedef unsigned long DeviceId;

    class Device: public AtomicRefCountable
    {
    public:
      Device() noexcept;

      AtomicCounter write_size_meter;

    protected:
      virtual
      ~Device() noexcept = default;
    };

    typedef IntrusivePtr<Device>
      Device_var;

    typedef std::unordered_map<DeviceId, Device_var>
      DeviceMap;

    typedef std::unordered_map<int, Device_var> FileMap;

    typedef Gears::RWLock DevicesSyncPolicy;

    typedef Gears::RWLock FilesSyncPolicy;

  protected:
    mutable BasicBufAllocator_var allocator_;
    const uint64_t min_free_space_;
    const unsigned long free_space_check_size_period_;

    Stat_var stat_;

    DevicesSyncPolicy devices_lock_;
    DeviceMap devices_;

    FilesSyncPolicy files_lock_;
    FileMap files_;
  };

  class StatImpl: public PosixFileController::Stat
  {
  public:
    struct Counters
    {
      Time max_time;
      Time sum_time;
      unsigned long count;
      uint64_t sum_size;

      Counters() noexcept;

      Time
      avg_time() const noexcept;

      void
      print(std::ostream& out) const noexcept;
    };

  public:
    Counters
    read_counters() const noexcept;

    Counters
    write_counters() const noexcept;

    IntrusivePtr<StatImpl>
    reset() noexcept;

  protected:
    typedef Gears::Mutex SyncPolicy;

    struct CountersHolder: public Counters
    {
      mutable SyncPolicy lock;
    };

  protected:
    virtual
    ~StatImpl() noexcept = default;

    virtual void
    add_read_time_(
      const Time& start,
      const Time& stop,
      unsigned long size)
      noexcept;

    virtual void
    add_write_time_(
      const Time& start,
      const Time& stop,
      unsigned long size)
      noexcept;

    void
    add_time_(
      CountersHolder& counters_holder,
      const Time& start,
      const Time& stop,
      unsigned long size)
      noexcept;

  protected:
    CountersHolder read_counters_;
    CountersHolder write_counters_;
  };

  typedef IntrusivePtr<StatImpl> StatImpl_var;

  class SSDFileController: public FileController
  {
  public:
    using FileController::Exception;

    SSDFileController(
      FileController* delegate_file_controller,
      unsigned long write_block_size = 128*1024)
      noexcept;

    virtual SmartMemBuf_var
    create_buffer() const noexcept;

    virtual int
    open(const char* file_name, int flags, mode_t mode)
      /*throw(Exception)*/;

    virtual void
    close(int fd) /*throw(Exception)*/;

    virtual ssize_t
    pread(int fd, void* val, unsigned long read_size, unsigned long fd_pos)
      /*throw(Exception)*/;

    virtual ssize_t
    read(int fd, void* val, unsigned long read_size)
      /*throw(Exception)*/;

    virtual ssize_t
    write(int fd, const void* val, unsigned long write_size)
      /*throw(Exception)*/;

  protected:
    typedef Gears::RWLock SyncPolicy;

  protected:
    FileController_var delegate_file_controller_;
    const unsigned long write_block_size_;
    SyncPolicy operations_lock_;
  };
}

#endif /*FILECONTROLLER_HPP_*/
