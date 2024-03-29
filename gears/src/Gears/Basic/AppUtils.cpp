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

#include <cstring>

#include "AppUtils.hpp"

namespace Gears
{
  namespace AppUtils
  {
    OptionCont
    OptionCont::operator ||(const OptionCont& right) noexcept
    {
      OptionCont ret(*this);

      ret.eq_options.insert(ret.eq_options.end(),
        right.eq_options.begin(), right.eq_options.end());
      ret.short_options.insert(ret.short_options.end(),
        right.short_options.begin(), right.short_options.end());

      return ret;
    }

    /**X Args::ParseState */
    Args::ParseState::ParseState(unsigned long argc_val,
      const char* const *argv_val, const char* arg_pos_val) noexcept
      : argc_(argc_val), argv_(argv_val), arg_pos_(arg_pos_val)
    {
    }

    bool
    Args::ParseState::next_word() noexcept
    {
      ++argv_;
      --argc_;
      arg_pos_ = argc_ > 0 ? *argv_ : 0;
      return argc_ > 0;
    };

    bool
    Args::ParseState::end() noexcept
    {
      return argc_ == 0;
    }

    const char*
    Args::ParseState::current_pos() noexcept
    {
      return arg_pos_;
    }

    void
    Args::ParseState::current_pos(const char* pos) noexcept
    {
      arg_pos_ = pos;
    }

    /**X Args */
    bool
    Args::parse_eq_op_(ParseState& parse_state)
      /*throw (Gears::Exception, Exception, InvalidParam)*/
    {
      static const char* FUN = "Args::parse_eq_op_()";

      const char* cur_opt = parse_state.current_pos();
      if (cur_opt[0] == '-' && cur_opt[1] == '-')
      {
        cur_opt += 2;
        const char* eq_pos = ::strchr(cur_opt, '=');
        if (eq_pos)
        {
          std::string opt_name(cur_opt, eq_pos - cur_opt);
          OptionSetterMap::iterator it = eq_options_.find(opt_name);
          if (it != eq_options_.end())
          {
            if (!it->second->require_value())
            {
              Gears::ErrorStream ostr;
              ostr << FUN << ": defined value for option without values";
              throw Exception(ostr.str());
            }

            it->second->set(opt_name.c_str(), eq_pos + 1);
          }
          else
          {
            Gears::ErrorStream ostr;
            ostr << FUN << ": unknown long option '" << opt_name << "'.";
            throw Exception(ostr.str());
          }
        }
        else
        {
          std::string opt_name(cur_opt);
          OptionSetterMap::iterator it = eq_options_.find(opt_name);
          if (it != eq_options_.end())
          {
            if (it->second->require_value())
            {
              Gears::ErrorStream ostr;
              ostr << FUN << ": undefined value for option '" << opt_name << "'";
              throw Exception(ostr.str());
            }

            it->second->set(opt_name.c_str(), 0);
          }
          else
          {
            Gears::ErrorStream ostr;
            ostr << FUN << ": unknown long option '" << cur_opt << "'";
            throw Exception(ostr.str());
          }
        }

        parse_state.next_word();
        return true;
      }

      return false;
    }

    bool
    Args::parse_short_opt_seq_(ParseState& parse_state)
      /*throw (Exception, InvalidParam)*/
    {
      static const char* FUN = "Args::parse_short_opt_seq_()";

      const char* cur_opt = parse_state.current_pos();
      if (*cur_opt == 0)
      {
        Gears::ErrorStream ostr;
        ostr << FUN << ": empty op.";
        throw Exception(ostr.str());
      }

      if (*cur_opt != '-')
      {
        return false;
      }

      if (*++cur_opt == 0)
      {
        Gears::ErrorStream ostr;
        ostr << FUN << ": empty op name after '-'.";
        throw Exception(ostr.str());
      }

      parse_state.current_pos(cur_opt);

      while (!parse_state.end() && parse_short_opt_(parse_state))
      {}

      return true;
    }

    bool
    Args::parse_short_opt_(ParseState& parse_state)
      /*throw (Exception, InvalidParam)*/
    {
      const char* cur_opt = parse_state.current_pos();
      const char* next = cur_opt + ::strlen(cur_opt);

      for (; next > cur_opt; --next)
      {
        std::string opt_name(cur_opt, next - cur_opt);
        OptionSetterMap::iterator short_it = short_options_.find(opt_name);

        if (short_it != short_options_.end())
        {
          parse_state.current_pos(next);
          parse_short_op_value_(short_it, opt_name.c_str(), parse_state);
          return false;
        }
      }

      return false;
    }

    void
    Args::parse_short_op_value_(
      OptionSetterMap::iterator it,
      const char* opt_name,
      ParseState& parse_state)
      /*throw (Exception, InvalidParam)*/
    {
      static const char* FUN = "Args::parse_short_op_value_()";

      if (it->second->require_value())
      {
        if (parse_state.end())
        {
          Gears::ErrorStream ostr;
          ostr << FUN << ": undefined value after option '" <<
            opt_name << "'";
          throw Exception(ostr.str());
        }

        if (*parse_state.current_pos())
        {
          it->second->set(opt_name, parse_state.current_pos());
          parse_state.next_word();
        }
        else
        {
          if (parse_state.next_word())
          {
            it->second->set(opt_name, parse_state.current_pos());
            parse_state.next_word();
          }
          else
          {
            Gears::ErrorStream ostr;
            ostr << FUN << ": found end of command after option.";
            throw Exception(ostr.str());
          }
        }
      }
      else
      {
        it->second->set(opt_name, 0);

        if (*parse_state.current_pos() == 0)
        {
          parse_state.next_word();
        }
      }
    }

    void
    Args::parse(int argc, const char* const argv[])
      /*throw (Gears::Exception, Exception, InvalidParam)*/
    {
      static const char* FUN = "Args::parse()";

      int command_counter = 0;

      ParseState parse_state(argc, argv, argv[0]);
      while (!parse_state.end())
      {
        if (!parse_eq_op_(parse_state) &&
          !parse_short_opt_seq_(parse_state))
        {
          if (command_count_ == -1 || command_counter < command_count_)
          {
            ++command_counter;
            commands_.emplace_back(parse_state.current_pos());
            parse_state.next_word();
          }
          else
          {
            Gears::ErrorStream ostr;
            ostr << FUN << ": unknown option: '" << parse_state.current_pos() << "'";
            throw Exception(ostr.str());
          }
        }
      }
    }

    void
    Args::usage(std::ostream& ostr) const
      /*throw (Gears::Exception)*/
    {
      for (Usage::const_iterator itor(usage_.begin()); itor != usage_.end();
        ++itor)
      {
        ostr << itor->second << "\n";
      }
    }
  }
}
