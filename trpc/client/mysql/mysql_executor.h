#pragma once

#include <iostream>
using namespace std;
#include <mysql/mysql.h>
#include <chrono>
using namespace chrono;
#include <mutex>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>
#include "trpc/client/mysql/mysql_binder.h"
#include "trpc/client/mysql/mysql_statement.h"
#include "trpc/util/time.h"

struct MYSQL_RES;
struct MYSQL;
struct MYSQL_BIND;

namespace trpc::mysql {

class MysqlExecutor;

///@brief Just specialize class MysqlResults
class OnlyExec {};

class NativeString {};

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
///  Notice that the template args need to match the query field. The error handle has not been
///  implemented yet.
template <typename... Args>
class MysqlResults {
  friend class MysqlExecutor;

 public:
  MysqlResults() : affected_rows(0), error(false), has_value_(false) {
    if constexpr (is_native_string) {
      result_set = std::vector<std::vector<std::string>>();
    } else {
      result_set = std::vector<std::tuple<Args...>>();
    }
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

 private:
  static constexpr bool is_only_exec = std::is_same_v<std::tuple<Args...>, std::tuple<OnlyExec>>;

  static constexpr bool is_native_string = std::is_same_v<std::tuple<Args...>, std::tuple<NativeString>>;

  // // Used to store query results.
  // std::vector<std::tuple<Args...>> tuple_result_set;

  // std::vector<std::vector<std::string>> string_result_set;

  std::variant<std::vector<std::tuple<Args...>>, std::vector<std::vector<std::string>>> result_set;

  // Null flags
  std::vector<std::vector<uint8_t>> null_flags;

  std::string error_message;

  size_t affected_rows;

  bool error;

  bool has_value_;

  // Todo: Field Type
};

/// @brief A MySQL connection class that wraps the MySQL C API.
/// @note This class is not thread-safe.
class MysqlExecutor {
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
  ///@param query The SQL query to be executed as a string which uses "?" as placeholders.
  ///@param args The input arguments to be bound to the query placeholders.
  ///
  template <typename... InputArgs, typename... OutputArgs>
  void QueryAll(MysqlResults<OutputArgs...>& mysql_results, const std::string& query, const InputArgs&... args);

  template <typename... InputArgs>
  void Execute(MysqlResults<OnlyExec>& mysql_results, const std::string& query, const InputArgs&... args);

  bool Commit();

  bool Transaction();

  void RefreshAliveTime();

  uint64_t GetAliveTime();

 private:
  ///@brief Executes an SQL query and retrieves all resulting rows, storing each row as a tuple.
  ///
  /// This function executes the provided SQL query with the specified input arguments.
  /// The results of the query are stored in the `results` vector, where each row is represented
  /// as a tuple containing the specified output types.
  ///
  ///@param results Each element in this vector is a tuple containing the values of a single row.
  ///@param null_flag Null flags correspond to `results`.
  ///@param query The SQL query to be executed as a string which uses "?" as placeholders.
  ///@param args The input arguments to be bound to the query placeholders.
  ///
  template <typename... InputArgs, typename... OutputArgs>
  void QueryAll(std::vector<std::tuple<OutputArgs...>>& results, std::vector<std::vector<uint8_t>>& null_flags,
                const std::string& query, const InputArgs&... args);

  template <typename... InputArgs>
  void QueryAll(std::vector<std::vector<std::string>>& results, std::vector<std::vector<uint8_t>>& null_flags,
                const std::string& query, const InputArgs&... args);

  ///@brief Executes an SQL
  ///@param query The SQL query to be executed as a string which uses "?" as placeholders.
  ///@param args The input arguments to be bound to the query placeholders.
  ///@return Affected rows.
  template <typename... InputArgs>
  int Execute(const std::string& query, const InputArgs&... args);

  template <typename... InputArgs>
  void BindInputArgs(std::vector<MYSQL_BIND>& params, const InputArgs&... args);

  template <typename... OutputArgs>
  void BindOutputs(std::vector<MYSQL_BIND>& output_binds, std::vector<std::vector<std::byte>>& output_buffers,
                   std::vector<uint8_t>& null_flags);

  void ExecuteStatement(std::vector<MYSQL_BIND>& output_binds, MysqlStatement& statement);

  void ExecuteStatement(MysqlStatement& statement);

  template <typename... OutputArgs>
  void FetchResults(MysqlStatement& statement, std::vector<MYSQL_BIND>& output_binds,
                    std::vector<uint8_t>& null_flag_buffer, std::vector<std::tuple<OutputArgs...>>& results,
                    std::vector<std::vector<uint8_t>>& null_flags);

  void FreeResult();

