#include "NumericTreeLearner.hpp"

#include <algorithm>
#include <cmath>
#include <future>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <tuple>
#include <unordered_map>

namespace Vanga
{
  namespace
  {
    struct Stats
    {
      double gradient = 0;
      double hessian = 0;
      std::size_t count = 0;
    };

    Stats add(Stats left, const Stats& right)
    {
      left.gradient += right.gradient;
      left.hessian += right.hessian;
      left.count += right.count;
      return left;
    }

    Stats subtract(const Stats& left, const Stats& right)
    {
      return {
        left.gradient - right.gradient, left.hessian - right.hessian, left.count - right.count};
    }

    double loss(double raw, double label)
    {
      return std::max(raw, 0.0) - label * raw + std::log1p(std::exp(-std::abs(raw)));
    }

    void validate(const NumericDataset& data)
    {
      if (data.rows.empty() || data.rows.size() != data.labels.size() ||
          (!data.prior_predictions.empty() && data.prior_predictions.size() != data.rows.size()))
      {
        throw DTree::Exception("numeric training requires a nonempty, labeled dataset");
      }

      for (std::size_t i = 0; i < data.rows.size(); ++i)
      {
        if (!std::isfinite(data.labels[i]) || data.labels[i] < 0 || data.labels[i] > 1)
        {
          throw DTree::Exception("numeric training targets must be finite and in [0, 1]");
        }

        if (!data.prior_predictions.empty() && !std::isfinite(data.prior_predictions[i]))
        {
          throw DTree::Exception("non-finite legacy prior prediction");
        }

        uint32_t previous = 0;
        for (const auto& feature : data.rows[i])
        {
          if (feature.first <= previous || std::isinf(feature.second))
          {
            throw DTree::Exception("numeric rows require sorted unique IDs and finite/NaN values");
          }
          previous = feature.first;
        }
      }
    }

    struct Feature
    {
      uint32_t id;
      std::vector<float> borders;
    };

    std::vector<Feature> quantize(const NumericDataset& data, unsigned long border_count)
    {
      struct Sample
      {
        std::vector<float> values;
        std::size_t present = 0;
        std::size_t finite = 0;
        bool nan = false;
        float minimum = std::numeric_limits<float>::max();
        float maximum = std::numeric_limits<float>::lowest();
      };
      std::map<uint32_t, Sample> samples;
      std::mt19937_64 rng(0);
      constexpr std::size_t capacity = 4096;
      for (const auto& row : data.rows)
      {
        for (const auto& feature : row)
        {
          auto& sample = samples[feature.first];
          ++sample.present;
          if (std::isnan(feature.second))
          {
            sample.nan = true;
            continue;
          }
          ++sample.finite;
          sample.minimum = std::min(sample.minimum, feature.second);
          sample.maximum = std::max(sample.maximum, feature.second);
          if (sample.values.size() < capacity)
          {
            sample.values.push_back(feature.second);
          }
          else
          {
            std::uniform_int_distribution<std::size_t> distribution(0, sample.finite - 1);
            const auto index = distribution(rng);
            if (index < capacity)
            {
              sample.values[index] = feature.second;
            }
          }
        }
      }
      std::vector<Feature> result;
      for (auto& entry : samples)
      {
        auto& sample = entry.second;
        auto& values = sample.values;
        if (sample.finite)
        {
          values.push_back(sample.minimum);
          values.push_back(sample.maximum);
        }

        if (sample.present < data.rows.size() || values.empty())
        {
          values.push_back(0);
        }
        std::sort(values.begin(), values.end());
        values.erase(std::unique(values.begin(), values.end()), values.end());
        Feature feature{entry.first, {}};
        const auto count = std::min<std::size_t>(border_count, values.size() - 1);
        for (std::size_t i = 0; i < count; ++i)
        {
          const auto index = count == values.size() - 1
            ? i
            : std::min(values.size() - 2, (i + 1) * values.size() / (count + 1));
          const float low = values[index], high = values[index + 1];
          float border = static_cast<float>((static_cast<double>(low) + high) * 0.5);
          // Adjacent float32 values may round their midpoint to the upper endpoint.
          if (!(border < high))
          {
            border = low;
          }
          feature.borders.push_back(border);
        }

        if (sample.nan)
        {
          feature.borders.push_back(values.back());
        }

        if (!feature.borders.empty())
        {
          result.push_back(std::move(feature));
        }
      }

      return result;
    }

