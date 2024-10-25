//
// Created by kosmos on 10/13/24.
//

#include <sstream>
#include <iomanip>
#include "trpc/client/mysql/executor/mysql_type.h"
#include "trpc/util/time.h"

namespace trpc::mysql {


MysqlTime::MysqlTime() {
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

MysqlTime::MysqlTime(MYSQL_TIME my_time) {
  mt = my_time;
}


MysqlTime &MysqlTime::SetYear(unsigned int year) {
  mt.year = year;
  return *this;
}

MysqlTime &MysqlTime::SetMonth(unsigned int month) {
  if(month > 0 && month <= 12)
    mt.month = month;
  return *this;
}

MysqlTime &MysqlTime::SetDay(unsigned int day) {
  mt.day = day;
  return *this;
}

MysqlTime &MysqlTime::SetHour(unsigned int hour) {
  if(hour <= 24)
    mt.hour = hour;
  return *this;
}

MysqlTime &MysqlTime::SetMinute(unsigned int minute) {
  if(minute <= 60)
    mt.minute = minute;
  return *this;
}

MysqlTime &MysqlTime::SetSecond(unsigned int second) {
  if(second <= 60)
    mt.second = second;
  return *this;
}

MysqlTime &MysqlTime::SetSecondPart(unsigned long second_part) {
  mt.second_part = second_part;
  return *this;
}


MysqlTime &MysqlTime::SetTimeType(enum_mysql_timestamp_type time_type) {
  mt.time_type = time_type;
  return *this;
}

unsigned int MysqlTime::GetYear() const { return mt.year; }

unsigned int MysqlTime::GetMonth() const { return mt.month; }

unsigned int MysqlTime::GetDay() const { return mt.day; }

unsigned int MysqlTime::GetHour() const { return mt.hour; }

unsigned int MysqlTime::GetMinute() const { return mt.minute; }

unsigned int MysqlTime::GetSecond() const { return mt.second; }

unsigned long MysqlTime::SetSecondPart() const { return mt.second_part; }

enum_mysql_timestamp_type MysqlTime::GetTimeType() const { return mt.time_type; }

//std::string MysqlTime::ToString() const {
//  std::ostringstream oss;
//  oss << std::setw(4) << std::setfill('0') << mt.year << '-'
//      << std::setw(2) << std::setfill('0') << mt.month << '-'
//      << std::setw(2) << std::setfill('0') << mt.day << ' '
//      << std::setw(2) << std::setfill('0') << mt.hour << ':'
//      << std::setw(2) << std::setfill('0') << mt.minute << ':'
//      << std::setw(2) << std::setfill('0') << mt.second;
//  return oss.str();
//}

std::string MysqlTime::ToString() const {
  return time::StringFormat("%04d-%02d-%02d %02d:%02d:%02d", mt.year, mt.month,
                      mt.day, mt.hour, mt.minute, mt.second);
}

void MysqlTime::FromString(const std::string &timeStr) {
  std::istringstream iss(timeStr);
  char delimiter;
  iss >> mt.year >> delimiter >> mt.month >> delimiter >> mt.day
      >> mt.hour >> delimiter >> mt.minute >> delimiter >> mt.second;
}

const char *MysqlTime::DataConstPtr() const {
  return reinterpret_cast<const char *>(&mt);
}



MysqlBlob::MysqlBlob(MysqlBlob &&other) noexcept:
        data_(std::move(other.data_)
        ) {
}


MysqlBlob::MysqlBlob(const std::string &data) : data_(data) {}

MysqlBlob::MysqlBlob(std::string &&data) noexcept: data_(std::move(data)) {}

MysqlBlob::MysqlBlob(const char *data, std::size_t length) : data_(data, length) {}

MysqlBlob &MysqlBlob::operator=(const MysqlBlob &other) {
  if (this != &other) {
    data_ = other.data_;
  }
  return *this;
}

MysqlBlob &MysqlBlob::operator=(MysqlBlob &&other)
noexcept {
  if (this != &other) {
    data_ = std::move(other.data_);
  }
  return *this;
}

bool MysqlBlob::operator==(const MysqlBlob &other) const {
  return data_ == other.data_;
}

const char *MysqlBlob::data_ptr() const {
  return data_.data();
}

size_t MysqlBlob::size() const {
  return data_.size();
}

std::string_view MysqlBlob::AsStringView() {
  return data_;
}
}