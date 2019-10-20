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

#ifndef LEVELPROFILEMAP_HPP
#define LEVELPROFILEMAP_HPP

#include <vector>
#include <Gears/Basic/Lock.hpp>
#include <Gears/Threading/Scheduler.hpp>
#include <Gears/Threading/TaskRunner.hpp>
#include <Gears/Threading/CompositeActiveObject.hpp>

#include "ProfileMap.hpp"
#include "BaseLevel.hpp"
#include "RWMemLevel.hpp"
#include "LoadingProgressCallbackBase.hpp"
#include "FileController.hpp"

namespace Gears
{
  struct LevelMapTraits
  {
    enum Mode
    {
      BLOCK_RUNTIME = 0,
      NONBLOCK_RUNTIME,
    };

    LevelMapTraits(
      Mode mode_val,
      unsigned long rw_buffer_size_val,
      uint64_t rwlevel_max_size_val,
      uint64_t max_undumped_size_val,
      unsigned long max_levels0_val,
      const Time& expire_time_val,
      FileController* file_controller_val = 0)
      throw()
      : mode(mode_val),
        rw_buffer_size(rw_buffer_size_val),
        rwlevel_max_size(rwlevel_max_size_val),
        max_undumped_size(max_undumped_size_val),
        max_levels0(max_levels0_val),
        expire_time(expire_time_val),
        file_controller(add_ref(file_controller_val))
    {}

    Mode mode;
    unsigned long rw_buffer_size;
    uint64_t rwlevel_max_size;
    uint64_t max_undumped_size;
    unsigned long max_levels0;
    Time expire_time;
    FileController_var file_controller;
  };