    bool decision(const DTree::Branch& branch, const FloatFeatureArray& row)
    {
      const auto value = row.get(branch.feature_id);
      return branch.numeric
        ? (std::isnan(value.second) ? branch.nan_go_right : value.second > branch.border)
        : value.first;
    }

    struct Candidate
    {
      DTree* node = nullptr;
      DTree::Branch split{};
      double gain = 0;
      double yes = 0;
      double no = 0;
      bool constant = false;
      std::vector<std::size_t> rows;
    };

    bool better(const Candidate& candidate, const Candidate& best)
    {
      if (candidate.gain > best.gain + 1e-12)
      {
        return true;
      }

      return candidate.gain > 1e-12 && std::abs(candidate.gain - best.gain) <= 1e-12 &&
        std::tie(candidate.split.feature_id, candidate.split.border,
          candidate.split.nan_go_right) <
          std::tie(best.split.feature_id, best.split.border, best.split.nan_go_right);
    }

    struct Histogram
    {
      std::vector<Stats> bins;
      Stats nan;
      Stats present;
    };

    Candidate best_split(const NumericDataset& data, const std::vector<Feature>& features,
      const std::unordered_map<uint32_t, std::size_t>& indexes, const std::vector<Stats>& gradients,
      const std::vector<std::size_t>& rows, const NumericTreeLearner::Options& options,
      bool allow_split)
    {
      Stats total;
      std::map<std::size_t, Histogram> histograms;
      for (auto row : rows)
      {
        total = add(total, gradients[row]);
        if (!allow_split)
        {
          continue;
        }
        for (const auto& value : data.rows[row])
        {
          const auto found = indexes.find(value.first);
          if (found == indexes.end())
          {
            continue;
          }
          const auto index = found->second;
          auto& histogram = histograms[index];
          if (histogram.bins.empty())
          {
            histogram.bins.resize(features[index].borders.size() + 1);
          }
          histogram.present = add(histogram.present, gradients[row]);
          if (std::isnan(value.second))
          {
            histogram.nan = add(histogram.nan, gradients[row]);
          }
          else
          {
            const auto& borders = features[index].borders;
            const auto bin =
              std::lower_bound(borders.begin(), borders.end(), value.second) - borders.begin();
            histogram.bins[bin] = add(histogram.bins[bin], gradients[row]);
          }
        }
      }

      std::vector<std::size_t> active;
      for (auto& entry : histograms)
      {
        const auto& borders = features[entry.first].borders;
        const auto zero = std::lower_bound(borders.begin(), borders.end(), 0.0f) - borders.begin();
        auto& histogram = entry.second;
        histogram.bins[zero] = add(histogram.bins[zero], subtract(total, histogram.present));
        active.push_back(entry.first);
      }
      const auto score = [&](const Stats& stats)
      { return stats.gradient * stats.gradient / (std::max(stats.hessian, 0.0) + options.l2); };
      const auto weight = [&](const Stats& stats)
      {
        const double value = stats.gradient / (std::max(stats.hessian, 0.0) + options.l2);
        return options.learning_rate * std::clamp(value, -10.0, 10.0);
      };
      const auto workers = std::min<std::size_t>(options.threads, active.size());
      const auto scan = [&](std::size_t worker)
      {
        Candidate best;
        for (std::size_t i = worker; i < active.size(); i += workers)
        {
          const auto index = active[i];
          const auto& feature = features[index];
          const auto& histogram = histograms.at(index);
          Stats prefix;
          for (std::size_t border = 0; border < feature.borders.size(); ++border)
          {
            prefix = add(prefix, histogram.bins[border]);
            for (int nan_right = 0; nan_right < (histogram.nan.count ? 2 : 1); ++nan_right)
            {
              const Stats no = nan_right ? prefix : add(prefix, histogram.nan);
              const Stats yes = subtract(total, no);
              if (yes.count < options.min_leaf_size || no.count < options.min_leaf_size)
              {
                continue;
              }
              Candidate candidate;
              candidate.split.feature_id = feature.id;
              candidate.split.numeric = true;
              candidate.split.border = feature.borders[border];
              candidate.split.nan_go_right = nan_right;
              candidate.gain = 0.5 * (score(yes) + score(no));
              candidate.yes = weight(yes);
              candidate.no = weight(no);
              if (better(candidate, best))
              {
                best = std::move(candidate);
              }
            }
          }
        }
        return best;
      };

      Candidate best;
      best.constant = true;
      best.gain = 0.5 * score(total);
      best.yes = best.no = weight(total);
      std::vector<std::future<Candidate>> tasks;
      for (std::size_t i = 1; i < workers; ++i)
      {
        tasks.push_back(std::async(std::launch::async, scan, i));
      }

      if (workers)
      {
        auto candidate = scan(0);
        if (better(candidate, best))
        {
          best = std::move(candidate);
        }
      }

      for (auto& task : tasks)
      {
        auto candidate = task.get();
        if (better(candidate, best))
        {
          best = std::move(candidate);
        }
      }

      return best;
    }

