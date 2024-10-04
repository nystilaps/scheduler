// #include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdexcept>

#include "action.h"
#include "heft.h"
#include "input.h"

using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::HasSubstr;
using ::testing::Pair;
using ::testing::ThrowsMessage;
using ::testing::UnorderedElementsAre;
using ::testing::UnorderedElementsAreArray;

TEST(InputTests, BadFilePath) {
  EXPECT_THAT([]() { builder::load_actions("/asdfasdf/asdfasdf/test.txt"); },
              ThrowsMessage<std::runtime_error>(HasSubstr("does not exist")));
}

TEST(InputTests, BadDuration1) {
  std::string testInput = R"(a 10000100101010101010101)";
  std::stringstream testStream(testInput);
  EXPECT_THAT(
      [&]() { builder::load_actions(testStream); },
      ThrowsMessage<std::runtime_error>(HasSubstr("Duration of action")));
}

TEST(InputTests, BadDuration2) {
  std::string testInput = R"(a -1)";
  std::stringstream testStream(testInput);
  EXPECT_THAT(
      [&]() { builder::load_actions(testStream); },
      ThrowsMessage<std::runtime_error>(HasSubstr("Input file format error")));
}

TEST(InputTests, FormatErrorNoDuration) {
  std::string testInput = R"(a )";
  std::stringstream testStream(testInput);
  EXPECT_THAT(
      [&]() { builder::load_actions(testStream); },
      ThrowsMessage<std::runtime_error>(HasSubstr("Input file format error")));
}

TEST(InputTests, UndeclaredDependency) {
  std::string testInput = R"(a 1 1)";
  std::stringstream testStream(testInput);

  EXPECT_THAT(
      [&]() { builder::load_actions(testStream); },
      ThrowsMessage<std::runtime_error>(HasSubstr("Dependency of target")));
}

TEST(InputTests, DoubleDefinitions) {
  std::string testInput = R"(
                                a 1
                                a 1)";
  std::stringstream testStream(testInput);

  EXPECT_THAT([&]() { builder::load_actions(testStream); },
              ThrowsMessage<std::runtime_error>(
                  HasSubstr("is already defined, must be defined only once")));
}

TEST(ScheduleTests, BasicChecks) {
  std::string testInput = R"( 
                                a 1
                                b 1  a
                                c 1   a  b)";
  std::stringstream testStream(testInput);
  auto actions = builder::load_actions(testStream);

  builder::calculateRanks(actions);
  const auto rankShas = computeRankShas(actions);
  schedule(2, rankShas, actions);

  const auto execPlan = getExecutionPlan(actions);
  const auto criticalPath = getCriticalPath(actions);

  EXPECT_EQ(actions.size(), (3 + 2));
  EXPECT_EQ(actions.count("a"), 1);
  EXPECT_EQ(actions.count("b"), 1);
  EXPECT_EQ(actions.count("c"), 1);
  EXPECT_EQ(actions.count(builder::Start.sha1), 1);
  EXPECT_EQ(actions.count(builder::End.sha1), 1);

  EXPECT_EQ(actions.at(builder::Start.sha1).sha1, "$tart");
  EXPECT_EQ(actions.at(builder::Start.sha1).duration, 1);
  EXPECT_EQ(actions.at(builder::Start.sha1).dependencies.size(), 0);

  EXPECT_EQ(actions.at(builder::End.sha1).sha1, "#nd");
  EXPECT_EQ(actions.at(builder::End.sha1).duration, 1);
  EXPECT_EQ(actions.at(builder::End.sha1).dependencies.size(), 1);

  EXPECT_EQ(actions.at("a").sha1, "a");
  EXPECT_EQ(actions.at("a").duration, 1);
  EXPECT_THAT(actions.at("a").dependencies,
              UnorderedElementsAre(builder::Start.sha1));
  EXPECT_EQ(actions.at("a").startTime, 0);
  EXPECT_EQ(actions.at("a").endTime, 1);

  EXPECT_EQ(actions.at("b").sha1, "b");
  EXPECT_EQ(actions.at("b").duration, 1);
  EXPECT_THAT(actions.at("b").dependencies, UnorderedElementsAre("a"));
  EXPECT_EQ(actions.at("b").startTime, 1);
  EXPECT_EQ(actions.at("b").endTime, 2);

  EXPECT_EQ(actions.at("c").sha1, "c");
  EXPECT_EQ(actions.at("c").duration, 1);
  EXPECT_EQ(actions.at("c").dependencies.size(), 2);
  EXPECT_THAT(actions.at("c").dependencies, UnorderedElementsAre("a", "b"));
  EXPECT_EQ(actions.at("c").startTime, 2);
  EXPECT_EQ(actions.at("c").endTime, 3);

  EXPECT_EQ(criticalPath.infiniteExecutorsLength, 3);
  EXPECT_EQ(criticalPath.actualExecutorsLength, 3);
}

