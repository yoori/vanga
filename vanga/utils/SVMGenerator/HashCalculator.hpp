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

#ifndef HASHCALCULATOR_HPP_
#define HASHCALCULATOR_HPP_

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>
#include <Gears/Basic/IntrusivePtr.hpp>
#include <Gears/Basic/Hash.hpp>

using HashArray = std::vector<std::pair<uint32_t, uint32_t>>;

struct Murmur32v3Adapter : public Gears::Murmur32v3Hasher
{
  Murmur32v3Adapter(std::size_t hash_seed);

  template <typename Value> void add(const Value& value) throw();

  void add(const std::string& value) throw();
};

inline Murmur32v3Adapter::Murmur32v3Adapter(std::size_t hash_seed)
  : Gears::Murmur32v3Hasher(hash_seed)
{
}

template <typename Value> void Murmur32v3Adapter::add(const Value& value) throw()
{
  Gears::Murmur32v3Hasher::add(&value, sizeof(value));
}

inline void Murmur32v3Adapter::add(const std::string& value) throw()
{
  Gears::Murmur32v3Hasher::add(value.data(), value.size());
}

struct HashCalculateParams
{
  using Value = std::vector<std::string>;
  using ValueArray = std::vector<Value>;

  ValueArray values;
};

using HashDictionary = std::map<unsigned long, std::string>;

class HashCalculator : public ReferenceCounting::AtomicImpl
{
public:
  HashCalculator(unsigned long shifter, const char* name);

  virtual void eval_hashes(HashArray& result_hashes, Murmur32v3Adapter& hash_adapter,
    const HashCalculateParams& params) throw() = 0;

  virtual void fill_dictionary(HashDictionary& dictionary, Murmur32v3Adapter& hash_adapter,
    const HashCalculateParams& params, std::vector<String::SubString>& value_path) throw() = 0;

protected:
  virtual ~HashCalculator() throw();

  bool hash_index_(uint32_t& index, uint32_t hash) const;

  void add_dictionary_value_(HashDictionary& dictionary, uint32_t index,
    const std::vector<String::SubString>& value_path, const String::SubString* add_value);

protected:
  const unsigned long shifter_;
  const std::string name_;
};

inline HashCalculator::~HashCalculator() throw() = default;
inline HashCalculator::HashCalculator(unsigned long shifter, const char* name)
  : shifter_(shifter), name_(name)
{
}

inline bool HashCalculator::hash_index_(uint32_t& index, uint32_t hash) const
{
  // shifter > 0 => index > 1
  index = (hash >> shifter_) + 1;
  return true;
}

inline void HashCalculator::add_dictionary_value_(HashDictionary& dictionary, uint32_t index,
  const std::vector<String::SubString>& value_path, const String::SubString* add_value)
{
  auto dict_it = dictionary.find(index);
  if (dict_it == dictionary.end())
  {
    std::string key;
    for (auto path_it = value_path.begin(); path_it != value_path.end(); ++path_it)
    {
      if (path_it != value_path.begin())
      {
        key += '/';
      }

      key += path_it->str();
    }

    if (add_value)
    {
      if (!value_path.empty())
      {
        key += '/';
      }

      key += add_value->str();

      dictionary.insert(std::make_pair(index, name_ + ":" + key));
    }
  }
}

using HashCalculator_var = ReferenceCounting::SmartPtr<HashCalculator>;

// HashCalculatorFinalImpl
class HashCalculatorFinalImpl : public HashCalculator
{
public:
  HashCalculatorFinalImpl(unsigned long shifter, const char* name,
    unsigned long feature_index) throw();

  virtual void eval_hashes(HashArray& result_hashes, Murmur32v3Adapter& hash_adapter,
    const HashCalculateParams& params) throw();

  void fill_dictionary(HashDictionary& dictionary, Murmur32v3Adapter& hash_adapter,
    const HashCalculateParams& params, std::vector<String::SubString>& value_path) throw();

protected:
  virtual ~HashCalculatorFinalImpl() throw();

protected:
  const unsigned long feature_index_;
};

inline HashCalculatorFinalImpl::~HashCalculatorFinalImpl() throw() = default;
inline HashCalculatorFinalImpl::HashCalculatorFinalImpl(unsigned long shifter, const char* name,
  unsigned long feature_index) throw()
  : HashCalculator(shifter, name), feature_index_(feature_index)
{
}

