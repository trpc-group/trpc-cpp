//
// Created by kosmos on 9/23/24.
//

#pragma once

#include <string>
#include <tuple>
#include <variant>
#include <vector>
#include "mysqlclient/mysql.h"
#include "mysql_binder.h"
#include "mysql_binder.h"


namespace trpc::mysql {

class MysqlResultsOption {
 public:
  size_t dynamic_buffer_init_size = 64;
};


///@brief Just specialize class MysqlResults
class OnlyExec {};

class NativeString {};

/// @brief Mode for `MysqlResults`
enum class MysqlResultsMode {
  BindType,      // Bind query result data to tuples.
  OnlyExec,      // For SQL which will not return a result set data.
  NativeString,  // Return result data as vector of string_view.
};


template <typename... Args>
struct ResultSetMapper {
  using type = std::vector<std::tuple<Args...>>;
  static constexpr MysqlResultsMode mode = MysqlResultsMode::BindType;
};


template <>
struct ResultSetMapper<NativeString> {
  using type = std::vector<std::vector<std::string_view>>;
  static constexpr MysqlResultsMode mode = MysqlResultsMode::NativeString;
};

template <>
struct ResultSetMapper<OnlyExec> {
  using type = std::vector<std::tuple<OnlyExec>>;
  static constexpr MysqlResultsMode mode = MysqlResultsMode::OnlyExec;

};


///
///@brief A class used to store the results of a MySQL query executed by the MysqlConnection class.
///
/// The class template parameter `Args...` is used to define the types of data stored in the result set.
///
///- If `Args...` is `OnlyExec`, the class is intended for operations
///  that execute without returning a result set (e.g., INSERT, UPDATE).
///
///- If `Args...` is `NativeString`, the class handles operations that
///  return a `vector<vector<string>>>` result set.
///
///- If `Args...` includes common data types (e.g., int, std::string),
///  the class handles operations that return a `vector<tuple<Args...>>` result set.
///  The template args need to match the query field.  Notice that in this case, it will use
///  prepare statement to execute SQL. Pay attention to the bind type args. If the bind type args missmatch
///  the MySQL type in the table, it will be an undefined behaviour and will not raise an error message.
template <typename... Args>
class MysqlResults {
  friend class MysqlExecutor;

 public:

  static constexpr MysqlResultsMode mode = ResultSetMapper<Args...>::mode;

  using ResultSetType = typename ResultSetMapper<Args...>::type;

 public:
  MysqlResults();

  explicit MysqlResults(const MysqlResultsOption& option);

  MysqlResults(MysqlResults&& other) noexcept;

  MysqlResults& operator=(MysqlResults&& other) noexcept;

  MysqlResults(const MysqlResults& other) = delete;

  MysqlResults& operator=(const MysqlResults&) = delete;

  ~MysqlResults();

  auto& MutableResultSet();

  const auto& ResultSet() const;

  template <typename T>
  bool GetResultSet(T& res);

  const std::vector<std::vector<uint8_t>>& GetNullFlag();

  const std::vector<std::string>& GetFieldsName() const;

  const MysqlResultsOption& GetOption();

  size_t GetAffectedRowNum() const;

  bool IsValueNull(size_t row_index, size_t col_index) const;

  bool OK() const;

  void Clear();

  const std::string& GetErrorMessage();

 private:
  void SetRawMysqlRes(MYSQL_RES* res);

  void SetFieldsName(MYSQL_RES* res);

  bool CheckFieldsType(MYSQL_RES* res);

  size_t SetAffectedRows(size_t n_rows);

  std::string& SetErrorMessage(const std::string& message);

  std::string& SetErrorMessage(std::string&& message);

 private:
  MysqlResultsOption option_;

  ResultSetType result_set;

  std::vector<std::string> fields_name_;

  std::vector<std::vector<uint8_t>> null_flags;

  std::string error_message;

  size_t affected_rows;

  bool has_value_;