class ScheduleTests2 : public ::testing::TestWithParam<int32_t> {
protected:
  virtual void SetUp() {
    // check that concurency influences the scheduling of Actions DAG correctly
    concurrency = GetParam();
  }

  void parseAndSchedule(std::string testInput) {
    std::stringstream testStream(testInput);
    actions = builder::load_actions(testStream);

    builder::calculateRanks(actions);
    rankShas = computeRankShas(actions);
    schedule(concurrency, rankShas, actions);

    execPlan = getExecutionPlan(actions);
    criticalPath = getCriticalPath(actions);
  }

  virtual void TearDown() {}

  int32_t concurrency; ///< Number of executors to test scheduling on
  builder::Actions actions;
  builder::RankShas rankShas;
  builder::CriticalPath criticalPath;
  builder::ExecutionPlan execPlan;

  const int32_t actionsNum{2 * 3 * 4 * 5 *
                           100}; ///< A (relatively big) number of actions,
                                 ///< it is multiple of tested concurrency
                                 ///< values below in INSTANTIATE_TEST_SUITE_P
};

TEST_P(ScheduleTests2, BasicScheduleChecks) {
  std::string testInput = R"( 
    a 1
    b 1  a
    c 1  b)";

  parseAndSchedule(testInput);

  EXPECT_THAT(rankShas, ElementsAre(Pair(4, "a"), Pair(3, "b"), Pair(2, "c")));
  EXPECT_THAT(execPlan, ElementsAre(Pair(0, "a"), Pair(1, "b"), Pair(2, "c")));

  EXPECT_EQ(criticalPath.infiniteExecutorsLength, 3);
  EXPECT_EQ(criticalPath.actualExecutorsLength, 3);
  EXPECT_THAT(criticalPath.actionsShas, ElementsAre("a", "b", "c"));
}

TEST_P(ScheduleTests2, AbsolutelyConcurrentScheduleChecksForEqualDurations) {
  std::string testInput = R"(
    a 1
    b 1  
    c 1  )";
  parseAndSchedule(testInput);

  EXPECT_THAT(rankShas,
              UnorderedElementsAre(Pair(2, "a"), Pair(2, "b"), Pair(2, "c")));
  EXPECT_EQ(criticalPath.infiniteExecutorsLength, 1);
  // For lower concurrency one can't say which action is sceduled first, hence
  // no expectation
  if (concurrency >= 3) {
    EXPECT_EQ(criticalPath.actualExecutorsLength, 1);
  }
}

TEST_P(ScheduleTests2, AbsoluteConcurrentScheduleChecksForDifferentDurations) {
  std::string testInput = R"(
    a 1
    b 1  
    c 2  )";
  parseAndSchedule(testInput);

  const std::vector<builder::SHA> expecedCriticalPath{"c"};
  int32_t expectedCriticalPathLength =
      actions.at("c").duration; // length of longes action 'c'

  EXPECT_THAT(rankShas,
              UnorderedElementsAre(Pair(2, "a"), Pair(2, "b"), Pair(3, "c")));
  EXPECT_EQ(criticalPath.infiniteExecutorsLength, expectedCriticalPathLength);
  // For lower concurrency one can't say which action is sceduled first, hence
  // no expectation
  if (concurrency >= 2) {
    EXPECT_EQ(criticalPath.actualExecutorsLength, expectedCriticalPathLength);
  }
  EXPECT_THAT(criticalPath.actionsShas, ElementsAreArray(expecedCriticalPath));
}

