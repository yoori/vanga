#include "FloatFeatures.hpp"
#include "DTree.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <string>

namespace Vanga
{
  std::pair<bool, float> FloatFeatureArray::get(uint32_t id) const throw()
  {
    const auto it = std::lower_bound(begin(), end(), id,
      [](const value_type& feature, uint32_t key) { return feature.first < key; });
    const float value = it != end() && it->first == id ? it->second : 0.0f;
    return std::make_pair(value > 0.0000001f, value);
  }

  bool load_float_features(std::istream& input, FloatFeatureArray& features)
  {
    features.clear();
    std::string line;
    while (std::getline(input, line))
    {
      std::istringstream tokens(line);
      std::string token;
      if (!(tokens >> token) || token[0] == '#')
      {
        continue;
      }
      char* end = nullptr;
      const double label = std::strtod(token.c_str(), &end);
      if (end != token.c_str() + token.size() || !std::isfinite(label))
      {
        throw DTree::Exception("Invalid numeric LibSVM label: " + token);
      }
      while (tokens >> token)
      {
        if (token[0] == '#')
        {
          break;
        }
        const auto colon = token.find(':');
        const std::string id_text = token.substr(0, colon);
        if (id_text.empty() || id_text.find_first_not_of("0123456789") != std::string::npos)
        {
          throw DTree::Exception("Invalid LibSVM feature ID: " + token);
        }
        const unsigned long id = std::strtoul(id_text.c_str(), &end, 10);
        if (end != id_text.c_str() + id_text.size() || id == 0 ||
            id > std::numeric_limits<uint32_t>::max())
        {
          throw DTree::Exception("LibSVM feature ID must be in [1, 2^32-1]: " + token);
        }
        float value = 1;
        if (colon != std::string::npos)
        {
          const std::string value_text = token.substr(colon + 1);
          const double parsed = std::strtod(value_text.c_str(), &end);
          if (value_text.empty() || end != value_text.c_str() + value_text.size())
          {
            throw DTree::Exception("Invalid LibSVM feature value: " + token);
          }
          value = static_cast<float>(parsed);
        }
        features.emplace_back(static_cast<uint32_t>(id), value);
      }
      std::sort(features.begin(), features.end(),
        [](const FloatFeatureArray::value_type& a, const FloatFeatureArray::value_type& b)
        { return a.first < b.first; });
      for (size_t i = 1; i < features.size(); ++i)
      {
        if (features[i - 1].first == features[i].first)
        {
          throw DTree::Exception("Duplicate LibSVM feature ID");
        }
      }
      return true;
    }

    if (!input.eof() || input.bad())
    {
      throw DTree::Exception("Cannot read numeric LibSVM input");
    }
    return false;
  }
}