inline void HashCalculatorFinalImpl::eval_hashes(HashArray& result_hashes,
  Murmur32v3Adapter& hash_adapter, const HashCalculateParams& params) throw()
{
  const HashCalculateParams::Value& value = params.values[feature_index_];

  if (!value.empty())
  {
    for (auto it = value.begin(); it != value.end(); ++it)
    {
      // need local hasher
      Murmur32v3Adapter hash_adapter_copy(hash_adapter);
      hash_adapter_copy.add(*it);

      uint32_t index;
      if (hash_index_(index, hash_adapter_copy.finalize()))
      {
        result_hashes.push_back(std::make_pair(index, 1));
      }
    }
  }
  else
  {
    hash_adapter.add(static_cast<uint32_t>(0));

    uint32_t index;
    if (hash_index_(index, hash_adapter.finalize()))
    {
      result_hashes.push_back(std::make_pair(index, 1));
    }
  }
}

inline void HashCalculatorFinalImpl::
  fill_dictionary(HashDictionary& dictionary, Murmur32v3Adapter& hash_adapter,
    const HashCalculateParams& params, std::vector<String::SubString>& value_path) throw()
{
  const HashCalculateParams::Value& value = params.values[feature_index_];

  if (!value.empty())
  {
    for (auto it = value.begin(); it != value.end(); ++it)
    {
      // need local hasher
      Murmur32v3Adapter hash_adapter_copy(hash_adapter);
      hash_adapter_copy.add(*it);

      uint32_t index;
      if (hash_index_(index, hash_adapter_copy.finalize()))
      {
        String::SubString val(*it);

        add_dictionary_value_(dictionary, index, value_path, &val);
      }
    }
  }
  else
  {
    hash_adapter.add(static_cast<uint32_t>(0));

    uint32_t index;
    if (hash_index_(index, hash_adapter.finalize()))
    {
      String::SubString empty_ss;
      add_dictionary_value_(dictionary, index, value_path, &empty_ss);
    }
  }
}

// HashCalculatorDelegateImpl
class HashCalculatorDelegateImpl : public HashCalculator
{
public:
  HashCalculatorDelegateImpl(unsigned long shifter, HashCalculator* next_calculator,
    unsigned long feature_index) throw();

  virtual void eval_hashes(HashArray& result_hashes, Murmur32v3Adapter& hash_adapter,
    const HashCalculateParams& params) throw();

  void fill_dictionary(HashDictionary& dictionary, Murmur32v3Adapter& hash_adapter,
    const HashCalculateParams& params, std::vector<String::SubString>& value_path) throw();

protected:
  virtual ~HashCalculatorDelegateImpl() throw();

protected:
  const HashCalculator_var next_calculator_;
  const unsigned long feature_index_;
};

inline HashCalculatorDelegateImpl::~HashCalculatorDelegateImpl() throw() = default;
inline HashCalculatorDelegateImpl::HashCalculatorDelegateImpl(unsigned long shifter,
  HashCalculator* next_calculator, unsigned long feature_index) throw()
  : HashCalculator(shifter, ""), next_calculator_(ReferenceCounting::add_ref(next_calculator)),
    feature_index_(feature_index)
{
}

inline void HashCalculatorDelegateImpl::eval_hashes(HashArray& result_hashes,
  Murmur32v3Adapter& hash_adapter, const HashCalculateParams& params) throw()
{
  const HashCalculateParams::Value& value = params.values[feature_index_];

  if (!value.empty())
  {
    for (auto it = value.begin(); it != value.end(); ++it)
    {
      Murmur32v3Adapter hash_adapter_copy(hash_adapter);
      hash_adapter_copy.add(*it);
      next_calculator_->eval_hashes(result_hashes, hash_adapter_copy, params);
    }
  }
  else
  {
    hash_adapter.add(static_cast<uint32_t>(0));
    next_calculator_->eval_hashes(result_hashes, hash_adapter, params);
  }
}

inline void HashCalculatorDelegateImpl::
  fill_dictionary(HashDictionary& dictionary, Murmur32v3Adapter& hash_adapter,
    const HashCalculateParams& params, std::vector<String::SubString>& value_path) throw()
{
  const HashCalculateParams::Value& value = params.values[feature_index_];

  if (!value.empty())
  {
    for (auto it = value.begin(); it != value.end(); ++it)
    {
      Murmur32v3Adapter hash_adapter_copy(hash_adapter);
      hash_adapter_copy.add(*it);
      value_path.push_back(*it);
      next_calculator_->fill_dictionary(dictionary, hash_adapter_copy, params, value_path);
      value_path.pop_back();
    }
  }
  else
  {
    hash_adapter.add(static_cast<uint32_t>(0));
    value_path.push_back(String::SubString());
    next_calculator_->fill_dictionary(dictionary, hash_adapter, params, value_path);
    value_path.pop_back();
  }
}

#endif /*HASHCALCULATOR_HPP_*/
