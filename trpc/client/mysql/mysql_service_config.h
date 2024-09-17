
#pragma once

#include <cstdint>
#include <string>

namespace trpc::mysql {

struct MysqlConnectPoolConf {
  // Connection pool configuration parameters
  uint32_t min_size{0};       // Minimum number of connections in the pool
  uint32_t max_size{0};       // Maximum number of connections in the pool
  uint64_t max_idle_time{0};  // Maximum idle time for connections
  uint32_t timeout{0};        // Timeout for acquiring a connection from the pool

  // Comparison operator
  bool operator==(const MysqlConnectPoolConf& other) const;
};

struct MysqlClientConf {
  /// @brief User nmae
  std::string user_name;

  /// @brief user password
  std::string password;

  /// @brief db name
  std::string dbname;

  /// @brief ip
  std::string ip;

  /// @brief port
  uint32_t port;

  /// @brief Whether enable auth
  bool enable{true};

  MysqlConnectPoolConf connectpool;
};

}  // namespace trpc::mysql