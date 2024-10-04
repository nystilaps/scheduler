#include "heft.h"
#include "input.h"

#include <boost/program_options.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

namespace po = boost::program_options;

std::string helpMessage = R"(
This is action planning program according to a given action dependency graph 
  direct acyclic graph (DAG). 

Plan execution of actions according to the Heterogenious Earliest-Finish-Time HEFT algorithm
  (www.scribd.com/document/261991586/2002-Topcuoglu-TPDS).
Outputs:
  Scheduled execution time of each task SHA.
  Critical path langth and order of actions SHAs for it.

Expected input file format is each line defining action duration and its dependencies like:
  action_sha duration
or
  action_sha duration dependency_sha1 dependency_sha2 ...
Actions must be defined before mentioned as a dependency,
hence no circular dependency is not possible to define (and cicrular deps are not supported).
Empty lines with only whitespaces are allowed and discarded.

Schedule output file format:
  sha scheduledTime

Allowed options: )";

/// @brief Output scheduled execution plan to a given file
/// @param executionPlan vector of pair<startTime, SHA> sorted in non-decreasing
/// order of startTime
/// @param scheduledExecutionPlanOutputPath path to file to output results to
void outputScheduledExecutionPlanToGivenPath(
    const builder::ExecutionPlan &executionPlan,
    std::string &scheduledExecutionPlanOutputPath);

/// @brief Output critical path to stdout
/// @param criticalPath execution times of critical path and SHAs of actions in
/// order of execution
void outputCriticalPath(const builder::CriticalPath &criticalPath);

int main(int argc, char *argv[]) try {
  int32_t concurrency{10};
  std::string inputPath{""};
  std::string scheduledExecutionPlanOutputPath{""};
  bool doOutputCriticalPath{false};

  po::options_description desc(helpMessage);
  desc.add_options()("help,h", "produce this help message")(
      "input,i", po::value<std::string>(&inputPath)->default_value(""),
      "input file path")("concurrency,c",
                         po::value<int32_t>(&concurrency)->default_value(10),
                         "number of executors")(
      "critical-path,p",
      po::bool_switch(&doOutputCriticalPath)->default_value(false),
      "output critical path")(
      "output,o",
      po::value<std::string>(&scheduledExecutionPlanOutputPath)
          ->default_value(""),
      "output full schedule to a given path");
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return 0;
  }
  if (concurrency < 1) {
    std::cout << "Need concurrency value more oe equal to one, otherwise input "
                 "is useless."
              << std::endl;
    return 0;
  }
  if (inputPath.empty()) {
    std::cout << "Need input file to operate on, please read the parameter "
                 "description below:"
              << std::endl;
    std::cout << desc << "\n";
    return 0;
  }

  std::cout << "Run parameters: " << std::endl;
  std::cout << "  actions input file path: '" << inputPath << "'" << std::endl;
  std::cout << "  concurrency (numer of executors to schedule execution on): "
            << concurrency << std::endl;
  std::cout << "  scheduled execution plan output file path: '"
            << scheduledExecutionPlanOutputPath << "'" << std::endl;
  std::cout << "  do output critical path: " << std::boolalpha
            << doOutputCriticalPath << std::endl;
  std::cout << std::endl;

  const bool doOutputExecutionPlan = scheduledExecutionPlanOutputPath.length();

  if (doOutputCriticalPath || doOutputExecutionPlan) {
    std::cout << "Reading input file: '" << inputPath << "'" << std::endl;

    auto actions = builder::load_actions(inputPath);
    builder::calculateRanks(actions);
    const auto rankShas = computeRankShas(actions);
    schedule(2, rankShas, actions);

    if (doOutputExecutionPlan) {
      outputScheduledExecutionPlanToGivenPath(getExecutionPlan(actions),
                                              scheduledExecutionPlanOutputPath);
    } else {
      std::cout << "Scheduled execution plan not requested." << std::endl;
    }
    if (doOutputCriticalPath) {
      outputCriticalPath(getCriticalPath(actions));
    } else {
      std::cout << "Critical path output not requested." << std::endl;
    }
  } else {
    std::cout << "No output requested, exiting." << std::endl;
  }
  return 0;
} catch (std::exception &e) {
  std::cerr << "Error happened: " << e.what() << std::endl;
} catch (...) {
  std::cerr << "Unknown error happened: " << std::endl;
}

//--- Output implementetion below ---

void outputScheduledExecutionPlanToGivenPath(
    const builder::ExecutionPlan &executionPlan,
    std::string &scheduledExecutionPlanOutputPath) {
  std::cout << std::endl;
  std::cout << "Outputting execution plan to this file path: "
            << scheduledExecutionPlanOutputPath << std::endl;
  std::ofstream of(scheduledExecutionPlanOutputPath,
                   std::ofstream::out | std::ofstream::trunc);
  if (!of) {
    throw std::runtime_error("Couldn't open output file '" +
                             scheduledExecutionPlanOutputPath + "'");
  }
  for (auto &[startTime, sha] : executionPlan) {
    // const auto &action = actions.at(sha);
    of << sha << "\t" << startTime << std::endl;
    if (!of) {
      throw std::runtime_error("Error outputting execution plan to '" +
                               scheduledExecutionPlanOutputPath + "'");
    }
  }
  std::cout << "Output of execution plan finished." << std::endl;
  std::cout << std::endl;
}

void outputCriticalPath(const builder::CriticalPath &criticalPath) {
  std::cout << std::endl;
  std::cout << "Infinite executors Critical Path length = "
            << criticalPath.infiniteExecutorsLength << std::endl;
  std::cout << "Real scheduled Critical Path finish time = "
            << criticalPath.actualExecutorsLength << std::endl;
  std::cout << "Critical path of actions, SHAs in order of execution:"
            << std::endl;
  for (auto &sha : criticalPath.actionsShas) {
    std::cout << "  " << sha << std::endl;
  }
  std::cout << "End of critical path." << std::endl;
  std::cout << std::endl;
}