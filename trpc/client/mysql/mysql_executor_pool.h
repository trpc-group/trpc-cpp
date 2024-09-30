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
class MysqlExecutorPool {
 public:
  // static MysqlExecutorPool* getConnectPool(const MysqlClientConf& conf);

  MysqlExecutorPool(const MysqlExecutorPool& obj) = delete;
  MysqlExecutorPool& operator=(const MysqlExecutorPool& obj) = delete;

  MysqlExecutorPool(const MysqlExecutorPoolOption& option, const NodeAddr& node_addr);

  std::shared_ptr<MysqlExecutor> GetExecutor();

  ~MysqlExecutorPool();

  void Destory();

  void ReconnectWithBackoff(MysqlExecutor* executor);

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
  std::string m_db_name_;
  unsigned short m_port_;

  uint32_t m_min_size_;
  uint32_t m_max_size_;

  uint32_t m_timeout_;
  uint64_t m_max_idle_time_;

  uint64_t m_recycle_interval_;

  std::queue<MysqlExecutor*> m_connectQ_;
  std::mutex m_mutexQ_;
  std::condition_variable m_cond_produce_;
  std::condition_variable m_cond_consume_;
};
}  // namespace mysql
}  // namespace trpc
