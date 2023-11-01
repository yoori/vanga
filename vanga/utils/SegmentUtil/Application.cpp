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
#include <array>
#include <iterator>
#include <iostream>
#include <fstream>

#include <Gears/Basic/Algs.hpp>
#include <Gears/Basic/Hash.hpp>
#include <Gears/Basic/MemBuf.hpp>
#include <Gears/Basic/AppUtils.hpp>
#include <Gears/Basic/FileManip.hpp>
#include <Gears/String/Csv.hpp>

#include <Gears/ProfileStorage/KeySerializer.hpp>
#include <Gears/ProfileStorage/LevelProfileMap.hpp>
#include <Gears/ProfileStorage/TransactionProfileMap.hpp>

#include <Gears/Threading/ActiveObjectCallback.hpp>

#include "SegmentProfile.hpp"
#include "Application.hpp"

namespace
{
  const char USAGE[] =
    "\nUsage: \nSegmentUtil <COMMAND> <PARAMS>\n"
    "Commands: \n"
    "  profile\n"
    "  request\n"
    "  print\n"
    "  get-segments\n";
}

struct SegmentMatchesLess
{
  bool
  operator()(
    const Profiles::SegmentMatchesReader& segment_matches,
    unsigned long segment_id)
    const
  {
    return segment_matches.segment_id() < segment_id;
  }

  bool
  operator()(
    unsigned long segment_id,
    const Profiles::SegmentMatchesReader& segment_matches)
    const
  {
    return segment_id < segment_matches.segment_id();
  }
};

struct SegmentMatchesMerge
{
  SegmentMatchesMerge(const Gears::Time& ts)
    : timestamp_(ts)
  {}

  Profiles::SegmentMatchesWriter
  operator()(
    const Profiles::SegmentMatchesReader& reader,
    unsigned long /*segment_id*/)
    const
  {
    Profiles::SegmentMatchesWriter res;

    res.segment_id() = reader.segment_id();
    auto ins_ts_it = std::lower_bound(
      reader.timestamps().begin(),
      reader.timestamps().end(),
      timestamp_.tv_sec);
    res.timestamps().reserve(reader.timestamps().size() + 1);
    std::copy(
      reader.timestamps().begin(),
      ins_ts_it,
      std::back_inserter(res.timestamps()));
    res.timestamps().push_back(timestamp_.tv_sec);
    std::copy(
      ins_ts_it,
      reader.timestamps().end(),
      std::back_inserter(res.timestamps()));

    return res;
  }

  Profiles::SegmentMatchesWriter
  operator()(
    unsigned long segment_id,
    const Profiles::SegmentMatchesReader& reader)
    const
  {
    return this->operator()(reader, segment_id);
  }

protected:
  const Gears::Time timestamp_;
};

struct SegmentMatchesConverter
{
  SegmentMatchesConverter(const Gears::Time& ts)
    : timestamp_(ts)
  {}

  const Profiles::SegmentMatchesWriter&
  operator()(
    const Profiles::SegmentMatchesWriter& segment_matches)
    const
  {
    return segment_matches;
  }

  Profiles::SegmentMatchesWriter
  operator()(
    const Profiles::SegmentMatchesReader& reader)
    const
  {
    Profiles::SegmentMatchesWriter res;

    res.segment_id() = 1;

    res.segment_id() = reader.segment_id();
    res.timestamps().reserve(reader.timestamps().size());
    std::copy(
      reader.timestamps().begin(),
      reader.timestamps().end(),
      std::back_inserter(res.timestamps()));
    return res;
  }

  Profiles::SegmentMatchesWriter
  operator()(unsigned long segment_id)
    const
  {
    Profiles::SegmentMatchesWriter res;
    res.segment_id() = segment_id;
    res.timestamps().push_back(timestamp_.tv_sec);
    return res;
  }

protected:
  const Gears::Time timestamp_;
};

Application_::Application_()
  throw()
{}

Application_::~Application_() throw()
{}

uint64_t
string_to_hash(const std::string& value)
{
  uint64_t res;

  {
    Gears::Murmur64Hash hasher(res);
    hasher.add(value.data(), value.size());
  }

  return res;
}

Gears::IntrusivePtr<
  Gears::TransactionProfileMap<uint64_t> >
