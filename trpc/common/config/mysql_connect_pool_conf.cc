#include "trpc/common/config/mysql_connect_pool_conf.h"

#include "trpc/util/log/logging.h"

namespace trpc {

void MysqlConnectPoolConf::Display() const {
  TRPC_LOG_DEBUG("min_size: " << min_size);
  TRPC_LOG_DEBUG("max_size: " << max_size);
  TRPC_LOG_DEBUG("max_idle_time: " << max_idle_time);
  TRPC_LOG_DEBUG("timeout: " << timeout);
}

// Comparison operator
bool MysqlConnectPoolConf::operator==(const MysqlConnectPoolConf& other) const {
  return min_size == other.min_size && max_size == other.max_size && max_idle_time == other.max_idle_time &&
         timeout == other.timeout;
}

}  // namespace trpc
