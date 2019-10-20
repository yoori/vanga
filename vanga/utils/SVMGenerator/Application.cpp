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

#include <list>
#include <vector>
#include <iterator>
#include <iostream>
#include <fstream>

#include <Gears/Basic/AppUtils.hpp>
#include <Gears/String/Csv.hpp>
#include <Gears/ProfileStorage/FileWriter.hpp>
#include <xsd/Utils/SVMGeneratorConfig.hpp>

#include "HashCalculator.hpp"
#include "Application.hpp"

namespace
{
  const char USAGE[] =
    "\nUsage: \n"
    "SVMGenerator generate-svm <CONFIG DATA FILE> <FEATURE COLUMNS>\n"
    "\n";

  typedef const Gears::Ascii::Char2Category<',', '|'>
    ListSepType;
}

struct Config
{
  typedef std::vector<std::string> BasicFeatureNameArray;
  typedef std::vector<BasicFeatureNameArray> FeatureArray;
  FeatureArray features;
  unsigned long dimension;
};

void
parse_config(Config& res_config, const Gears::SubString& str)
{
  Gears::JsonValue root;
  Gears::JsonAllocator json_allocator;
  Gears::ArrayAutoPtr<char> str_holder(str.size() + 1);
  memcpy(str.data(), str_holder.get(), str.size());
  (*str_holder)[str.size()] = 0;
  char* parse_end;
  JsonParseStatus status = json_parse(
    str_holder.get(),
    parse_end,
    &root,
    json_allocator);

  if(root.get_tag() == Gears::JSON_TAG_OBJECT)
  {
  }
}

// Application
Application_::Application_()
  throw()
{}

Application_::~Application_() throw()
{}

void
Application_::main(int& argc, char** argv)
  throw(Gears::Exception)
{
  Gears::AppUtils::CheckOption opt_help;
  Gears::AppUtils::StringOption opt_dictionary;
  Gears::AppUtils::StringOption opt_name_dictionary;
  Gears::AppUtils::StringOption opt_cc_to_ccg_dictionary;
  Gears::AppUtils::StringOption opt_cc_to_campaign_dictionary;
  Gears::AppUtils::StringOption opt_tag_to_publisher_dictionary;
  Gears::AppUtils::CheckOption opt_out_hashes;
  Gears::AppUtils::Args args(-1);

  args.add(
    Gears::AppUtils::equal_name("help") ||
    Gears::AppUtils::short_name("h"),
    opt_help);

  args.add(
    Gears::AppUtils::equal_name("dictionary") ||
    Gears::AppUtils::equal_name("dict") ||
    Gears::AppUtils::short_name("d"),
    opt_dictionary);

  args.add(
    Gears::AppUtils::equal_name("hashes"),
    opt_out_hashes);

  args.parse(argc - 1, argv + 1);

  const Gears::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.empty() || opt_help.enabled() ||
     *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }

  std::string command = *commands.begin();

  if(command == "generate-svm")
  {
    Gears::AppUtils::Args::CommandList::const_iterator cmd_it =
      ++commands.begin();

    if(cmd_it == args.commands().end())
    {
      ErrorStream ostr;
      ostr << "generate-svm: config data file not defined";
      throw Exception(ostr.str());
    }

    const std::string config_data_file = *cmd_it;

    if(++cmd_it == args.commands().end())
    {
      ErrorStream ostr;
      ostr << "generate-svm: feature columns not defined";
      throw Exception(ostr.str());
    }

    const std::string feature_columns = *cmd_it;

    generate_svm_(
      std::cout,
      std::cin,
      config_data_file.c_str(),
      feature_columns.c_str(),
      opt_dictionary->c_str());
  }
  else
  {
    ErrorStream ostr;
    ostr << "unknown command '" << command << "', "
      "see help for more info" << std::endl;
    throw Exception(ostr.str());
  }
}