open_storage(
  Gears::ActiveObject_var& activator,
  const char* filename,
  unsigned long level0_size)
  /*throw (Gears::Exception)*/
{
  std::string root;
  std::string prefix;
  Gears::FileManip::split_path(filename, &root, &prefix, false);

  Gears::IntrusivePtr<Gears::LevelProfileMap<uint64_t, Gears::NumericSerializer> > level_map =
    new Gears::LevelProfileMap<uint64_t, Gears::NumericSerializer>(
      Gears::ActiveObjectCallback_var(
        new Gears::ActiveObjectCallbackStreamImpl(std::cerr, "SegmentUtil")),
      root.c_str(),
      prefix.c_str(),
      Gears::LevelMapTraits(
        Gears::LevelMapTraits::BLOCK_RUNTIME,
        10*1024*1024,
        level0_size, // level 0 size
        level0_size * 2, // max undumped size
        3, // max levels 0
        Gears::Time::ZERO));

  Gears::IntrusivePtr<Gears::TransactionProfileMap<uint64_t> > user_map =
    new Gears::TransactionProfileMap<uint64_t>(level_map);

  activator->activate_object();

  return user_map;
}

void
Application_::parse_hashes_(
  std::vector<uint32_t>& res,
  const Gears::SubString& str)
{
  Gears::CategoryRepeatableTokenizer<
    Gears::Ascii::SepSpace> tokenizer(str);

  Gears::SubString token;
  while(tokenizer.get_token(token))
  {
    if(!token.empty())
    {
      uint32_t feature_id;
      if(!Gears::StringManip::str_to_int(token, feature_id))
      {
        Gears::ErrorStream ostr;
        ostr << "can't parse feature_id value '" << token << "'";
        throw Exception(ostr.str());
      }

      res.push_back(feature_id);
    }
  }
}

void
Application_::load_hash_dictionary_(
  Dictionary& dict,
  const char* file)
{
  std::ifstream in(file);
  if(!in.is_open())
  {
    Gears::ErrorStream ostr;
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
    Gears::Csv::parse_line(values, line);

    if(values.size() != 2)
    {
      Gears::ErrorStream ostr;
      ostr << "invalid dictionary line '" << line << "'";
      throw Exception(ostr.str());
    }

    parse_hashes_(dict[values[0]], values[1]);
  }
}

void
Application_::save_hash_name_dictionary_(
  const char* file_name,
  const HashNameDictionary& dict)
{
  std::ofstream file(file_name);
  for(auto dict_it = dict.begin(); dict_it != dict.end(); ++dict_it)
  {
    file << dict_it->first << ",";
    Gears::Csv::write_escaped(file, dict_it->second);
    file << std::endl;
  }
}

void
Application_::load_hash_name_dictionary_(
  HashNameDictionary& dict,
  const char* file)
{
  std::ifstream in(file);
  if(!in.is_open())
  {
    Gears::ErrorStream ostr;
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
    Gears::Csv::parse_line(values, line);

    if(values.size() != 2)
    {
      Gears::ErrorStream ostr;
      ostr << "invalid dictionary line '" << line << "'";
      throw Exception(ostr.str());
    }

    uint32_t feature_id;
    if(!Gears::StringManip::str_to_int(values[0], feature_id))
    {
      Gears::ErrorStream ostr;
      ostr << "can't parse feature_id value in dictionary: '" << values[0] << "'";
      throw Exception(ostr.str());
    }

    dict[feature_id] = values[1];
  }
}

