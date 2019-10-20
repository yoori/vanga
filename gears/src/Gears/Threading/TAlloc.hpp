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

#ifndef GEARS_TALLOC_HPP
#define GEARS_TALLOC_HPP

#include <memory>
#include <cassert>

#include <Gears/Basic/Lock.hpp>
#include "ThreadKey.hpp"

namespace Gears
{
  /**
   * Allocators to use in objects which allocate elements by one.
   * They are list, [multi_]set, [multi_]map.
   * They are NOT vector, deque.
   * unordered_map and unordered_set requires HASH_HACK to be set, but
   * unordered_set does not allow to have pointers as a key then.
   *
   * Type is usually rebound and does not matter in initial declaration.
   * SIZE is amount of elements (memory equivalent) to allocate at once.
   * HASH_HACK true if rebound for SomeType* is used in vector.
   *
   * AllocOnly<Type, SIZE, HASH_HACK>
   * CASE: long-live object with seldom (or never) removed elements.
   * Different threads may add elements.
   * Removal of element does not free memory or makes it available.
   * Destruction of object frees the cached memory.
   *
   * Aggregated<Type, SIZE, HASH_HACK>
   * CASE: long-live object with frequently added and removed elements.
   * Different threads may add and remove elements.
   * removal of element does not free memory but makes it available for
   * future allocations within the object.
   * Destruction of object frees the cached memory.
   *
   * ThreadPool<Type, SIZE, HASH_HACK>
   * CASE: the same thread is used for frequent addition and removal of
   * elements (including total destruction of the object).
   * Cached memory is freed only on process shutdown, therefore RSS may
   * grow if the allocator is used incorrectly.
   *
   *
   * Special case for vectors.
   * VectorBuf<Type, SIZE>
   * CASE: Usually vector has a small size but sometimes it may be big.
   */
  namespace TAlloc
  {
    /**
     * Deallocates the memory only on own destruction.
     * Allocates memory by SIZE packs of Type.
     */
    template <typename Type, const size_t SIZE,
      const bool HASH_HACK = false>
    class AllocOnly : public std::allocator<Type>
    {
    private:
      static_assert(SIZE > 1, "SIZE must be larger");

    public:
      template <typename Other>
      struct rebind
      {
        typedef AllocOnly<Other, SIZE, HASH_HACK> other;
      };

      AllocOnly() throw ();
      AllocOnly(const AllocOnly&) throw ();
      template <typename Other>
      AllocOnly(const AllocOnly<Other, SIZE, HASH_HACK>&) throw ();
      ~AllocOnly() throw ();

      Type*
      allocate(size_t n, const void* = 0) throw (Gears::Exception);

      void
      deallocate(Type* ptr, size_t) throw ();

    private:
      struct Item
      {
        char data[sizeof(Type)];
      };
      struct Block
      {
        Item items[SIZE];
        Block* next;
      };
      Block* all_;
      Item* cur_;
      Item* end_;
    };


    /**
     * Hack for hashes
     */
    template <typename Type, const size_t SIZE>
    class AllocOnly<Type*, SIZE, true> : public std::allocator<Type*>
    {
    public:
      AllocOnly() throw ();
      template <typename Other>
      AllocOnly(const AllocOnly<Other, SIZE, true>&) throw ();

      template <typename Other>
      struct rebind
      {
        typedef AllocOnly<Other, SIZE, true> other;
      };
    };


    /**
     * Helper class for Aggregated to combine logic for different Types
     * with the same sizeof(Type) and SIZE (used for ThreadPoolBase)
     */
    template <const size_t TYPE, const size_t SIZE>
    class AggregatedBase
    {
    private:
      static_assert(SIZE > 1, "SIZE must be larger");

    public:
      AggregatedBase() throw ();
      AggregatedBase(const AggregatedBase&) throw ();
      ~AggregatedBase() throw ();

      void*
      allocate() throw (Gears::Exception);

      void
      deallocate(void* ptr) throw ();

    private:
      union Item
      {
        Item* next;
        char data[TYPE];
      };
      struct Block
      {
        Item items[SIZE];
        Block* next;
      };
      Block* all_;
      Item* head_;
      Item* cur_;
      Item* end_;
    };


