#include "mysql_executor.h"
#include <mysql/mysql.h>

namespace trpc::mysql {

std::mutex MysqlExecutor::mysql_mutex;

MysqlExecutor::MysqlExecutor(const char* const hostname, const char* const username, const char* const password,
                             const char* const database, const uint16_t port) {
  {
    std::lock_guard<std::mutex> lock(mysql_mutex);
    mysql_ = mysql_init(NULL);
  }
  mysql_set_character_set(mysql_, "utf8");
  MYSQL* ret = mysql_real_connect(mysql_, hostname, username, password, database, port, nullptr, 0);

  if (nullptr == ret) {
    std::runtime_error conn_error("Connection failed: " + std::string(mysql_error(mysql_)));
    mysql_close(mysql_);
    throw conn_error;
  }
}

MysqlExecutor::~MysqlExecutor() {
  Close();
  // FreeResult();
}

void MysqlExecutor::Close() {
  if (mysql_ != nullptr) {
    mysql_close(mysql_);
    mysql_ = nullptr;
  }
}

void MysqlExecutor::ExecuteStatement(std::vector<MYSQL_BIND>& output_binds, MysqlStatement& statement) {
  if (mysql_stmt_bind_result(statement.STMTPointer(), output_binds.data()) != 0) {
    throw std::runtime_error(mysql_stmt_error(statement.STMTPointer()));
  }

  if (mysql_stmt_execute(statement.STMTPointer()) != 0) {
    // std::cout << mysql_stmt_errno(statement.STMTPointer()) << std::endl;
    throw std::runtime_error(mysql_stmt_error(statement.STMTPointer()));
  }
}

void MysqlExecutor::ExecuteStatement(MysqlStatement& statement) {
  if (mysql_stmt_execute(statement.STMTPointer()) != 0) {
    // std::cout << mysql_stmt_errno(statement.STMTPointer()) << std::endl;
    throw std::runtime_error(mysql_stmt_error(statement.STMTPointer()));
  }
}

uint64_t MysqlExecutor::GetAliveTime() {
  uint64_t now = trpc::GetSteadyMicroSeconds();
  uint64_t alive_time = now - m_alivetime;
  return alive_time;
}

void MysqlExecutor::RefreshAliveTime() { m_alivetime = trpc::GetSteadyMicroSeconds(); }

}  // namespace trpc::mysql
