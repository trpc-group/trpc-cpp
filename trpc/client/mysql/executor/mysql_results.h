//
// Created by kosmos on 9/23/24.
//

#ifndef TRPC_CPP_MYSQL_RESULTS_H
#define TRPC_CPP_MYSQL_RESULTS_H

#include <mysql/mysql.h>
#include <vector>
#include <string>
#include <tuple>
#include <variant>

namespace trpc::mysql {


/// @brief This is an abstract representation of a row pointer and does not contain any actual data.
/// Therefore, MysqlResult<IterMode> will not create or store MysqlRow instances.
/// The operator* of MysqlRowIterator does not return a reference to MysqlRow.
class MysqlRow {
 public:

  class MysqlFieldIterator {
   public:
    MysqlFieldIterator(MysqlRow* mrow, unsigned int index) : mrow_(mrow), index_(index) {}

    MysqlFieldIterator& operator++() {
      if(index_ < mrow_->num_fields_)
        ++index_;
      return *this;
    }

    bool operator!=(const MysqlFieldIterator& other) const {
      return index_ != other.index_ || mrow_ != other.mrow_;
    }

    bool operator==(const MysqlFieldIterator& other) const {
      return index_ == other.index_ && mrow_ == other.mrow_;
    }

    std::string operator*() const {
      if (!mrow_ || index_ >= mrow_->num_fields_) {
        return "";
      }
      return mrow_->row_[index_] ? std::string(mrow_->row_[index_], mrow_->lengths_[index_]) : "";
    }

    bool IsNull() {
      return mrow_->row_[index_] == nullptr;
    }

    std::string GetFieldName() const {
      return mrow_->fields_[index_].name;
    }


   private:
    MysqlRow* mrow_;
    unsigned int index_;
  };



  MysqlFieldIterator begin() {
    return {this, 0};
  }

  MysqlFieldIterator end() {
    return {this, num_fields_};
  }

  std::string GetFieldData(unsigned int column_index) {
    return {row_[column_index], lengths_[column_index]};
  }

  std::vector<std::string> GetFieldsName() {
    std::vector<std::string> fields_name;

    for(unsigned int i = 0; i < num_fields_; i++)
      fields_name.emplace_back(fields_[i].name);

    return fields_name;
  }

  unsigned int NumFields() {
    return num_fields_;
  }

  MysqlRow(MYSQL_FIELD* fields, MYSQL_ROW row, unsigned long* length, unsigned int num_field)
          : fields_(fields), row_(row), lengths_(length), num_fields_(num_field){

  }

 private:
  MYSQL_FIELD* fields_;
  MYSQL_ROW row_;
  unsigned long* lengths_;
  unsigned int num_fields_;
};



///@brief Just specialize class MysqlResults
class OnlyExec {
};

class NativeString {
};

class IterMode {

};


class MysqlResultsOption {
 public:
  bool use_string_view = false;

  size_t dynamic_buffer_init_size = 64;
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
template<typename... Args>
class MysqlResults {
  friend class MysqlExecutor;
 public:
  static constexpr bool is_only_exec = std::is_same_v<std::tuple<Args...>, std::tuple<OnlyExec>>;

  static constexpr bool is_native_string = std::is_same_v<std::tuple<Args...>, std::tuple<NativeString>>;

  static constexpr bool is_iter_mode = std::is_same_v<std::tuple<Args...>, std::tuple<IterMode>>;


 public:
  MysqlResults();

  explicit MysqlResults(const MysqlResultsOption &option);

  MysqlResults(MysqlResults&& other) noexcept;

  MysqlResults& operator=(MysqlResults&& other) noexcept;

  MysqlResults(const MysqlResults& other) = delete;

  MysqlResults& operator=(const  MysqlResults&) = delete;


  ~MysqlResults();

  void SetRawMysqlRes(MYSQL_RES* res);

  auto &GetResultSet();

  int GetResultSet(std::vector<std::vector<std::string>> &res);

  int GetResultSet(std::vector<std::tuple<Args...>> &res);

  std::vector<std::vector<uint8_t>> &GetNullFlag();

  const MysqlResultsOption &GetOption();

  size_t GetAffectedRows() const;

  size_t SetAffectedRows(size_t n_rows);

  bool IsSuccess() const;

  void Clear();

  const std::string &GetErrorMessage();

  std::string &SetErrorMessage(const std::string &message);

  std::string &SetErrorMessage(std::string &&message);

  class MysqlRowIterator {
   public:
    MysqlRowIterator(MYSQL_RES* res, MYSQL_ROW row, unsigned long* lengths, size_t row_index) :
                  result_(res), row_(row), lengths_(lengths), current_row_index_(row_index){
    }

    MysqlRowIterator& operator++() {
      row_ = mysql_fetch_row(result_);
      lengths_ = mysql_fetch_lengths(result_);
      ++current_row_index_;
      return *this;
    }


    MysqlRow operator*() {
      return {mysql_fetch_fields(result_), row_, lengths_, mysql_num_fields(result_)};
    }

    bool operator!=(const MysqlRowIterator& other) const {
      return row_ != other.row_;
    }

