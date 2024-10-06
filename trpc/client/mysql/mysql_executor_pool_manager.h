#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "trpc/client/mysql/mysql_executor_pool.h"
#include "trpc/client/mysql/mysql_service_config.h"
#include "trpc/transport/common/transport_message_common.h"
#include "trpc/util/concurrency/lightly_concurrent_hashmap.h"
// #include "trpc/transport/client/trans_info.h"

namespace trpc {
namespace mysql {

using MysqlExecutorPtr = std::shared_ptr<MysqlExecutor>;

class MysqlExecutorPoolManager : public RefCounted<MysqlExecutorPoolManager> {
 public:
  explicit MysqlExecutorPoolManager(MysqlExecutorPoolOption&& option);

  //  MysqlExecutorPtr GetExecutor(const std::string& ip_port);

  MysqlExecutorPool* Get(const NodeAddr& node_addr);

  void Stop();

  void Destroy();

 private:
  MysqlExecutorPool* CreateExecutorPool(const NodeAddr& node_addr);

 private:
  concurrency::LightlyConcurrentHashMap<std::string, MysqlExecutorPool*> execuctor_pools_;
  MysqlExecutorPoolOption option_;
};

}  // namespace mysql
}  // namespace trpc
