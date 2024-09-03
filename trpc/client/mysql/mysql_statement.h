#pragma once

#include <mysql/mysql.h>
#include <string>

namespace trpc::mysql {

class MysqlStatement {
public:
  MysqlStatement(const std::string& sql, MYSQL* conn);
  ~MysqlStatement();
  MysqlStatement(MysqlStatement&& rhs) = default;
  MysqlStatement(MysqlStatement& rhs) = delete;

  void CloseStatement();

  unsigned int GetFieldCount() { return field_count_; }

  MYSQL_STMT* STMTPointer() { return mysql_stmt_; }

  bool IsValid() { return mysql_stmt_ == nullptr; }


private:
  MYSQL_STMT* mysql_stmt_;
  unsigned int field_count_;
};
}  // namespace trpc::mysql
