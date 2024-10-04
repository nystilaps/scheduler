#pragma once

#include <cstdint>
#include <limits>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace builder {

using Dependencies = std::unordered_set<std::string>;
using SHA = std::string;
using Duration = int32_t;
using Time = int64_t;
using Id = int32_t;

/// @brief Describes parameters of actions t obe scheduled
struct Action {
  SHA sha1;                  ///< SHA code of action
  int32_t duration{0};       ///< duration of action
  Dependencies dependencies; ///< List of dependencies

  // Heterogenious Earliest-Finish-Time (HEFT) parameters
  Time rank{
      0}; ///< HEFT rank for ordering used for actions execution scheduling
  Time startTime{0}; ///< HEFT scheduled start time of action
  Time endTime{0};   ///< HEFT scheduled finish time of action
  Id executorId{-1}; ///< HEFT Id of executor on which this action is scheduled

  // Critical path parameters
  SHA predecessor{
      ""}; ///< Critical path sha of the following node with longest path
  Time longestPath{
      0}; ///< Critical path sha of the following node with longest path
};

using Actions = std::unordered_map<std::string, Action>;

// Impossible names and actions for specific single start and end targets
const Duration phonyDuration{1};                ///< Duration of phony actions
const Action Start{"$tart", phonyDuration, {}}; ///< Virtual start action
const Action End{"#nd", phonyDuration, {}};     ///< Virtual end action

/// @brief Output operator for Action
/// @param os stream to output to
/// @param a action to output
/// @return referemce to os stream
inline std::ostream &operator<<(std::ostream &os, const builder::Action &a) {
  os << "Action(sha=" << a.sha1 << ",\tduration=" << a.duration << ",\tdeps: [";
  std::string comma = "";
  for (auto &s : a.dependencies) {
    os << comma << s;
    comma = ",";
  }
  os << "],\trank=" << a.rank << ",\tstartTime=" << a.startTime
     << ",\tendTime=" << a.endTime << ",\texecutorId=" << a.executorId
     << ",\tpredecessor=" << a.predecessor
     << ",\tlongestPath=" << a.longestPath;
  return os << ")";
}

} // namespace builder