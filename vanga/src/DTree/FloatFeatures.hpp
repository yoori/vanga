#ifndef VANGA_FLOAT_FEATURES_HPP_
#define VANGA_FLOAT_FEATURES_HPP_

#include <cstdint>
#include <istream>
#include <utility>
#include <vector>

namespace Vanga
{
  // Numeric values remain available in .second, including negative values and NaNs.
  // The boolean follows the native Vanga positive-feature presence convention.
  struct FloatFeatureArray : public std::vector<std::pair<uint32_t, float>>
  {
    std::pair<bool, float> get(uint32_t id) const throw();
  };

  // One-based LibSVM IDs. Missing values are zero; explicit NaN/Inf are retained.
  bool load_float_features(std::istream& input, FloatFeatureArray& features);
}

#endif