void
Application_::make_profile_(
  std::istream& in,
  const char* filename,
  const char* columns,
  unsigned long level0_size)
  /*throw (Gears::Exception)*/
{
  // parse column heads
  std::vector<std::string> values;
  std::vector<std::pair<unsigned long, std::unique_ptr<Dictionary> > > dictionaries;
  unsigned long id_column_i = 0;
  unsigned long timestamp_column_i = 0;
  std::vector<unsigned long> hash_columns;

  {
    // parse column heads
    Gears::Csv::parse_line(values, Gears::SubString(columns));

    unsigned long column_i = 0;
    for(auto it = values.begin(); it != values.end(); ++it, ++column_i)
    {
      if(*it == "id")
      {
        id_column_i = column_i;
      }
      else if(*it == "timestamp")
      {
        timestamp_column_i = column_i;
      }
      else if(*it == "hashes")
      {
        hash_columns.push_back(column_i);
      }
      else if(it->compare(0, 4, "ref:") == 0)
      {
        std::string file(*it, 4);
        std::pair<unsigned long, std::unique_ptr<Dictionary> > dict;
        dict.first = column_i;
        dict.second.reset(new Dictionary());
        load_hash_dictionary_(*dict.second, file.c_str());
        dictionaries.push_back(std::move(dict));
      }
    }
  }

  //const Gears::Time base_time = Gears::Time::ZERO;

  Gears::ActiveObject_var activator;
  auto user_map = open_storage(activator, filename, level0_size);

  std::string line;
  unsigned long line_i = 0;
  unsigned long timestamp_count = 0;
  unsigned long max_timestamp_count = 0;

  while(!in.eof())
  {
    std::getline(in, line);
    ++line_i;

    if(line.empty())
    {
      break;
    }

    values.clear();
    Gears::Csv::parse_line(values, line);

    std::string hashes_line;
    for(auto hash_column_it = hash_columns.begin();
      hash_column_it != hash_columns.end(); ++hash_column_it)
    {
      if(!hashes_line.empty() && *hashes_line.rbegin() != ' ')
      {
        hashes_line += ' ';
      }
      hashes_line += values[*hash_column_it];
    }

    Gears::CategoryRepeatableTokenizer<
      Gears::Ascii::SepSpace> tokenizer(hashes_line);

    const std::string user_id = values[id_column_i];
    Gears::Time timestamp;

    {
      uint32_t ts;
      if(!Gears::StringManip::str_to_int(values[timestamp_column_i], ts))
      {
        Gears::ErrorStream ostr;
        ostr << "can't parse timestamp value '" << values[timestamp_column_i] << "'";
        throw Exception(ostr.str());
      }

      timestamp = Gears::Time(ts);
    }

    //Gears::Time timestamp = values[timestamp_column_i];

    std::vector<uint32_t> features;

    for(auto hash_column_it = hash_columns.begin(); hash_column_it != hash_columns.end(); ++hash_column_it)
    {
      parse_hashes_(features, values[*hash_column_it]);
    }

    for(auto dict_it = dictionaries.begin(); dict_it != dictionaries.end(); ++dict_it)
    {
      auto dict_value_it = dict_it->second->find(values[dict_it->first]);
      if(dict_value_it != dict_it->second->end())
      {
        std::copy(
          dict_value_it->second.begin(),
          dict_value_it->second.end(),
          std::back_inserter(features));
      }
    }

    std::sort(features.begin(), features.end());
    auto er_it = std::unique(features.begin(), features.end());
    features.erase(er_it, features.end());

    /*
    std::cerr << "request for " << user_id << ": ";
    Gears::print(std::cerr, features.begin(), features.end());
    std::cerr << std::endl;
    */

    Gears::ConstSmartMemBuf_var mb = user_map->get_profile(string_to_hash(user_id));
    Profiles::SegmentProfileWriter profile_writer;

    if(mb.in())
    {
      Profiles::SegmentProfileReader profile_reader(mb->membuf().data(), mb->membuf().size());

      profile_writer.segment_matches().reserve(profile_reader.segment_matches().size() + features.size());

      Gears::merge_unique(
        profile_reader.segment_matches().begin(),
        profile_reader.segment_matches().end(),
        features.begin(),
        features.end(),
        Gears::modify_inserter(
          std::back_inserter(profile_writer.segment_matches()),
          SegmentMatchesConverter(timestamp)),
        SegmentMatchesLess(),
        SegmentMatchesMerge(timestamp));
    }
    else
    {
      profile_writer.segment_matches().reserve(features.size());

      std::copy(
        features.begin(),
        features.end(),
        Gears::modify_inserter(
          std::back_inserter(profile_writer.segment_matches()),
          SegmentMatchesConverter(timestamp)));
    }

    // save profile
    unsigned long sz = profile_writer.size();

    Gears::SmartMemBuf_var mem_buf = new Gears::SmartMemBuf(sz);
    profile_writer.save(mem_buf->membuf().data(), sz);

    //print_profile_from_buf_(user_id.c_str(), mem_buf->membuf());

    Gears::ConstSmartMemBuf_var c_mem_buf = Gears::transfer_membuf(mem_buf);

    user_map->save_profile(
      string_to_hash(user_id),
      c_mem_buf,
      Gears::Time::ZERO,
      Gears::OP_RUNTIME);

    timestamp_count += features.size();
    max_timestamp_count = std::max(
      max_timestamp_count,
      profile_writer.segment_matches().size());

    if(line_i % 50000 == 0)
    {
      std::cerr << Gears::Time::get_time_of_day().gm_ft() <<
        ": processed " << line_i << " lines"
        ", avg timestamps = " << (timestamp_count / line_i) <<
        ", max timestamps = " << max_timestamp_count <<
        std::endl;
    }
  }

  //user_map->dump();

  activator->deactivate_object();
  activator->wait_object();
}

