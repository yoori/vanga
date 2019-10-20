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

#ifndef GEARS_INPUTMEMORYSTREAM_HPP
#define GEARS_INPUTMEMORYSTREAM_HPP

#include <streambuf>
#include <istream>

#include "Macro.hpp"
#include "Exception.hpp"

/**
 * InputMemoryStreamBuffer
 * InputMemoryStream
 */
OPEN_NAMESPACE(Gears)

  /**
   * Input memory buffer
   * Using supplied memory region as a stream content
   * No allocations are performed
   */
  template <typename Elem, typename Traits>
  class InputMemoryStreamBuffer : public std::basic_streambuf<Elem, Traits>
  {
  public:
    typedef typename Traits::int_type Int;
    typedef typename Traits::pos_type Position;
    typedef typename Traits::off_type Offset;

    typedef Elem* Pointer;
    typedef const Elem* ConstPointer;
    typedef size_t Size;

    /**
     * Constructor
     * @param ptr address of memory region
     * @param size size of memory region
     */
    InputMemoryStreamBuffer(Pointer ptr, Size size)
      throw(Gears::Exception);

    /**
     * @return The pointer to data not read yet
     */
    ConstPointer
    data() const throw();

    /**
     * @return The size of data not read yet
     */
    Size
    size() const throw();

  protected:
    virtual Position
    seekoff(
      Offset off,
      std::ios_base::seekdir way,
      std::ios_base::openmode which)
      throw(Gears::Exception);

    virtual Position
    seekpos(Position pos, std::ios_base::openmode which)
      throw(Gears::Exception);

    virtual Int
    underflow() throw();
  };

  /**
   * Input memory stream. Uses InputMemoryBuffer for data access.
   */
  template <typename Elem, typename Traits = std::char_traits<Elem> >
  class InputMemoryStream:
    public InputMemoryStreamBuffer<Elem, Traits>,
    public std::basic_istream<Elem, Traits>
  {
  private:
    typedef InputMemoryStreamBuffer<Elem, Traits> StreamBuffer;
    typedef std::basic_istream<Elem, Traits> Stream;

  public:
    typedef Elem* Pointer;
    typedef const Elem* ConstPointer;
    typedef size_t Size;

    /**
     * Constructor
     * Passes data and Traits::length(data) to InputMemoryBlock's
     * constructor
     * @param data address of memory region
     */
    InputMemoryStream(ConstPointer data)
      throw(Gears::Exception);

    /**
     * Constructor
     * Passes parameters to InputMemoryBlock's constructor
     * @param data address of memory region
     * @param size size of memory region
     */
    InputMemoryStream(ConstPointer data, Size size)
      throw(Gears::Exception);

    /**
     * Constructor
     * Passes str.data() and str.size() to InputMemoryBlock's constructor
     * @param str memory region, should not be temporal
     */
    template <typename Allocator>
    InputMemoryStream(const std::basic_string<Elem, Allocator>& str)
      throw(Gears::Exception);
  };

CLOSE_NAMESPACE

#include "InputMemoryStream.tpp"

#endif /*GEARS_INPUTMEMORYSTREAM_HPP*/