    /**
     * AllocOnly with reuse of deallocated memory.
     */
    template <typename Type, const size_t SIZE,
      const bool HASH_HACK = false>
    class Aggregated :
      public std::allocator<Type>,
      private AggregatedBase<sizeof(Type), SIZE>
    {
    public:
      template <typename Other>
      struct rebind
      {
        typedef Aggregated<Other, SIZE, HASH_HACK> other;
      };

      Aggregated() throw () = default;
      template <typename Other>
      Aggregated(const Aggregated<Other, SIZE, HASH_HACK>&) throw ();

      Type*
      allocate(size_t n, const void* = 0) throw (Gears::Exception);

      void
      deallocate(Type* ptr, size_t) throw ();
    };


    /**
     * Hack for hashes
     */
    template <typename Type, const size_t SIZE>
    class Aggregated<Type*, SIZE, true> : public std::allocator<Type*>
    {
    public:
      Aggregated() throw () = default;
      template <typename Other>
      Aggregated(const Aggregated<Other, SIZE, true>&) throw ();

      template <typename Other>
      struct rebind
      {
        typedef Aggregated<Other, SIZE, true> other;
      };
    };


    /**
     * Helper class for ThreadPool to combine pools of different Types
     * with the same sizeof(Type) and SIZE
     */
    template <const size_t TYPE, const size_t SIZE>
    class ThreadPoolBase
    {
    private:
      static_assert(SIZE > 1, "SIZE must be larger");

    protected:
      static
      void*
      allocate_() throw (Gears::Exception);

      static
      void
      deallocate_(void* ptr) throw ();

    private:
      class MemoryHolder : public AggregatedBase<TYPE, SIZE>
      {
      public:
        MemoryHolder* next;
      };

      class GlobalMemoryHolder : private Uncopyable
      {
      public:
        ~GlobalMemoryHolder() throw ();

        MemoryHolder*
        operator ->() const throw (Gears::Exception);

      private:
        static
        void
        delete_holder_(void* holder) throw ();

        static ThreadKey<MemoryHolder> key_;
        static SpinLock lock_;
        static MemoryHolder* head_;
      };

      static GlobalMemoryHolder holder_;
    };


    /**
     * Thread shared pool of Type elements.
     * Allocates memory by SIZE packs of Type.
     * Deallocated elements are stored as a single linked list.
     * When thread is terminated elements go to the global pool to be
     * given to a newly created thread.
     * Never frees memory.
     */
    template <typename Type, const size_t SIZE,
      const bool HASH_HACK = false>
    class ThreadPool :
      public std::allocator<Type>,
      private ThreadPoolBase<sizeof(Type), SIZE>
    {
    public:
      template <typename Other>
      struct rebind
      {
        typedef ThreadPool<Other, SIZE, HASH_HACK> other;
      };

      ThreadPool() throw () = default;
      ThreadPool(const ThreadPool&) throw ();
      template <typename Other>
      ThreadPool(const ThreadPool<Other, SIZE, HASH_HACK>&) throw ();

      Type*
      allocate(size_t n, const void* = 0) throw (Gears::Exception);

      void
      deallocate(Type* ptr, size_t) throw ();
    };


    /**
     * Hack for hashes
     */
    template <typename Type, const size_t SIZE>
    class ThreadPool<Type*, SIZE, true> : public std::allocator<Type*>
    {
    public:
      ThreadPool() throw () = default;
      template <typename Other>
      ThreadPool(const ThreadPool<Other, SIZE, true>&) throw ();

      template <typename Other>
      struct rebind
      {
        typedef ThreadPool<Other, SIZE, true> other;
      };
    };


    /**
     * Global shared pool of Type elements.
     * Allocates memory by SIZE packs of Type.
     * Deallocated elements are stored as a single linked list.
     * Never frees memory.
     * Slow, don't use it.
     */
    template <typename Type, const size_t SIZE,
      const bool HASH_HACK = false>
    class GlobalPool : public std::allocator<Type>
    {
    private:
      static_assert(SIZE > 1, "SIZE must be larger");

    public:
      template <typename Other>
      struct rebind
      {
        typedef GlobalPool<Other, SIZE> other;
      };

      GlobalPool() throw () = default;
      GlobalPool(const GlobalPool&) throw ();
      template <typename Other>
      GlobalPool(const GlobalPool<Other, SIZE>&) throw ();

      Type*
      allocate(size_t n, const void* = 0) throw (Gears::Exception);

      void
      deallocate(Type* ptr, size_t) throw ();

    private:
      class MemoryHolder : private Uncopyable
      {
      public:
        MemoryHolder() throw ();
        ~MemoryHolder() throw () = default;

        void*
        allocate() throw (Gears::Exception);
        void
        deallocate(void* ptr) throw ();

      private:
        union Block
        {
          Block* next;
          char data[sizeof(Type)];
        };