  /// @brief For NativeString, it will represent real data.
  MYSQL_RES* mysql_res_;
};

template <typename... Args>
const std::vector<std::string>& MysqlResults<Args...>::GetFieldsName() const {
  return fields_name_;
}

template <typename... Args>
void MysqlResults<Args...>::SetFieldsName(MYSQL_RES* res) {
  if (!res) return;

  MYSQL_FIELD* fields_meta = mysql_fetch_fields(res);
  unsigned long fields_num = mysql_num_fields(res);

  for (unsigned long i = 0; i < fields_num; ++i)
    fields_name_.emplace_back(fields_meta[i].name);
}

template <typename... Args>
bool MysqlResults<Args...>::CheckFieldsType(MYSQL_RES* res) {
  MYSQL_FIELD* fields_meta = mysql_fetch_fields(res);

  unsigned long i = 0;
  std::vector<unsigned long> failed_index;
  ((OutputTypeValid<Args>(fields_meta[i].type) ? (void)i++ : failed_index.push_back(i++)), ...);

  if(!failed_index.empty()) {
    std::string error = "Bind output type warning for fields: (";
    error += std::string(fields_meta[failed_index[0]].name);
    for(i = 1; i < failed_index.size(); i++)
      error.append(", ").append(fields_meta[failed_index[i]].name);
    error.append(").");
    error_message.append(error);
    return false;
  }
  return true;
}

template <typename... Args>
MysqlResults<Args...>& MysqlResults<Args...>::operator=(MysqlResults&& other) noexcept {
  if (this != &other) {
    option_ = std::move(other.option_);
    result_set = std::move(other.result_set);
    fields_name_ = std::move(other.fields_name_);
    null_flags = std::move(other.null_flags);
    error_message = std::move(other.error_message);
    affected_rows = other.affected_rows;
    has_value_ = other.has_value_;
    mysql_res_ = other.mysql_res_;

    other.mysql_res_ = nullptr;
  }
  return *this;
}

template <typename... Args>
MysqlResults<Args...>::MysqlResults(MysqlResults&& other) noexcept
    : option_(std::move(other.option_)),
      result_set(std::move(other.result_set)),
      fields_name_(std::move(other.fields_name_)),
      null_flags(std::move(other.null_flags)),
      error_message(std::move(other.error_message)),
      affected_rows(other.affected_rows),
      has_value_(other.has_value_),
      mysql_res_(other.mysql_res_) {
  other.mysql_res_ = nullptr;
}

template <typename... Args>
void MysqlResults<Args...>::SetRawMysqlRes(MYSQL_RES* res) {
  mysql_res_ = res;
}

template <typename... Args>
MysqlResults<Args...>::~MysqlResults() {
  if (mysql_res_) {
    mysql_free_result(mysql_res_);
    mysql_res_ = nullptr;
  }
}

template <typename... Args>
std::string& MysqlResults<Args...>::SetErrorMessage(std::string&& message) {
  error_message = std::move(message);
  return error_message;
}

template <typename... Args>
std::string& MysqlResults<Args...>::SetErrorMessage(const std::string& message) {
  error_message = message;
  return error_message;
}

template <typename... Args>
const std::string& MysqlResults<Args...>::GetErrorMessage() {
  return error_message;
}

template <typename... Args>
MysqlResults<Args...>::MysqlResults() : affected_rows(0), has_value_(false), mysql_res_(nullptr) {
  // dummy
}

template <typename... Args>
MysqlResults<Args...>::MysqlResults(const MysqlResultsOption& option) : MysqlResults() {
  option_ = option;
}

template <typename... Args>
auto& MysqlResults<Args...>::MutableResultSet() {
  return result_set;
}

template <typename... Args>
const auto& MysqlResults<Args...>::ResultSet() const {
  return result_set;
}

template <typename... Args>
template <typename T>
bool MysqlResults<Args...>::GetResultSet(T& res) {
  if (!has_value_) return false;


  if constexpr (mode == MysqlResultsMode::NativeString) {
    res.clear();
    for (const auto& row : result_set) {
      res.emplace_back(row.begin(), row.end());
    }

    return true;
  } else {
    res = std::move(result_set);
    has_value_ = false;
    return true;
  }
}

template <typename... Args>
const std::vector<std::vector<uint8_t>>& MysqlResults<Args...>::GetNullFlag() {
  return null_flags;
}

template <typename... Args>
const MysqlResultsOption& MysqlResults<Args...>::GetOption() {
  return option_;
}

template <typename... Args>
size_t MysqlResults<Args...>::GetAffectedRowNum() const {
  return affected_rows;
}

template <typename... Args>
size_t MysqlResults<Args...>::SetAffectedRows(size_t n_rows) {
  affected_rows = n_rows;
  return affected_rows;
}

template <typename... Args>
bool MysqlResults<Args...>::IsValueNull(size_t row_index, size_t col_index) const {
  return null_flags[row_index][col_index];
}


template <typename... Args>
bool MysqlResults<Args...>::OK() const {
  return error_message.empty();
}

template <typename... Args>
void MysqlResults<Args...>::Clear() {
  null_flags.clear();
  error_message.clear();
  fields_name_.clear();
  has_value_ = false;
  affected_rows = 0;
  MutableResultSet().clear();

  if (mysql_res_) {
    mysql_free_result(mysql_res_);
    mysql_res_ = nullptr;
  }
}

}  // namespace trpc::mysql
