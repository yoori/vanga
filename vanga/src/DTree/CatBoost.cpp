#include "DTree.hpp"

#include <cmath>
#include <cctype>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <sstream>

#include <Gears/String/Gason.hpp>

namespace Vanga
{
  namespace
  {
    using Gears::JsonValue;

    [[noreturn]] void invalid(const std::string& message)
    {
      throw DTree::Exception("CatBoost JSON: " + message);
    }

    const JsonValue* member(const JsonValue& object, const char* key)
    {
      if (object.get_tag() != Gears::JSON_TAG_OBJECT)
      {
        invalid("expected an object for field " + std::string(key));
      }
      const JsonValue* result = nullptr;
      for (const auto* node : object)
      {
        if (std::string(node->key) == key)
        {
          if (result)
          {
            invalid("duplicate field " + std::string(key));
          }
          result = &node->value;
        }
      }
      return result;
    }

    const JsonValue& required(const JsonValue& object, const char* key)
    {
      const auto* value = member(object, key);
      if (!value)
      {
        invalid("missing field " + std::string(key));
      }
      return *value;
    }

    std::vector<JsonValue> array(const JsonValue& value, const char* name)
    {
      if (value.get_tag() != Gears::JSON_TAG_ARRAY)
      {
        invalid(std::string(name) + " must be an array");
      }
      std::vector<JsonValue> result;
      for (const auto* node : value)
      {
        result.push_back(node->value);
      }
      return result;
    }

    std::string string(const JsonValue& value, const char* name)
    {
      if (value.get_tag() != Gears::JSON_TAG_STRING)
      {
        invalid(std::string(name) + " must be a string");
      }
      return value.to_string().str();
    }

    double number(const JsonValue& value, const char* name)
    {
      if (value.get_tag() != Gears::JSON_TAG_NUMBER)
      {
        invalid(std::string(name) + " must be a number");
      }
      const std::string text = value.to_string().str();
      char* end = nullptr;
      const double result = std::strtod(text.c_str(), &end);
      if (end != text.c_str() + text.size() || !std::isfinite(result))
      {
        invalid(std::string(name) + " must be finite");
      }
      return result;
    }

    uint32_t index(const JsonValue& value, const char* name)
    {
      const double result = number(value, name);
      if (result < 0 || result > std::numeric_limits<uint32_t>::max() - 1 ||
          std::floor(result) != result)
      {
        invalid(std::string(name) + " is not a supported feature index");
      }
      return static_cast<uint32_t>(result);
    }

    struct FloatFeature
    {
      uint32_t id;
      bool nan_go_right;
    };
  }

  DTree::DTree(std::istream& catboost_json) : DTree()
  {
    load_catboost_(catboost_json);
  }

  DTree::DTree(const std::string& catboost_json) : DTree()
  {
    std::istringstream input(catboost_json);
    load_catboost_(input);
  }

