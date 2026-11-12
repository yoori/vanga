#include <cmath>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>

#include <DTree/FastFeatureSet.hpp>
#include <DTree/NumericTreeLearner.hpp>

namespace
{
  void require(bool value, const char* message)
  {
    if (!value)
    {
      throw std::runtime_error(message);
    }
  }

  Vanga::NumericDataset load(const std::string& text)
  {
    std::istringstream input(text);
    return Vanga::NumericDataset::load(input);
  }

  void interval_training()
  {
    std::ostringstream text;
    for (int repeat = 0; repeat < 15; ++repeat)
    {
      for (float value : {-2.0f, -1.0f, -0.1f, 0.1f, 1.0f, 2.0f})
      {
        text << (std::abs(value) < 0.2f ? 1 : 0) << " 4000000000:" << value
             << " 1:" << (repeat % 2 ? 0.25 : 0.75) << '\n';
      }
    }
    auto data = load(text.str());
    require(!data.binary && data.rows[0].get(4000000000U).second == -2,
      "numeric loader binarized a training value");
    Vanga::NumericTreeLearner::Options options;
    options.steps = 30;
    options.max_depth = 1;
    options.l2 = 0.2;
    options.learning_rate = 0.5;
    auto result = Vanga::NumericTreeLearner(options).fit(data);
    require(Vanga::NumericTreeLearner::logloss(*result.model, data) < 0.25,
      "numeric learner did not separate the middle interval");
    std::set<float> borders;
    for (const auto& branch : result.model->branches_)
    {
      require(branch.numeric && branch.feature_id == 4000000000U, "numeric split lost metadata");
      borders.insert(branch.border);
    }
    require(borders.size() >= 2, "different thresholds of one feature were merged");
    std::stringstream saved;
    result.model->save(saved);
    require(saved.str().find("dtree-float\n") == 0, "numeric model saved as binary");
    auto restored = Vanga::DTree::load(saved);
    auto unpruned = Vanga::NumericTreeLearner::prune(*restored, data, 0);
    auto pruned = Vanga::NumericTreeLearner::prune(*restored, data, 1);
    require(pruned->node_count() == 1, "numeric coverage pruning retained uncovered children");
    Vanga::FastFeatureSet fast;
    for (const auto& row : data.rows)
    {
      fast.set(row.begin(), row.end());
      require(restored->fpredict(row) == result.model->fpredict(row), "float round-trip changed");
      require(unpruned->fpredict(row) == restored->fpredict(row),
        "zero-cover pruning changed model");
      require(restored->fpredict(fast) == restored->fpredict(row),
        "numeric fast prediction changed");
      fast.rollback(row);
    }
    options.threads = 3;
    auto parallel = Vanga::NumericTreeLearner(options).fit(data);
    std::ostringstream left, right;
    result.model->save(left);
    parallel.model->save(right);
    require(left.str() == right.str(), "numeric training depends on thread count");
  }

  void missing_and_nan()
  {
    const auto data = load("0 7:-2\n1\n1 7:1\n0 7:-2\n1\n1 7:1\n");
    Vanga::NumericTreeLearner::Options options;
    options.steps = 10;
    auto model = Vanga::NumericTreeLearner(options).fit(data).model;
    require(model->fpredict(data.rows[1]) > model->fpredict(data.rows[0]),
      "implicit sparse zero was not counted in numeric histograms");
    const auto nan = load("0\n1 7:nan\n0 7:0\n1 7:nan\n");
    auto nan_model = Vanga::NumericTreeLearner(options).fit(nan).model;
    require(!nan_model->branches_.empty() && nan_model->branches_[0].nan_go_right,
      "learner failed to separate NaNs from constant finite values");
    require(nan_model->fpredict(nan.rows[1]) > nan_model->fpredict(nan.rows[0]),
      "numeric NaN routing is incorrect");
    std::stringstream saved;
    nan_model->save(saved);
    auto restored = Vanga::DTree::load(saved);
    require(restored->fpredict(nan.rows[1]) == nan_model->fpredict(nan.rows[1]),
      "NaN routing changed after save/load");
    Vanga::NumericDataset adjacent;
    for (int repeat = 0; repeat < 10; ++repeat)
    {
      for (int high = 0; high < 2; ++high)
      {
        Vanga::FloatFeatureArray row;
        row.emplace_back(7, high ? std::nextafter(0.1f, 1.0f) : 0.1f);
        adjacent.rows.push_back(row);
        adjacent.labels.push_back(high);
      }
    }
    auto close = Vanga::NumericTreeLearner(options).fit(adjacent).model;
    require(close->fpredict(adjacent.rows[0]) < close->fpredict(adjacent.rows[1]),
      "float32 midpoint rounding merged adjacent feature values");
  }

