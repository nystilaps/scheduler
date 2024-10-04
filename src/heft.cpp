#include "heft.h"
#include "action.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

namespace builder {

void calculateRanks(Actions &actions) {
  // Dijkstra's wave set, start with end node
  std::unordered_set<SHA> activeNodesSha{End.sha1};
  std::unordered_set<SHA> nextActiveNodesSha{};

  // By HEFT algorithm set rank of end node to its duration
  // This will make all ranks +End.duration, but ordering will be the same
  actions.at(End.sha1).rank += actions.at(End.sha1).duration;

  while (!activeNodesSha.empty()) {
    for (auto &nodeSha : activeNodesSha) {
      auto &node{actions.at(nodeSha)};
      for (auto &dependencySha : node.dependencies) {
        // Rank calculation
        nextActiveNodesSha.insert(dependencySha);
        auto &dependencyNode{actions.at(dependencySha)};
        dependencyNode.rank =
            std::max(dependencyNode.rank, node.rank + dependencyNode.duration);

        // Separate critical path calculation
        const Time newLongestPath = node.longestPath + dependencyNode.duration;
        if (dependencyNode.longestPath < newLongestPath) {
          dependencyNode.longestPath = newLongestPath;
          dependencyNode.predecessor = node.sha1;
        }
      }
    }
    std::swap(nextActiveNodesSha, activeNodesSha);
    nextActiveNodesSha.clear();
  }
}

RankShas computeRankShas(const Actions &actions) {
  RankShas rankShas;
  for (auto &[sha, action] : actions) {
    if (sha != builder::Start.sha1 && sha != builder::End.sha1) {
      rankShas.emplace_back(action.rank, sha);
    }
  }
  // Sort pair<rank, sha> in non-decreasing order to take highest rank first for
  // scheduling
  std::sort(rankShas.begin(), rankShas.end(), std::greater<>());
  return rankShas;
}

void schedule(Id numberOfExecutors, const RankShas &rankShas,
              Actions &actions) {
  // Create set of pairs of availableTime and executorId
  std::set<std::pair<Time, Id>> availableTimeExecutor;
  for (Id id = 0; id < numberOfExecutors; ++id) {
    availableTimeExecutor.emplace(0, id);
  }

  for (auto &[_, sha] : rankShas) {
    auto &action = actions.at(sha);
    // Select the soonest execution time based on executor availability and
    // dependecies finish times

    // Find executor that gets free first and extract it from the executors set
    auto soonestExecutionTimeAndExecutor =
        availableTimeExecutor.extract(availableTimeExecutor.begin()).value();

    // Check if we need to postpone execution till last dependecy is finished
    Time soonestExecutionTime = soonestExecutionTimeAndExecutor.first;
    for (auto &dependencySha : action.dependencies) {
      auto &dependencyAction = actions.at(dependencySha);
      soonestExecutionTime =
          std::max(dependencyAction.endTime, soonestExecutionTime);
    }

    // Write executor and start/finish times to action
    action.startTime = soonestExecutionTime;
    action.endTime = soonestExecutionTime + action.duration;
    action.executorId = soonestExecutionTimeAndExecutor.second;

    // Return the executor to the executors set with updated available time
    soonestExecutionTimeAndExecutor.first = action.endTime;
    availableTimeExecutor.insert(soonestExecutionTimeAndExecutor);
  }
}

ExecutionPlan getExecutionPlan(const Actions &actions) {
  ExecutionPlan plan;
  for (auto &[sha, action] : actions) {
    if (sha != builder::Start.sha1 && sha != builder::End.sha1) {
      plan.emplace_back(action.startTime, action.sha1);
    }
  }
  std::sort(plan.begin(), plan.end());
  return plan;
}

CriticalPath getCriticalPath(const Actions &actions) {
  CriticalPath path;
  // Start with the phony Start action and follow the predecessor
  // till the phony End action
  SHA currentActionSha = Start.sha1;
  const auto &startAction = actions.at(currentActionSha);
  while (currentActionSha != End.sha1) {
    if (currentActionSha != Start.sha1) {
      path.actionsShas.push_back(currentActionSha);
    }
    auto &action = actions.at(currentActionSha);
    path.infiniteExecutorsLength += action.duration;
    currentActionSha = action.predecessor;
  }
  // Start action is phony, so its execution time should be subtracted
  path.infiniteExecutorsLength -= startAction.duration;
  path.actualExecutorsLength = actions.at(path.actionsShas.back()).endTime;
  return path;
}

} // namespace builder