  void DTree::load_catboost_(std::istream& input)
  {
    std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (input.bad() || (input.fail() && !input.eof()))
    {
      invalid("cannot read input stream");
    }

    if (text.find('\0') != std::string::npos)
    {
      invalid("embedded NUL in input");
    }

    text.push_back('\0');
    Gears::JsonAllocator allocator;
    JsonValue root;
    char* end = nullptr;
    const auto status = Gears::json_parse(&text[0], &end, &root, allocator);
    if (status != Gears::JSON_PARSE_OK)
    {
      invalid(Gears::json_parse_error(status));
    }

    while (end && std::isspace(static_cast<unsigned char>(*end)))
    {
      ++end;
    }

    if (!end || *end)
    {
      invalid("trailing data after JSON model");
    }

    if (member(root, "trees"))
    {
      invalid("non-symmetric trees are not supported; expected oblivious_trees");
    }
    const auto& info = required(root, "features_info");
    for (const auto* name : {"categorical_features", "text_features", "embedding_features", "ctrs"})
    {
      const auto* unsupported = member(info, name);
      if (unsupported && !array(*unsupported, name).empty())
      {
        invalid(std::string(name) + " are not supported; expected float features");
      }
    }
    std::map<uint32_t, FloatFeature> features;
    std::set<uint32_t> flat_indexes;
    if (const auto* float_features = member(info, "float_features"))
    {
      for (const auto& feature : array(*float_features, "float_features"))
      {
        const auto feature_index = index(required(feature, "feature_index"), "feature_index");
        const auto flat_index =
          index(required(feature, "flat_feature_index"), "flat_feature_index");
        std::string nan_mode = "AsIs";
        if (const auto* mode = member(feature, "nan_value_treatment"))
        {
          nan_mode = string(*mode, "nan_value_treatment");
        }

        if (nan_mode != "AsIs" && nan_mode != "AsFalse" && nan_mode != "AsTrue")
        {
          invalid("unsupported nan_value_treatment " + nan_mode);
        }

        if (!features.emplace(feature_index, FloatFeature{flat_index + 1, nan_mode == "AsTrue"})
              .second ||
            !flat_indexes.insert(flat_index).second)
        {
          invalid("duplicate feature_index or flat_feature_index");
        }
      }
    }
    double scale = 1;
    double bias = 0;
    if (const auto* scale_and_bias = member(root, "scale_and_bias"))
    {
      const auto values = array(*scale_and_bias, "scale_and_bias");
      if (values.size() != 2)
      {
        invalid("scale_and_bias must contain scale and bias");
      }

      scale = number(values[0], "scale");
      const auto biases = array(values[1], "bias");

      if (biases.size() != 1)
      {
        invalid("only scalar models are supported (one bias/leaf value)");
      }
      bias = number(biases[0], "bias");
    }

    tree_id = 1;
    delta_prob = bias;
    numeric_model_ = true;
    unsigned long next_id = 2;
    for (const auto& tree : array(required(root, "oblivious_trees"), "oblivious_trees"))
    {
      std::vector<Branch> splits;
      const auto* split_values = member(tree, "splits");
      if (split_values && split_values->get_tag() != Gears::JSON_TAG_NULL)
      {
        for (const auto& split : array(*split_values, "splits"))
        {
          if (string(required(split, "split_type"), "split_type") != "FloatFeature")
          {
            invalid("only FloatFeature splits are supported");
          }
          const auto feature_index =
            index(required(split, "float_feature_index"), "float_feature_index");
          const auto feature = features.find(feature_index);
          if (feature == features.end())
          {
            invalid("split references an unknown float_feature_index");
          }
          Branch branch;
          branch.feature_id = feature->second.id;
          branch.numeric = true;
          branch.border = static_cast<float>(number(required(split, "border"), "border"));
          if (!std::isfinite(branch.border))
          {
            invalid("border is outside float32 range");
          }
          branch.nan_go_right = feature->second.nan_go_right;
          splits.push_back(branch);
        }
      }

      const auto leaves = array(required(tree, "leaf_values"), "leaf_values");
      if (splits.size() >= std::numeric_limits<size_t>::digits ||
          leaves.size() != (size_t(1) << splits.size()))
      {
        invalid("leaf_values count must equal 2^depth; only scalar leaves are supported");
      }
      std::vector<double> values;
      for (const auto& leaf : leaves)
      {
        const double value = scale * number(leaf, "leaf value");
        if (!std::isfinite(value))
        {
          invalid("scaled leaf value is not finite");
        }
        values.push_back(value);
      }

      std::function<DTree_var(size_t, size_t)> build = [&](size_t level, size_t leaf_index)
      {
        DTree_var node = new DTree();
        node->tree_id = next_id++;
        if (level == splits.size())
        {
          node->delta_prob = values[leaf_index];
        }
        else
        {
          Branch branch = splits[level];
          // CatBoost stores level 0 in the least significant leaf-index bit.
          branch.no_tree = build(level + 1, leaf_index);
          branch.yes_tree = build(level + 1, leaf_index | (size_t(1) << level));
          node->branches_.push_back(branch);
        }
        return node;
      };
      const auto converted = build(0, 0);
      delta_prob += converted->delta_prob;
      branches_.insert(branches_.end(), converted->branches_.begin(), converted->branches_.end());
    }

    if (!std::isfinite(delta_prob))
    {
      invalid("combined bias is not finite");
    }
  }
}
