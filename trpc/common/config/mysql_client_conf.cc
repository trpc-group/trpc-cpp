#include "trpc/common/config/mysql_client_conf.h"  // 包含 MysqlClientConf 头文件
#include "trpc/util/log/logging.h"                 // 包含日志功能

namespace trpc {

void MysqlClientConf::Display() const {
  TRPC_LOG_DEBUG("user_name: " << user_name);
  TRPC_LOG_DEBUG("password: " << password);
  TRPC_LOG_DEBUG("dbname: " << dbname);
  TRPC_LOG_DEBUG("ip: " << ip);
  TRPC_LOG_DEBUG("port: " << port);
  TRPC_LOG_DEBUG("enable: " << (enable ? "true" : "false"));
  connectpool.Display();
}

}  // namespace trpc
