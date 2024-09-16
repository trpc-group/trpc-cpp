#include "trpc/client/mysql/mysql_executor_pool_manager.h"
#include <sstream>

namespace trpc {
namespace mysql {

std::unordered_map<std::string, std::unique_ptr<MysqlExecutorPool>> MysqlExecutorPoolManager::pools_;
std::mutex MysqlExecutorPoolManager::mutex_;

MysqlExecutorPool* MysqlExecutorPoolManager::getPool(const MysqlClientConf& conf) {
  std::string pool_key = generatePoolKey(conf);

  std::lock_guard<std::mutex> lock(mutex_);

  auto it = pools_.find(pool_key);
  if (it != pools_.end()) {
    return it->second.get();
  }

  auto pool = std::make_unique<MysqlExecutorPool>(conf);
  MysqlExecutorPool* pool_ptr = pool.get();
  pools_[pool_key] = std::move(pool);
  return pool_ptr;
}

// 停止并销毁所有连接池
void MysqlExecutorPoolManager::stopAndDestroyPools() {
  std::lock_guard<std::mutex> lock(mutex_);
  pools_.clear();
}

// 生成连接池的唯一键
std::string MysqlExecutorPoolManager::generatePoolKey(const MysqlClientConf& conf) {
  std::stringstream ss;
  ss << conf.ip << ":" << conf.port << ":" << conf.dbname;
  return ss.str();
}

}  // namespace mysql
}  // namespace trpc
