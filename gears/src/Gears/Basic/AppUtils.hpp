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

#ifndef GEARS_APPUTILS_HPP
#define GEARS_APPUTILS_HPP

#include <list>
#include <map>

#include "Uncopyable.hpp"
#include "Exception.hpp"
#include "RefCountable.hpp"
#include "IntrusivePtr.hpp"
#include "InputMemoryStream.hpp"
#include "OutputMemoryStream.hpp"

//#include <Generics/Function.hpp>

namespace Gears
{
  namespace AppUtils
  {
    DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);
    DECLARE_GEARS_EXCEPTION(InvalidParam, Exception);

    struct OptionCont
    {
      typedef std::list<std::string> StringList;

      OptionCont
      operator ||(const OptionCont& right) throw ();

      StringList eq_options;
      StringList short_options;
    };

    class CheckOption
    {
    public:
      CheckOption() throw ();

      bool
      require_value() const throw ();

      bool
      enabled() const throw ();

      void
      set(const char* opt_name, const char* val) throw ();

    protected:
      bool enabled_;
    };

    template <typename Object>
    class Option
    {
    public:
      Option() throw ();

      explicit
      Option(const Object& val) throw ();

      const Object&
      operator *() const throw ();

      const Object*
      operator ->() const throw ();

      bool
      require_value() const throw ();

      bool
      installed() const throw ();

      void
      set(const char* opt_name, const char* val) throw (InvalidParam);

      void
      set_value(const Object& val) throw ();

    protected:
      Object val_;
      bool installed_;
    };

    /**
     * The same as Option<> but each call to set method save options to
     * container that support push_back. That is the Object type must
     * support push_back operation.
     */
    template <typename Object>
    class OptionsSet : public Option<Object>
    {
    public:
      typedef typename Object::value_type ValueType;
      /**
       * Construct options container object in not installed state
       */
      OptionsSet() throw () = default;

      /**
       * Construct options container object in not installed state,
       * and assign value
       * @param val value to be assigned
       */
      explicit
      OptionsSet(const Object& val) throw ();

      /**
       * Parse source and store read value to OptionsSet object
       * @param opt_name Not used
       * @param val Value string to be parsed and save result
       */
      void
      set(const char* opt_name, const char* val) throw (InvalidParam);
    };

    class StringOption : public Option<std::string>
    {
    public:
      StringOption() throw () = default;
      StringOption(const std::string& val) throw ();

      void
      set(const char* opt_name, const char* val) throw (InvalidParam);
    };

    class Args: private Uncopyable
    {
    public:
      typedef std::list<std::string> CommandList;

      explicit
      Args(long command_count = 0) throw (Gears::Exception);

      template <typename Option>
      void
      add(const OptionCont& cont, Option& opt, const char* comment = 0,
        const char* arg_name = 0) throw (Gears::Exception);

      void
      parse(int argc, const char* const argv[])
        throw (Gears::Exception, Exception, InvalidParam);

      const CommandList&
      commands() const throw ();

      void
      usage(std::ostream& ostr) const throw (Gears::Exception);

    protected:
      struct ParseState
      {
      public:
        ParseState(unsigned long argc_val, const char* const* argv_val,
          const char* arg_pos_val) throw ();

        bool
        next_word() throw ();
        bool
        end() throw ();

        const char*
        current_pos() throw ();
        void
        current_pos(const char* pos) throw ();

      protected:
        unsigned long argc_;
        const char* const* argv_;
        const char* arg_pos_;
      };

      class OptionSetter : public Gears::RefCountable
      {
      protected:
        virtual
        ~OptionSetter() throw () = default;

      public:
        virtual
        void
        set(const char* opt_name, const char* val) throw (InvalidParam) = 0;

        virtual
        bool
        require_value() const throw () = 0;
      };

      typedef Gears::IntrusivePtr<OptionSetter> OptionSetter_var;

      typedef std::map<std::string, OptionSetter_var>
        OptionSetterMap;

      bool
      parse_eq_op_(ParseState& parse_state)
        throw (Gears::Exception, Exception, InvalidParam);

      bool
      parse_short_opt_seq_(ParseState& parse_state)
        throw (Exception, InvalidParam);

      bool
      parse_short_opt_(ParseState& parse_state)
        throw (Exception, InvalidParam);

      void
      parse_short_op_value_(OptionSetterMap::iterator it,
        const char* opt_name, ParseState& parse_state)
        throw (Exception, InvalidParam);

    private:
      template <typename Option>
      class OptionSetterImpl : public OptionSetter
      {
      public:
        OptionSetterImpl(Option& opt) throw ();

        virtual
        void
        set(const char* opt_name, const char* val) throw (InvalidParam);

        virtual
        bool
        require_value() const throw ();

      protected:
        virtual
        ~OptionSetterImpl() throw () = default;

      private:
        Option& opt_;
      };

      static
      void
      append_flag_(const std::string& flag, bool short_opt,
        std::string& flags, std::string& usage) throw (Gears::Exception);

      typedef std::map<std::string, std::string> Usage;

      long command_count_;
      CommandList commands_;
      OptionSetterMap eq_options_;
      OptionSetterMap short_options_;
      Usage usage_;
    };

    inline
    OptionCont
    equal_name(const char* name)
    {
      OptionCont ret;
      ret.eq_options.push_back(name);
      return ret;
    }

    inline
    OptionCont
    short_name(const char* name)
    {
      OptionCont ret;
      ret.short_options.push_back(name);
      return ret;
    }
  }
}

namespace Gears
{
  namespace AppUtils
  {
    //
    // CheckOption class
    //

