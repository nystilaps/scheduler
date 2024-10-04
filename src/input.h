#pragma once

#include "action.h"

#include <filesystem>
#include <unordered_map>

namespace builder {

/// @brief Loads actions data from given input stream
/// @param fi input stream to lasd data from
/// @return map from Action.sha to Action object
Actions load_actions(std::istream &fi);

/// @brief Loads actions data from given input file.
/// @param file Path to file to load
/// @return map from Action.sha to Action object
Actions load_actions(std::filesystem::path file);

} // namespace builder