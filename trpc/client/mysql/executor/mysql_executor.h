#pragma once

#include "mysqlclient/mysql.h"
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>
#include "trpc/client/mysql/executor/mysql_binder.h"
#include "trpc/client/mysql/executor/mysql_results.h"
#include "trpc/client/mysql/executor/mysql_statement.h"
#include "trpc/util/ref_ptr.h"
#include "trpc/util/string_util.h"
#include "trpc/util/time.h"

namespace trpc::mysql {

/// @brief Converts placeholders when the executor is not using the prepared statement API.
/// There may be potential bugs, and currently, it does not support types from mysql_type.h.
class Formatter {
 public:
  template <typename T>
  static T SpecialConvert(const T& arg) {
    return arg;
  }

  template <size_t N>
  static std::string SpecialConvert(const char (&arg)[N]) {
    return SpecialConvert(static_cast<const char*>(arg));
  }

  static std::string ConvertPlaceHolder(const std::string& sql) {
    std::string result;
    size_t len = sql.length();

    for (size_t i = 0; i < len; ++i) {
      if (sql[i] == '?' && (i == 0 || sql[i - 1] != '\\')) {
        result += "{}";
      } else {
        result += sql[i];
      }
    }

    return result;
  }

  static std::string SpecialConvert(const std::string& arg) { return "'" + arg + "'"; }

  static std::string SpecialConvert(const char* const& arg) { return arg ? "'" + std::string(arg) + "'" : "NULL"; }

  static std::string SpecialConvert(const MysqlTime& arg) { return "'" + arg.ToString() + "'"; }

  template <typename... Args>
  static std::string FormatQuery(const std::string& query, const Args&... args) {
    return util::FormatString(ConvertPlaceHolder(query), SpecialConvert(args)...);
  }
};

/// @brief A MySQL connection class that wraps the MySQL C API.
/// @note This class is not thread-safe. Ensure exclusive ownership during queries.
class MysqlExecutor : public RefCounted<MysqlExecutor> {

  template <typename... OutputArgs>
  class QueryHandle {
   public:
    using DataBufferT = std::vector<std::vector<std::byte>>;
    using FlagBufferT = std::vector<uint8_t>;

    QueryHandle(MysqlResults<OutputArgs...>* mysql_results, MysqlStatement* statement, size_t field_count);

    MysqlResults<OutputArgs...>* mysql_results = nullptr;
    MysqlStatement* statement = nullptr;

    std::unique_ptr<std::vector<MYSQL_BIND>> output_binds;
    std::unique_ptr<DataBufferT> output_buffer;
    std::unique_ptr<std::vector<unsigned long>> output_length;
    std::unique_ptr<FlagBufferT> null_flag_buffer;

    // Indicate which column are variable-length data. It will be used in MysqlExecutro::FetchTruncatedResults.
    // Only variable-length data column may be truncated.
    std::vector<size_t> dynamic_buffer_index;

   private:

    template <std::size_t... Indices>
    void ResizeBuffers(std::index_sequence<Indices...>);

    void ResizeOutputBuffer();

   private:
    size_t dynamic_buffer_size_;    // the initial buffer size of variant size type (string, blob)
  };

 public:

  MysqlExecutor(const std::string& hostname, const std::string& username, const std::string& password,
                const std::string& database, uint16_t port = 0, const std::string& char_set = "utf8mb4");

  ~MysqlExecutor();

  MysqlExecutor(const MysqlExecutor& rhs) = delete;
  MysqlExecutor(MysqlExecutor&& rhs) = delete;
  MysqlExecutor& operator=(const MysqlExecutor& rhs) = delete;
  MysqlExecutor& operator=(MysqlExecutor&& rhs) = delete;

  ///@brief Connect to mysql server.
  bool Connect();

  ///@brief Close the mysql connection and free the MYSQL*.
  void Close();

  ///@brief set auto commit for current session
  ///@param mode true if enable auto commit else false
  ///@return true if no error
  bool Autocommit(bool mode);

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

  ///@brief Same as QueryAll, but does not fetch result set, only returns affected rows
  template <typename... InputArgs>
  bool Execute(MysqlResults<OnlyExec>& mysql_results, const std::string& query, const InputArgs&... args);

  std::string GetErrorMessage();

  void RefreshAliveTime();

  uint64_t GetAliveTime();

  bool IsConnectionValid();

  bool Reconnect();

  bool StartReconnect();

  void SetExecutorId(uint64_t eid);

  uint64_t GetExecutorId() const;

  std::string GetIp() const;

  uint16_t  GetPort() const;


 private:

  ///@note: Only this overload will use mysql prepared statement api.
  template <typename... InputArgs, typename... OutputArgs>
  bool QueryAllInternal(MysqlResults<OutputArgs...>& mysql_results, const std::string& query, const InputArgs&... args);

  template <typename... InputArgs>
  bool QueryAllInternal(MysqlResults<NativeString>& mysql_results, const std::string& query, const InputArgs&... args);