TEST_P(ScheduleTests2, PartialConcurrentScheduleChecks) {
  std::string testInput = R"( 
    a 1
    b 1  
    c 1  a)";
  parseAndSchedule(testInput);

  const std::vector<builder::SHA> expecedCriticalPath{"a", "c"};
  EXPECT_THAT(rankShas,
              UnorderedElementsAre(Pair(3, "a"), Pair(2, "b"), Pair(2, "c")));
  EXPECT_EQ(criticalPath.infiniteExecutorsLength, expecedCriticalPath.size());

  // For lower concurrency one can't say which action is sceduled first, hence
  // no expectation
  if (concurrency >= 2) {
    EXPECT_EQ(criticalPath.actualExecutorsLength, expecedCriticalPath.size());
  }
  EXPECT_THAT(criticalPath.actionsShas, ElementsAreArray(expecedCriticalPath));
}

TEST_P(ScheduleTests2, PartialConcurrentScheduleChecksComplexDAG) {
  std::string testInput = R"( 
    a 1
    b 1  
    c 1  a
    d 1  c
    e 1  b)";

  parseAndSchedule(testInput);

  const std::vector<builder::SHA> expecedCriticalPath{"a", "c", "d"};
  EXPECT_EQ(criticalPath.infiniteExecutorsLength, expecedCriticalPath.size());
  EXPECT_THAT(rankShas,
              UnorderedElementsAre(Pair(4, "a"), Pair(3, "b"), Pair(3, "c"),
                                   Pair(2, "d"), Pair(2, "e")));

  // For lower concurrency one can't say which action is sceduled first, hence
  // no expectation
  if (concurrency >= 2) {
    EXPECT_EQ(criticalPath.actualExecutorsLength, expecedCriticalPath.size());
  }
  EXPECT_THAT(criticalPath.actionsShas,
              testing::ElementsAreArray(expecedCriticalPath));
}

TEST_P(ScheduleTests2, HugeTrivialDAG) {
  std::string testInput = "";
  std::vector<builder::SHA> expecedCriticalPath{};

  for (auto i = actionsNum; i > 0; i--) {
    testInput += "\n" + std::to_string(i) + " 1 ";
    if (i < actionsNum) {
      // add dependency on the previous task
      testInput += std::to_string(i + 1);
    }
    expecedCriticalPath.push_back(std::to_string(i));
  }
  parseAndSchedule(testInput);

  EXPECT_EQ(criticalPath.infiniteExecutorsLength, expecedCriticalPath.size());
  EXPECT_EQ(criticalPath.actualExecutorsLength, expecedCriticalPath.size());
  EXPECT_THAT(criticalPath.actionsShas,
              testing::ElementsAreArray(expecedCriticalPath));
}

TEST_P(ScheduleTests2, HugeConcurrentDAG) {
  std::string testInput = "";
  std::vector<builder::SHA> expecedCriticalPath{};

  for (auto i = actionsNum; i > 0; i--) {
    testInput += "\n" + std::to_string(i) + " 1 ";
    // note No dependeny of actions on one another
    expecedCriticalPath.push_back(std::to_string(i));
  }
  parseAndSchedule(testInput);

  EXPECT_EQ(criticalPath.infiniteExecutorsLength, 1);
  EXPECT_EQ(execPlan.back().first + 1, actionsNum / concurrency);
}

INSTANTIATE_TEST_SUITE_P(InstantiationName, ScheduleTests2,
                         ::testing::Values(1, 2, 3, 4, 5));