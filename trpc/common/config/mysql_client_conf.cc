#include "trpc/common/config/mysql_client_conf.h"
#include "trpc/util/log/logging.h"

namespace trpc {

void MysqlClientConf::Display() const {
  TRPC_LOG_DEBUG("user_name: " << user_name);
  TRPC_LOG_DEBUG("password: " << password);
  TRPC_LOG_DEBUG("dbname: " << dbname);
  TRPC_LOG_DEBUG("char_set: " << char_set);
  TRPC_LOG_DEBUG("thread_num: " << thread_num);
  TRPC_LOG_DEBUG("thread_bind_core: " << (thread_bind_core ? "true" : "false"));
  TRPC_LOG_DEBUG("num_shard_group: " << num_shard_group);
}

}  // namespace trpc
