#include "trpc/client/mysql/mysql_executor_pool.h"
#include <stdexcept>
#include <thread>
#include "trpc/client/mysql/executor/mysql_executor.h"
#include "trpc/client/mysql/mysql_service_config.h"

namespace trpc {
namespace mysql {

MysqlExecutorPoolImpl::MysqlExecutorPoolImpl(const MysqlExecutorPoolOption& option, const NodeAddr& node_addr)
    : m_ip_(node_addr.ip),
      m_port_(node_addr.port),
      m_user_(option.user_name),
      m_passwd_(option.password),
      m_db_name_(option.dbname),
      m_char_set_(option.char_set),
      num_shard_group_(option.num_shard_group),
      max_conn_(option.max_size),
      max_idle_time_(option.max_idle_time){

  executor_shards_ = std::make_unique<Shard[]>(num_shard_group_);
  max_num_per_shard_ = std::ceil(max_conn_ / num_shard_group_);
}

RefPtr<MysqlExecutor> MysqlExecutorPoolImpl::GetOrCreate() {
  RefPtr<MysqlExecutor> executor{nullptr};
  RefPtr<MysqlExecutor> idle_executor{nullptr};

  uint32_t shard_id = shard_id_gen_.fetch_add(1);
  int retry_num = 3;

  while (retry_num > 0) {
    auto& shard = executor_shards_[shard_id % num_shard_group_];

    {
      std::scoped_lock _(shard.lock);
      if (shard.mysql_executors.size() > 0) {
        executor = shard.mysql_executors.back();
        TRPC_ASSERT(executor != nullptr);

        shard.mysql_executors.pop_back();

        if (executor->IsConnectionValid()) {
          if (!IsIdleTimeout(executor))
            return executor;
          else
            idle_executor = executor;
        }
        else {
          idle_executor = executor;
        }
      } else {
        break;
      }
    }

    if (idle_executor != nullptr) {
      idle_executor->Close();
    }

    --retry_num;
  }

  executor = CreateExecutor(shard_id);

  if(executor->Connect()) {
    executor_num_.fetch_add(1, std::memory_order_relaxed);
    return executor;
  }

  return nullptr;
}

RefPtr<MysqlExecutor> MysqlExecutorPoolImpl::CreateExecutor(uint32_t shard_id) {
  uint64_t executor_id = static_cast<uint64_t>(shard_id) << 32;
  executor_id |= executor_id_gen_.fetch_add(1, std::memory_order_relaxed);

  auto executor = MakeRefCounted<MysqlExecutor>(m_ip_, m_user_, m_passwd_, m_db_name_, m_port_, m_char_set_);
  executor->SetExecutorId(executor_id);
  return executor;
}

void MysqlExecutorPoolImpl::Reclaim(int ret, RefPtr<MysqlExecutor>&& executor) {

  if(ret == 0) {
    uint32_t shard_id = (executor->GetExecutorId() >> 32);
    auto& shard = executor_shards_[shard_id % num_shard_group_];

    std::scoped_lock _(shard.lock);
    if ((shard.mysql_executors.size() <= max_num_per_shard_) &&
        (executor_num_.load(std::memory_order_relaxed) <= max_conn_)) {
      executor->RefreshAliveTime();
      shard.mysql_executors.push_back(std::move(executor));
      return;
    }
  }

  executor_num_.fetch_add(1, std::memory_order_relaxed);
  executor->Close();
}

void MysqlExecutorPoolImpl::Stop() {
  for (uint32_t i = 0; i != num_shard_group_; ++i) {
    auto&& shard = executor_shards_[i];

    std::list<RefPtr<MysqlExecutor>> mysql_executors;
    {
      std::scoped_lock _(shard.lock);
      mysql_executors = shard.mysql_executors;
    }

    for (const auto& executor : mysql_executors) {
      TRPC_ASSERT(executor != nullptr);
      executor->Close();
    }
  }
}

void MysqlExecutorPoolImpl::Destroy() {
  for (uint32_t i = 0; i != num_shard_group_; ++i) {
    auto&& shard = executor_shards_[i];

    std::list<RefPtr<MysqlExecutor>> mysql_executors;
    {
      std::scoped_lock _(shard.lock);
      mysql_executors.swap(shard.mysql_executors);
    }

    // Just destruct the MysqlExecutor by Refcount.
  }
}

RefPtr<MysqlExecutor> MysqlExecutorPoolImpl::GetExecutor() {
  return GetOrCreate();
}

bool MysqlExecutorPoolImpl::IsIdleTimeout(RefPtr<MysqlExecutor> executor) {
  if (executor != nullptr) {
    if (max_idle_time_ == 0 ||
        executor->GetAliveTime() < max_idle_time_) {
      return false;
    }
    return true;
  }
  return false;
}


}  // namespace mysql
}  // namespace trpc