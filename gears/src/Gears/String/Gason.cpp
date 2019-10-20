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

#include <stdlib.h>
#include "Gason.hpp"

namespace Gears
{
  static unsigned char ctype[256];

  static const struct ctype_init_t
  {
    ctype_init_t()
    {
      ctype[(int)'\0'] |= 002;
      for (const char *s = "\t\n\v\f\r\x20"; *s; ++s) ctype[(int)*s] |= 001;
      for (const char *s = ",:]}"; *s; ++s) ctype[(int)*s] |= 002;
      for (const char *s = "+-"; *s; ++s) ctype[(int)*s] |= 004;
      for (const char *s = "0123456789"; *s; ++s) ctype[(int)*s] |= 010;
      for (const char *s = "ABCDEF" "abcdef"; *s; ++s) ctype[(int)*s] |= 020;
    }
  } ctype_init;

  bool
  is_space(char c)
  {
    return (ctype[(int)(unsigned char)c] & 001) != 0;
  }

  bool
  is_delim(char c)
  {
    return (ctype[(int)(unsigned char)c] & 003) != 0;
  }

  bool
  is_sign(char c)
  {
    return (ctype[(int)(unsigned char)c] & 004) != 0;
  }

  bool
  is_dec(char c)
  {
    return (ctype[(int)(unsigned char)c] & 010) != 0;
  }

  bool
  is_hex(char c)
  {
    return (ctype[(int)(unsigned char)c] & 030) != 0;
  }

  inline int
  char2int(char c)
  {
    if (c >= 'a') return c - 'a' + 10;
    if (c >= 'A') return c - 'A' + 10;
    return c - '0';
  }

  void
  consume_float(char *str, char **endptr)
  {
    if(is_sign(*str))
    {
      ++str;
    }

    while(is_dec(*str))
    {
      ++str;
    }

    if(*str == '.')
    {
      ++str;

      while (is_dec(*str))
      {
        ++str;
      }
    }

    if(*str == 'e' || *str == 'E')
    {
      ++str;

      if(is_sign(*str))
      {
        ++str;
      }

      while(is_dec(*str))
      {
        ++str;
      }
    }

    *endptr = str;
  }

  double
  str2float(const char* str)
  {
    double sign = is_sign(*str) && *str++ == '-' ? -1 : 1;
    double result = 0;
    while(is_dec(*str))
    {
      result = (result * 10) + (*str++ - '0');
    }

    if(*str == '.')
    {
      ++str;
      double fraction = 1;
      while (is_dec(*str))
      {
        fraction *= 0.1;
        result += (*str++ - '0') * fraction;
      }
    }

    if(*str == 'e' || *str == 'E')
    {
      ++str;
      double base = is_sign(*str) && *str++ == '-' ? 0.1 : 10;
      int exponent = 0;
      while (is_dec(*str))
      {
        exponent = (exponent * 10) + (*str++ - '0');
      }

      double power = 1;
      for (; exponent; exponent >>= 1, base *= base)
      {
        if (exponent & 1)
        {
          power *= base;
        }
      }

      result *= power;
    }

    return sign * result;
  }

  JsonAllocator::~JsonAllocator()
  {
    while(head)
    {
      Zone *temp = head->next;
      free(head);
      head = temp;
    }
  }

  inline void*
  align_pointer(void *x, size_t align)
  {
    return (void *)(((uintptr_t)x + (align - 1)) & ~(align - 1));
  }

  void*
  JsonAllocator::allocate(size_t n, size_t align)
  {
    if (head)
    {
      char* p = static_cast<char*>(align_pointer(head->end, align));
      if (p + n <= reinterpret_cast<char*>(head) + JSON_ZONE_SIZE)
      {
        head->end = p + n;
        return p;
      }
    }

    size_t zone_size = sizeof(Zone) + n + align;
    Zone *z = (Zone*)malloc(zone_size <= JSON_ZONE_SIZE ? JSON_ZONE_SIZE : zone_size);
    char *p = static_cast<char*>(align_pointer(z + 1, align));
    z->end = p + n;

    if(zone_size <= JSON_ZONE_SIZE || !head)
    {
      z->next = head;
      head = z;
    }
    else
    {
      z->next = head->next;
      head->next = z;
    }

    return p;
  }