    inline
    CheckOption::CheckOption() throw ()
      : enabled_(false)
    {}

    inline
    bool
    CheckOption::require_value() const throw ()
    {
      return false;
    }

    inline
    bool
    CheckOption::enabled() const throw ()
    {
      return enabled_;
    }

    inline
    void
    CheckOption::set(const char* /*opt_name*/, const char* /*val*/) throw ()
    {
      enabled_ = true;
    }

    //
    // Option class
    //

    template <typename Object>
    Option<Object>::Option() throw ()
      : installed_(false)
    {}

    template <typename Object>
    Option<Object>::Option(const Object& val) throw ()
      : val_(val), installed_(false)
    {}

    template <typename Object>
    const Object&
    Option<Object>::operator *() const throw ()
    {
      return val_;
    }

    template <typename Object>
    const Object*
    Option<Object>::operator ->() const throw ()
    {
      return &val_;
    }

    template <typename Object>
    bool
    Option<Object>::require_value() const throw ()
    {
      return true;
    }

    template <typename Object>
    bool
    Option<Object>::installed() const throw ()
    {
      return installed_;
    }

    template <typename Object>
    void
    Option<Object>::set(const char* /*opt_name*/, const char* val)
      throw (InvalidParam)
    {
      static const char* FUN = "Option<>::set()";

      if(installed())
      {
        Gears::ErrorStream ostr;
        ostr << FUN << ": second time defined value '" << val << "'";
        throw InvalidParam(ostr.str());
      }

      Gears::InputMemoryStream<char> istr(val);
      istr >> val_;
      if (istr.bad() || (istr.peek(), !istr.eof()))
      {
        Gears::ErrorStream ostr;
        ostr << FUN << ": bad value '" << val << "'";
        throw InvalidParam(ostr.str());
      }

      installed_ = true;
    }

    template <typename Object>
    void
    Option<Object>::set_value(const Object& val) throw ()
    {
      val_ = val;
      installed_ = true;
    }

    //
    // OptionsSet class
    //

    template <typename Object>
    OptionsSet<Object>::OptionsSet(const Object& val) throw ()
      : Option<Object>(val)
    {}

    template <typename Object>
    void
    OptionsSet<Object>::set(const char* /*opt_name*/, const char* val)
      throw (InvalidParam)
    {
      static const char* FUN = "OptionsSet<>::set";

      Gears::InputMemoryStream<char> istr(val);
      ValueType value;
      istr >> value;
      if (istr.bad() || !istr.eof())
      {
        Gears::ErrorStream ostr;
        ostr << FUN << ": bad value '" << val << "'";
        throw InvalidParam(ostr.str());
      }
      this->val_.insert(this->val_.end(), value);
      this->installed_ = true;
    }

    //
    // StringOption class
    //

    inline
    StringOption::StringOption(const std::string& val) throw ()
      : Option<std::string>(val)
    {}

    inline void
    StringOption::set(const char* /*opt_name*/, const char* val)
      throw (InvalidParam)
    {
      static const char* FUN = "StringOption<>::set";

      if (installed())
      {
        Gears::ErrorStream ostr;
        ostr << FUN << ": second time defined value '" << val << "'";
        throw InvalidParam(ostr.str());
      }

      val_ = val;
      installed_ = true;
    }

    //
    // Args::OptionSetterImpl class
    //

    template <typename Option>
    Args::OptionSetterImpl<Option>::OptionSetterImpl(Option& opt) throw ()
      : opt_(opt)
    {}

    template <typename Option>
    void
    Args::OptionSetterImpl<Option>::set(const char* opt_name,
      const char* val) throw (InvalidParam)
    {
      opt_.set(opt_name, val);
    }

    template <typename Option>
    bool
    Args::OptionSetterImpl<Option>::require_value() const throw ()
    {
      return opt_.require_value();
    }

    //
    // Args class
    //

    inline
    Args::Args(long command_count) throw (Gears::Exception)
      : command_count_(command_count)
    {}

    inline
    const Args::CommandList&
    Args::commands() const throw ()
    {
      return commands_;
    }

    inline void
    Args::append_flag_(
      const std::string& flag,
      bool short_opt,
      std::string& flags,
      std::string& usage) throw (Gears::Exception)
    {
      flags.append(flag);
      if (!usage.empty())
      {
        usage.push_back(',');
      }
      usage.append(short_opt ? " -" : " --");
      usage.append(flag);
    }

    template <typename Option>
    void
    Args::add(
      const OptionCont& cont,
      Option& opt,
      const char* comment,
      const char* arg_name)
      throw (Gears::Exception)
    {
      std::string flags, usage;

      for (OptionCont::StringList::const_iterator it =
        cont.short_options.begin(); it != cont.short_options.end(); ++it)
      {
        short_options_.insert(OptionSetterMap::value_type(*it,
          OptionSetter_var(new OptionSetterImpl<Option>(opt))));
        append_flag_(*it, true, flags, usage);
      }

      for (OptionCont::StringList::const_iterator it =
        cont.eq_options.begin(); it != cont.eq_options.end(); ++it)
      {
        eq_options_.insert(OptionSetterMap::value_type(*it,
          OptionSetter_var(new OptionSetterImpl<Option>(opt))));
        append_flag_(*it, false, flags, usage);
      }

      if (opt.require_value())
      {
        if (arg_name)
        {
          usage.append("=");
          usage.append(arg_name);
        }
        else
        {
          usage.append("=arg");
        }
      }

      if (comment)
      {
        usage.append(" - ");
        usage.append(comment);
      }

      usage_[flags].swap(usage);
    }
  }
}

#endif