void
Application_::request_profile_(
  std::istream& in,
  const char* filename,
  const char* columns,
  const SegmentRuleArray& segment_config,
  HashNameDictionary* res_hash_dictionary,
  const HashNameDictionary* base_hash_dictionary,
  bool svm_format,
  std::unordered_set<unsigned long>* filter_features,
  unsigned long dimension)
  /*throw (Gears::Exception)*/
{
  // parse column heads
  std::vector<std::string> values;
  unsigned long id_column_i = 0;
  unsigned long timestamp_column_i = 0;
  std::vector<unsigned long> push_columns;
  std::unordered_map<unsigned long, HashDescr> res_hash_descr_dictionary;

  {
    // parse column heads
    Gears::Csv::parse_line(values, Gears::SubString(columns));

    unsigned long column_i = 0;
    for(auto it = values.begin(); it != values.end(); ++it, ++column_i)
    {
      std::string col = *it;

      if(!col.empty() && col[0] == '+')
      {
        push_columns.push_back(column_i);
        col = std::string(col, 1);
      }

      if(col == "id")
      {
        id_column_i = column_i;
      }
      else if(col == "timestamp")
      {
        timestamp_column_i = column_i;
      }
    }
  }

  //const Gears::Time base_time = Gears::Time::ZERO;

  Gears::ActiveObject_var activator;

  auto user_map = open_storage(activator, filename, 5);

  std::set<Gears::Time> times_order;
  for(auto rule_it = segment_config.begin(); rule_it != segment_config.end(); ++rule_it)
  {
    times_order.insert(rule_it->time_to);
  }

  std::vector<Gears::Time> TIMES(times_order.begin(), times_order.end());

  //const Gears::Time MAX_IN_TIMES = *std::max_element(TIMES.begin(), TIMES.end());

  std::vector<unsigned long> time_counters(TIMES.size(), 0);
  std::string line;

  while(!in.eof())
  {
    std::getline(in, line);
    if(line.empty())
    {
      break;
    }

    values.clear();
    Gears::Csv::parse_line(values, line);

    const std::string user_id = values[id_column_i];
    Gears::Time timestamp;

    {
      uint32_t ts;
      if(!Gears::StringManip::str_to_int(values[timestamp_column_i], ts))
      {
        Gears::ErrorStream ostr;
        ostr << "can't parse timestamp value '" << values[timestamp_column_i] << "'";
        throw Exception(ostr.str());
      }

      timestamp = Gears::Time(ts);
    }

    Gears::ConstSmartMemBuf_var mb = user_map->get_profile(string_to_hash(user_id));
    Profiles::SegmentProfileReader profile_reader(
      mb->membuf().data(),
      mb->membuf().size());

    std::vector<uint32_t> hashes;

    for(auto segment_it = profile_reader.segment_matches().begin();
      segment_it != profile_reader.segment_matches().end(); ++segment_it)
    {
      std::fill(time_counters.begin(), time_counters.end(), 0);

      auto ts_it = std::upper_bound(
        (*segment_it).timestamps().begin(),
        (*segment_it).timestamps().end(),
        timestamp.tv_sec + 1);

      auto cur_time_it = TIMES.begin();

      for(Profiles::SegmentMatchesReader::timestamps_Container::const_reverse_iterator rev_ts_it(ts_it);
        rev_ts_it != (*segment_it).timestamps().rend() && cur_time_it != TIMES.end();
        ++rev_ts_it)
      {
        if(timestamp - *rev_ts_it > *cur_time_it)
        {
          ++cur_time_it;
        }

        ++time_counters[cur_time_it - TIMES.begin()];

        /*
        std::cout << "ADD TS[" << (cur_time_it - TIMES.begin()) << "]: " <<
          "ts = " << *rev_ts_it <<
          ", cur_time_it = " << *cur_time_it <<
          ", timestamp = " << timestamp.tv_sec <<
          "(" << (*segment_it).segment_id() << ")" << std::endl;
        */
      }

      /*
      unsigned long i = 0;
      for(auto tc_it = time_counters.begin(); tc_it != time_counters.end(); ++tc_it, ++i)
      {
        std::cout << "TS[" << i << "]: " << *tc_it << "(" << (*segment_it).segment_id() << ")" << std::endl;
      }
      */

      unsigned long cur_sum = 0;
      for(auto counter_it = time_counters.begin(); counter_it != time_counters.end(); ++counter_it)
      {
        cur_sum += *counter_it;

        for(auto rule_it = segment_config.begin(); rule_it != segment_config.end(); ++rule_it)
        {
          if(rule_it->time_to >= TIMES[counter_it - time_counters.begin()] &&
            rule_it->min_visits <= cur_sum)
          {
            uint32_t seed = rule_it->min_visits * 10000000 + rule_it->time_to.tv_sec;
            Gears::Murmur32v3Hasher hash_adapter(seed);
            uint32_t segment_id = (*segment_it).segment_id();
            hash_adapter.add(&segment_id, sizeof(segment_id));

            uint32_t res_hash = hash_adapter.finalize();

            res_hash = res_hash >> (32 - dimension);

            if(res_hash_dictionary)
            {
              res_hash_descr_dictionary.insert(
                std::make_pair(res_hash, HashDescr(segment_id, &*rule_it)));
            }

            if(!filter_features || filter_features->find(res_hash) != filter_features->end())
            {
              hashes.push_back(res_hash);
            }

            /*
            std::cerr << "WORK #" << segment_id << ":" << rule_it->min_visits << " per " <<
              TIMES[counter_it - time_counters.begin()] << " sec => " <<
              hashes.back() <<
              "(seed = " << seed <<
              ", time_to = " << rule_it->time_to <<
              ", mv = " << rule_it->min_visits <<
              ", cur_sum = " << cur_sum << ")" <<
              std::endl;
            */
          }
        }
      }
    }

    std::sort(hashes.begin(), hashes.end());
    auto er_it = std::unique(hashes.begin(), hashes.end());
    hashes.erase(er_it, hashes.end());

    const char SEP = (svm_format ? ' ' : ',');

    //std::cout << "hashes.size() = " << hashes.size() << std::endl;
    for(auto push_it = push_columns.begin(); push_it != push_columns.end(); ++push_it)
    {
      Gears::Csv::write_escaped(std::cout, values[*push_it]);
      std::cout << SEP;
    }

    if(!svm_format)
    {
      Gears::print(std::cout, hashes.begin(), hashes.end(), " ");
    }
    else
    {
      for(auto hash_it = hashes.begin(); hash_it != hashes.end(); ++hash_it)
      {
        if(hash_it != hashes.begin())
        {
          std::cout << SEP;
        }

        std::cout << *hash_it << ":1";
      }
    }

    std::cout << std::endl;
    // TODO : print result
  }

  //user_map->dump();

  if(res_hash_dictionary)
  {
    for(auto hash_it = res_hash_descr_dictionary.begin();
      hash_it != res_hash_descr_dictionary.end(); ++hash_it)
    {
      std::ostringstream name_ostr;

      if(base_hash_dictionary)
      {
        auto hash_name_it = base_hash_dictionary->find(hash_it->second.base_hash);
        if(hash_name_it != base_hash_dictionary->end())
        {
          name_ostr << hash_name_it->second;
        }
      }

      name_ostr << "(v=" << hash_it->second.rule->min_visits <<
        ",t=" << hash_it->second.rule->time_to.tv_sec << ")";

      res_hash_dictionary->insert(
        std::make_pair(hash_it->first, name_ostr.str()));
    }
  }
  
  activator->deactivate_object();
  activator->wait_object();
}

