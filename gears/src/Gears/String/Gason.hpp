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

#ifndef GASON_HPP
#define GASON_HPP

#include <stdint.h>
#include <assert.h>

#include <Gears/Basic/SubString.hpp>

namespace Gears
{
  const char*
  find_end_of_number(const char* str)
    noexcept;

  const size_t JSON_ZONE_SIZE = 4096;
  const size_t JSON_STACK_SIZE = 32;

  struct JsonAllocator
  {
    struct Zone
    {
      Zone *next;
      char *end;
    };

    Zone *head;

    JsonAllocator()
      : head()
    {};

    ~JsonAllocator();

    void*
    allocate(size_t n, size_t align = 8);
  };

  const uint64_t JSON_VALUE_PAYLOAD_MASK = 0x00007FFFFFFFFFFFULL;
  const uint64_t JSON_VALUE_NAN_MASK = 0x7FF8000000000000ULL;
  const uint64_t JSON_VALUE_NULL = 0x7FFF800000000000ULL;
  const uint64_t JSON_VALUE_TAG_MASK = 0xF;
  const uint64_t JSON_VALUE_TAG_SHIFT = 47;

  enum JsonTag
  {
    JSON_TAG_NUMBER,
    JSON_TAG_STRING,
    JSON_TAG_BOOL,
    JSON_TAG_ARRAY,
    JSON_TAG_OBJECT,
    JSON_TAG_NULL = 0xF
  };

  double
  str2float(const char *str);

  struct JsonNode;

  struct JsonValue
  {
    union
    {
      uint64_t i;
      double f;
    } data;

    JsonValue();

    JsonValue(JsonTag tag, const void *p);

    explicit
    JsonValue(double x);

    bool
    is_double() const;

    JsonTag
    get_tag() const;

    uint64_t
    get_payload() const;

    double
    to_number() const;

    bool
    to_bool() const;

    JsonNode*
    to_node() const;

    // FIXME: ensure to_string safety
    SubString
    to_string() const;
  };

  struct JsonNode
  {
    JsonValue value;
    JsonNode* next;
    char* key;
  };

  struct JsonIterator
  {
    JsonIterator(JsonNode* node)
      : p(node)
    {}

    JsonNode *p;

    void
    operator++()
    {
      p = p->next;
    }

    bool
    operator!=(const JsonIterator &x) const
    {
      return p != x.p;
    }

    JsonNode*
    operator*() const
    {
      return p;
    }

    JsonNode*
    operator->() const
    {
      return p;
    }
  };

  inline JsonIterator
  begin(JsonValue o)
  {
    return JsonIterator(o.to_node());
  }

  inline JsonIterator end(JsonValue)
  {
    return JsonIterator(0);
  }

  enum JsonParseStatus
  {
    JSON_PARSE_OK,
    JSON_PARSE_BAD_NUMBER,
    JSON_PARSE_BAD_STRING,
    JSON_PARSE_BAD_IDENTIFIER,
    JSON_PARSE_STACK_OVERFLOW,
    JSON_PARSE_STACK_UNDERFLOW,
    JSON_PARSE_MISMATCH_BRACKET,
    JSON_PARSE_UNEXPECTED_CHARACTER,
    JSON_PARSE_UNQUOTED_KEY,
    JSON_PARSE_BREAKING_BAD
  };

  JsonParseStatus
  json_parse(char *str, char **endptr, JsonValue *value, JsonAllocator &allocator);

  std::string
  json_parse_error(JsonParseStatus status);
}

namespace Gears
{
  inline
  JsonValue::JsonValue()
  {
    data.i = JSON_VALUE_NULL;
  }

  inline
  JsonValue::JsonValue(JsonTag tag, const void *p)
  {
    uint64_t x = reinterpret_cast<uint64_t>(p);
    assert(static_cast<uint64_t>(tag) <= JSON_VALUE_TAG_MASK);
    assert(x <= JSON_VALUE_PAYLOAD_MASK);
    data.i = JSON_VALUE_NAN_MASK | ((uint64_t)tag << JSON_VALUE_TAG_SHIFT) | x;
  }

  inline
  JsonValue::JsonValue(double x)
  {
    data.f = x;
  }

  inline bool
  JsonValue::is_double() const
  {
    return (int64_t)data.i <= (int64_t)JSON_VALUE_NAN_MASK;
  }

  inline JsonTag
  JsonValue::get_tag() const
  {
    return is_double() ? JSON_TAG_NUMBER : JsonTag((data.i >> JSON_VALUE_TAG_SHIFT) & JSON_VALUE_TAG_MASK);
  }

  inline uint64_t
  JsonValue::get_payload() const
  {
    assert(!is_double());
    return data.i & JSON_VALUE_PAYLOAD_MASK;
  }

  inline double
  JsonValue::to_number() const
  {
    assert(get_tag() == JSON_TAG_NUMBER);
    return str2float(
      reinterpret_cast<const char*>(get_payload()));
  }

  inline bool
  JsonValue::to_bool() const
  {
    assert(get_tag() == JSON_TAG_BOOL);
    return (bool)get_payload();
  }

  inline JsonNode*
  JsonValue::to_node() const
  {
    assert(get_tag() == JSON_TAG_ARRAY || get_tag() == JSON_TAG_OBJECT);
    return (JsonNode *)get_payload();
  }

  inline SubString
  JsonValue::to_string() const
  {
    assert(get_tag() == JSON_TAG_STRING || get_tag() == JSON_TAG_NUMBER);

    if(get_tag() == JSON_TAG_STRING)
    {
      return SubString(reinterpret_cast<const char*>(get_payload()));
    }
    else if(get_tag() == JSON_TAG_NUMBER)
    {
      const char* payload = reinterpret_cast<const char*>(get_payload());
      return SubString(payload, find_end_of_number(payload));
    }
    // boolean?

    return SubString();
  }

  inline const char*
  find_end_of_number(const char* str)
    noexcept
  {
    if(*str == '-' || *str == '+')
    {
      ++str;
    }

    while (static_cast<unsigned>(*str - '0') < 10)
    {
      ++str;
    }

    if (*str == '.')
    {
      ++str;
    }

    while (static_cast<unsigned>(*str - '0') < 10)
    {
      ++str;
    }

    if (*str == 'e' || *str == 'E')
    {
      ++str;

      if(*str == '-' || *str == '+')
      {
        ++str;
      }

      while (static_cast<unsigned>(*str - '0') < 10)
      {
        ++str;
      }
    }

    return str;
  }
}

#endif /*GASON_HPP*/
