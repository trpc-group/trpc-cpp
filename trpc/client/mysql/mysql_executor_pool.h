#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include "trpc/client/mysql/mysql_executor.h"
namespace trpc {
namespace mysql {
class MysqlExecutorPool {
 public:
  static MysqlExecutorPool* getConnectPool(const std::string& ip = "", unsigned short port = 0,
                                           const std::string& userName = "", const std::string& password = "",
                                           const std::string& dbName = "", int minSize = 0, int maxSize = 0,
                                           uint64_t maxIdTime = 0, int timeout = 0);

  MysqlExecutorPool(const MysqlExecutorPool& obj) = delete;
  MysqlExecutorPool& operator=(const MysqlExecutorPool& obj) = delete;

  MysqlExecutorPool(const std::string& ip, unsigned short port, const std::string& userName,
                    const std::string& password, const std::string& dbName, int minSize, int maxSize,
                    uint64_t maxIdTime, int timeout);

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

  int m_minSize;
  int m_maxSize;

  int m_timeout;
  uint64_t m_maxIdTime;

  std::queue<MysqlExecutor*> m_connectQ;
  std::mutex m_mutexQ;
  // std::condition_variable m_cond;
  std::condition_variable m_cond_produce;
  std::condition_variable m_cond_consume;
};
}  // namespace mysql
}  // namespace trpc
