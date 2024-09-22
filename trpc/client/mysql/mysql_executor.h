#pragma once

#include <iostream>
#include <mysql/mysql.h>
#include <chrono>
#include <mutex>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>
#include "trpc/client/mysql/mysql_binder.h"
#include "trpc/client/mysql/mysql_statement.h"
#include "trpc/util/time.h"
#include "trpc/util/string_util.h"

struct MYSQL_RES;
struct MYSQL;
struct MYSQL_BIND;

namespace trpc::mysql {

class MysqlExecutor;

///@brief Just specialize class MysqlResults
class OnlyExec {};

class NativeString {};


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
///  prepare statement to execute SQL.
template <typename... Args>
class MysqlResults {
  friend class MysqlExecutor;

 public:
  MysqlResults() : affected_rows(0), has_value_(false) {
    if constexpr (is_native_string) {
      result_set = std::vector<std::vector<std::string>>();
    } else {
      result_set = std::vector<std::tuple<Args...>>();
    }
  }

  MysqlResults(const MysqlResultsOption& option) : MysqlResults() {
    option_ = option;
  }


  inline auto& GetResultSet() {
    if constexpr (is_native_string) {
      return std::get<std::vector<std::vector<std::string>>>(result_set);
    } else {
      return std::get<std::vector<std::tuple<Args...>>>(result_set);
    }
  }

  inline int GetResultSet(std::vector<std::vector<std::string>>& res) {
    if (!is_native_string || !has_value_) return -1;
    res = std::move(std::get<std::vector<std::vector<std::string>>>(result_set));
    has_value_ = false;
    return 0;
  }

  inline int GetResultSet(std::vector<std::tuple<Args...>>& res) {
    if (is_native_string || is_only_exec || !has_value_) return -1;
    res = std::move(std::get<std::vector<std::tuple<Args...>>>(result_set));
    has_value_ = false;
    return 0;
  }

  std::vector<std::vector<uint8_t>>& GetNullFlag() { return null_flags; }

  const MysqlResultsOption& GetOption() { return option_; }

  size_t GetAffectedRows() { return affected_rows; }

  bool IsSuccess() { return error_message.empty(); }

  void Clear() { null_flags.clear(); error_message.clear(); has_value_ = false; GetResultSet().clear(); }

 private:
  static constexpr bool is_only_exec = std::is_same_v<std::tuple<Args...>, std::tuple<OnlyExec>>;

  static constexpr bool is_native_string = std::is_same_v<std::tuple<Args...>, std::tuple<NativeString>>;

  MysqlResultsOption option_;

  std::variant<std::vector<std::tuple<Args...>>, std::vector<std::vector<std::string>>> result_set;

  // Null flags
  std::vector<std::vector<uint8_t>> null_flags;

  std::string error_message;

  size_t affected_rows;

//  bool error;

  bool has_value_;

  // Todo: Field Type
};





class Formatter {
 public:
  template<typename T>
  static T QuotesIfNeeded(const T& arg) {
    return arg;
  }

  template<size_t N>
  static std::string QuotesIfNeeded(const char (&arg)[N]) {
    return QuotesIfNeeded(static_cast<const char*>(arg));
  }

  static std::string QuotesIfNeeded(const std::string& arg) {
    return "'" + arg + "'";
  }

  static std::string QuotesIfNeeded(const char* const& arg) {
    return arg ? "'" + std::string(arg) + "'" : "NULL";
  }



  template<typename... Args>
  static std::string FormatQuery(const std::string& query, const Args&... args) {
    return util::FormatString(query, QuotesIfNeeded(args)...);
  }
};



/// @brief A MySQL connection class that wraps the MySQL C API.
/// @note This class is not thread-safe.
class MysqlExecutor {

  template<typename... OutputArgs>
  class QueryHandle {

   public:
    using DataBufferT = std::vector<std::vector<std::byte>>;
    using FlagBufferT = std::vector<uint8_t>;

    QueryHandle(MysqlResults<OutputArgs...>* mysql_results, MysqlStatement* statement,
                size_t field_count);

    MysqlResults<OutputArgs...>* mysql_results = nullptr;
    MysqlStatement* statement = nullptr;

    std::unique_ptr<std::vector<MYSQL_BIND>> output_binds;
    std::unique_ptr<DataBufferT> output_buffer;
    std::unique_ptr<std::vector<unsigned long>> output_length;
    std::unique_ptr<FlagBufferT> null_flag_buffer;