        Gears::SpinLock lock_;
        Block* head_;
        Block* cur_;
        Block* end_;
      };

      static MemoryHolder holder_;
    };


    /**
     * Special Allocator for vectors.
     * If required size is not greater than SIZE
     * it doesn't use dynamic memory.
     * Use v.reserve(v.get_allocator().capacity()) in the beginning.
     */
    template <typename Type, const size_t SIZE>
    class VectorBuf : public std::allocator<Type>
    {
    private:
      static_assert(SIZE > 0, "SIZE must be larger");

    public:
      template <typename Other>
      struct rebind
      {
        typedef VectorBuf<Other, SIZE> other;
      };

      VectorBuf() throw ();
      VectorBuf(const VectorBuf&) throw ();
      template <typename Other>
      VectorBuf(const VectorBuf<Other, SIZE>&) throw ();

      Type*
      allocate(size_t n, const void* ptr = 0) throw (Gears::Exception);

      void
      deallocate(Type* ptr, size_t size) throw ();

      size_t
      capacity() const throw ();

    private:
      char buf_[sizeof(Type) * SIZE];
      bool used_;
    };
  }
}

namespace Gears
{
  namespace TAlloc
  {
    //
    // AllocOnly class
    //

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    AllocOnly<Type, SIZE, HASH_HACK>::AllocOnly() throw ()
      : all_(0), cur_(0), end_(0)
    {}

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    AllocOnly<Type, SIZE, HASH_HACK>::AllocOnly(const AllocOnly&) throw ()
      : std::allocator<Type>(), all_(0), cur_(0), end_(0)
    {}

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    template <typename Other>
    AllocOnly<Type, SIZE, HASH_HACK>::AllocOnly(
      const AllocOnly<Other, SIZE, HASH_HACK>&) throw ()
      : all_(0), cur_(0), end_(0)
    {}

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    AllocOnly<Type, SIZE, HASH_HACK>::~AllocOnly() throw ()
    {
      while (all_)
      {
        Block* next = all_->next;
        delete all_;
        all_ = next;
      }
    }

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    Type*
    AllocOnly<Type, SIZE, HASH_HACK>::allocate(size_t n, const void*)
      throw (Gears::Exception)
    {
      assert(n == 1);
      if (cur_ != end_)
      {
        Type* ptr = reinterpret_cast<Type*>(cur_);
        cur_++;
        return ptr;
      }
      Block* block = new Block;
      block->next = all_;
      all_ = block;
      Type* ptr = reinterpret_cast<Type*>(block->items);
      cur_ = block->items + 1;
      end_ = block->items + SIZE;
      return ptr;
    }

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    void
    AllocOnly<Type, SIZE, HASH_HACK>::deallocate(Type*, size_t) throw ()
    {}

    template <typename Type, const size_t SIZE>
    AllocOnly<Type*, SIZE, true>::AllocOnly() throw ()
    {}

    template <typename Type, const size_t SIZE>
    template <typename Other>
    AllocOnly<Type*, SIZE, true>::AllocOnly(
      const AllocOnly<Other, SIZE, true>&) throw ()
    {}

    //
    // AggregatedBase class
    //

    template <const size_t TYPE, const size_t SIZE>
    AggregatedBase<TYPE, SIZE>::AggregatedBase() throw ()
      : all_(0), head_(0), cur_(0), end_(0)
    {}

    template <const size_t TYPE, const size_t SIZE>
    AggregatedBase<TYPE, SIZE>::AggregatedBase(const AggregatedBase&)
      throw ()
      : all_(0), head_(0), cur_(0), end_(0)
    {}

    template <const size_t TYPE, const size_t SIZE>
    AggregatedBase<TYPE, SIZE>::~AggregatedBase() throw ()
    {
      while (all_)
      {
        Block* next = all_->next;
        delete all_;
        all_ = next;
      }
    }

    template <const size_t TYPE, const size_t SIZE>
    void*
    AggregatedBase<TYPE, SIZE>::allocate() throw (Gears::Exception)
    {
      if (head_)
      {
        Item* ptr = head_;
        head_ = head_->next;
        return ptr;
      }
      if (cur_ != end_)
      {
        void* ptr = cur_;
        cur_++;
        return ptr;
      }
      Block* block = new Block;
      block->next = all_;
      all_ = block;
      void* ptr = block->items;
      cur_ = block->items + 1;
      end_ = block->items + SIZE;
      return ptr;
    }

