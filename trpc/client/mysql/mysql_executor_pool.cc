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
      num_shard_group_(option.num_shard_group),
      max_conn_(option.max_size){

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

        if (executor->IsConnectionValid())
          return executor;
        else
          idle_executor = executor;
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

  auto executor = MakeRefCounted<MysqlExecutor>(m_ip_, m_user_, m_passwd_, m_db_name_, m_port_);
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



//----------------------------------------------

BackThreadExecutorPool::BackThreadExecutorPool(const MysqlExecutorPoolOption& option, const NodeAddr& node_addr)
    : m_ip_(node_addr.ip),
      m_port_(node_addr.port),
      m_user_(option.user_name),
      m_passwd_(option.password),
      m_db_name_(option.dbname),
      m_min_size_(option.min_size),
      m_max_size_(option.max_size),
      m_timeout_(option.timeout),
      m_max_idle_time_(option.max_idle_time) {
  for (uint32_t i = 0; i < m_min_size_; i++) {
    AddExecutor();
  }
  std::thread producer(&BackThreadExecutorPool::ProduceExecutor, this);
  std::thread recycler(&BackThreadExecutorPool::RecycleExecutor, this);
  producer.detach();
  recycler.detach();
}

BackThreadExecutorPool::~BackThreadExecutorPool() {
  m_stop.store(true);

  while (!m_connectQ_.empty()) {
    RefPtr<MysqlExecutor> conn = m_connectQ_.front();
    m_connectQ_.pop();
    conn->Close();
  }

  m_cond_produce_.notify_all();
  m_cond_consume_.notify_all();

}

void BackThreadExecutorPool::ProduceExecutor() {
  while (!m_stop) {
    std::unique_lock<std::mutex> locker(m_mutexQ_);
    while (m_connectQ_.size() >= static_cast<size_t>(m_max_size_)) {
      m_cond_produce_.wait(locker);
      if (m_stop.load()) return;
    }
    m_cond_consume_.notify_all();
  }
}

void BackThreadExecutorPool::RecycleExecutor() {
  while (!m_stop) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::lock_guard<std::mutex> locker(m_mutexQ_);


    while (m_connectQ_.size() > static_cast<size_t>(m_min_size_)) {
      RefPtr<MysqlExecutor> conn = m_connectQ_.front();
      if (conn->GetAliveTime() >= m_max_idle_time_) {
        m_connectQ_.pop();
      } else {
        break;
      }
    }
  }
}

void BackThreadExecutorPool::AddExecutor() {
  const char* hostname = m_ip_.c_str();
  const char* username = m_user_.c_str();
  const char* password = m_passwd_.c_str();
  const char* database = m_db_name_.c_str();
  uint16_t port = static_cast<uint16_t>(m_port_);

//  MysqlExecutor* conn = new MysqlExecutor(hostname, username, password, database, port);
  auto conn = MakeRefCounted<MysqlExecutor>(hostname, username, password, database, port);
  conn->RefreshAliveTime();
  m_connectQ_.push(conn);
}

RefPtr<MysqlExecutor> BackThreadExecutorPool::GetExecutor() {
  std::unique_lock<std::mutex> locker(m_mutexQ_);
  while (m_connectQ_.empty()) {
    if (m_cond_consume_.wait_for(locker, std::chrono::milliseconds(m_timeout_)) == std::cv_status::timeout) {
      if (m_connectQ_.empty()) {
        return nullptr;
      };
    }
  }
  RefPtr<MysqlExecutor> conn = m_connectQ_.front();
  m_connectQ_.pop();
  m_cond_produce_.notify_all();
  return conn;
}

void BackThreadExecutorPool::Reclaim(int ret, RefPtr<MysqlExecutor>&& conn) {
  std::lock_guard<std::mutex> locker(m_mutexQ_);
  conn->RefreshAliveTime();
  m_connectQ_.push(conn);
}


}  // namespace mysql
}  // namespace trpc