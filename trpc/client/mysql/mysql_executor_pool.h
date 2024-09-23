#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include "trpc/client/mysql/executor/mysql_executor.h"
#include "trpc/client/mysql/mysql_service_config.h"
namespace trpc {
namespace mysql {
class MysqlExecutorPool {
 public:
  // static MysqlExecutorPool* getConnectPool(const MysqlClientConf& conf);

  MysqlExecutorPool(const MysqlExecutorPool& obj) = delete;
  MysqlExecutorPool& operator=(const MysqlExecutorPool& obj) = delete;

  MysqlExecutorPool(const MysqlClientConf* conf);

  std::shared_ptr<MysqlExecutor> getConnection();

  ~MysqlExecutorPool();

 private:
  MysqlExecutorPool() = default;

  // 连接池操作
  void produceConnection();
  void recycleConnection();
  void addConnection();

 private:
  std::string m_ip;
  std::string m_user;
  std::string m_passwd;
  std::string m_dbName;
  unsigned short m_port;

  uint32_t m_minSize;
  uint32_t m_maxSize;

  uint32_t m_timeout;
  uint64_t m_maxIdTime;

  std::queue<MysqlExecutor*> m_connectQ;
  std::mutex m_mutexQ;
  std::condition_variable m_cond_produce;
  std::condition_variable m_cond_consume;
};
}  // namespace mysql
}  // namespace trpc
