#include "trpc/client/mysql/executor/mysql_executor.h"
#include <mysql/mysql.h>

namespace trpc::mysql {

std::mutex MysqlExecutor::mysql_mutex;

MysqlExecutor::MysqlExecutor(const std::string& hostname, const std::string& username, const std::string& password,
                             const std::string& database, const uint16_t port)
    : hostname_(hostname), username_(username), password_(password), database_(database), port_(port) {
  {
    std::lock_guard<std::mutex> lock(mysql_mutex);
    mysql_ = mysql_init(NULL);
  }
  mysql_set_character_set(mysql_, "utf8");
  MYSQL* ret = mysql_real_connect(mysql_, hostname.c_str(), username.c_str(), password.c_str(), database.c_str(), port, nullptr, 0);

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
  if (!IsConnectionValid()) {
    std::unique_ptr<MysqlExecutor> temp_conn = std::make_unique<MysqlExecutor>(
        hostname_.c_str(), username_.c_str(), password_.c_str(), database_.c_str(), port_);

    if (!temp_conn->IsConnectionValid()) {
      StartReconnectAsync();
      return false;
    }

    return temp_conn->ExecuteStatement(output_binds, statement);
  }
  if (mysql_stmt_bind_result(statement.STMTPointer(), output_binds.data()) != 0) return false;

  if (mysql_stmt_execute(statement.STMTPointer()) != 0) {
    return false;
  }

  return true;
}

bool MysqlExecutor::ExecuteStatement(MysqlStatement& statement) {
  if (!IsConnectionValid()) {
    std::unique_ptr<MysqlExecutor> temp_conn = std::make_unique<MysqlExecutor>(
        hostname_.c_str(), username_.c_str(), password_.c_str(), database_.c_str(), port_);

    if (!temp_conn->IsConnectionValid()) {
      StartReconnectAsync();
      return false;
    }

    return temp_conn->ExecuteStatement(statement);
  }
  if (mysql_stmt_execute(statement.STMTPointer()) != 0) return false;

  return true;
}

uint64_t MysqlExecutor::GetAliveTime() {
  uint64_t now = trpc::GetSteadyMicroSeconds();
  uint64_t alive_time = now - m_alivetime;
  return alive_time;
}

void MysqlExecutor::RefreshAliveTime() { m_alivetime = trpc::GetSteadyMicroSeconds(); }

std::string MysqlExecutor::ConvertPlaceholders(const std::string& sql) {
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

void MysqlExecutor::StartReconnectAsync() {
  std::thread([this]() {
    int retry_interval = 100;
    int max_retries = 5;
    for (int i = 0; i < max_retries; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(retry_interval));

      if (Reconnect()) {
        return;
      }
      retry_interval *= 2;
    }
  }).detach();
}

bool MysqlExecutor::Reconnect() {
  Close();
  mysql_ = mysql_init(nullptr);
  if (!mysql_) return false;

  if (!mysql_real_connect(mysql_, hostname_.c_str(), username_.c_str(), password_.c_str(), database_.c_str(), port_,
                          nullptr, 0)) {
    return false;
  }

  return true;
}

bool MysqlExecutor::IsConnectionValid() {
  if (mysql_ != nullptr && mysql_ping(mysql_) == 0) {
    return true;
  } else {
    return false;
  }
}

size_t MysqlExecutor::ExecuteInternal(const std::string &query, MysqlResults<OnlyExec> &mysql_results) {
  if(mysql_real_query(mysql_, query.c_str(), query.length()) != 0) {
    mysql_results.SetErrorMessage(mysql_error(mysql_));
    return 0;
  }

  return mysql_affected_rows(mysql_);
}

}  // namespace trpc::mysql
