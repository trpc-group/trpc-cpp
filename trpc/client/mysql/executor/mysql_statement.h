#pragma once

#include <mysql/mysql.h>
//#include "include/mysql.h"
#include <string>
#include <vector>
#include <cassert>

namespace trpc::mysql {

class MysqlStatement {
public:
  MysqlStatement(MYSQL* conn);
  ~MysqlStatement() { assert(mysql_stmt_ == nullptr); }
  MysqlStatement(MysqlStatement&& rhs) = default;
  MysqlStatement(MysqlStatement& rhs) = delete;

  bool Init(const std::string& sql);

  std::string GetErrorMessage();

  bool BindParam(std::vector<MYSQL_BIND>& bind_list);

  bool CloseStatement();

  unsigned int GetFieldCount() { return field_count_; }

  unsigned long GetParamsCount() { return params_count_; }

  MYSQL_RES* GetResultsMeta();

  MYSQL_STMT* STMTPointer() { return mysql_stmt_; }

  bool IsValid() { return mysql_stmt_ == nullptr; }

private:
  MYSQL_STMT* mysql_stmt_;
  MYSQL* mysql_;
  unsigned int field_count_;
  unsigned long params_count_;
};
}  // namespace trpc::mysql