void
Application_::print_profile_(
  const char* user_id,
  const char* filename)
  /*throw (Gears::Exception)*/
{
  Gears::ActiveObject_var activator;
  auto user_map = open_storage(activator, filename, 5);

  if(user_id[0])
  {
    Gears::ConstSmartMemBuf_var mb = user_map->get_profile(string_to_hash(user_id));

    if(!mb)
    {
      std::cerr << "Not found" << std::endl;
    }
    else
    {
      print_profile_from_buf_(user_id, mb->membuf());
    }
  }

  activator->deactivate_object();
  activator->wait_object();
}

void
Application_::print_profile_from_buf_(
  const char* user_id,
  const Gears::MemBuf& buf)
  throw()
{
  try
  {
    // print content
    Profiles::SegmentProfileReader profile_reader(
      buf.data(), buf.size());

    std::cout << "User: " << user_id << ":" << std::endl;
    for(auto segment_it = profile_reader.segment_matches().begin();
      segment_it != profile_reader.segment_matches().end(); ++segment_it)
    {
      std::cout << "  " << (*segment_it).segment_id() << ": ";
      Gears::print(std::cout, (*segment_it).timestamps().begin(), (*segment_it).timestamps().end());
      std::cout << std::endl;
    }
  }
  catch(const Gears::Exception& ex)
  {
    std::cerr << "Can't print profile for User '" << user_id <<
      "': " << ex.what() << std::endl;
  }
}

