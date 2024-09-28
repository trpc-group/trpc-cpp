



#include "trpc/client/mysql/mysql_executor_pool.h"
#include <stdexcept>
#include <thread>
#include "trpc/client/mysql/executor/mysql_executor.h"
#include "trpc/common/config/mysql_client_conf.h"

namespace trpc {
namespace mysql {

MysqlExecutorPool* MysqlExecutorPool::getConnectPool(const MysqlClientConf& conf) {
  static MysqlExecutorPool pool(conf);
  return &pool;
}

MysqlExecutorPool::MysqlExecutorPool(const MysqlClientConf& conf)
    : m_ip_(conf.ip),
      m_user_(conf.user_name),
      m_passwd_(conf.password),
      m_dbname_(conf.dbname),
      m_port_(conf.port),
      m_min_size_(conf.connectpool.min_size),
      m_max_size_(conf.connectpool.max_size),
      m_timeout_(conf.connectpool.timeout),
      m_max_idle_time_(conf.connectpool.max_idle_time) {
  for (uint32_t i = 0; i < m_min_size_; i++) {
    GetExecutor();
  }
  std::thread producer(&MysqlExecutorPool::ProduceExcutor, this);
  std::thread recycler(&MysqlExecutorPool::RecycleExcutor, this);
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
    while (m_connectQ.size() >= static_cast<size_t>(m_min_size_)) {
      m_cond_produce.wait(locker);
    }
    GetExecutor();
    m_cond_consume.notify_all();
  }
}

void MysqlExecutorPool::RecycleExcutor() {
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

void MysqlExecutorPool::GetExecutor() {
  const char* hostname = m_ip_.c_str();
  const char* username = m_user_.c_str();
  const char* password = m_passwd_.c_str();
  const char* database = m_dbname_.c_str();
  uint16_t port = static_cast<uint16_t>(m_port_);

  MysqlExecutor* conn = new MysqlExecutor(hostname, username, password, database, port);
  conn->RefreshAliveTime();
  m_connectQ.push(conn);
}

std::shared_ptr<MysqlExecutor> MysqlExecutorPool::getConnection() {
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

}  // namespace mysql
}  // namespace trpc

