#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <Gears/Threading/ActiveObjectCallback.hpp>
#include <DTree/DTree.hpp>
#include <DTree/FastFeatureSet.hpp>
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

  Vanga::DTree_var leaf(unsigned long id, double value)
  {
    Vanga::DTree_var tree = new Vanga::DTree();
    tree->tree_id = id;
    tree->delta_prob = value;
    return tree;
  }

  void features_test()
  {
    Vanga::FastFeatureSet fast;
    Vanga::FeatureArray features;
    features.push_back(std::make_pair(Vanga::FastFeatureSet::SIZE, 1));
    fast.set(features);
    require(fast.get(Vanga::FastFeatureSet::SIZE).first, "last fast feature missing");
    fast.rollback(features);
    require(fast.check(0), "fast feature rollback failed");
    fast.set(static_cast<unsigned long>(Vanga::FastFeatureSet::SIZE), uint32_t(1));
    fast.rollback(Vanga::FastFeatureSet::SIZE);
    fast.set(features.begin(), features.end());
    fast.rollback(features.begin(), features.end());
    require(fast.check(0), "iterator rollback failed");

    features.clear();
    features.push_back(std::make_pair(42, 0));
    require(!features.get(42).first, "zero-valued feature is active");
    fast.set(features);
    require(!fast.get(42).first, "zero-valued fast feature is active");

    std::istringstream input("1 42:0 43:1 44:-2\n");
    Vanga::BoolLabel label;
    auto row = Vanga::SVM<Vanga::BoolLabel>::load_line(input, label);
    require(row->features.size() == 1 && row->features.get(43).first,
      "SVM loader retained inactive features");
    std::istringstream unchecked_input("0 42:0 43:1\n");
    Vanga::SVM<Vanga::BoolLabel>::nocheck_load_line_(unchecked_input, label, features);
    require(features.size() == 1 && features[0].first == 43,
      "unchecked SVM loader retained inactive features");

    std::istringstream failed;
    failed.setstate(std::ios::failbit);
    bool rejected = false;
    try
    {
      Vanga::SVM<Vanga::BoolLabel>::load(failed);
    }
    catch (const Gears::Exception&)
    {
      rejected = true;
    }
    require(rejected, "failed SVM input was accepted");
  }

  void predictor_set_test()
  {
    auto tree = leaf(1, 0.25);
    Vanga::DTree::Branch branch;
    branch.feature_id = 42;
    branch.yes_tree = leaf(2, 1.0);
    branch.no_tree = leaf(3, -1.0);
    tree->branches_.push_back(branch);

    Vanga::PredictorSet_var inner = new Vanga::PredictorSet();
    inner->add(tree.in(), 0.5);
    inner->add(leaf(1, 2.0).in(), 0.25);
    Vanga::PredictorSet_var empty = new Vanga::PredictorSet();
    Vanga::PredictorSet_var nested = new Vanga::PredictorSet();
    nested->add(inner.in(), 0.5);
    nested->add(empty.in());
    Vanga::PredictorSet_var model = new Vanga::PredictorSet();
    model->add(nested.in(), 0.5);
    model->add(tree.in(), 0.25);
    model->add(inner.in(), 0.5);

    std::stringstream stream;
    model->save(stream);
    auto loaded = Vanga::PredictorSet::load(stream);
    Vanga::FeatureArray features;
    for (int i = 0; i != 2; ++i)
    {
      require(std::fabs(model->fpredict(features) - loaded->fpredict(features)) < 1e-6,
        "nested ensemble prediction changed after save/load");
      features.push_back(std::make_pair(42, 1));
    }

    std::stringstream empty_stream;
    empty->save(empty_stream);
    auto loaded_empty = Vanga::PredictorSet::load(empty_stream);
    require(loaded_empty->fpredict(features) == 0.0, "empty ensemble failed to load");

    std::istringstream invalid("union-sum\ninvalid\n");
    bool rejected = false;
    try
    {
      Vanga::PredictorSet::load(invalid);
    }
    catch (const Gears::Exception&)
    {
      rejected = true;
    }
    require(rejected, "invalid ensemble coefficient was accepted");
  }
}

int main()
{
  try
  {
    features_test();
    predictor_set_test();
    std::ostringstream output;
    Gears::ActiveObjectCallback_var callback = new Gears::
      ActiveObjectCallbackStreamImpl(output, "test");
    callback->report_error(Gears::ActiveObjectCallback::ERROR, Gears::SubString("test message"));
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
  return 0;
}
