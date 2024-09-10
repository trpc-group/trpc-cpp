#include "trpc/client/mysql/mysql_executor_pool.h"
#include <stdexcept>
#include <thread>
#include "trpc/client/mysql/mysql_executor.h"

namespace trpc {
namespace mysql {

MysqlExecutorPool* MysqlExecutorPool::getConnectPool(const std::string& ip, unsigned short port,
                                                     const std::string& userName, const std::string& password,
                                                     const std::string& dbName, int minSize, int maxSize, int maxIdTime,
                                                     int timeout) {
  static MysqlExecutorPool pool(ip, port, userName, password, dbName, minSize, maxSize, maxIdTime, timeout);
  return &pool;
}

MysqlExecutorPool::MysqlExecutorPool(const std::string& ip, unsigned short port, const std::string& userName,
                                     const std::string& password, const std::string& dbName, int minSize, int maxSize,
                                     int maxIdTime, int timeout)
    : m_ip(ip),
      m_user(userName),
      m_port(port),
      m_passwd(password),
      m_dbName(dbName),
      m_minSize(minSize),
      m_maxSize(maxSize),
      m_timeout(timeout),
      m_maxIdTime(maxIdTime) {
  for (int i = 0; i < m_minSize; i++) {
    addConnection();
  }
  std::thread producer(&MysqlExecutorPool::produceConnection, this);
  std::thread recycler(&MysqlExecutorPool::recycleConnection, this);
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

void MysqlExecutorPool::produceConnection() {
  while (true) {
    std::unique_lock<std::mutex> locker(m_mutexQ);
    while (m_connectQ.size() >= static_cast<size_t>(m_minSize)) {
      m_cond_produce.wait(locker);
    }
    addConnection();
    m_cond_consume.notify_all();
  }
}

void MysqlExecutorPool::recycleConnection() {
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::lock_guard<std::mutex> locker(m_mutexQ);
    while (m_connectQ.size() > static_cast<size_t>(m_minSize)) {
      MysqlExecutor* conn = m_connectQ.front();
      if (conn->GetAliveTime() >= m_maxIdTime) {
        m_connectQ.pop();
        delete conn;
      } else {
        break;
      }
    }
  }
}

void MysqlExecutorPool::addConnection() {
  const char* hostname = m_ip.c_str();
  const char* username = m_user.c_str();
  const char* password = m_passwd.c_str();
  const char* database = m_dbName.c_str();
  uint16_t port = static_cast<uint16_t>(m_port);

  MysqlExecutor* conn = new MysqlExecutor(hostname, username, password, database, port);
  conn->RefreshAliveTime();
  m_connectQ.push(conn);
}

std::shared_ptr<MysqlExecutor> MysqlExecutorPool::getConnection() {
  std::unique_lock<std::mutex> locker(m_mutexQ);
  while (m_connectQ.empty()) {
    if (m_cond_consume.wait_for(locker, std::chrono::milliseconds(m_timeout)) == std::cv_status::timeout) {
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