  void resume_and_validation()
  {
    auto legacy = load("1,0.25 7:1\n0,-0.25\n");
    require(legacy.binary && legacy.prior_predictions == std::vector<double>({0.25, -0.25}),
      "legacy binary label,pred annotations were lost");
    auto data = load("0 7:0.1\n1 7:0.9\n0 7:0.1\n1 7:0.9\n");
    std::istringstream source(
      "dtree-float\n1\t0\t7:2:3:0.12345679104328156:1\n2\t0.1\t\n3\t-0.1\t\n");
    auto initial = Vanga::DTree::load(source);
    std::ostringstream before;
    initial->save(before);
    Vanga::NumericTreeLearner::Options options;
    options.steps = 10;
    auto result = Vanga::NumericTreeLearner(options).fit(data, {}, initial);
    const double initial_loss = Vanga::NumericTreeLearner::logloss(*initial, data);
    require(Vanga::NumericTreeLearner::logloss(*result.model, data) < initial_loss,
      "numeric warm-start did not improve");
    const auto& branch = result.model->branches_[0];
    require(branch.border == initial->branches_[0].border && branch.nan_go_right,
      "warm-start lost the existing numeric predicate");
    std::ostringstream after;
    initial->save(after);
    require(before.str() == after.str(), "learner mutated its initial model");
    auto validation = data;
    for (auto& label : validation.labels)
    {
      label = 1 - label;
    }
    result = Vanga::NumericTreeLearner(options).fit(data, {validation});
    require(result.best_model->node_count() == 1, "validation did not retain the best checkpoint");
    require(result.best_loss == Vanga::NumericTreeLearner::logloss(*result.best_model, validation),
      "best checkpoint logloss disagrees with its model");
    options.min_leaf_size = 100;
    result = Vanga::NumericTreeLearner(options).fit(data);
    require(result.steps == 0 && result.model->uses_numeric_features(),
      "constant numeric model lost its format or ignored leaf size");
    auto soft = load("0.2 7:0.1\n0.8 7:0.9\n0.2 7:0.1\n0.8 7:0.9\n");
    options.min_leaf_size = 1;
    result = Vanga::NumericTreeLearner(options).fit(soft);
    require(Vanga::NumericTreeLearner::logloss(*result.model, soft) < std::log(2.0),
      "probability targets were truncated");
    std::istringstream constant_source("dtree-float\n1\t0\t\n");
    auto constant = Vanga::DTree::load(constant_source);
    auto intercept_data = load("0.8\n0.8\n0.8\n0.8\n");
    result = Vanga::NumericTreeLearner(options).fit(intercept_data, {}, constant);
    require(result.model->node_count() == 1 && result.model->delta_prob > 0,
      "warm-start failed to update an intercept without feature splits");
  }
}

int main()
{
  try
  {
    interval_training();
    missing_and_nan();
    resume_and_validation();
    for (const auto& text : {"", "0.5 7:inf\n", "2 7:1\n", "0 7:1 7:2\n"})
    {
      bool rejected = false;
      try
      {
        load(text);
      }
      catch (const Gears::Exception&)
      {
        rejected = true;
      }
      require(rejected, "invalid numeric training input was accepted");
    }
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << std::endl;
    return 1;
  }
}
