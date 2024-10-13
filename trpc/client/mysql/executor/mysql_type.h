//
// Created by kosmos on 10/13/24.
//

#pragma once

#include "mysqlclient/mysql.h"
#include <string>

namespace trpc::mysql {

/// @brief A common type for MySQL time.
/// @note Ensure that sizeof(MYSQL_TIME) == sizeof(MysqlTime).
///       The generic templates StepInputBind and StepOutputBind rely on this.
///       If MysqlTime changes and this condition is not met,
///       additional overloads for data binding functions for MysqlTime will be required.
class MysqlTime
{
 public:
  //Todo: Friendly constructor and member functions
  MysqlTime() {
    mt.year = 2024;
    mt.month = 1;
    mt.day = 1;
    mt.hour = 0;
    mt.minute = 0;
    mt.second = 0;
    mt.second_part = 0;
    mt.time_type = MYSQL_TIMESTAMP_DATETIME;
    mt.neg = 0;
  }

  MYSQL_TIME mt;
};


class MysqlBlob {
 public:
  MysqlBlob() = default;

  MysqlBlob(const MysqlBlob& other)  = default;

  MysqlBlob(MysqlBlob&& other) noexcept;


  explicit MysqlBlob(const std::string& data);

  explicit MysqlBlob(std::string&& data) noexcept;

  MysqlBlob(const char* data, std::size_t length);

  MysqlBlob& operator=(const MysqlBlob& other);

  MysqlBlob& operator=(MysqlBlob&& other) noexcept;

  bool operator==(const MysqlBlob& other) const;

  const char* data_ptr() const;

  size_t size() const;

 private:
  std::string data_;
};

}