    // Indicate which column variable-length data. It will be used in MysqlExecutro::FetchTruncatedResults.
    // Only variable-length data column may be truncated.
    std::vector<size_t> dynamic_buffer_index;
   private:
    size_t dynamic_buffer_size_;

    template<std::size_t... Indices>
    void ResizeBuffers(std::index_sequence<Indices...>);

    void ResizeOutputBuffer();

  };

 public:
  ///@brief Init and connect to the mysql server.
  ///@param hostname
  ///@param username
  ///@param password
  ///@param database
  ///@param port Could be ignored
  MysqlExecutor(const char* const hostname, const char* const username, const char* const password,
                const char* const database, const uint16_t port = 0);

  ~MysqlExecutor();

  MysqlExecutor(const MysqlExecutor& rhs) = delete;
  MysqlExecutor(MysqlExecutor&& rhs) = delete;
  MysqlExecutor& operator=(const MysqlExecutor& rhs) = delete;
  MysqlExecutor& operator=(MysqlExecutor&& rhs) = delete;

  ///@brief Close the mysql conection and free the MYSQL*.
  void Close();

  ///@brief Executes an SQL query and retrieves all resulting rows, storing each row as a tuple.
  ///
  /// This function executes the provided SQL query with the specified input arguments.
  /// The results of the query are stored in the `results` vector, where each row is represented
  /// as a tuple containing the specified output types.
  ///
  ///@param mysql_results Each element in this vector is a tuple containing the values of a single row.
  ///@param query The SQL query to be executed as a string which uses "?" as
  /// placeholders (see [MySQL C API Documentation](https://dev.mysql.com/doc/c-api/8.0/en/mysql-stmt-prepare.html)).
  ///@param args The input arguments to be bound to the query placeholders.
  ///
  template <typename... InputArgs, typename... OutputArgs>
  bool QueryAll(MysqlResults<OutputArgs...>& mysql_results, const std::string& query, const InputArgs&... args);

  template <typename... InputArgs>
  bool Execute(MysqlResults<OnlyExec>& mysql_results, const std::string& query, const InputArgs&... args);

  bool Commit();

  bool Transaction();

  void RefreshAliveTime();

  uint64_t GetAliveTime();

 private:

  template <typename... InputArgs, typename... OutputArgs>
  bool QueryAllInternal(MysqlResults<OutputArgs...>& mysql_results, const std::string& query, const InputArgs&... args);

  template <typename... InputArgs>
  bool QueryAllInternal(MysqlResults<NativeString>& mysql_results, const std::string& query, const InputArgs&... args);

  ///@brief Executes an SQL
  ///@param query The SQL query to be executed as a string which uses "?" as placeholders.
  ///@param args The input arguments to be bound to the query placeholders.
  ///@return Affected rows.
  template <typename... InputArgs>
  int ExecuteInternal(const std::string& query, std::string& error_message, const InputArgs&... args);

  template <typename... InputArgs>
  void BindInputArgs(std::vector<MYSQL_BIND>& params, const InputArgs&... args);

  template <typename... OutputArgs>
  void BindOutputs(MysqlExecutor::QueryHandle<OutputArgs...>& handle);

  bool ExecuteStatement(std::vector<MYSQL_BIND>& output_binds, MysqlStatement& statement);

  bool ExecuteStatement(MysqlStatement& statement);

  template <typename... OutputArgs>
  bool FetchResults(MysqlExecutor::QueryHandle<OutputArgs...>& handle);

  template <typename... OutputArgs>
  bool FetchTruncatedResults(MysqlExecutor::QueryHandle<OutputArgs...>& handle);



  template<typename... Args>
  std::string FormatQuery(const std::string& query, const Args&... args) {
    return fmt::format(query, wrapWithQuotesIfString(args)...); // 自动为字符串参数添加引号
  }

  std::string ConvertPlaceholders(const std::string& sql);

  void FreeResult();

