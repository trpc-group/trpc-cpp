#include "trpc/client/mysql/executor/mysql_executor.h"
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

bool MysqlExecutor::ExecuteStatement(std::vector<MYSQL_BIND>& output_binds, MysqlStatement& statement) {
  if (mysql_stmt_bind_result(statement.STMTPointer(), output_binds.data()) != 0)
    return false;

  if (mysql_stmt_execute(statement.STMTPointer()) != 0) {
    return false;
  }

  return true;
}

bool MysqlExecutor::ExecuteStatement(MysqlStatement& statement) {
  if (mysql_stmt_execute(statement.STMTPointer()) != 0)
    return false;

  return true;
}

uint64_t MysqlExecutor::GetAliveTime() {
  uint64_t now = trpc::GetSteadyMicroSeconds();
  uint64_t alive_time = now - m_alivetime;
  return alive_time;
}

void MysqlExecutor::RefreshAliveTime() { m_alivetime = trpc::GetSteadyMicroSeconds(); }

void MysqlExecutor::FreeResult() {
  if (res_) {
    mysql_free_result(res_);
    res_ = nullptr;
  }
}

std::string MysqlExecutor::ConvertPlaceholders(const std::string &sql) {
  std::string result;
  size_t len = sql.length();

  for (size_t i = 0; i < len; ++i) {
    if (sql[i] == '?' && (i == 0 || sql[i - 1] != '\\')) {
      result += "{}";
    } else {
      result += sql[i];
    }
  }

  return result;
}



}  // namespace trpc::mysql
