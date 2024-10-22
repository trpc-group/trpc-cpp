
#pragma once

#include <cstdint>
#include <string>

namespace trpc::mysql {

struct MysqlExecutorPoolOption {
  /// @brief User nmae
  std::string user_name;

  /// @brief user password
  std::string password;

  /// @brief db name
  std::string dbname;

  std::string char_set;

  uint32_t max_size{0};       // Maximum number of connections in the pool
  uint64_t max_idle_time{0};  // Maximum idle time for connections
  uint32_t num_shard_group{4};

};

}  // namespace trpc::mysql