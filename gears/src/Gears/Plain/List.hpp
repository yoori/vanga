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

#ifndef PLAIN_LIST_HPP
#define PLAIN_LIST_HPP

#include <list>
#include <Gears/Threading/TAlloc.hpp>

namespace PlainTypes
{
  //typedef Generics::TAlloc::AllocOnly<void*, 512> ListAlloc;
  typedef Gears::TAlloc::ThreadPool<void*, 512> ListAlloc;

  /**
   * plain representation:
   *   array<Type>:
   *     elements_begin word
   *     elements_end word
   *   list<Type>:
   *     elements_begin word
   *     elements_end word
   *     dynamic_elements_values_end word
   */
  template<typename ElementType>
  struct PlainArrayHelper
  {
    static
    std::pair<const unsigned char*, const unsigned char*>
    get_positions(const void* buf, unsigned long size)
      throw(PlainTypes::CorruptedStruct);

    template<typename ElementIteratorType>
    static void
    dyn_save(
      void* fixed_buf,
      void* dyn_buf,
      ElementIteratorType it,
      ElementIteratorType end)
      throw();
  };

  template<typename ElementType>
  struct BaseList: public std::list<ElementType, ListAlloc> 
  //  struct BaseList: public std::list<ElementType>
  {
    void init_default() throw();
  };

  /* List: ElementType have init/save/dyn_size_ methods, FIXED_SIZE value */
  template<typename ElementType>
  struct List: public BaseList<ElementType>
  {
    void unsafe_init(const void* buf, unsigned long size)
      throw(CorruptedStruct);

    void init(const void* buf, unsigned long size)
      throw(CorruptedStruct);

    unsigned long dyn_size_() const throw();

    void save_(void* fixed_buf, void* dyn_buf) const throw();
  };

  /* SimpleList:
   *   ElementType
   *   ElementRecordPolicy
   *     ElementType read_cast(const void*)
   *     ElementType& write_cast(void*)
   */
  template<typename ElementType,
    ElementType ReadCastFun(const void*),
    ElementType& WriteCastFun(void*),
    const unsigned long STEP>
  struct SimpleList: public BaseList<ElementType>
  {
    void init(const void* buf, unsigned long size)
      throw(CorruptedStruct);

    unsigned long dyn_size_() const throw();

    void save_(void* fixed_buf, void* dyn_buf) const throw();
  };
}

namespace PlainTypes
{
  // PlainArrayHelper<>
  template<typename ElementType>
  std::pair<const unsigned char*, const unsigned char*>
  PlainArrayHelper<ElementType>::get_positions(
    const void* buf,
    unsigned long size)
    throw(PlainTypes::CorruptedStruct)
  {
    static const char* FUN = "PlainArrayHelper<ElementType>::get_positions()";

    uint32_t start_pos = *static_cast<const uint32_t*>(buf);
    uint32_t end_pos = *(static_cast<const uint32_t*>(buf) + 1);

    if(end_pos > size || start_pos > end_pos)
    {
      Gears::ErrorStream ostr;
      ostr << FUN << ": last or first element position "
        " is great then buffer size = " << size <<
        ", start_pos = " << start_pos <<
        ", end_pos = " << end_pos;
      throw PlainTypes::CorruptedStruct(ostr.str());
    }

    if((end_pos - start_pos) % ElementType::FIXED_SIZE != 0)
    {
      Gears::ErrorStream ostr;
      ostr << FUN << ": array size isn't multiple of fixed element size = " <<
        ElementType::FIXED_SIZE;
      throw PlainTypes::CorruptedStruct(ostr.str());
    }

    return std::make_pair(
      static_cast<const unsigned char*>(buf) + start_pos,
      static_cast<const unsigned char*>(buf) + end_pos);
  }

  template<typename ElementType>
  template<typename ElementIteratorType>
  void
  PlainArrayHelper<ElementType>::dyn_save(
    void* fixed_buf,
    void* dyn_buf,
    ElementIteratorType it,
    ElementIteratorType end)
    throw()
  {
    unsigned char* fixed_ptr = static_cast<unsigned char*>(dyn_buf);
    unsigned char* dyn_ptr = fixed_ptr +
      std::distance(it, end) * ElementType::FIXED_SIZE;

    for(; it != end; ++it)
    {
      it->save_(fixed_ptr, dyn_ptr);

      fixed_ptr += ElementType::FIXED_SIZE;
      dyn_ptr += it->dyn_size_();
    }

    *static_cast<uint32_t*>(fixed_buf) =
      static_cast<unsigned char*>(dyn_buf) - static_cast<unsigned char*>(fixed_buf);
    *(static_cast<uint32_t*>(fixed_buf) + 1) =
      fixed_ptr - static_cast<unsigned char*>(fixed_buf);
  }

