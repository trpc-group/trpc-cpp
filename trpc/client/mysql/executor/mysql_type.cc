//
// Created by kosmos on 10/13/24.
//

#include "trpc/client/mysql/executor/mysql_type.h"

namespace trpc::mysql {

MysqlBlob::MysqlBlob(MysqlBlob &&other) noexcept:
        data_(std::move(other.data_)
        ) {
}


MysqlBlob::MysqlBlob(const std::string &data) : data_(data) {}

MysqlBlob::MysqlBlob(std::string&& data) noexcept: data_(std::move(data)) {}

MysqlBlob::MysqlBlob(const char* data, std::size_t length) : data_(data, length) {}

MysqlBlob &MysqlBlob::operator=(const MysqlBlob& other) {
  if (this != &other) {
    data_ = other.data_;
  }
  return *this;
}

MysqlBlob &MysqlBlob::operator=(MysqlBlob&& other)
noexcept {
  if (this != &other) {
    data_ = std::move(other.data_);
  }
  return *this;
}

bool MysqlBlob::operator==(const MysqlBlob& other) const {
  return data_ == other.data_;
}

const char *MysqlBlob::data_ptr() const {
  return data_.data();
}

size_t MysqlBlob::size() const {
  return data_.size();
}

}