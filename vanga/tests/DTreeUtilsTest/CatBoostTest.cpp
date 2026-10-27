#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

#include <DTree/DTree.hpp>
#include <DTree/FloatFeatures.hpp>
#include <DTree/PredictorSet.hpp>

namespace
{
  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  const std::string MODEL = R"({
    "features_info":{"float_features":[
      {"feature_index":0,"flat_feature_index":2,"nan_value_treatment":"AsFalse"},
      {"feature_index":1,"flat_feature_index":0,"nan_value_treatment":"AsTrue"}]},
    "scale_and_bias":[2.5,[-0.3]],
    "oblivious_trees":[
      {"splits":[
        {"split_type":"FloatFeature","float_feature_index":0,"border":0.5},
        {"split_type":"FloatFeature","float_feature_index":1,"border":-1.0}],
       "leaf_values":[0.12345678901234567,0.2,0.3,0.4]},
      {"splits":null,"leaf_values":[0.05]}]})";

  void predictions()
  {
    Vanga::DTree_var model = new Vanga::DTree(MODEL);
    std::istringstream source(MODEL);
    Vanga::DTree_var streamed = new Vanga::DTree(source);
    std::istringstream auto_source(" \n" + MODEL);
    const auto automatic = Vanga::DTree::load(auto_source);
    require(model->uses_numeric_features(), "numeric model flag missing");
    require(model->branches_.size() == 1, "constant tree was not folded into bias");
    require(model->to_string("").find(" > 0.5") != std::string::npos,
      "numeric split not shown in model description");
    const auto copy = model->copy();
    std::stringstream saved;
    model->save(saved);
    require(saved.str().find("dtree-float\n") == 0, "numeric format header missing");
    const auto restored = Vanga::DTree::load(saved);

    Vanga::PredictorSet_var ensemble = new Vanga::PredictorSet();
    ensemble->add(model.in(), 0.5);
    std::stringstream ensemble_data;
    ensemble->save(ensemble_data);
    const auto restored_ensemble = Vanga::PredictorSet::load(ensemble_data);

    const double leaves[] = {0.12345678901234567, 0.2, 0.3, 0.4};
    for (unsigned i = 0; i < 4; ++i)
    {
      Vanga::FloatFeatureArray row;
      row.emplace_back(1, (i & 2) ? 0.0f : -2.0f);
      row.emplace_back(3, (i & 1) ? 1.0f : 0.5f);
      const double expected = -0.3 + 2.5 * (0.05 + leaves[i]);
      for (const auto* candidate :
        {model.in(), streamed.in(), automatic.in(), copy.in(), restored.in()})
      {
        require(std::fabs(candidate->fpredict(row) - expected) < 1e-14,
          "leaf bit order, flat feature mapping, scale/bias or round-trip changed prediction");
      }
      require(std::fabs(restored_ensemble->fpredict(row) - 0.5 * expected) < 1e-14,
        "numeric ensemble lost prediction precision");
    }
    Vanga::FloatFeatureArray absent;
    require(std::fabs(model->fpredict(absent) - (-0.3 + 2.5 * (0.05 + leaves[2]))) < 1e-14,
      "absent feature must be numeric zero");
    Vanga::FloatFeatureArray nan;
    nan.emplace_back(1, std::numeric_limits<float>::quiet_NaN());
    nan.emplace_back(3, std::numeric_limits<float>::quiet_NaN());
    require(model->fpredict(nan) == model->fpredict(absent), "NaN routing changed");
    Vanga::FloatFeatureArray near;
    near.emplace_back(1, -2);
    near.emplace_back(3, static_cast<float>(0.5 + 1e-9));
    require(std::fabs(model->fpredict(near) - (-0.3 + 2.5 * (0.05 + leaves[0]))) < 1e-14,
      "CatBoost float32 threshold rounding changed");

    std::istringstream data("0 3:-2.5 1:nan\n0\n");
    Vanga::FloatFeatureArray parsed;
    require(Vanga::load_float_features(data, parsed) && parsed.size() == 2,
      "numeric LibSVM reader failed");
    require(std::isnan(parsed.get(1).second) && parsed.get(3).second == -2.5f,
      "numeric reader discarded negative/NaN values");
    require(Vanga::load_float_features(data, parsed) && parsed.empty(), "empty sparse row failed");
    require(!Vanga::load_float_features(data, parsed), "EOF not detected");

    const std::string constant =
      R"({"features_info":{},"oblivious_trees":[],"scale_and_bias":[1,[0.12345678901234567]]})";
    Vanga::DTree_var base = new Vanga::DTree(constant);
    std::stringstream constant_data;
    base->save(constant_data);
    const auto base_loaded = Vanga::DTree::load(constant_data);
    require(base->fpredict(absent) == base_loaded->fpredict(absent),
      "constant model lost precision");
  }

  void malformed()
  {
    for (const auto& json : {std::string(), std::string("{"), std::string("[]"),
           std::string("null"), MODEL + " trailing"})
    {
      bool rejected = false;
      try
      {
        Vanga::DTree_var invalid = new Vanga::DTree(json);
      }
      catch (const Gears::Exception&)
      {
        rejected = true;
      }
      require(rejected, "invalid CatBoost model was accepted");
    }
  }
}

int main()
{
  try
  {
    predictions();
    malformed();
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << std::endl;
    return 1;
  }
}