    void find_candidate(Candidate& best, DTree* node, unsigned long depth,
      const std::vector<std::size_t>& rows, const NumericDataset& data,
      const std::vector<Feature>& features,
      const std::unordered_map<uint32_t, std::size_t>& indexes, const std::vector<Stats>& gradients,
      const NumericTreeLearner::Options& options)
    {
      if (!node || rows.empty())
      {
        return;
      }
      auto candidate = best_split(data, features, indexes, gradients, rows, options,
        depth < options.max_depth && rows.size() / 2 >= options.min_leaf_size);
      if (better(candidate, best))
      {
        candidate.node = node;
        candidate.rows = rows;
        best = std::move(candidate);
      }

      if (depth >= options.max_depth)
      {
        return;
      }

      for (auto& branch : node->branches_)
      {
        std::vector<std::size_t> yes, no;
        for (auto row : rows)
        {
          (decision(branch, data.rows[row]) ? yes : no).push_back(row);
        }
        find_candidate(best, branch.yes_tree, depth + 1, yes, data, features, indexes, gradients,
          options);
        find_candidate(best, branch.no_tree, depth + 1, no, data, features, indexes, gradients,
          options);
      }
    }

    unsigned long max_id(const DTree& tree)
    {
      unsigned long result = tree.tree_id;
      for (const auto& branch : tree.branches_)
      {
        if (branch.yes_tree)
        {
          result = std::max(result, max_id(*branch.yes_tree));
        }

        if (branch.no_tree)
        {
          result = std::max(result, max_id(*branch.no_tree));
        }
      }
      return result;
    }

    void update_model(Candidate& best, double factor, unsigned long& next_id)
    {
      if (best.constant)
      {
        best.node->delta_prob += factor * best.yes;
        return;
      }
      auto& branches = best.node->branches_;
      auto found = std::find_if(branches.begin(), branches.end(),
        [&](const auto& branch)
        {
          return branch.numeric && branch.feature_id == best.split.feature_id &&
            branch.border == best.split.border &&
            branch.nan_go_right == best.split.nan_go_right;
        });

      if (found == branches.end())
      {
        branches.push_back(best.split);
        found = branches.end() - 1;
      }

      for (auto* child : {&found->yes_tree, &found->no_tree})
      {
        if (!*child)
        {
          if (next_id == std::numeric_limits<unsigned long>::max())
          {
            throw DTree::Exception("numeric model node ID overflow");
          }
          *child = new DTree();
          (*child)->tree_id = ++next_id;
          (*child)->delta_prob = 0;
        }
      }
      found->yes_tree->delta_prob += factor * best.yes;
      found->no_tree->delta_prob += factor * best.no;
    }
  }

