#include "trpc/client/mysql/mysql_executor_pool.h"
#include <stdexcept>
#include <thread>
#include "trpc/client/mysql/executor/mysql_executor.h"
#include "trpc/client/mysql/mysql_service_config.h"

namespace trpc {
namespace mysql {

// Define thread_local_executors_ as a pointer
thread_local std::list<MysqlExecutor*>* MysqlExecutorPool::thread_local_executors_ = nullptr;

MysqlExecutorPool::MysqlExecutorPool(const MysqlExecutorPoolOption& option, const NodeAddr& node_addr)
    : m_user_(option.user_name),
      m_passwd_(option.password),
      m_dbname_(option.db_name),
      m_min_size_(option.min_size),
      m_max_size_(option.max_size),
      m_timeout_(option.timeout),
      m_max_idle_time_(option.max_idle_time) {
  for (uint32_t i = 0; i < m_min_size_; i++) {
    AddExecutor();
  }
  std::thread producer(&MysqlExecutorPool::ProduceExcutor, this);
  std::thread recycler(&MysqlExecutorPool::RecycleExcutorThread, this);
  producer.detach();
  recycler.detach();
}

MysqlExecutorPool::~MysqlExecutorPool() {
  while (!m_connectQ.empty()) {
    MysqlExecutor* conn = m_connectQ.front();
    m_connectQ.pop();
    delete conn;
  }
}

void MysqlExecutorPool::ProduceExcutor() {
  while (true) {
    std::unique_lock<std::mutex> locker(m_mutexQ);
    while (m_connectQ.size() >= static_cast<size_t>(m_max_size_)) {
      m_cond_produce.wait(locker);
    }
    GetExecutor();
    m_cond_consume.notify_all();
  }
}

void MysqlExecutorPool::RecycleExcutorThread() {
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::lock_guard<std::mutex> locker(m_mutexQ);
    while (m_connectQ.size() > static_cast<size_t>(m_min_size_)) {
      MysqlExecutor* conn = m_connectQ.front();
      if (conn->GetAliveTime() >= m_max_idle_time_) {
        m_connectQ.pop();
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
  const char* database = m_dbname_.c_str();
  uint16_t port = static_cast<uint16_t>(m_port_);

  MysqlExecutor* conn = new MysqlExecutor(hostname, username, password, database, port);
  conn->RefreshAliveTime();
  m_connectQ.push(conn);
}

std::shared_ptr<MysqlExecutor> MysqlExecutorPool::GetExecutor() {
  std::unique_lock<std::mutex> locker(m_mutexQ);
  while (m_connectQ.empty()) {
    if (m_cond_consume.wait_for(locker, std::chrono::milliseconds(m_timeout_)) == std::cv_status::timeout) {
      if (m_connectQ.empty()) continue;
    }
  }
  std::shared_ptr<MysqlExecutor> connptr(m_connectQ.front(), [this](MysqlExecutor* conn) {
    std::lock_guard<std::mutex> locker(m_mutexQ);
    conn->RefreshAliveTime();
    m_connectQ.push(conn);
  });
  m_connectQ.pop();
  m_cond_produce.notify_all();
  return connptr;
}

void MysqlExecutorPool::RecycleExecutor(MysqlExecutor* executor) {
  thread_local_executors_->push_back(executor);  // Store in thread-local storage
}

}  // namespace mysql
}  // namespace trpc