   private:
    MYSQL_RES* result_;
    MYSQL_ROW row_;
    unsigned long* lengths_;
    size_t current_row_index_;
  };


  template <typename T = void, typename = std::enable_if_t<is_iter_mode, T>>
  MysqlRowIterator begin() const {
    mysql_data_seek(mysql_res_, 0);
    MYSQL_ROW row = mysql_fetch_row(mysql_res_);
    unsigned long* lengths = mysql_fetch_lengths(mysql_res_);
    return MysqlRowIterator(mysql_res_, row, lengths, 0);
  }


  MysqlRowIterator end() const {
    return MysqlRowIterator(mysql_res_, nullptr, nullptr, mysql_num_rows(mysql_res_));
  }

 private:

  MysqlResultsOption option_;

  std::variant<std::vector<std::tuple<Args...>>, std::vector<std::vector<std::string>>> result_set;

  std::vector<std::vector<uint8_t>> null_flags;

  std::string error_message;

  size_t affected_rows;

  bool has_value_;

  MYSQL_RES* mysql_res_;



  // Todo: Field Type
};

template<typename... Args>
MysqlResults<Args...>& MysqlResults<Args...>::operator=(MysqlResults &&other) noexcept {
  if (this != &other) {
    option_ = std::move(other.option_);
    result_set = std::move(other.result_set);
    null_flags = std::move(other.null_flags);
    error_message = std::move(other.error_message);
    affected_rows = other.affected_rows;
    has_value_ = other.has_value_;
    mysql_res_ = other.mysql_res_;

    other.mysql_res_ = nullptr;
  }
  return *this;
}

template<typename... Args>
MysqlResults<Args...>::MysqlResults(MysqlResults&& other) noexcept
        : option_(std::move(other.option_)),
          result_set(std::move(other.result_set)),
          null_flags(std::move(other.null_flags)),
          error_message(std::move(other.error_message)),
          affected_rows(other.affected_rows),
          has_value_(other.has_value_),
          mysql_res_(other.mysql_res_)
{
  other.mysql_res_ = nullptr;
}


template<typename... Args>
void MysqlResults<Args...>::SetRawMysqlRes(MYSQL_RES* res) {
  mysql_res_ = res;
}

template<typename... Args>
MysqlResults<Args...>::~MysqlResults() {
  if(mysql_res_) {
    mysql_free_result(mysql_res_);
    mysql_res_ = nullptr;
  }
}


template<typename... Args>
std::string &MysqlResults<Args...>::SetErrorMessage(std::string &&message) {
  error_message = std::move(message);
  return error_message;
}

template<typename... Args>
std::string &MysqlResults<Args...>::SetErrorMessage(const std::string &message) {
  error_message = message;
  return error_message;
}

template<typename... Args>
const std::string &MysqlResults<Args...>::GetErrorMessage() {
  return error_message;
}

template<typename... Args>
MysqlResults<Args...>::MysqlResults() : affected_rows(0), has_value_(false), mysql_res_(nullptr) {
  if constexpr (is_native_string) {
    result_set = std::vector<std::vector<std::string>>();
  } else {
    result_set = std::vector<std::tuple<Args...>>();
  }
}

template<typename... Args>
MysqlResults<Args...>::MysqlResults(const MysqlResultsOption &option) : MysqlResults() {
  option_ = option;
}

template<typename... Args>
auto &MysqlResults<Args...>::GetResultSet() {
  if constexpr (is_native_string) {
    return std::get<std::vector<std::vector<std::string>>>(result_set);
  } else {
    return std::get<std::vector<std::tuple<Args...>>>(result_set);
  }
}

template<typename... Args>
int MysqlResults<Args...>::GetResultSet(std::vector<std::vector<std::string>> &res) {
  if (!is_native_string || !has_value_) return -1;
  res = std::move(std::get<std::vector<std::vector<std::string>>>(result_set));
  has_value_ = false;
  return 0;
}

template<typename... Args>
int MysqlResults<Args...>::GetResultSet(std::vector<std::tuple<Args...>> &res) {
  if (is_native_string || is_only_exec || !has_value_) return -1;
  res = std::move(std::get<std::vector<std::tuple<Args...>>>(result_set));
  has_value_ = false;
  return 0;
}

template<typename... Args>
std::vector<std::vector<uint8_t>> &MysqlResults<Args...>::GetNullFlag() { return null_flags; }

template<typename... Args>
const MysqlResultsOption &MysqlResults<Args...>::GetOption() { return option_; }

template<typename... Args>
size_t MysqlResults<Args...>::GetAffectedRows() const { return affected_rows; }

template<typename... Args>
size_t MysqlResults<Args...>::SetAffectedRows(size_t n_rows) {
  affected_rows = n_rows;
  return affected_rows;
}

template<typename... Args>
bool MysqlResults<Args...>::IsSuccess() const { return error_message.empty(); }

template<typename... Args>
void MysqlResults<Args...>::Clear() {
  null_flags.clear();
  error_message.clear();
  has_value_ = false;
  GetResultSet().clear();
}

}

#endif //TRPC_CPP_MYSQL_RESULTS_H