void
Application_::load_dictionary_(
  std::map<std::string, std::string>& dict,
  const char* file)
{
  std::ifstream in(file);
  if(!in.is_open())
  {
    ErrorStream ostr;
    ostr << "can't open '" << file << "'";
    throw Exception(ostr.str());
  }

  std::vector<std::string> values;

  while(!in.eof())
  {
    // parse line
    std::string line;
    std::getline(in, line);
    if(line.empty())
    {
      continue;
    }

    values.resize(0);
    Commons::CsvReader::parse_line(values, line);

    if(values.size() != 2)
    {
      ErrorStream ostr;
      ostr << "invalid dictionary line '" << line << "'";
      throw Exception(ostr.str());
    }

    dict[values[0]] = values[1];
  }
}

struct HashCalculatorHolder
{
  std::size_t seed;
  HashCalculator_var hash_calculator;
};

void
Application_::generate_svm_(
  std::ostream& out,
  std::istream& in,
  const char* config_file,
  const char* feature_columns_str,
  const char* dictionary_file_path)
{
  using namespace xsd::AdServer;

  Config::ErrorHandler error_handler;
  std::unique_ptr<Configuration::SVMGeneratorType> config;

  try
  {
    config = Configuration::SVMGenerator(config_file, error_handler);
  }
  catch(const xml_schema::parsing& e)
  {
    ErrorStream ostr;
    ostr << "Can't parse config file '" << config_file << "': ";
    if(error_handler.has_errors())
    {
      std::string error_string;
      ostr << error_handler.text(error_string);
    }

    throw Exception(ostr.str());
  }

  // parse model config
  unsigned long dimension = config->Model().features_dimension();
  unsigned long index_shifter = sizeof(uint32_t)*8 - dimension;

  // parse columns (name => {index, soil})
  std::map<std::string, unsigned long> feature_columns;
  std::set<unsigned long> key_columns;

  {
    std::vector<std::string> feature_column_names;
    Commons::CsvReader::parse_line(
      feature_column_names,
      String::SubString(feature_columns_str));

    unsigned long column_i = 0;
    for(auto it = feature_column_names.begin(); it != feature_column_names.end(); ++it, ++column_i)
    {
      String::AsciiStringManip::to_lower(*it);

      if(!it->empty() && (*it)[0] == '+')
      {
        key_columns.insert(column_i);
        *it = std::string(*it, 1);
      }

      feature_columns.insert(std::make_pair(*it, column_i));
    }
  }

  // configure model
  std::vector<HashCalculatorHolder> hash_calculators;

  for(Configuration::ModelType::Feature_sequence::const_iterator feature_it =
        config->Model().Feature().begin();
      feature_it != config->Model().Feature().end(); ++feature_it)
  {
    // eval seed
    std::string hc_name;
    HashCalculatorHolder hash_calculator_holder;

    {
      Gears::Murmur32v3Hash hash(hash_calculator_holder.seed);
      for(Configuration::FeatureType::BasicFeature_sequence::
            const_iterator basic_feature_it =
              feature_it->BasicFeature().begin();
          basic_feature_it != feature_it->BasicFeature().end();
          ++basic_feature_it)
      {
        std::string name = basic_feature_it->name();
        String::AsciiStringManip::to_lower(name);
        hash.add(name.data(), name.size());

        if(basic_feature_it != feature_it->BasicFeature().begin())
        {
          hc_name += '/';
        }

        hc_name += name;
      }
    }

    for(Configuration::FeatureType::BasicFeature_sequence::
          const_iterator basic_feature_it =
            feature_it->BasicFeature().begin();
        basic_feature_it != feature_it->BasicFeature().end();
        ++basic_feature_it)
    {
      auto column_it = feature_columns.find(basic_feature_it->name());

      if(column_it == feature_columns.end())
      {
        ErrorStream ostr;
        ostr << "Invalid basic feature name: '" << basic_feature_it->name() << "'";
        throw Exception(ostr.str());
      }

      HashCalculator_var prev_hash_calculator = hash_calculator_holder.hash_calculator;
      if(prev_hash_calculator)
      {
        hash_calculator_holder.hash_calculator = new HashCalculatorDelegateImpl(
          index_shifter,
          prev_hash_calculator,
          column_it->second);
      }
      else
      {
        hash_calculator_holder.hash_calculator = new HashCalculatorFinalImpl(
          index_shifter,
          hc_name.c_str(),
          column_it->second);
      }
    }

    hash_calculators.push_back(hash_calculator_holder);
  }

  std::unique_ptr<HashDictionary> dict_table;
  if(dictionary_file_path[0])
  {
    dict_table.reset(new HashDictionary());
  }

  // fetch input (values)
  unsigned long line_i = 0;
  HashCalculateParams params;
  params.values.reserve(feature_columns.size());

  while(!in.eof())
  {
    // parse line
    std::string line;
    std::getline(in, line);
    if(line.empty())
    {
      continue;
    }

    params.values.clear();

    bool skip_line = false;

    std::vector<std::string> column_values;

    try
    {
      Commons::CsvReader::parse_line(column_values, line);

      for(auto column_value_it = column_values.begin();
        column_value_it != column_values.end();
        ++column_value_it)
      {
        params.values.push_back(HashCalculateParams::Value());

        String::CategoryRepeatableTokenizer<
          String::Ascii::SepSpace> tokenizer(*column_value_it);
        String::SubString token;
        while(tokenizer.get_token(token))
        {
          params.values.back().push_back(token.str());
        }
      }
    }
    catch(const Gears::Exception& ex)
    {
      ErrorStream ostr;
      ostr << "Line #" << line_i << ": " << ex.what() << ex.what();
      std::cerr << ostr.str() << std::endl;

      skip_line = true;
    }

    if(skip_line)
    {
      continue;
    }

    // eval hashes
    HashArray hashes;

    for(auto hash_calc_it = hash_calculators.begin();
      hash_calc_it != hash_calculators.end(); ++hash_calc_it)
    {
      Murmur32v3Adapter hash_adapter(hash_calc_it->seed);
      hash_calc_it->hash_calculator->eval_hashes(
        hashes,
        hash_adapter,
        params);
    }

    // output hashes
    std::ostringstream res_line_ostr;

    for(auto key_it = key_columns.begin(); key_it != key_columns.end(); ++key_it)
    {
      res_line_ostr << column_values[*key_it] << ',';
    }

    std::map<unsigned long, unsigned long> ordered_hashes;

    for(auto hash_it = hashes.begin();
      hash_it != hashes.end(); ++hash_it)
    {
      ordered_hashes.insert(*hash_it);
    }

    for(auto hash_it = ordered_hashes.begin();
      hash_it != ordered_hashes.end(); ++hash_it)
    {
      if(hash_it != ordered_hashes.begin())
      {
        res_line_ostr << ' ';
      }
      res_line_ostr << hash_it->first;
    }

    if(dict_table.get())
    {
      // fill dict
      std::vector<String::SubString> value_path;
      for(auto hash_calc_it = hash_calculators.begin();
        hash_calc_it != hash_calculators.end(); ++hash_calc_it)
      {
        Murmur32v3Adapter hash_adapter(hash_calc_it->seed);
        hash_calc_it->hash_calculator->fill_dictionary(
          *dict_table,
          hash_adapter,
          params,
          value_path);
      }
    }

    // push line to output
    out << res_line_ostr.str() << std::endl;
    ++line_i;
  }

  // dump dictionaries
  if(dict_table.get())
  {
    std::ofstream dictionary_file(dictionary_file_path, std::ios_base::out);
    for(auto it = dict_table->begin(); it != dict_table->end(); ++it)
    {
      dictionary_file << it->first << ",";
      if(!it->second.empty())
      {
        std::string res = it->second;

        AdServer::LogProcessing::write_not_empty_string_as_csv(
          dictionary_file,
          res);
      }
      dictionary_file << std::endl;
    }
    dictionary_file.close();
  }
}

int main(int argc, char** argv)
{
  Application_* app = 0;

  try
  {
    app = &Application::instance();
  }
  catch (...)
  {
    std::cerr << "main(): Critical: Got exception while "
      "creating application object.\n";
    return -1;
  }

  assert(app);

  try
  {
    app->main(argc, argv);
  }
  catch(const Gears::Exception& ex)
  {
    std::cerr << "Caught Gears::Exception: " << ex.what() << std::endl;
    return -1;
  }

  return 0;
}


