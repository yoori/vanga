#include <array>
#include <sstream>

#include <DTree/DTree.hpp>
#include <DTree/FloatFeatures.hpp>
#include <DTree/NumericTreeLearner.hpp>

static_assert(__cplusplus > 201703L, "Vanga::DTree must enable the project's C++20 standard");

int main()
{
  std::istringstream input("dtree-float\n1\t0.25\t7:2:3:1.5:0\n2\t0.5\t\n3\t-0.5\t\n");
  auto tree = Vanga::DTree::load(input);
  std::array<float, 2> values{0.0f, 2.0f};
  std::array<double, 2> expected{-0.25, 0.75};
  std::size_t index = 0;
  for (float value : values)
  {
    Vanga::FloatFeatureArray features;
    features.emplace_back(7, value);
    if (tree->fpredict(features) != expected[index++])
    {
      return 1;
    }
  }
  std::istringstream training("0 7:0.1\n1 7:0.9\n0 7:0.1\n1 7:0.9\n");
  auto data = Vanga::NumericDataset::load(training);
  Vanga::NumericTreeLearner::Options options;
  options.steps = 3;
  auto result = Vanga::NumericTreeLearner(options).fit(data);
  if (!result.model->uses_numeric_features() ||
      result.model->fpredict(data.rows[0]) >= result.model->fpredict(data.rows[1]))
  {
    return 2;
  }
  return 0;
}
