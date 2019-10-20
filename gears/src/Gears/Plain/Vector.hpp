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

#ifndef PLAIN_VECTOR_HPP
#define PLAIN_VECTOR_HPP

#include <vector>
#include <Gears/Threading/TAlloc.hpp>

#include "List.hpp"

namespace PlainTypes
{
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
  struct BaseVector: public std::vector<ElementType>
  {
    void init_default() throw();
  };

  /* List: ElementType have init/save/dyn_size_ methods, FIXED_SIZE value */
  template<typename ElementType>
  struct Vector: public BaseVector<ElementType>
  {
    void unsafe_init(const void* buf, unsigned long size)
      throw(CorruptedStruct);

    void init(const void* buf, unsigned long size)
      throw(CorruptedStruct);

    unsigned long dyn_size_() const throw();

    void save_(void* fixed_buf, void* dyn_buf) const throw();
  };

  /* SimpleVector:
   *   ElementType
   *   ElementRecordPolicy
   *     ElementType read_cast(const void*)
   *     ElementType& write_cast(void*)
   */
  template<typename ElementType,
    ElementType ReadCastFun(const void*),
    ElementType& WriteCastFun(void*),
    const unsigned long STEP>
  struct SimpleVector: public BaseVector<ElementType>
  {
    void init(const void* buf, unsigned long size)
      throw(CorruptedStruct);

    unsigned long dyn_size_() const throw();

    void save_(void* fixed_buf, void* dyn_buf) const throw();
  };
}

namespace PlainTypes
{
  // BaseVector<> impl
  template<typename ElementType>
  void
  BaseVector<ElementType>::init_default() throw()
  {}

  // Vector<> impl
  template<typename ElementType>
  void
  Vector<ElementType>::unsafe_init(const void* buf, unsigned long size)
    throw(CorruptedStruct)
  {
    std::pair<const unsigned char*, const unsigned char*> buf_poses =
      PlainArrayHelper<ElementType>::get_positions(buf, size);

    const unsigned char* buf_ptr = buf_poses.first;
    const unsigned char* end_buf_ptr = buf_poses.second;

    this->reserve((end_buf_ptr - buf_ptr) / ElementType::FIXED_SIZE);

    for(; buf_ptr < end_buf_ptr; buf_ptr += ElementType::FIXED_SIZE)
    {
      this->push_back(ElementType());
      // header bounds checked call unsafe_init for performance
      this->back().unsafe_init(buf_ptr, size);
    }
  }

  template<typename ElementType>
  void
  Vector<ElementType>::init(const void* buf, unsigned long size)
    throw(CorruptedStruct)
  {
    static const char* FUN = "Vector<ElementType>::init()";

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
  Vector<ElementType>::dyn_size_() const throw()
  {
    unsigned long res = this->size() * ElementType::FIXED_SIZE;
    for(typename BaseVector<ElementType>::const_iterator it =
          this->begin();
        it != this->end(); ++it)
    {
      res += it->dyn_size_();
    }
    return res;
  }

  template<typename ElementType>
  void
  Vector<ElementType>::save_(void* fixed_buf, void* dyn_buf) const
    throw()
  {
    PlainArrayHelper<ElementType>::dyn_save(
      fixed_buf,
      dyn_buf,
      this->begin(),
      this->end());
  }

  // SimpleVector
  template<typename ElementType,
    ElementType ReadCastFun(const void*),
    ElementType& WriteCastFun(void*),
    const unsigned long STEP>
  void
  SimpleVector<ElementType, ReadCastFun, WriteCastFun, STEP>::
  init(const void* buf, unsigned long /*size*/)
    throw(CorruptedStruct)
  {
    uint32_t start_pos = *static_cast<const uint32_t*>(buf);
    uint32_t end_pos = *(static_cast<const uint32_t*>(buf) + 1);

    const unsigned char* buf_ptr =
      static_cast<const unsigned char*>(buf) + start_pos;
    const unsigned char* end_buf_ptr =
      static_cast<const unsigned char*>(buf) + end_pos;

    this->resize((end_buf_ptr - buf_ptr) / STEP);
    typename BaseVector<ElementType>::iterator it = this->begin();
    for(; buf_ptr < end_buf_ptr; buf_ptr += STEP, ++it)
    {
      *it = ReadCastFun(buf_ptr);
    }
  }

  template<typename ElementType,
    ElementType ReadCastFun(const void*),
    ElementType& WriteCastFun(void*),
    const unsigned long STEP>
  unsigned long
  SimpleVector<ElementType, ReadCastFun, WriteCastFun, STEP>::
  dyn_size_() const throw()
  {
    return this->size() * STEP;
  }

  template<typename ElementType,
    ElementType ReadCastFun(const void*),
    ElementType& WriteCastFun(void*),
    const unsigned long STEP>
  void
  SimpleVector<ElementType, ReadCastFun, WriteCastFun, STEP>::
  save_(void* fixed_buf, void* dyn_buf) const
    throw()
  {
    unsigned char* dyn_ptr = static_cast<unsigned char*>(dyn_buf);

    for(typename BaseVector<ElementType>::const_iterator it =
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

#endif /*PLAIN_VECTOR_HPP*/
