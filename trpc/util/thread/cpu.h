//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/internal/cpu.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <cinttypes>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace trpc {

namespace numa {

/// @brief NumaNode
struct Node {
  unsigned id;                         ///< node id
  std::vector<unsigned> logical_cpus;  ///< CPU logical groups.
};

/// @brief Get available nodes.
/// @return A vector array of available nodes.
std::vector<Node> GetAvailableNodes();

/// @brief A vector type that contains information about available nodes.
/// @param [out] cpu cpu id of current processor
/// @param[ out] node node id of current processor
void GetCurrentProcessor(unsigned& cpu, unsigned& node);

/// @brief Get the node ID of the current thread.
/// @return Numa node id
/// @note that node IDs are NOT necessarily contiguous. If you want need index of current node in vector returned
///       by `GetAvailableNodes()`. call `GetNodeIndexOf` or `GetCurrentNodeIndex()` instead.
unsigned GetCurrentNode();

/// @brief It's an impossible node index, so it is safe. Anyway, i think int(-1) is better.
constexpr std::size_t kUnexpectedNodeIndex = std::numeric_limits<std::size_t>::max();

/// @brief Get the node ID by index.
/// @param index The index of node
/// @return Node ID of type int.
int GetNodeId(std::size_t index);

/// @brief Get the index of the current node.
/// @return Node index of type std::size_t.
std::size_t GetCurrentNodeIndex();

/// @brief Get the corresponding node index from a specified node ID and CPU ID address.
/// @param node_id Node id
/// @param [out] cpu Address where the CPU ID is stored.
/// @return Node index of type std::size_t.
std::size_t GetNodeIndexOf(unsigned node_id, unsigned* cpu = nullptr);

/// @brief Get the number of available nodes.
/// @return Available nodes of type std::size_t..
std::size_t GetNumberOfNodesAvailable();

/// @brief Get the NUMA node ID of a specified CPU.
/// @return An integer type, where a value >= 0: the NUMA node ID; -1: the CPU is not available.
int GetNodeOfProcessor(unsigned cpu);

}  // namespace numa

/// @brief Get the current CPU ID.
/// @return An unsigned type representing the CPU ID.
/// @note FIXME: Naming is not consistent here, replace `CPU`s below with `Processor`.
unsigned GetCurrentProcessorId();

/// @brief Get the number of available CPUs for the program.
/// @return An std::size_t type representing the available CPU ID.
/// @note CAUTION: If some processors are disabled, we may find processors whose ID is greater than number of
///                processors.
std::size_t GetNumberOfProcessorsAvailable();

/// @brief Get the number of CPUs on the host machine.
/// @return An std::size_t type representing the number of CPUs on the host machine.
std::size_t GetNumberOfProcessorsConfigured();

/// @brief Check if there are any unavailable CPUs.
/// @return A bool type, true if there are unavailable CPUs, false otherwise.
bool IsInaccessibleProcessorPresent();

/// @brief Check if a specified CPU is available.
/// @param cpu The specified CPU number.
/// @return A bool type, true if there are unavailable CPUs, false otherwise.
bool IsProcessorAccessible(unsigned cpu);

/// @brief Parse the CPU group configuration.
/// @param s The configuration, e.g. "1-10,21,-1".
/// @return Get the list of CPUs parsed from the configuration.
/// @note Not sure if this is the right place to declare it though..
std::optional<std::vector<unsigned>> TryParseProcesserList(const std::string& s);

}  // namespace trpc
