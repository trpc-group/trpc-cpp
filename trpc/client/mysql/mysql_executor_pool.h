#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <list>
#include "trpc/client/mysql/executor/mysql_executor.h"
#include "trpc/client/mysql/mysql_service_config.h"
#include "trpc/transport/common/transport_message_common.h"


namespace trpc::mysql {

class MysqlExecutorPool {
 public:

  virtual ~MysqlExecutorPool() = default;

  virtual RefPtr<MysqlExecutor> GetExecutor() = 0;

  virtual void Reclaim(int ret, RefPtr<MysqlExecutor>&&) = 0;

  virtual void Stop() {}

  virtual void Destroy() {}
};




class MysqlExecutorPoolImpl : public MysqlExecutorPool {
 public:
  MysqlExecutorPoolImpl(const MysqlExecutorPoolOption& option, const NodeAddr& node_addr);

  RefPtr<MysqlExecutor> GetExecutor() override;

  void Reclaim(int ret, RefPtr<MysqlExecutor>&&) override;

  void Stop() override;

  void Destroy() override;

 private:
  RefPtr<MysqlExecutor> CreateExecutor(uint32_t shard_id);

  RefPtr<MysqlExecutor> GetOrCreate();

  bool IsIdleTimeout(RefPtr<MysqlExecutor> executor);

 private:
  std::string m_ip_;
  uint16_t m_port_;
  std::string m_user_;
  std::string m_passwd_;
  std::string m_db_name_;
  std::string m_char_set_;

  uint32_t num_shard_group_{4};
  std::atomic<uint32_t> executor_num_{0};

  // The maximum number of connections that can be stored per `Shard` in `conn_shards_`
  uint32_t max_num_per_shard_{0};

  uint32_t max_conn_{0};

  uint64_t max_idle_time_{0};


  struct alignas(hardware_destructive_interference_size) Shard {
    std::mutex lock;
    std::list<RefPtr<MysqlExecutor>> mysql_executors;
  };

  std::unique_ptr<Shard[]> executor_shards_;



  std::atomic<uint32_t> shard_id_gen_{0};
  std::atomic<uint32_t> executor_id_gen_{0};


};


} // namespace trpc::mysql

