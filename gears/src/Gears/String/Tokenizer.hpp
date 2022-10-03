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

#ifndef GEARS_TOKENIZER_HPP_
#define GEARS_TOKENIZER_HPP_

#include <Gears/Basic/SubString.hpp>

namespace Gears
{  
  /**
   * BasicTokenizer allow to separate string to tokens
   * TokenFingerType strategy for find seperator position in SubString
   */
  template<typename TokenFingerType>
  class BasicTokenizer
  {
  public:
    BasicTokenizer(
      const SubString& value,
      const TokenFingerType& split_op)
      noexcept;

    bool
    get_token(SubString& token)
      noexcept;

  protected:
    const SubString value_;
    SubString cur_value_;
    const TokenFingerType token_finger_;
  };

  /**
   * FirstAppearanceFinger
   */
  template<typename CategoryType>
  class FirstAppearanceFinger
  {
  public:
    FirstAppearanceFinger(
      const CategoryType& category = CategoryType());

    SubString
    operator()(const SubString& str, const SubString&)
      const;

  protected:
    const CategoryType category_;
  };

  /**
   * RepeatableAppearanceFinger
   */
  template<typename CategoryType>
  class RepeatableAppearanceFinger
  {
  public:
    RepeatableAppearanceFinger(
      const CategoryType& category = CategoryType());

    SubString
    operator()(const SubString& str, const SubString&)
      const;

  protected:
    const CategoryType category_;
  };

  template<typename CategoryType>
  class CategoryTokenizer:
    public BasicTokenizer<FirstAppearanceFinger<CategoryType> >
  {
  public:
    CategoryTokenizer(
      const SubString& value,
      const CategoryType& category = CategoryType())
      noexcept
      : BasicTokenizer<FirstAppearanceFinger<CategoryType> >(
          value,
          FirstAppearanceFinger<CategoryType>(category))
    {}
  };

  template<typename CategoryType>
  class CategoryRepeatableTokenizer:
    public BasicTokenizer<RepeatableAppearanceFinger<CategoryType> >
  {
  public:
    CategoryRepeatableTokenizer(
      const SubString& value,
      const CategoryType& category = CategoryType())
      noexcept
      : BasicTokenizer<RepeatableAppearanceFinger<CategoryType> >(
          value,
          RepeatableAppearanceFinger<CategoryType>(category))
    {}
  };
}

namespace Gears
{
  //
  // BasicTokenizer
  //
  template<typename TokenFingerType>
  BasicTokenizer<TokenFingerType>::BasicTokenizer(
    const SubString& value,
    const TokenFingerType& token_finger)
    noexcept
    : value_(value),
      cur_value_(value),
      token_finger_(token_finger)
  {}

  template<typename TokenFingerType>
  bool
  BasicTokenizer<TokenFingerType>::get_token(Gears::SubString& token)
    noexcept
  {
    if(cur_value_.empty())
    {
      return false;
    }

    SubString sub_token = token_finger_(cur_value_, value_);
    token.assign(cur_value_.begin(), sub_token.begin());
    cur_value_.assign(sub_token.end(), cur_value_.end());

    return true;
  }

  //
  // FirstAppearanceFinger
  //
  template<typename CategoryType>
  FirstAppearanceFinger<CategoryType>::FirstAppearanceFinger(
    const CategoryType& category)
    : category_(category)
  {}

  template<typename CategoryType>
  SubString
  FirstAppearanceFinger<CategoryType>::operator()(
    const SubString& str,
    const SubString&) const
  {
    const char* pos = category_.find_owned(str.begin(), str.end());
    return pos == str.end() ?
      SubString(str.end(), str.end()) :
      SubString(pos, pos + 1);
  }

  //
  // RepeatableAppearanceFinger
  //
  template<typename CategoryType>
  RepeatableAppearanceFinger<CategoryType>::RepeatableAppearanceFinger(
    const CategoryType& category)
    : category_(category)
  {}

  template<typename CategoryType>
  SubString
  RepeatableAppearanceFinger<CategoryType>::operator()(
    const SubString& str,
    const SubString&) const
  {
    const char* pos = category_.find_owned(str.begin(), str.end());
    if(pos == str.end())
    {
      return SubString(str.end(), str.end());
    }
    const char* begin_pos = pos;
    ++pos;

    while(pos != str.end() && category_.is_owned(*pos))
    {
      ++pos;
    }

    return SubString(begin_pos, pos);
  }
}

#endif /*GEARS_TOKENIZER_HPP_*/
