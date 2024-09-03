#include "mysql_statement.h"
#include <stdexcept>

namespace trpc::mysql {

MysqlStatement::MysqlStatement(const std::string& sql, MYSQL* conn) 
                :mysql_stmt_(nullptr) {
  mysql_stmt_ = mysql_stmt_init(conn);
  if (mysql_stmt_ == nullptr) {
      std::runtime_error conn_error("Mysql prepared statement init failed.");
      throw conn_error;
  }

  if(mysql_stmt_prepare(mysql_stmt_, sql.c_str(), sql.length()) != 0) {
    std::string msg = "Mysql prepared statement create failed. ";
    std::string msg_error(mysql_error(conn));
    msg.append(msg_error);
    std::runtime_error conn_error(msg);
    CloseStatement();
    throw conn_error;
  }

  field_count_ = mysql_stmt_field_count(mysql_stmt_);

}

MysqlStatement::~MysqlStatement() {
  CloseStatement();
}

void MysqlStatement::CloseStatement() {
  if(mysql_stmt_ != nullptr) {
    mysql_stmt_free_result(mysql_stmt_);
    mysql_stmt_close(mysql_stmt_);
    mysql_stmt_ = nullptr;
  }
}


}  // namespace trpc::mysql