  template<typename KeyType, typename KeySerializerType>
  class LevelProfileMap:
    public virtual ProfileMap<KeyType>,
    public CompositeActiveObject
  {
  public:
    DECLARE_GEARS_EXCEPTION(Exception, typename ProfileMap<KeyType>::Exception);
    DECLARE_GEARS_EXCEPTION(Blocked, Exception);
    DECLARE_GEARS_EXCEPTION(NotActive, Exception);

    typedef KeySerializerType KeySerializerT;

  public:
    LevelProfileMap(
      ActiveObjectCallback* callback,
      const char* directory,
      const char* file_prefix,
      const LevelMapTraits& traits,
      LoadingProgressCallbackBase* progress_checker_parent = 0)
      throw (Gears::Exception);

    virtual void
    wait_preconditions(const KeyType&, OperationPriority) const
      throw(NotActive, Exception);

    virtual bool
    check_profile(const KeyType& key) const throw(Exception);

    virtual ConstSmartMemBuf_var
    get_profile(
      const KeyType& key,
      Time* last_access_time = 0)
      throw(Exception);

    virtual void
    save_profile(
      const KeyType& key,
      ConstSmartMemBuf* mem_buf,
      const Time& now = Time::get_time_of_day(),
      OperationPriority op_priority = OP_RUNTIME)
      throw(NotActive, Blocked, Exception);

    virtual bool
    remove_profile(
      const KeyType& key,
      OperationPriority op_priority = OP_RUNTIME)
      throw(NotActive, Blocked, Exception);

    virtual void
    copy_keys(typename ProfileMap<KeyType>::KeyList& keys) throw(Exception);

    virtual void
    clear_expired(const Time& /*expire_time*/)
      throw(Exception)
    {}

    virtual unsigned long
    size() const throw();

    virtual uint64_t
    area_size() const throw();

    bool
    empty() const throw();

    void
    dump() throw();

    // ActiveObject interface
    virtual void
    activate_object() throw(ActiveObject::Exception);

    void
    deactivate_object() throw();

    void
    wait_object() throw();

  public:
    class LevelAssigner;

    typedef Gears::Mutex MapHolderChangeSyncPolicy;
    typedef Gears::Mutex MergeTaskCountSyncPolicy;
    typedef Gears::Mutex OpSyncPolicy;
    typedef Gears::RWLock SyncPolicy;

    struct LevelHolder: public Gears::AtomicRefCountable
    {
      LevelHolder() throw();

      unsigned long index;
      unsigned long sub_index;
      unsigned long uniq_index;
      Gears::IntrusivePtr<ReadBaseLevel<KeyType> > read_level;
      std::string full_index_file_name;
      std::string full_body_file_name;
      bool to_remove;
      bool backup;

    protected:
      virtual ~LevelHolder() throw();
    };

    struct LevelHolderPtrLess;
    
    typedef IntrusivePtr<LevelHolder> LevelHolder_var;
    typedef std::vector<LevelHolder_var> LevelHolderArray;

    struct MapHolder: public Gears::AtomicRefCountable
    {
      IntrusivePtr<
        RWMemLevel<KeyType, KeySerializerType> > rw_level;
      LevelHolderArray levels;

    protected:
      virtual ~MapHolder() throw()
      {}
    };

    typedef Gears::IntrusivePtr<MapHolder> MapHolder_var;
    typedef Gears::IntrusivePtr<const MapHolder> ConstMapHolder_var;
    typedef LevelProfileMap<KeyType, KeySerializerType> ThisMap;
    typedef Gears::IntrusivePtr<ThisMap> ThisMap_var;

    class DumpRWLevelTask;
    class DumpMemLevelTask;
    class MergeLevelTask;
    class ClearExpiredTask;

    struct FileNames
    {
      std::string regular;
      std::string dump_temporary;
    };

  protected:
    virtual
    ~LevelProfileMap() throw();

    // task methods
    void
    exchange_rw_level_i_()
      throw(Exception);

    void
    dump_mem_level_(
      unsigned long index,
      unsigned long sub_index)
      throw(Gears::Exception);

    void
    merge_levels_()
      throw();

    void
    add_level_(
      MapHolder* map_holder,
      LoadingProgressCallbackBase_var progress_checker_parent,
      const char* directory,
      const char* file_name)
      throw(Exception);

    typename ReadBaseLevel<KeyType>::Iterator_var
    create_filter_iterator_(
      typename ReadBaseLevel<KeyType>::Iterator* it)
      throw();

    ConstMapHolder_var
    get_map_holder_() const throw();

    template<typename IteratorType, typename MapHolderType>
    IteratorType
    find_level_(
      MapHolderType* map_holder,
      unsigned long index,
      unsigned long sub_index)
      throw();

    bool
    rw_level_dump_check_by_size_i_(
      const MapHolder* map_holder)
      const throw();

    bool
    rw_level_dump_check_by_time_i_(
      const Time& now)
      const throw();

    static
    bool
    merge_check_(const MapHolder* map_holder)
      throw();

    FileNames
    index_file_name_(
      unsigned long index,
      unsigned long sub_index,
      unsigned long uniq_index)
      const throw();

    FileNames
    body_file_name_(
      unsigned long index,
      unsigned long sub_index,
      unsigned long uniq_index)
      const throw();

    std::string
    index_merge_file_name_()
      const throw();

    std::string
    body_merge_file_name_()
      const throw();

    static void
    print_levels_(std::ostream& ostr, const MapHolder* map_holder)
      throw();

    bool
    signal_by_undumped_size_(
      unsigned long prev_undumped_size,
      unsigned long new_undumped_size)
      const throw();

    bool
    signal_by_levels_change_(
      unsigned long prev_levels0,
      unsigned long new_levels0)
      const throw();

  private:
    const std::string file_directory_;
    const std::string file_prefix_;

    const LevelMapTraits::Mode mode_;
    const unsigned long rw_buffer_size_;
    const uint64_t rwlevel_max_size_;
    const Time max_dump_period_;
    const uint64_t max_undumped_size_;
    const unsigned long max_levels0_;
    const uint64_t max_background_undumped_size_;
    const unsigned long max_background_levels0_;
    const Time expire_time_;
    const FileController_var file_controller_;

    ActiveObjectCallback_var callback_;
    Scheduler_var planner_;
    TaskRunner_var task_runner_;

    mutable MergeTaskCountSyncPolicy merge_tasks_count_lock_;
    mutable Gears::Condition merge_tasks_count_change_;
    unsigned long merge_tasks_count_;
    volatile sig_atomic_t stop_merge_;

    mutable MapHolderChangeSyncPolicy map_holder_change_lock_;

    mutable OpSyncPolicy::Mutex rw_level_lock_;
    mutable Gears::Condition undumped_size_change_;
    bool active_;
    uint64_t undumped_size_;
    unsigned long levels0_;
    Time last_rw_level_dump_time_;

    mutable SyncPolicy map_holder_lock_;
    MapHolder_var map_holder_;
  };
}

#include "LevelProfileMap.tpp"

#endif /*LEVELPROFILEMAP_HPP*/