  /* BaseList */
  template<typename ElementType>
  void
  BaseList<ElementType>::init_default() throw()
  {}

  /* List */
  template<typename ElementType>
  void
  List<ElementType>::unsafe_init(const void* buf, unsigned long size)
    throw(CorruptedStruct)
  {
    std::pair<const unsigned char*, const unsigned char*> buf_poses =
      PlainArrayHelper<ElementType>::get_positions(buf, size);

    const unsigned char* buf_ptr = buf_poses.first;
    const unsigned char* end_buf_ptr = buf_poses.second;

    for(; buf_ptr < end_buf_ptr; buf_ptr += ElementType::FIXED_SIZE)
    {
      this->push_back(ElementType());
      // header bounds checked call unsafe_init for performance
      this->back().unsafe_init(buf_ptr, size);
    }
  }

  template<typename ElementType>
  void
  List<ElementType>::init(const void* buf, unsigned long size)
    throw(CorruptedStruct)
  {
    static const char* FUN = "List<ElementType>::init()";

    if(size < 8)
    {
      Gears::ErrorStream ostr;
      ostr << FUN << ": buffer size = " << size << " is less then list header size";
      throw PlainTypes::CorruptedStruct(ostr.str());
    }

    unsafe_init(buf, size);
  }

  template<typename ElementType>
  unsigned long
  List<ElementType>::dyn_size_() const throw()
  {
    unsigned long res = this->size() * ElementType::FIXED_SIZE;
    for(typename std::list<ElementType>::const_iterator it =
          this->begin();
        it != this->end(); ++it)
    {
      res += it->dyn_size_();
    }
    return res;
  }

  template<typename ElementType>
  void
  List<ElementType>::save_(void* fixed_buf, void* dyn_buf) const
    throw()
  {
    unsigned char* fixed_ptr = static_cast<unsigned char*>(dyn_buf);
    unsigned char* dyn_ptr = fixed_ptr +
      this->size() * ElementType::FIXED_SIZE;

    for(typename std::list<ElementType>::const_iterator it = this->begin();
        it != this->end(); ++it)
    {
      it->save_(fixed_ptr, dyn_ptr);

      fixed_ptr += ElementType::FIXED_SIZE;
      dyn_ptr += it->dyn_size_();
    }

    *static_cast<uint32_t*>(fixed_buf) =
      static_cast<unsigned char*>(dyn_buf) - static_cast<unsigned char*>(fixed_buf);
    *(static_cast<uint32_t*>(fixed_buf) + 1) =
      fixed_ptr - static_cast<unsigned char*>(fixed_buf);
  }

  /* SimpleList */
  template<typename ElementType,
    ElementType ReadCastFun(const void*),
    ElementType& WriteCastFun(void*),
    const unsigned long STEP>
  void
  SimpleList<ElementType, ReadCastFun, WriteCastFun, STEP>::
  init(const void* buf, unsigned long /*size*/)
    throw(CorruptedStruct)
  {
    uint32_t start_pos = *static_cast<const uint32_t*>(buf);
    uint32_t end_pos = *(static_cast<const uint32_t*>(buf) + 1);

    const unsigned char* buf_ptr =
      static_cast<const unsigned char*>(buf) + start_pos;
    const unsigned char* end_buf_ptr =
      static_cast<const unsigned char*>(buf) + end_pos;

    for(; buf_ptr < end_buf_ptr; buf_ptr += STEP)
    {
      this->push_back(ReadCastFun(buf_ptr));
    }
  }

  template<typename ElementType,
    ElementType ReadCastFun(const void*),
    ElementType& WriteCastFun(void*),
    const unsigned long STEP>
  unsigned long
  SimpleList<ElementType, ReadCastFun, WriteCastFun, STEP>::
  dyn_size_() const throw()
  {
    return this->size() * STEP;
  }

  template<typename ElementType,
    ElementType ReadCastFun(const void*),
    ElementType& WriteCastFun(void*),
    const unsigned long STEP>
  void
  SimpleList<ElementType, ReadCastFun, WriteCastFun, STEP>::
  save_(void* fixed_buf, void* dyn_buf) const
    throw()
  {
    unsigned char* dyn_ptr = static_cast<unsigned char*>(dyn_buf);

    for(typename std::list<ElementType>::const_iterator it =
          this->begin();
        it != this->end(); ++it, dyn_ptr += STEP)
    {
      WriteCastFun(dyn_ptr) = *it;
    }

    *static_cast<uint32_t*>(fixed_buf) =
      static_cast<unsigned char*>(dyn_buf) - static_cast<unsigned char*>(fixed_buf);
    *(static_cast<uint32_t*>(fixed_buf) + 1) =
      dyn_ptr - static_cast<unsigned char*>(fixed_buf);
  }
}

#endif /*PLAIN_LIST_HPP*/
