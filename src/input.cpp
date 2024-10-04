#include "input.h"

#include <fstream>
#include <regex>
#include <sstream>

#include <iostream>

namespace builder {

Actions load_actions(std::istream &fi) {
  std::unordered_map<std::string, Action> actions;
  // Line in the format:
  // sha1 duration [dependency1 dependency2...]
  // sha 123 sha1   sha2 sha3
  static std::regex r("\\s*(\\w+)\\s+(\\d+)((\\s+\\w+)*)\\s*");
  static std::regex emptyLineRegex("\\s*");

  // Dependencies of start node are nodes with no real dependencies
  auto start = Start;
  // Dependencies of end node are nodes with no real dependencies
  auto end = End;

  std::unordered_set<std::string> nodesWithNoDependencies;
  std::unordered_set<std::string> nodesWhichNoNodeDependsOn;

  std::string s{};
  while (std::getline(fi, s)) {
    if (std::regex_match(s, emptyLineRegex)) {
      continue;
    }

    std::smatch sm;
    if (std::regex_match(s, sm, r)) {
      const SHA sha = sm[1];
      const std::string durationStr = sm[2];
      const int duration{std::atoi(durationStr.c_str())};

      if (duration <= 0) {
        throw std::runtime_error(
            "Duration of action " + sha + " is negative or zero " +
            std::to_string(duration) +
            ", must be positive parsed from string: " + durationStr);
      }
      if (durationStr != std::to_string(duration)) {
        throw std::runtime_error(
            "Duration of action " + sha + " was incorrectly parsed + " +
            std::to_string(duration) + ", parsed from string: " + durationStr);
      }
      if (actions.count(sha)) {
        throw std::runtime_error(
            "Action " + sha +
            " is already defined, must be defined only once.");
      }

      builder::Dependencies dependencies;
      std::stringstream dependenciesStream(sm[3]);
      std::string dep;
      while (dependenciesStream >> dep) {
        if (actions.count(dep) == 0) {
          throw std::runtime_error("Dependency of target " + sha + " called: " +
                                   dep + " must be declared before use.");
        }
        nodesWhichNoNodeDependsOn.erase(dep);
        dependencies.insert(std::move(dep));
      }

      // Make nodes virtually dependent on the single start node
      if (dependencies.empty()) {
        dependencies.insert(start.sha1);
      }
      actions[sha] = Action{sha, duration, dependencies};
      nodesWhichNoNodeDependsOn.insert(sha);
    } else {
      throw std::runtime_error(
          "Input file format error, faulty input line = '" + s + "'");
    }
  }
  // Algorithm fails on empty input, and it is easier to fail here
  if (actions.empty()) {
    throw std::runtime_error(
        "There must be at least one action to schedule, got zero actions.");
  }
  end.dependencies.insert(nodesWhichNoNodeDependsOn.begin(),
                          nodesWhichNoNodeDependsOn.end());
  actions[start.sha1] = std::move(start);
  actions[end.sha1] = std::move(end);
  return actions;
}

Actions load_actions(std::filesystem::path file) try {
  if (!std::filesystem::exists(file)) {
    throw std::runtime_error("File '" + file.string() + "' does not exist.");
  }
  std::ifstream fi(file);
  return load_actions(fi);
} catch (std::exception &e) {
  throw std::runtime_error("Error during reading file '" + file.string() +
                           "'. " + e.what());
}

} // namespace builder