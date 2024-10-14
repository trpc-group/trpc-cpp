//
// Created by kosmos on 9/23/24.
//

#include "mysql_results.h"

namespace trpc::mysql {

MysqlRow::MysqlFieldIterator::MysqlFieldIterator(MysqlRow* mrow, unsigned int index)
        : mrow_(mrow), index_(index) {}

MysqlRow::MysqlFieldIterator& MysqlRow::MysqlFieldIterator::operator++() {
  if (index_ < mrow_->num_fields_)
    ++index_;
  return *this;
}

bool MysqlRow::MysqlFieldIterator::operator!=(const MysqlFieldIterator& other) const {
  return index_ != other.index_ || mrow_ != other.mrow_;
}

bool MysqlRow::MysqlFieldIterator::operator==(const MysqlFieldIterator& other) const {
  return index_ == other.index_ && mrow_ == other.mrow_;
}

std::string_view MysqlRow::MysqlFieldIterator::operator*() const {
  if (!mrow_ || index_ >= mrow_->num_fields_) {
    return "";
  }
  return mrow_->row_[index_] ?
          std::string_view(mrow_->row_[index_], mrow_->lengths_[index_]) : std::string_view();
}

bool MysqlRow::MysqlFieldIterator::IsNull() { return mrow_->row_[index_] == nullptr; }

std::string MysqlRow::MysqlFieldIterator::GetFieldName() const { return mrow_->fields_[index_].name; }

MysqlRow::MysqlFieldIterator MysqlRow::begin() { return {this, 0}; }

MysqlRow::MysqlFieldIterator MysqlRow::end() { return {this, num_fields_}; }

std::string_view MysqlRow::GetFieldData(unsigned int column_index) {
  return {row_[column_index], lengths_[column_index]};
}

bool MysqlRow::IsFieldNull(unsigned int column_index) {
  return row_[column_index] == nullptr;
}

std::vector<std::string> MysqlRow::GetFieldsName() {
  std::vector<std::string> fields_name;

  for (unsigned int i = 0; i < num_fields_; i++)
    fields_name.emplace_back(fields_[i].name);

  return fields_name;
}

unsigned int MysqlRow::NumFields() { return num_fields_; }

MysqlRow::MysqlRow(MYSQL_FIELD* fields,
                   MYSQL_ROW row,
                   unsigned long* length,
                   unsigned int num_field)
        : fields_(fields), row_(row), num_fields_(num_field) {

  for (unsigned int i = 0; i < num_field; i++) lengths_.push_back(length[i]);
}

}

