#pragma once

#include "mysqlclient/mysql.h"
#include <string>

namespace trpc::mysql {

/// @brief A common type for MySQL time.
class MysqlTime {
 public:
  MysqlTime();

  explicit MysqlTime(MYSQL_TIME my_time);

  MysqlTime& SetYear(unsigned int year);

  MysqlTime& SetMonth(unsigned int month);

  MysqlTime& SetDay(unsigned int day);

  MysqlTime& SetHour(unsigned int hour);

  MysqlTime& SetMinute(unsigned int minute);

  MysqlTime& SetSecond(unsigned int second);

  MysqlTime& SetSecondPart(unsigned long second_part);

  MysqlTime& SetTimeType(enum_mysql_timestamp_type time_type);

  unsigned int GetYear() const;

  unsigned int GetMonth() const;

  unsigned int GetDay() const;

  unsigned int GetHour() const;

  unsigned int GetMinute() const;

  unsigned int GetSecond() const;

  unsigned long SetSecondPart() const;

  enum_mysql_timestamp_type GetTimeType() const;


  /// @brief Converts the MYSQL_TIME object to a string representation.
  /// The format of the string is "YYYY-MM-DD HH:MM:SS".
  std::string ToString() const;


  /// @brief Parses a string in the format "YYYY-MM-DD HH:MM:SS"
  /// and updates the MYSQL_TIME object accordingly.
  /// The input string must match the expected format.
  void FromString(const std::string& timeStr);


  /// @brief For mysql_binder.h
  const char* DataConstPtr() const;

 private:
  MYSQL_TIME mt{};
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

  std::string_view AsStringView();

 private:
  std::string data_;
};

}