  NumericDataset
  NumericDataset::load(std::istream& input, const std::unordered_set<unsigned long>& features)
  {
    NumericDataset result;
    FloatFeatureArray row;
    double label, prior;
    while (load_float_features(input, row, label, &prior))
    {
      row.erase(
        std::remove_if(
          row.begin(),
          row.end(),
          [&](const auto& value)
          {
            return value.second == 0 || (!features.empty() && !features.count(value.first));
          }),
        row.end());

      for (const auto& value : row)
      {
        result.binary = result.binary && value.second == 1;
      }
      result.binary = result.binary && (label == 0 || label == 1);
      if (prior != 0 && result.prior_predictions.empty())
      {
        result.prior_predictions.resize(result.labels.size(), 0);
      }

      if (!result.prior_predictions.empty() || prior != 0)
      {
        result.prior_predictions.push_back(prior);
      }

      result.rows.push_back(std::move(row));
      result.labels.push_back(label);
    }

    validate(result);
    return result;
  }

  NumericTreeLearner::NumericTreeLearner(const Options& options) : options_(options)
  {
    if (!options.steps || !options.max_depth || options.max_depth > 64 || !options.border_count ||
        options.border_count > 1024 || !options.min_leaf_size || !options.threads ||
        options.threads > 256 || !std::isfinite(options.learning_rate) ||
        options.learning_rate <= 0 || !std::isfinite(options.l2) || options.l2 <= 0)
    {
      throw DTree::Exception("invalid numeric learner options (depth 1..64, borders 1..1024, "
        "threads 1..256, positive learning rate, leaf size and l2 required)");
    }
  }

  double NumericTreeLearner::logloss(const DTree& model, const NumericDataset& data)
  {
    validate(data);
    double total = 0;
    for (std::size_t i = 0; i < data.rows.size(); ++i)
    {
      const double raw = model.fpredict(data.rows[i]);
      if (!std::isfinite(raw))
      {
        throw DTree::Exception("non-finite numeric model prediction");
      }
      total += loss(raw, data.labels[i]);
    }

    return total / data.rows.size();
  }

  DTree_var
  NumericTreeLearner::prune(const DTree& model, const NumericDataset& data, double min_cover)
  {
    validate(data);
    if (!std::isfinite(min_cover) || min_cover < 0 || min_cover > 1)
    {
      throw DTree::Exception("min-cover must be finite and in [0, 1]");
    }
    auto result = model.copy();
    std::function<void(DTree*, const std::vector<std::size_t>&)> visit;

    visit = [&](DTree* node, const std::vector<std::size_t>& rows) {
      for (auto& branch : node->branches_)
      {
        std::vector<std::size_t> yes, no;
        for (auto row : rows)
        {
          (decision(branch, data.rows[row]) ? yes : no).push_back(row);
        }
        const auto filter = [&](DTree_var& child, const std::vector<std::size_t>& selected)
        {
          if (child)
          {
            if (selected.size() < min_cover * data.rows.size())
            {
              child = DTree_var();
            }
            else
            {
              visit(child, selected);
            }
          }
        };
        filter(branch.yes_tree, yes);
        filter(branch.no_tree, no);
      }
    };
    std::vector<std::size_t> rows(data.rows.size());
    std::iota(rows.begin(), rows.end(), 0);
    visit(result, rows);
    return result;
  }

