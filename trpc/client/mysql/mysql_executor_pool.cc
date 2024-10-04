#include "trpc/client/mysql/mysql_executor_pool.h"
#include <stdexcept>
#include <thread>
#include "trpc/client/mysql/executor/mysql_executor.h"
#include "trpc/client/mysql/mysql_service_config.h"

namespace trpc {
namespace mysql {

MysqlExecutorPool::MysqlExecutorPool(const MysqlExecutorPoolOption& option, const NodeAddr& node_addr)
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
  std::thread producer(&MysqlExecutorPool::ProduceExcutor, this);
  std::thread recycler(&MysqlExecutorPool::RecycleExcutor, this);
  producer.detach();
  recycler.detach();
}

MysqlExecutorPool::~MysqlExecutorPool() {
  m_stop.store(true);

  while (!m_connectQ_.empty()) {
    MysqlExecutor* conn = m_connectQ_.front();
    m_connectQ_.pop();
    delete conn;
  }

  m_cond_produce_.notify_all();
  m_cond_consume_.notify_all();
}

void MysqlExecutorPool::ProduceExcutor() {
  while (!m_stop) {
    std::unique_lock<std::mutex> locker(m_mutexQ_);
    while (m_connectQ_.size() >= static_cast<size_t>(m_max_size_)) {
      m_cond_produce_.wait(locker);
      if(m_stop.load())
        return;
    }
    m_cond_consume_.notify_all();
  }
}

void MysqlExecutorPool::RecycleExcutor() {
  while (!m_stop) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::lock_guard<std::mutex> locker(m_mutexQ_);
    while (m_connectQ_.size() > static_cast<size_t>(m_min_size_)) {
      MysqlExecutor* conn = m_connectQ_.front();
      if (conn->GetAliveTime() >= m_max_idle_time_) {
        m_connectQ_.pop();
        delete conn;
      } else {
        break;
      }
    }
  }
}

void MysqlExecutorPool::AddExecutor() {
  const char* hostname = m_ip_.c_str();
  const char* username = m_user_.c_str();
  const char* password = m_passwd_.c_str();
  const char* database = m_db_name_.c_str();
  uint16_t port = static_cast<uint16_t>(m_port_);

  MysqlExecutor* conn = new MysqlExecutor(hostname, username, password, database, port);
  conn->RefreshAliveTime();
  m_connectQ_.push(conn);
}

std::shared_ptr<MysqlExecutor> MysqlExecutorPool::GetExecutor() {
  std::unique_lock<std::mutex> locker(m_mutexQ_);
  while (m_connectQ_.empty()) {
    if (m_cond_consume_.wait_for(locker, std::chrono::milliseconds(m_timeout_)) == std::cv_status::timeout) {
      if (m_connectQ_.empty()) continue;
    }
  }
  std::shared_ptr<MysqlExecutor> connptr(m_connectQ_.front(), [this](MysqlExecutor* conn) {
    std::lock_guard<std::mutex> locker(m_mutexQ_);
    conn->RefreshAliveTime();
    m_connectQ_.push(conn);
  });
  m_connectQ_.pop();
  m_cond_produce_.notify_all();
  return connptr;
}

void MysqlExecutorPool::ReconnectWithBackoff(MysqlExecutor* executor) {
  int attempt = 0;
  int max_attempts = 5;
  int delay = 100;

  while (attempt < max_attempts) {
    if (executor->Reconnect()) {
      return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    delay = std::min(delay * 2, 30000);
    attempt++;
  }

  //
  // HandleClusterUnavailable();
}

}  // namespace mysql
}  // namespace trpc