    template <const size_t TYPE, const size_t SIZE>
    void
    AggregatedBase<TYPE, SIZE>::deallocate(void* ptr) throw ()
    {
      Item* p = static_cast<Item*>(ptr);
      p->next = head_;
      head_ = p;
    }


    //
    // Aggregated class
    //

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    template <typename Other>
    Aggregated<Type, SIZE, HASH_HACK>::Aggregated(
      const Aggregated<Other, SIZE, HASH_HACK>&) throw ()
    {}

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    Type*
    Aggregated<Type, SIZE, HASH_HACK>::allocate(size_t n, const void*)
      throw (Gears::Exception)
    {
      assert(n == 1);
      return static_cast<Type*>(
        AggregatedBase<sizeof(Type), SIZE>::allocate());
    }

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    void
    Aggregated<Type, SIZE, HASH_HACK>::deallocate(Type* ptr, size_t)
      throw ()
    {
      AggregatedBase<sizeof(Type), SIZE>::deallocate(ptr);
    }


    template <typename Type, const size_t SIZE>
    template <typename Other>
    Aggregated<Type*, SIZE, true>::Aggregated(
      const Aggregated<Other, SIZE, true>&) throw ()
    {}

    //
    // ThreadPool::GlobalMemoryHolder class
    //

    template <const size_t TYPE, const size_t SIZE>
    Gears::ThreadKey<typename ThreadPoolBase<TYPE, SIZE>::MemoryHolder>
      ThreadPoolBase<TYPE, SIZE>::GlobalMemoryHolder::key_(delete_holder_);

    template <const size_t TYPE, const size_t SIZE>
    Gears::SpinLock
      ThreadPoolBase<TYPE, SIZE>::GlobalMemoryHolder::lock_;

    template <const size_t TYPE, const size_t SIZE>
    typename ThreadPoolBase<TYPE, SIZE>::MemoryHolder*
      ThreadPoolBase<TYPE, SIZE>::GlobalMemoryHolder::head_(0);

    template <const size_t TYPE, const size_t SIZE>
    ThreadPoolBase<TYPE, SIZE>::GlobalMemoryHolder::~GlobalMemoryHolder()
      throw ()
    {
      while (head_)
      {
        MemoryHolder* next = head_->next;
        delete head_;
        head_ = next;
      }
    }

    template <const size_t TYPE, const size_t SIZE>
    typename ThreadPoolBase<TYPE, SIZE>::MemoryHolder*
    ThreadPoolBase<TYPE, SIZE>::GlobalMemoryHolder::operator ->() const
      throw (Gears::Exception)
    {
      MemoryHolder* holder = key_.get_data();
      if (holder)
      {
        return holder;
      }

      {
        Gears::SpinLock::WriteGuard guard(lock_);
        if (head_)
        {
          holder = head_;
          head_ = holder->next;
        }
      }

      if (!holder)
      {
        holder = new MemoryHolder;
      }

      key_.set_data(holder);
      return holder;
    }

    template <const size_t TYPE, const size_t SIZE>
    void
    ThreadPoolBase<TYPE, SIZE>::GlobalMemoryHolder::
      delete_holder_(void* pholder) throw ()
    {
      if (!pholder)
      {
        return;
      }
      MemoryHolder* holder = static_cast<MemoryHolder*>(pholder);
      Gears::SpinLock::WriteGuard guard(lock_);
      holder->next = head_;
      head_ = holder;
    }

    //
    // ThreadPoolBase class
    //

    template <const size_t TYPE, const size_t SIZE>
    typename ThreadPoolBase<TYPE, SIZE>::GlobalMemoryHolder
      ThreadPoolBase<TYPE, SIZE>::holder_;

    template <const size_t TYPE, const size_t SIZE>
    void*
    ThreadPoolBase<TYPE, SIZE>::allocate_() throw (Gears::Exception)
    {
      return holder_->allocate();
    }

    template <const size_t TYPE, const size_t SIZE>
    void
    ThreadPoolBase<TYPE, SIZE>::deallocate_(void* ptr) throw ()
    {
      holder_->deallocate(ptr);
    }