  struct JsonList
  {
    JsonList()
    {}

    JsonList(JsonTag tag_val, const JsonValue& node_val, char* key_val)
      : tag(tag_val),
        node(node_val),
        key(key_val)
    {}

    JsonTag tag;
    JsonValue node;
    char *key;

    void
    grow_the_tail(JsonNode *p)
    {
      JsonNode *tail = (JsonNode *)node.get_payload();
      if (tail)
      {
        p->next = tail->next;
        tail->next = p;
      }
      else
      {
        p->next = p;
      }

      node = JsonValue(tag, p);
    }

    JsonValue
    cut_the_head()
    {
      JsonNode *tail = (JsonNode *)node.get_payload();
      if (tail)
      {
        JsonNode *head = tail->next;
        tail->next = 0;
        return JsonValue(tag, head);
      }

      return node;
    }
  };

  JsonParseStatus
  json_parse(char *str, char **endptr, JsonValue *value, JsonAllocator &allocator)
  {
    JsonList stack[JSON_STACK_SIZE];
    int top = -1;
    bool separator = true;
    while (*str)
    {
      JsonValue o;
      while (*str && is_space(*str)) ++str;
      *endptr = str++;
      switch (**endptr)
      {
        case '\0':
          continue;
        case '-':
          if (!is_dec(*str) && *str != '.') return *endptr = str, JSON_PARSE_BAD_NUMBER;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          o = JsonValue(JSON_TAG_NUMBER, *endptr);
          consume_float(*endptr, &str);
          if (!is_delim(*str)) return *endptr = str, JSON_PARSE_BAD_NUMBER;
          break;
        case '"':
          o = JsonValue(JSON_TAG_STRING, str);
          for (char *s = str; *str; ++s, ++str)
          {
            int c = *s = *str;
            if (c == '\\')
            {
              c = *++str;
              switch (c)
              {
                case '\\':
                case '"':
                case '/': *s = c; break;
                case 'b': *s = '\b'; break;
                case 'f': *s = '\f'; break;
                case 'n': *s = '\n'; break;
                case 'r': *s = '\r'; break;
                case 't': *s = '\t'; break;
                case 'u':
                  c = 0;
                  for (int i = 0; i < 4; ++i)
                  {
                    if (!is_hex(*++str)) return *endptr = str, JSON_PARSE_BAD_STRING;
                    c = c * 16 + char2int(*str);
                  }
 
                  if (c < 0x80)
                  {
                    *s = c;
                  }
                  else if (c < 0x800)
                  {
                    *s++ = 0xC0 | (c >> 6);
                    *s = 0x80 | (c & 0x3F);
                  }
                  else
                  {
                    *s++ = 0xE0 | (c >> 12);
                    *s++ = 0x80 | ((c >> 6) & 0x3F);
                    *s = 0x80 | (c & 0x3F);
                  }
                  break;
                default:
                  return *endptr = str, JSON_PARSE_BAD_STRING;
              }
            }
            else if (c == '"')
            {
              *s = 0;
              ++str;
              break;
            }
          }

          if (!is_delim(*str)) return *endptr = str, JSON_PARSE_BAD_STRING;
          break;
        case 't':
          for (const char *s = "rue"; *s; ++s, ++str)
          {
            if (*s != *str) return JSON_PARSE_BAD_IDENTIFIER;
          }
          if (!is_delim(*str)) return JSON_PARSE_BAD_IDENTIFIER;
          o = JsonValue(JSON_TAG_BOOL, (void *)true);
          break;
        case 'f':
          for (const char *s = "alse"; *s; ++s, ++str)
          {
            if (*s != *str) return JSON_PARSE_BAD_IDENTIFIER;
          }
          if (!is_delim(*str)) return JSON_PARSE_BAD_IDENTIFIER;
          o = JsonValue(JSON_TAG_BOOL, (void *)false);
          break;
        case 'n':
          for (const char *s = "ull"; *s; ++s, ++str)
          {
            if (*s != *str) return JSON_PARSE_BAD_IDENTIFIER;
          }
          if (!is_delim(*str)) return JSON_PARSE_BAD_IDENTIFIER;
          break;
        case ']':
          if (top == -1) return JSON_PARSE_STACK_UNDERFLOW;
          if (stack[top].tag != JSON_TAG_ARRAY) return JSON_PARSE_MISMATCH_BRACKET;
          o = stack[top--].cut_the_head();
          break;
        case '}':
          if (top == -1) return JSON_PARSE_STACK_UNDERFLOW;
          if (stack[top].tag != JSON_TAG_OBJECT) return JSON_PARSE_MISMATCH_BRACKET;
          o = stack[top--].cut_the_head();
          break;
        case '[':
          if (++top == static_cast<int>(JSON_STACK_SIZE))
          {
            return JSON_PARSE_STACK_OVERFLOW;
          }
          stack[top] = JsonList(JSON_TAG_ARRAY, JsonValue(JSON_TAG_ARRAY, 0), 0);
          continue;
        case '{':
          if (++top == static_cast<int>(JSON_STACK_SIZE))
          {
            return JSON_PARSE_STACK_OVERFLOW;
          }
          stack[top] = JsonList(JSON_TAG_OBJECT, JsonValue(JSON_TAG_OBJECT, 0), 0);
          continue;
        case ':':
          if (separator || stack[top].key == 0) return JSON_PARSE_UNEXPECTED_CHARACTER;
          separator = true;
          continue;
        case ',':
          if (separator || stack[top].key != 0) return JSON_PARSE_UNEXPECTED_CHARACTER;
          separator = true;
          continue;
        default:
          return JSON_PARSE_UNEXPECTED_CHARACTER;
      }

      separator = false;

      if (top == -1)
      {
        *endptr = str;
        *value = o;
        return JSON_PARSE_OK;
      }

      if (stack[top].tag == JSON_TAG_OBJECT)
      {
        if (!stack[top].key)
        {
          if (o.get_tag() != JSON_TAG_STRING) return JSON_PARSE_UNQUOTED_KEY;
          stack[top].key = reinterpret_cast<char*>(o.get_payload());
          continue;
        }

        JsonNode *p = (JsonNode *)allocator.allocate(sizeof(JsonNode));
        p->value = o;
        p->key = stack[top].key;
        stack[top].key = 0;
        stack[top].grow_the_tail((JsonNode *)p);
        continue;
      }

      JsonNode *p = (JsonNode *)allocator.allocate(sizeof(JsonNode) - sizeof(char *));
      p->value = o;
      stack[top].grow_the_tail(p);
    }

    return JSON_PARSE_BREAKING_BAD;
  }

  std::string
  json_parse_error(JsonParseStatus status)
  {
    switch(status)
    {
    case JSON_PARSE_OK:
      return "ok";

    case JSON_PARSE_BAD_NUMBER :
      return "bad number";

    case JSON_PARSE_BAD_STRING :
      return "bad string";

    case JSON_PARSE_BAD_IDENTIFIER :
      return "bad identifier";

    case JSON_PARSE_STACK_OVERFLOW :
      return "stack overflow";

    case JSON_PARSE_STACK_UNDERFLOW :
      return "stack underflow";

    case JSON_PARSE_MISMATCH_BRACKET :
      return "mismatch bracket";

    case JSON_PARSE_UNEXPECTED_CHARACTER :
      return "unexpected character";

    case JSON_PARSE_UNQUOTED_KEY :
      return "unquoted key";

    case JSON_PARSE_BREAKING_BAD :
      return "breaking bad";
    };

    return "unknown";
  }
}
