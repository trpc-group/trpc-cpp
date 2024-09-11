#pragma once

#include <cstdint>
#include <string>

#include "yaml-cpp/yaml.h"

namespace trpc {

/// @brief Connect pool config for accessing mysql
/// Mainly contains authentication information
struct MysqlConnectPoolConf {
  // Connection pool configuration parameters
  uint32_t min_size{0};       // Minimum number of connections in the pool
  uint32_t max_size{0};       // Maximum number of connections in the pool
  uint64_t max_idle_time{0};  // Maximum idle time for connections
  uint32_t timeout{0};        // Timeout for acquiring a connection from the pool

  // Display method to output connection pool configuration details
  void Display() const;
};

}  // namespace trpc
