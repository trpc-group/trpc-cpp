#include "trpc/common/config/mysql_connect_pool_conf.h"

#include "trpc/util/log/logging.h"

namespace trpc {

void MysqlConnectPoolConf::Display() const {
  TRPC_LOG_DEBUG("minSize: " << minSize);
  TRPC_LOG_DEBUG("maxSize: " << maxSize);
  TRPC_LOG_DEBUG("maxIdleTime: " << maxIdleTime);
  TRPC_LOG_DEBUG("timeout: " << timeout);
}

}  // namespace trpc
