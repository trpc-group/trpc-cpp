#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "trpc/client/mysql/mysql_executor_pool.h"
#include "trpc/client/mysql/mysql_service_config.h"

namespace trpc {
namespace mysql {

class MysqlExecutorPoolManager {
 public:
  static MysqlExecutorPool* getPool(const MysqlClientConf* conf);

  static void stopAndDestroyPools();

 private:
  static std::string generatePoolKey(const MysqlClientConf* conf);

  static std::unordered_map<std::string, std::unique_ptr<MysqlExecutorPool>> pools_;

  static std::mutex mutex_;
};

}  // namespace mysql
}  // namespace trpc