  ///@brief Executes an SQL with prepareed statement.
  ///@param query The SQL query to be executed as a string which uses "?" as placeholders.
  ///@param args The input arguments to be bound to the query placeholders.
  ///@details Use prepared statements of mysql api.
  ///@return Affected rows.
  template <typename... InputArgs>
  size_t ExecuteInternal(const std::string& query, MysqlResults<OnlyExec>& mysql_results, const InputArgs&... args);

  ///@brief This overload exists because some SQLs are not supported in mysql prepared statement api.
  size_t ExecuteInternal(const std::string& query, MysqlResults<OnlyExec>& mysql_results);

  template <typename... InputArgs>
  void BindInputArgs(std::vector<MYSQL_BIND>& params, const InputArgs&... args);

  template <typename... OutputArgs>
  void BindOutputs(MysqlExecutor::QueryHandle<OutputArgs...>& handle);

  ExecuteStatus ExecuteStatement(std::vector<MYSQL_BIND>& output_binds, MysqlStatement& statement);

  ExecuteStatus ExecuteStatement(MysqlStatement& statement);

  template <typename... OutputArgs>
  bool FetchResults(MysqlExecutor::QueryHandle<OutputArgs...>& handle);

  template <typename... OutputArgs>
  bool FetchTruncatedResults(MysqlExecutor::QueryHandle<OutputArgs...>& handle);


 private:
  /// Just protects the `mysql_init` api
  static std::mutex mysql_mutex;

  bool is_connected;

  bool auto_commit_{true};

  MYSQL* mysql_;

  uint64_t m_alivetime;

  uint64_t executor_id_{0};

  std::string hostname_;

  std::string username_;

  std::string password_;

  std::string database_;