 private:
  static std::mutex mysql_mutex;
  MYSQL* mysql_;
  MYSQL_RES* res_;
  uint64_t m_alivetime;  // 初始化活跃时间
};

// template <typename... InputArgs>
// void MysqlConnection::BindInputArgs(std::vector<MYSQL_BIND>& params, const InputArgs&... args) {
//     auto args_tuple = std::tie(args...);
//     BindInputImpl(params, args_tuple, std::index_sequence_for<InputArgs...>{});
// }

template <typename... InputArgs, typename... OutputArgs>
void MysqlExecutor::QueryAll(MysqlResults<OutputArgs...>& mysql_results, const std::string& query,
                             const InputArgs&... args) {
  // FreeResult();
  static_assert(!MysqlResults<OutputArgs...>::is_only_exec, "MysqlResults<OnlyExec> cannot be used with QueryAll.");

  auto& result_set = mysql_results.GetResultSet();

  if constexpr (MysqlResults<OutputArgs...>::is_native_string) {
    QueryAll(result_set, mysql_results.null_flags, query, args...);
  } else
    QueryAll(result_set, mysql_results.null_flags, query, args...);

  mysql_results.has_value_ = true;
}

template <typename... InputArgs>
void MysqlExecutor::Execute(MysqlResults<OnlyExec>& mysql_results, const std::string& query, const InputArgs&... args) {
  mysql_results.affected_rows = Execute(query, args...);
}

template <typename... InputArgs>
void MysqlExecutor::BindInputArgs(std::vector<MYSQL_BIND>& params, const InputArgs&... args) {
  BindInputImpl(params, args...);
}

template <typename... OutputArgs>
void MysqlExecutor::BindOutputs(std::vector<MYSQL_BIND>& output_binds,
                                std::vector<std::vector<std::byte>>& output_buffers,
                                std::vector<uint8_t>& null_flag_buffer) {
  BindOutputImpl<OutputArgs...>(output_binds, output_buffers, null_flag_buffer);
}

template <typename... InputArgs, typename... OutputArgs>
void MysqlExecutor::QueryAll(std::vector<std::tuple<OutputArgs...>>& results,
                             std::vector<std::vector<uint8_t>>& null_flags, const std::string& query,
                             const InputArgs&... args) {
  results.clear();
  null_flags.clear();

  MysqlStatement stmt(query, mysql_);
  std::vector<MYSQL_BIND> input_binds;
  BindInputArgs(input_binds, args...);

  if (mysql_stmt_bind_param(stmt.STMTPointer(), input_binds.data()) != 0) {
    // handle
  }

  std::vector<MYSQL_BIND> output_binds(stmt.GetFieldCount());
  std::vector<std::vector<std::byte>> output_buffers(stmt.GetFieldCount());
  std::vector<unsigned long> output_length(stmt.GetFieldCount());
  std::vector<uint8_t> null_flag_buffer(stmt.GetFieldCount());

  BindOutputs<OutputArgs...>(output_binds, output_buffers, null_flag_buffer);
  for (size_t i = 0; i < output_binds.size(); i++) {
    output_binds[i].length = &output_length[i];
  }

  ExecuteStatement(output_binds, stmt);
  FetchResults(stmt, output_binds, null_flag_buffer, results, null_flags);
}

template <typename... InputArgs>
void MysqlExecutor::QueryAll(std::vector<std::vector<std::string>>& results,
                             std::vector<std::vector<uint8_t>>& null_flags, const std::string& query,
                             const InputArgs&... args) {
  std::cout << "Native String Called\n";
}

template <typename... OutputArgs>
void MysqlExecutor::FetchResults(MysqlStatement& statement, std::vector<MYSQL_BIND>& output_binds,
                                 std::vector<uint8_t>& null_flag_buffer,
                                 std::vector<std::tuple<OutputArgs...>>& results,
                                 std::vector<std::vector<uint8_t>>& null_flags) {
  int status = 0;
  while (true) {
    status = mysql_stmt_fetch(statement.STMTPointer());
    if (status == 1 || status == MYSQL_NO_DATA) break;

    if (status == MYSQL_DATA_TRUNCATED) {
      // To Do
    } else {
      std::tuple<OutputArgs...> row_res;
      SetResultTuple(row_res, output_binds);
      results.push_back(std::move(row_res));
      null_flags.emplace_back(null_flag_buffer);
    }
  }

  if (status == 1) {
    std::runtime_error(mysql_stmt_error(statement.STMTPointer()));
  }
}

template <typename... InputArgs>
int MysqlExecutor::Execute(const std::string& query, const InputArgs&... args) {
  MysqlStatement stmt(query, mysql_);
  std::vector<MYSQL_BIND> input_binds;
  BindInputArgs(input_binds, args...);

  if (mysql_stmt_bind_param(stmt.STMTPointer(), input_binds.data()) != 0) {
    // handle
  }

  ExecuteStatement(stmt);

  size_t affected_row = mysql_affected_rows(mysql_);

  return affected_row;
}

void MysqlExecutor::FreeResult() {
  if (res_) {
    mysql_free_result(res_);
    res_ = nullptr;
  }
}

}  // namespace trpc::mysql
