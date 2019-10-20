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

#include <iostream>
#include <fstream>
#include <Gears/Basic/AppUtils.hpp>
#include <Gears/Plain/Parsing/Parser.hpp>
#include <Gears/Plain/Cpp/Generator.hpp>
#include <Gears/Plain/Code/Tracer.hpp>
#include <Gears/Plain/Declaration/Utils.hpp>
#include <Gears/String/AsciiStringManip.hpp>

namespace
{
  const char USAGE[] =
    "PlainCppCompiler [options] target-file\n"
    "Options:\n"
    "  --help, -h : output help\n"
    "  --trace, -t : output compile trace\n"
    "  --trace-types, -tt : trace types\n"
    "  --output-hpp : result hpp file name\n"
    "  --output-cpp : result cpp file name";
}

int main(int argc, char** argv)
{
  Gears::AppUtils::CheckOption opt_trace;
  Gears::AppUtils::CheckOption opt_trace_types;
  Gears::AppUtils::CheckOption opt_help;
  Gears::AppUtils::StringOption opt_out_hpp(""), opt_out_cpp("");

  Gears::AppUtils::Args args(-1);
  args.add(
    Gears::AppUtils::equal_name("trace") ||
    Gears::AppUtils::short_name("t"),
    opt_trace);
  args.add(
    Gears::AppUtils::equal_name("trace-types") ||
    Gears::AppUtils::short_name("tt"),
    opt_trace_types);
  args.add(
    Gears::AppUtils::equal_name("output-hpp"),
    opt_out_hpp);
  args.add(
    Gears::AppUtils::equal_name("output-cpp"),
    opt_out_cpp);
  args.add(
    Gears::AppUtils::equal_name("help") ||
    Gears::AppUtils::short_name("h"),
    opt_help);

  args.parse(argc - 1, argv + 1);

  if(opt_help.enabled())
  {
    std::cout << USAGE << std::endl;
    return 0;
  }

  const Gears::AppUtils::Args::CommandList& commands = args.commands();
  if(commands.empty())
  {
    std::cerr << "not defined target" << std::endl;
    return 1;
  }

  Code::ElementList_var elements(new Code::ElementList);
  Parsing::Parser_var parser = new Parsing::Parser();
  std::fstream file(commands.front().c_str(), std::ios_base::in);
  if(!file)
  {
    std::cerr << "can't open source file : " << *commands.begin() << std::endl;
    return 1;
  }

  Declaration::Namespace_var root_namespace;

  if(parser->parse(std::cerr, elements, file, &root_namespace))
  {
    if(opt_trace_types.enabled())
    {
      const Declaration::Namespace::BaseTypeMap& types =
        root_namespace->types();
      for(Declaration::Namespace::BaseTypeMap::const_iterator type_it =
            types.begin();
          type_it != types.end(); ++type_it)
      {
        std::cout << type_it->first << ": ";
        Declaration::type_traits(std::cout, type_it->second);
        std::cout << std::endl;
      }
    }
    else if(opt_trace.enabled())
    {
      Code::Tracer_var tracer(new Code::Tracer());
      tracer->generate(std::cout, elements);
    }
    else
    {
      Cpp::Generator_var generator(new Cpp::Generator());

      std::string include_guard("PLAIN_TYPES_");

      {
        std::string upper_out_hpp = *opt_out_hpp;
        Gears::Ascii::to_upper(upper_out_hpp);
        for(std::string::iterator it = upper_out_hpp.begin();
            it != upper_out_hpp.end(); ++it)
        {
          if(!((*it >= '0' && *it <= '9') ||
               (*it >= 'A' && *it <= 'Z')))
          {
            *it = '_';
          }
        }
        include_guard += upper_out_hpp + "_INCLUDE_GUARD";
      }
      
      std::fstream out_hpp(opt_out_hpp->c_str(), std::fstream::out);
      out_hpp << "#ifndef " << include_guard << std::endl <<
        "#define " << include_guard << std::endl << std::endl;
      std::fstream out_cpp(opt_out_cpp->c_str(), std::fstream::out);
      out_cpp << "#include \"" << *opt_out_hpp << "\"" << std::endl;

      generator->generate(out_hpp, out_hpp, out_cpp, elements);

      out_hpp << "#endif /*" << include_guard << "*/" << std::endl;
      out_hpp.close();
      out_cpp.close();
    }

    return 0;
  }

  return 1;
}