  NumericTreeLearner::Result
  NumericTreeLearner::fit(const NumericDataset& train,
    const std::vector<NumericDataset>& validation, const DTree* initial, const Callback& callback)
    const
  {
    validate(train);
    if (!train.prior_predictions.empty())
    {
      throw DTree::Exception("numeric training requires an initial model via train-add, "
        "not per-row label,pred scores");
    }
    for (const auto& data : validation)
    {
      validate(data);
      if (!data.prior_predictions.empty())
      {
        throw DTree::Exception("numeric validation does not accept per-row label,pred scores");
      }
    }
    Result result;
    result.model = initial ? initial->copy() : DTree_var(new DTree());
    result.model->numeric_model_ = true;
    if (!initial)
    {
      const double mean =
        std::accumulate(train.labels.begin(), train.labels.end(), 0.0) / train.labels.size();
      const double prior = std::clamp(mean, 1e-6, 1.0 - 1e-6);
      result.model->tree_id = 1;
      result.model->delta_prob = std::log(prior / (1 - prior));
    }
    auto next_id = max_id(*result.model);
    const auto features = quantize(train, options_.border_count);
    std::unordered_map<uint32_t, std::size_t> indexes;
    for (std::size_t i = 0; i < features.size(); ++i)
    {
      indexes.emplace(features[i].id, i);
    }
    std::vector<double> predictions;
    for (const auto& row : train.rows)
    {
      predictions.push_back(result.model->fpredict(row));
      if (!std::isfinite(predictions.back()))
      {
        throw DTree::Exception("non-finite initial numeric model prediction");
      }
    }
    const auto& selection = validation.empty() ? train : validation.front();
    result.best_loss = logloss(*result.model, selection);
    result.best_model = result.model->copy();
    std::vector<std::size_t> rows(train.rows.size());
    std::iota(rows.begin(), rows.end(), 0);
    for (unsigned long step = 1; step <= options_.steps; ++step)
    {
      std::vector<Stats> gradients;
      gradients.reserve(rows.size());
      for (std::size_t i = 0; i < rows.size(); ++i)
      {
        const double exponent = std::exp(-std::abs(predictions[i]));
        const double probability =
          predictions[i] >= 0 ? 1 / (1 + exponent) : exponent / (1 + exponent);
        gradients.push_back({train.labels[i] - probability,
          std::max(probability * (1 - probability), 1e-12), 1});
      }
      Candidate best;
      find_candidate(best, result.model, 0, rows, train, features, indexes, gradients, options_);
      if (!best.node)
      {
        break;
      }
      double before = 0;
      for (auto row : best.rows)
      {
        before += loss(predictions[row], train.labels[row]);
      }
      double factor = 1;
      bool improved = false;
      for (int attempt = 0; attempt < 20; ++attempt)
      {
        double after = 0;
        for (auto row : best.rows)
        {
          const double delta =
            best.constant || decision(best.split, train.rows[row]) ? best.yes : best.no;
          after += loss(predictions[row] + factor * delta, train.labels[row]);
        }

        if (after < before - 1e-12)
        {
          improved = true;
          break;
        }
        factor *= 0.5;
      }

      if (!improved)
      {
        break;
      }
      update_model(best, factor, next_id);
      Progress progress{step, 0, {}};
      for (std::size_t row = 0; row < train.rows.size(); ++row)
      {
        predictions[row] = result.model->fpredict(train.rows[row]);
        if (!std::isfinite(predictions[row]))
        {
          throw DTree::Exception("non-finite numeric model prediction");
        }
        progress.train_logloss += loss(predictions[row], train.labels[row]);
      }
      progress.train_logloss /= train.rows.size();
      for (const auto& data : validation)
      {
        progress.validation_logloss.push_back(logloss(*result.model, data));
      }
      const double selected =
        validation.empty() ? progress.train_logloss : progress.validation_logloss.front();
      if (selected < result.best_loss)
      {
        result.best_loss = selected;
        result.best_model = result.model->copy();
      }
      result.steps = step;
      if (callback)
      {
        callback(progress, *result.model);
      }
    }

    return result;
  }
}
