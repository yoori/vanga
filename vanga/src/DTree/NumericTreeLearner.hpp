#ifndef VANGA_NUMERIC_TREE_LEARNER_HPP_
#define VANGA_NUMERIC_TREE_LEARNER_HPP_

#include <functional>
#include <unordered_set>

#include "DTree.hpp"

namespace Vanga
{
  struct NumericDataset
  {
    std::vector<FloatFeatureArray> rows;
    std::vector<double> labels;
    // Optional legacy label,pred scores; only the binary adapter consumes these.
    std::vector<double> prior_predictions;
    bool binary = true;

    static NumericDataset
    load(std::istream& input, const std::unordered_set<unsigned long>& features = {});
  };

  class NumericTreeLearner
  {
  public:
    struct Options
    {
      unsigned long steps = 10;
      unsigned long max_depth = 5;
      unsigned long border_count = 32;
      unsigned long min_leaf_size = 1;
      unsigned long threads = 1;
      double learning_rate = 1.0;
      double l2 = 1.0;
    };

    struct Progress
    {
      unsigned long step;
      double train_logloss;
      std::vector<double> validation_logloss;
    };

    struct Result
    {
      DTree_var model;
      DTree_var best_model;
      double best_loss;
      unsigned long steps = 0;
    };

    using Callback = std::function<void(const Progress&, const DTree&)>;

    explicit NumericTreeLearner(const Options& options);

    Result fit(const NumericDataset& train, const std::vector<NumericDataset>& validation = {},
      const DTree* initial = nullptr, const Callback& callback = {}) const;

    static double logloss(const DTree& model, const NumericDataset& data);

    static DTree_var prune(const DTree& model, const NumericDataset& data, double min_cover);

  private:
    Options options_;
  };
}

#endif