 private:
  static std::mutex mysql_mutex;
  MYSQL* mysql_;
  MYSQL_RES* res_;
  uint64_t m_alivetime;  // 初始化活跃时间
};



template<typename... OutputArgs>
MysqlExecutor::QueryHandle<OutputArgs...>::QueryHandle(MysqlResults<OutputArgs...>* mysql_results, MysqlStatement* statement,
                                                       size_t field_count)
                                          : mysql_results(mysql_results),
                                            statement(statement),
                                            dynamic_buffer_size_(mysql_results->GetOption().dynamic_buffer_init_size){

  output_binds = std::make_unique<std::vector<MYSQL_BIND>>(field_count);
  output_buffer = std::make_unique<DataBufferT>(field_count);
  null_flag_buffer = std::make_unique<FlagBufferT>(field_count);
  output_length = std::make_unique<std::vector<unsigned long>>(field_count);

  ResizeOutputBuffer();
}


template<typename... OutputArgs>
template<std::size_t... Indices>
void MysqlExecutor::QueryHandle<OutputArgs...>::ResizeBuffers(std::index_sequence<Indices...>) {
  ((std::is_same<OutputArgs, std::string>::value || std::is_same<OutputArgs, MysqlBlob>::value
    ? (output_buffer->at(Indices).resize(dynamic_buffer_size_),
      dynamic_buffer_index.push_back(Indices)): void()), ...);
}

template<typename... OutputArgs>
void MysqlExecutor::QueryHandle<OutputArgs...>::ResizeOutputBuffer() {
  ResizeBuffers(std::index_sequence_for<OutputArgs...>{});
}


template <typename... InputArgs, typename... OutputArgs>
bool MysqlExecutor::QueryAll(MysqlResults<OutputArgs...>& mysql_results, const std::string& query,
                             const InputArgs&... args) {
  // FreeResult();
  static_assert(!MysqlResults<OutputArgs...>::is_only_exec, "MysqlResults<OnlyExec> cannot be used with QueryAll.");

  if(!QueryAllInternal(mysql_results, query, args...))
    return false;

  mysql_results.has_value_ = true;
  return true;
}

template <typename... InputArgs>
bool MysqlExecutor::Execute(MysqlResults<OnlyExec>& mysql_results, const std::string& query, const InputArgs&... args) {
  mysql_results.affected_rows = ExecuteInternal(query, mysql_results.error_message, args...);
  return true;
}

template <typename... InputArgs>
void MysqlExecutor::BindInputArgs(std::vector<MYSQL_BIND>& params, const InputArgs&... args) {
  BindInputImpl(params, args...);
}

template <typename... OutputArgs>
void MysqlExecutor::BindOutputs(MysqlExecutor::QueryHandle<OutputArgs...>& handle) {
  BindOutputImpl<OutputArgs...>(*handle.output_binds, *handle.output_buffer, *handle.null_flag_buffer);
}

template <typename... InputArgs, typename... OutputArgs>
bool MysqlExecutor::QueryAllInternal(MysqlResults<OutputArgs...>& mysql_results,
                                     const std::string& query,
                                     const InputArgs&... args) {
  mysql_results.Clear();
  std::vector<MYSQL_BIND> input_binds;

  MysqlStatement stmt(mysql_);

  if(!stmt.Init(query)) {
    mysql_results.error_message = stmt.GetErrorMessage();
    stmt.CloseStatement();
    return false;
  }

  if(stmt.GetParamsCount() != sizeof...(InputArgs)) {
    mysql_results.error_message = util::FormatString("The query params count is {}, but you give {} InputputArgs.",
                                       stmt.GetParamsCount(), sizeof...(InputArgs));
    stmt.CloseStatement();
    return false;
  }

  if(stmt.GetFieldCount() != sizeof...(OutputArgs)) {
    mysql_results.error_message = util::FormatString("The query field count is {}, but you give {} OutputArgs.",
                                       stmt.GetFieldCount(), sizeof...(OutputArgs));
    stmt.CloseStatement();
    return false;
  }



  BindInputArgs(input_binds, args...);

  if (!stmt.BindParam(input_binds)) {
    mysql_results.error_message = stmt.GetErrorMessage();
    stmt.CloseStatement();
    return false;
  }


  QueryHandle handle = QueryHandle(&mysql_results, &stmt, stmt.GetFieldCount());


  BindOutputs<OutputArgs...>(handle);
  for (size_t i = 0; i < handle.output_binds->size(); i++) {
    handle.output_binds->at(i).length = &handle.output_length->at(i);
  }


  if(!ExecuteStatement(*handle.output_binds, stmt)) {
    mysql_results.error_message = stmt.GetErrorMessage();
    stmt.CloseStatement();
    return false;
  }


  if(!FetchResults(handle)) {
    mysql_results.error_message = stmt.GetErrorMessage();
    stmt.CloseStatement();
    return false;
  }


  return true;
}

template <typename... InputArgs>
bool MysqlExecutor::QueryAllInternal(MysqlResults<NativeString>& mysql_result,
                                     const std::string& query,
                                     const InputArgs&... args) {
  std::string query_str = Formatter::FormatQuery(ConvertPlaceholders(query), args...);
  MYSQL_ROW row;
  auto& results = mysql_result.GetResultSet();

  if (mysql_real_query(mysql_, query_str.c_str(), query_str.length())) {
    mysql_result.error_message =  mysql_error(mysql_);
    return false;
  }

  res_ = mysql_store_result(mysql_);
  if (res_ == nullptr) {
    mysql_result.error_message =  mysql_error(mysql_);
    return false;
  }

  unsigned long num_fields = mysql_num_fields(res_);

  while ((row = mysql_fetch_row(res_)) != nullptr) {
    unsigned long *lengths;
    results.emplace_back();
    mysql_result.null_flags.emplace_back(num_fields, false);
    lengths = mysql_fetch_lengths(res_);

    for(unsigned long i = 0; i < num_fields; i++) {
      if(row[i])
        results.back().emplace_back(row[i], lengths[i]);
      else {
        mysql_result.null_flags.back()[i] = true;
        results.back().emplace_back("");
      }
    }
  }

  FreeResult();

  return true;
}

template <typename... OutputArgs>
bool MysqlExecutor::FetchResults(MysqlExecutor::QueryHandle<OutputArgs...>& handle) {

  if(mysql_stmt_store_result(handle.statement->STMTPointer()) != 0)
    return false;

  int status = 0;
  auto& results = handle.mysql_results->GetResultSet();
  auto& null_flags = handle.mysql_results->GetNullFlag();
  while (true) {
    status = mysql_stmt_fetch(handle.statement->STMTPointer());
    if (status == 1 || status == MYSQL_NO_DATA) break;

    if (status == MYSQL_DATA_TRUNCATED) {
      // To Do
      // https://dev.mysql.com/doc/c-api/8.0/en/mysql-stmt-fetch.html
      FetchTruncatedResults(handle);

    }

    std::tuple<OutputArgs...> row_res;
    SetResultTuple(row_res, *handle.output_binds);
    results.push_back(std::move(row_res));
    null_flags.emplace_back(*handle.null_flag_buffer);
  }

  if (status == 1)
    return false;
  return true;
}

template<typename... OutputArgs>
bool MysqlExecutor::FetchTruncatedResults(MysqlExecutor::QueryHandle<OutputArgs...> &handle) {

  for(size_t i : handle.dynamic_buffer_index) {
    MYSQL_BIND& bind = handle.output_binds->at(i);
    size_t data_real_size = *(handle.output_binds->at(i).length);
    size_t buffer_old_size = handle.output_buffer->at(i).size();

    if(data_real_size <= buffer_old_size)
      continue;

    handle.output_buffer->at(i).resize(data_real_size);
    bind.buffer_length = data_real_size;
    bind.buffer = handle.output_buffer->at(i).data() + buffer_old_size;

    if(mysql_stmt_fetch_column(handle.statement->STMTPointer(), &bind, i, buffer_old_size) != 0)
      return false;

    bind.buffer = handle.output_buffer->at(i).data();
  }
  return true;
}

template <typename... InputArgs>
int MysqlExecutor::ExecuteInternal(const std::string& query, std::string& error_message, const InputArgs&... args) {
  MysqlStatement stmt(mysql_);
  std::vector<MYSQL_BIND> input_binds;

  if(!stmt.Init(query)) {
    error_message = stmt.GetErrorMessage();
    stmt.CloseStatement();
    return false;
  }
  BindInputArgs(input_binds, args...);

  if (!stmt.BindParam(input_binds)) {
    error_message = stmt.GetErrorMessage();
    stmt.CloseStatement();
    return false;
  }

  if(!ExecuteStatement(stmt)) {
    error_message = stmt.GetErrorMessage();
    stmt.CloseStatement();
    return false;
  }

  size_t affected_row = mysql_affected_rows(mysql_);

  return affected_row;
}



}  // namespace trpc::mysql