void
Application_::main(int& argc, char** argv)
  /*throw(Gears::Exception)*/
{
  Gears::AppUtils::CheckOption opt_help;
  Gears::AppUtils::CheckOption opt_plain;
  Gears::AppUtils::StringOption opt_user_id("");
  Gears::AppUtils::StringOption opt_storage_root("./");
  Gears::AppUtils::Option<unsigned long> opt_level0_size(5);
  Gears::AppUtils::StringOption opt_segment_config("0:1");
  Gears::AppUtils::StringOption opt_base_dictionary("");
  Gears::AppUtils::StringOption opt_res_dictionary_file("");
  Gears::AppUtils::CheckOption opt_svm_format;
  Gears::AppUtils::StringOption opt_features_file;
  Gears::AppUtils::Option<unsigned long> opt_dimension(16);

  Gears::AppUtils::Args args(-1);

  args.add(
    Gears::AppUtils::equal_name("help") ||
    Gears::AppUtils::short_name("h"),
    opt_help);
  args.add(
    Gears::AppUtils::equal_name("plain") ||
    Gears::AppUtils::short_name("p"),
    opt_plain);
  args.add(
    Gears::AppUtils::equal_name("user-id") ||
    Gears::AppUtils::equal_name("uid") ||
    Gears::AppUtils::short_name("u"),
    opt_user_id);
  args.add(
    Gears::AppUtils::equal_name("storage") ||
    Gears::AppUtils::short_name("s"),
    opt_storage_root);
  args.add(
    Gears::AppUtils::equal_name("level0-size") ||
    Gears::AppUtils::short_name("l0"),
    opt_level0_size);
  args.add(
    Gears::AppUtils::equal_name("segment-config") ||
    Gears::AppUtils::short_name("sc"),
    opt_segment_config);
  args.add(
    Gears::AppUtils::equal_name("base-dict") ||
    Gears::AppUtils::short_name("bd"),
    opt_base_dictionary);
  args.add(
    Gears::AppUtils::equal_name("dict") ||
    Gears::AppUtils::short_name("d"),
    opt_res_dictionary_file);
  args.add(
    Gears::AppUtils::equal_name("svm"),
    opt_svm_format);
  args.add(
    Gears::AppUtils::equal_name("features-file") ||
    Gears::AppUtils::short_name("ff"),
    opt_features_file);
  args.add(
    Gears::AppUtils::equal_name("dimension"),
    opt_dimension);

  args.parse(argc - 1, argv + 1);

  const Gears::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.empty() || opt_help.enabled() ||
     *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }

  auto command_it = commands.begin();
  std::string command = *command_it;

  if(command == "profile")
  {
    ++command_it;
    if(command_it == commands.end())
    {
      std::cerr << "columns not defined" << std::endl;
      return;
    }

    const std::string columns = *command_it;

    make_profile_(
      std::cin,
      opt_storage_root->c_str(),
      columns.c_str(),
      *opt_level0_size * 1024 * 1024 * 1024);
  }
  else if(command == "request")
  {
    ++command_it;
    if(command_it == commands.end())
    {
      std::cerr << "columns not defined" << std::endl;
      return;
    }

    const std::string columns = *command_it;

    SegmentRuleArray segment_config;

    {
      // parse segment config
      Gears::CategoryRepeatableTokenizer<
        Gears::Ascii::SepComma> tokenizer(*opt_segment_config);

      Gears::SubString token;
      while(tokenizer.get_token(token))
      {
        if(!token.empty())
        {
          Gears::CategoryRepeatableTokenizer<
            Gears::Ascii::SepColon> sub_tokenizer(token);

          Gears::SubString time_to_str;
          sub_tokenizer.get_token(time_to_str);

          Gears::SubString min_visits_str;
          sub_tokenizer.get_token(min_visits_str);

          unsigned long time_to;
          if(!Gears::StringManip::str_to_int(time_to_str, time_to))
          {
            Gears::ErrorStream ostr;
            ostr << "can't parse time_to value '" << time_to_str << "'";
            throw Exception(ostr.str());
          }

          unsigned long min_visits;
          if(!Gears::StringManip::str_to_int(min_visits_str, min_visits))
          {
            Gears::ErrorStream ostr;
            ostr << "can't parse min_visits value '" << min_visits_str << "'";
            throw Exception(ostr.str());
          }

          SegmentRule segment_rule;
          segment_rule.time_to = Gears::Time(time_to);
          segment_rule.min_visits = min_visits;
          segment_config.push_back(segment_rule);
        }
      }
    }

    std::unique_ptr<HashNameDictionary> base_dictionary;

    if(!opt_base_dictionary->empty())
    {
      base_dictionary.reset(new HashNameDictionary());
      load_hash_name_dictionary_(*base_dictionary, opt_base_dictionary->c_str());
    }

    std::unique_ptr<HashNameDictionary> res_dictionary;

    if(!opt_res_dictionary_file->empty())
    {
      res_dictionary.reset(new HashNameDictionary());
      if(Gears::FileManip::file_exists(*opt_res_dictionary_file))
      {
        load_hash_name_dictionary_(*res_dictionary, opt_res_dictionary_file->c_str());
      }
    }

    std::unordered_set<unsigned long> filter_features;

    if(!opt_features_file->empty())
    {
      std::ifstream ff(opt_features_file->c_str());
      if(!ff.is_open())
      {
        Gears::ErrorStream ostr;
        ostr << "can't open filter features file '" << *opt_features_file << "'";
        throw Exception(ostr.str());
      }

      while(!ff.eof())
      {
        std::string line;
        std::getline(ff, line);

        if(!line.empty())
        {
          unsigned long feature_id;
          if(!Gears::StringManip::str_to_int(line, feature_id))
          {
            Gears::ErrorStream ostr;
            ostr << "can't parse filter feature '" << line << "'";
            throw Exception(ostr.str());
          }

          filter_features.insert(feature_id);
        }
      }
    }

    request_profile_(
      std::cin,
      opt_storage_root->c_str(),
      columns.c_str(),
      segment_config,
      res_dictionary.get(),
      base_dictionary.get(),
      opt_svm_format.enabled(),
      !filter_features.empty() ? &filter_features : 0,
      *opt_dimension);

    if(res_dictionary.get())
    {
      save_hash_name_dictionary_(opt_res_dictionary_file->c_str(), *res_dictionary);
    }
  }
  else if(command == "print")
  {
    print_profile_(opt_user_id->c_str(), opt_storage_root->c_str());
  }
  else if(command == "get-segments")
  {
  }
  else if(command == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }
  else
  {
    std::cerr << "Unknown command '" << command << "'. "
      "See help for more info." << std::endl;
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

  if (app == 0)
  {
    std::cerr << "main(): Critical: got NULL application object.\n";
    return -1;
  }

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


