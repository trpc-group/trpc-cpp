#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include "trpc/client/mysql/executor/mysql_executor.h"
#include "trpc/client/mysql/mysql_service_config.h"
#include "trpc/transport/common/transport_message_common.h"


namespace trpc {
namespace mysql {


struct MysqlExecutorPoolOption {
  // Connection pool configuration parameters
  uint32_t min_size{0};       // Minimum number of connections in the pool
  uint32_t max_size{0};       // Maximum number of connections in the pool
  uint64_t max_idle_time{0};  // Maximum idle time for connections
  uint32_t timeout{0};        // Timeout for acquiring a connection from the pool

  std::string db_name;
  std::string user_name;
  std::string password;
};

class MysqlExecutorPool {
 public:
  // static MysqlExecutorPool* getConnectPool(const MysqlClientConf& conf);

  MysqlExecutorPool(const MysqlExecutorPool& obj) = delete;
  MysqlExecutorPool& operator=(const MysqlExecutorPool& obj) = delete;

//  MysqlExecutorPool(const MysqlClientConf* conf);

  MysqlExecutorPool(const MysqlExecutorPoolOption& option, const NodeAddr& node_addr);

  std::shared_ptr<MysqlExecutor> GetExecutor();

  ~MysqlExecutorPool();

 private:
  MysqlExecutorPool() = default;

  // 连接池操作
  void ProduceExcutor();
  void RecycleExcutor();
  void AddExecutor();

 private:
  std::string m_ip_;
  std::string m_user_;
  std::string m_passwd_;
  std::string m_dbname_;
  unsigned short m_port_;

  uint32_t m_min_size_;
  uint32_t m_max_size_;

  uint32_t m_timeout_;
  uint64_t m_max_idle_time_;

  std::queue<MysqlExecutor*> m_connectQ;
  std::mutex m_mutexQ;
  std::condition_variable m_cond_produce;
  std::condition_variable m_cond_consume;
};
}  // namespace mysql
}  // namespace trpc