  uint16_t port_;
};

template <typename... OutputArgs>
MysqlExecutor::QueryHandle<OutputArgs...>::QueryHandle(MysqlResults<OutputArgs...>* mysql_results,
                                                       MysqlStatement* statement, size_t field_count)
    : mysql_results(mysql_results),
      statement(statement),
      dynamic_buffer_size_(mysql_results->GetOption().dynamic_buffer_init_size) {
  output_binds = std::make_unique<std::vector<MYSQL_BIND>>(field_count);
  output_buffer = std::make_unique<DataBufferT>(field_count);
  null_flag_buffer = std::make_unique<FlagBufferT>(field_count);
  output_length = std::make_unique<std::vector<unsigned long>>(field_count);

  ResizeOutputBuffer();
}

template <typename... OutputArgs>
template <std::size_t... Indices>
void MysqlExecutor::QueryHandle<OutputArgs...>::ResizeBuffers(std::index_sequence<Indices...>) {
  ((std::is_same<OutputArgs, std::string>::value || std::is_same<OutputArgs, MysqlBlob>::value
        ? (output_buffer->at(Indices).resize(dynamic_buffer_size_), dynamic_buffer_index.push_back(Indices))
        : void()),
   ...);
}

template <typename... OutputArgs>
void MysqlExecutor::QueryHandle<OutputArgs...>::ResizeOutputBuffer() {
  ResizeBuffers(std::index_sequence_for<OutputArgs...>{});
}

template <typename... InputArgs, typename... OutputArgs>
bool MysqlExecutor::QueryAll(MysqlResults<OutputArgs...>& mysql_results, const std::string& query,
                             const InputArgs&... args) {
  //  static_assert(!MysqlResults<OutputArgs...>::is_only_exec, "MysqlResults<OnlyExec> cannot be used with QueryAll.");

  static_assert(MysqlResults<OutputArgs...>::mode != MysqlResultsMode::OnlyExec,
                "MysqlResults<OnlyExec> cannot be used with QueryAll.");

  if (!QueryAllInternal(mysql_results, query, args...)) return false;

  mysql_results.has_value_ = true;
  return true;
}

template <typename... InputArgs>
bool MysqlExecutor::Execute(MysqlResults<OnlyExec>& mysql_results, const std::string& query, const InputArgs&... args) {
  size_t affected_rows = ExecuteInternal(query, mysql_results, args...);
  mysql_results.SetAffectedRows(affected_rows);
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
bool MysqlExecutor::QueryAllInternal(MysqlResults<OutputArgs...>& mysql_results, const std::string& query,
                                     const InputArgs&... args) {
  mysql_results.Clear();
  std::vector<MYSQL_BIND> input_binds;

  MysqlStatement stmt(mysql_);

  if (!stmt.Init(query)) {
    mysql_results.error_message = stmt.GetErrorMessage();
    stmt.CloseStatement();
    return false;
  }

  if (stmt.GetParamsCount() != sizeof...(InputArgs)) {
    mysql_results.error_message = util::FormatString("The query params count is {}, but you give {} InputputArgs.",
                                                     stmt.GetParamsCount(), sizeof...(InputArgs));
    stmt.CloseStatement();
    return false;
  }

  if (stmt.GetFieldCount() != sizeof...(OutputArgs)) {
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

  auto status = ExecuteStatement(*handle.output_binds, stmt);
  if (!status.success) {
    mysql_results.error_message = status.error_message;
    stmt.CloseStatement();
    return false;
  }

  if (!FetchResults(handle)) {
    mysql_results.error_message = stmt.GetErrorMessage();
    stmt.CloseStatement();
    return false;
  }

  mysql_results.SetFieldsName(stmt.GetResultsMeta());
  mysql_results.CheckFieldsType(stmt.GetResultsMeta());
  stmt.CloseStatement();
  return true;
}

template <typename... InputArgs>
bool MysqlExecutor::QueryAllInternal(MysqlResults<NativeString>& mysql_result, const std::string& query,
                                     const InputArgs&... args) {
  mysql_result.Clear();
  std::string query_str = Formatter::FormatQuery(query, args...);
  MYSQL_ROW row;
  auto& results = mysql_result.MutableResultSet();

  if (mysql_real_query(mysql_, query_str.c_str(), query_str.length())) {
    mysql_result.SetErrorMessage(GetErrorMessage());
    return false;
  }

  MYSQL_RES* res_ptr = mysql_store_result(mysql_);
  if (res_ptr == nullptr) {
    mysql_result.SetErrorMessage(GetErrorMessage());
    return false;
  }

  unsigned long num_fields = mysql_num_fields(res_ptr);

  while ((row = mysql_fetch_row(res_ptr)) != nullptr) {
    unsigned long* lengths;
    results.emplace_back();
    mysql_result.null_flags.emplace_back(num_fields, false);
    lengths = mysql_fetch_lengths(res_ptr);

    for (unsigned long i = 0; i < num_fields; i++) {
      if (row[i])
        results.back().emplace_back(row[i], lengths[i]);
      else {
        mysql_result.null_flags.back()[i] = true;
        results.back().emplace_back("");
      }
    }
  }

  mysql_result.SetRawMysqlRes(res_ptr);
  mysql_result.SetFieldsName(res_ptr);
  return true;
}

template <typename... OutputArgs>
bool MysqlExecutor::FetchResults(MysqlExecutor::QueryHandle<OutputArgs...>& handle) {
  if (mysql_stmt_store_result(handle.statement->STMTPointer()) != 0) return false;

  int status = 0;
  auto& results = handle.mysql_results->MutableResultSet();
  auto& res_null_flags = handle.mysql_results->null_flags;
  while (true) {
    status = mysql_stmt_fetch(handle.statement->STMTPointer());
    if (status == 1 || status == MYSQL_NO_DATA) break;

    if (status == MYSQL_DATA_TRUNCATED) {
      // https://dev.mysql.com/doc/c-api/8.0/en/mysql-stmt-fetch.html
      FetchTruncatedResults(handle);
    }

    std::tuple<OutputArgs...> row_res;
    SetResultTuple(row_res, *handle.output_binds);
    results.push_back(std::move(row_res));
    res_null_flags.emplace_back(*handle.null_flag_buffer);
  }

  if (status == 1) return false;
  return true;
}

template <typename... OutputArgs>
bool MysqlExecutor::FetchTruncatedResults(MysqlExecutor::QueryHandle<OutputArgs...>& handle) {
  for (size_t i : handle.dynamic_buffer_index) {
    MYSQL_BIND& bind = handle.output_binds->at(i);
    size_t data_real_size = *(handle.output_binds->at(i).length);
    size_t buffer_old_size = handle.output_buffer->at(i).size();

    if (data_real_size <= buffer_old_size) continue;

    handle.output_buffer->at(i).resize(data_real_size);
    bind.buffer_length = data_real_size;
    bind.buffer = handle.output_buffer->at(i).data() + buffer_old_size;

    if (mysql_stmt_fetch_column(handle.statement->STMTPointer(), &bind, i, buffer_old_size) != 0) return false;

    bind.buffer = handle.output_buffer->at(i).data();
  }
  return true;
}

template <typename... InputArgs>
size_t MysqlExecutor::ExecuteInternal(const std::string& query, MysqlResults<OnlyExec>& mysql_results,
                                      const InputArgs&... args) {
  mysql_results.Clear();
  MysqlStatement stmt(mysql_);
  std::vector<MYSQL_BIND> input_binds;

  if (!stmt.Init(query)) {
    mysql_results.SetErrorMessage(stmt.GetErrorMessage());
    stmt.CloseStatement();
    return 0;
  }
  BindInputArgs(input_binds, args...);

  if (!stmt.BindParam(input_binds)) {
    mysql_results.SetErrorMessage(stmt.GetErrorMessage());
    stmt.CloseStatement();
    return 0;
  }

  auto status = ExecuteStatement(stmt);
  if (!status.success) {
    mysql_results.SetErrorMessage(status.error_message);
    stmt.CloseStatement();
    return 0;
  }

  size_t affected_row = mysql_affected_rows(mysql_);

  stmt.CloseStatement();
  return affected_row;
}

}  // namespace trpc::mysql