    //
    // ThreadPool class
    //

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    ThreadPool<Type, SIZE, HASH_HACK>::ThreadPool(const ThreadPool&)
      throw ()
      : std::allocator<Type>()
    {}

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    template <typename Other>
    ThreadPool<Type, SIZE, HASH_HACK>::ThreadPool(
      const ThreadPool<Other, SIZE, HASH_HACK>&) throw ()
    {}

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    Type*
    ThreadPool<Type, SIZE, HASH_HACK>::allocate(size_t n, const void*)
      throw (Gears::Exception)
    {
      assert(n == 1);
      return static_cast<Type*>(
        ThreadPoolBase<sizeof(Type), SIZE>::allocate_());
    }

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    void
    ThreadPool<Type, SIZE, HASH_HACK>::deallocate(Type* ptr, size_t)
      throw ()
    {
      ThreadPoolBase<sizeof(Type), SIZE>::deallocate_(ptr);
    }

    template <typename Type, const size_t SIZE>
    template <typename Other>
    ThreadPool<Type*, SIZE, true>::ThreadPool(
      const ThreadPool<Other, SIZE, true>&) throw ()
    {}

    //
    // GlobalPool::MemoryHolder class
    //

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    GlobalPool<Type, SIZE, HASH_HACK>::MemoryHolder::MemoryHolder()
      throw ()
      : head_(0), cur_(0), end_(0)
    {}

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    void*
    GlobalPool<Type, SIZE, HASH_HACK>::MemoryHolder::allocate()
      throw (Gears::Exception)
    {
      Gears::SpinLock::WriteGuard guard(lock_);
      if (head_)
      {
        Block* ptr = head_;
        head_ = head_->next;
        return ptr;
      }

      if (cur_ != end_)
      {
        Block* ptr = cur_;
        cur_++;
        return ptr;
      }

      Block* ptr = new Block[SIZE]; // FIXME
      cur_ = ptr + 1;
      end_ = ptr + SIZE;
      return ptr;
    }

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    void
    GlobalPool<Type, SIZE, HASH_HACK>::MemoryHolder::deallocate(void* ptr)
      throw ()
    {
      Gears::SpinLock::WriteGuard guard(lock_);
      Block* p = static_cast<Block*>(ptr);
      p->next = head_;
      head_ = p;
    }

    //
    // GlobalPool class
    //

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    typename GlobalPool<Type, SIZE, HASH_HACK>::MemoryHolder
      GlobalPool<Type, SIZE, HASH_HACK>::holder_;

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    GlobalPool<Type, SIZE, HASH_HACK>::GlobalPool(const GlobalPool&)
      throw ()
      : std::allocator<Type>()
    {}

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    template <typename Other>
    GlobalPool<Type, SIZE, HASH_HACK>::GlobalPool(
      const GlobalPool<Other, SIZE>&) throw ()
    {}

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    Type*
    GlobalPool<Type, SIZE, HASH_HACK>::allocate(size_t n, const void*)
      throw (Gears::Exception)
    {
      assert(n == 1);
      return static_cast<Type*>(holder_.allocate());
    }

    template <typename Type, const size_t SIZE, const bool HASH_HACK>
    void
    GlobalPool<Type, SIZE, HASH_HACK>::deallocate(Type* ptr, size_t)
      throw ()
    {
      holder_.deallocate(ptr);
    }

    //
    // VectorBuf
    //

    template <typename Type, const size_t SIZE>
    VectorBuf<Type, SIZE>::VectorBuf() throw ()
      : std::allocator<Type>(), used_(false)
    {}

    template <typename Type, const size_t SIZE>
    VectorBuf<Type, SIZE>::VectorBuf(const VectorBuf&) throw ()
      : std::allocator<Type>(), used_(false)
    {}

    template <typename Type, const size_t SIZE>
    template <typename Other>
    VectorBuf<Type, SIZE>::VectorBuf(const VectorBuf<Other, SIZE>&) throw ()
      : std::allocator<Type>(), used_(false)
    {}

    template <typename Type, const size_t SIZE>
    Type*
    VectorBuf<Type, SIZE>::allocate(size_t n, const void* ptr)
      throw (Gears::Exception)
    {
      if (!used_ && n <= SIZE)
      {
        used_ = true;
        return reinterpret_cast<Type*>(buf_);
      }
      return std::allocator<Type>::allocate(n, ptr);
    }

    template <typename Type, const size_t SIZE>
    void
    VectorBuf<Type, SIZE>::deallocate(Type* ptr, size_t size) throw ()
    {
      if (used_)
      {
        DEV_ASSERT(ptr == reinterpret_cast<Type*>(buf_));
        used_ = false;
      }
      else
      {
        std::allocator<Type>::deallocate(ptr, size);
      }
    }

    template <typename Type, const size_t SIZE>
    size_t
    VectorBuf<Type, SIZE>::capacity() const throw ()
    {
      return SIZE;
    }
  }
}

#endif
