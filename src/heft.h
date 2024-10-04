#pragma once

#include "action.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

namespace builder {

/// @brief For every node, recursively find the HEFT upper rank
/// @param actions [in, out] Map of sha to Action, which is updated by this
/// function
void calculateRanks(Actions &actions);

using RankShas = std::vector<std::pair<builder::Time, SHA>>;

/// @brief Create vector of shas and ranks and sort it in non-increasing order
/// of HEFT ranks
/// @param actions [in] list of action items
/// @return vector of pair<rank, sha> sorted in non-derceasing order
RankShas computeRankShas(const Actions &actions);

/// @brief Simplified HEFT algorithms for tasks planning
/// @param numberOfExecutors [in] number of identical executors to plan
/// execution on
/// @param rankShas [in] vector of pair<rank, sha> in non-decreasing order to
/// orders sha's to execute on
/// @param actions [in, out] map of sha to Action
void schedule(Id numberOfExecutors, const RankShas &rankShas, Actions &actions);

using ExecutionPlan = std::vector<std::pair<Time, SHA>>;

/// @brief Get Execution Plan from actions after
/// schedule() function was called on it
/// @param actions map of SHA to Action
/// @return vecvtor or pair<Time, SHA> of scheduled execution time of given
/// action SHA sorted by non-decreasing time
ExecutionPlan getExecutionPlan(const Actions &actions);

struct CriticalPath {
  Time infiniteExecutorsLength{
      0}; ///< execution time in case of infinite executors
  Time actualExecutorsLength{0};  ///< planned execution time length
  std::vector<SHA> actionsShas{}; ///< actions in order of execution
};

/// @brief Get Critical Path from actions after schedule() function was called
/// on it
/// @param actions
/// @return
CriticalPath getCriticalPath(const Actions &actions);

} // namespace builder