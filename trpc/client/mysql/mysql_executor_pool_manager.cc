#include "trpc/client/mysql/mysql_executor_pool_manager.h"
#include <sstream>

namespace trpc {
namespace mysql {

//// 静态成员变量定义
//std::unordered_map<std::string, std::unique_ptr<MysqlExecutorPool>> MysqlExecutorPoolManager::pools_;
//std::mutex MysqlExecutorPoolManager::mutex_;
//
//// 获取指定配置的连接池
//MysqlExecutorPool* MysqlExecutorPoolManager::getPool(const MysqlClientConf* conf) {
//  std::string pool_key = generatePoolKey(conf);
//
//  std::lock_guard<std::mutex> lock(mutex_);
//
//  auto it = pools_.find(pool_key);
//  if (it != pools_.end()) {
//    return it->second.get();
//  }
//
//  auto pool = std::make_unique<MysqlExecutorPool>(conf);
//  MysqlExecutorPool* pool_ptr = pool.get();
//  pools_[pool_key] = std::move(pool);
//  return pool_ptr;
//}
//
//// 停止并销毁所有连接池
//void MysqlExecutorPoolManager::stopAndDestroyPools() {
//  std::lock_guard<std::mutex> lock(mutex_);
//  pools_.clear();
//}
//
//// 生成连接池的唯一键
//std::string MysqlExecutorPoolManager::generatePoolKey(const MysqlClientConf* conf) {
//  std::stringstream ss;
//  ss << conf->ip << ":" << conf->port << ":" << conf->dbname;
//  return ss.str();
//}



MysqlExecutorPoolManager::MysqlExecutorPoolManager(MysqlExecutorPoolOption &&option) {
  option_ = std::move(option);
}

MysqlExecutorPool* MysqlExecutorPoolManager::Get(const NodeAddr& node_addr) {
  const int len = 64;
  std::string endpoint(len, 0x0);
  std::snprintf(const_cast<char*>(endpoint.c_str()), len, "%s:%d", node_addr.ip.c_str(), node_addr.port);

  MysqlExecutorPool* executor_pool{nullptr};
  bool ret = execuctor_pools_.Get(endpoint, executor_pool);

  if (ret) {
    return executor_pool;
  }

  MysqlExecutorPool* pool = CreateExecutorPool(node_addr);
  ret = execuctor_pools_.GetOrInsert(endpoint, pool, executor_pool);
  if (!ret) {
    return pool;
  }

  delete pool;
  pool = nullptr;

  return executor_pool;
}


MysqlExecutorPool* MysqlExecutorPoolManager::CreateExecutorPool(const NodeAddr& node_addr) {
  MysqlExecutorPool* new_pool = new MysqlExecutorPool(option_, node_addr);
  return new_pool;
}

}  // namespace mysql
}  // namespace trpc
