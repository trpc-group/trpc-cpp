#include <utility>
#include <vector>

#include "include/gtest/gtest.h"

//#define private public

#include "trpc/client/mysql/executor/mysql_executor.h"
#include "trpc/client/mysql/executor/mysql_statement.h"
#include "trpc/util/random.h"
namespace trpc::testing {

mysql::MysqlBlob GenRandomBlob(std::size_t length) {
  std::string random_data;
  random_data.reserve(length);

  for (std::size_t i = 0; i < length; ++i) {
    char random_byte = static_cast<char>(trpc::Random<uint8_t>(0, 255));
    random_data.push_back(random_byte);
  }

  return mysql::MysqlBlob(std::move(random_data));
}

void PrintResultTable(mysql::MysqlResults<mysql::NativeString>& res) {

  std::vector<std::string> fields_name =  res.GetFieldsName();
  bool flag = false;
  std::vector<size_t> column_widths;
  auto& res_set = res.ResultSet();

  for (auto& row : res_set) {
    if (!flag) {
      column_widths.resize(fields_name.size(), 0);
      for (size_t i = 0; i < fields_name.size(); ++i)
        column_widths[i] = std::max(column_widths[i], fields_name[i].length());
      flag = true;
    }

    size_t i = 0;
    for (auto field : row) {
      column_widths[i] = std::max(column_widths[i], field.length());
      ++i;
    }
  }

  for (size_t i = 0; i < fields_name.size(); ++i) {
    std::cout << std::left << std::setw(column_widths[i] + 2) << fields_name[i];
  }
  std::cout << std::endl;

  for (size_t i = 0; i < fields_name.size(); ++i) {
    std::cout << std::setw(column_widths[i] + 2) << std::setfill('-') << "";
  }
  std::cout << std::endl;
  std::cout << std::setfill(' ');

  for(int i = 0; i < res_set.size(); i++) {
    for(int j = 0; j < res_set[i].size(); j++) {
      std::cout << std::left << std::setw(column_widths[j] + 2) << (res.IsValueNull(i, j) ? "null" : res_set[i][j]);
    }
    std::cout << std::endl;
  }
}

/*
mysql> select * from users;
+----+----------+-------------------+---------------------+------------+
| id | username | email             | created_at          | meta       |
+----+----------+-------------------+---------------------+------------+
|  1 | alice    | alice@example.com | 2024-09-08 13:16:24 | NULL       |
|  2 | bob      | bob@abc.com       | 2024-09-08 13:16:24 | NULL       |
|  3 | carol    | carol@example.com | 2024-09-08 13:16:24 | NULL       |
|  4 | rose     | NULL              | 2024-09-08 13:16:53 | NULL       |
+----+----------+-------------------+---------------------+------------+
*/

std::string db_ip = "127.0.0.1";

TEST(Executor, QueryNoArgs) {
  trpc::mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<int, std::string, trpc::mysql::MysqlTime> res;
  conn.Connect();
  conn.QueryAll(res, "select id, username, created_at from users");

  auto& res_data = res.ResultSet();
  ASSERT_EQ(true, res.OK());
  EXPECT_EQ(4, res_data.size());
  EXPECT_EQ(2, std::get<0>(res_data[1]));
  EXPECT_EQ("alice", std::get<1>(res_data[0]));
  EXPECT_EQ(2024, std::get<2>(res_data[2]).GetYear());

  conn.Close();
}

TEST(Executor, QueryString) {
  trpc::mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<trpc::mysql::NativeString> res;
  conn.Connect();
  conn.QueryAll(res, "select * from users where id > ? or username = ?", 1, "alice");

  auto& res_data = res.ResultSet();
  EXPECT_EQ("alice", res_data[0][1]);
  conn.Close();
}
//
TEST(Executor, QueryNull) {
  trpc::mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<std::string, std::string> res;
  conn.Connect();
  conn.QueryAll(res, "select username, email from users");
  EXPECT_TRUE(res.GetNullFlag()[0][0] == 0);
  EXPECT_TRUE(res.GetNullFlag()[0][1] == 0);
  EXPECT_TRUE(res.GetNullFlag()[3][0] == 0);
  EXPECT_TRUE(res.GetNullFlag()[3][1] != 0);
  conn.Close();
}

TEST(Executor, QueryArgs) {
  trpc::mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<int, std::string, trpc::mysql::MysqlTime> res;
  conn.Connect();
  conn.QueryAll(res, "select id, email, created_at from users where id = ? or username = ?", 1, "carol");

  auto& res_data = res.ResultSet();
  EXPECT_EQ(2, res_data.size());
  EXPECT_EQ("alice@example.com", std::get<1>(res_data[0]));
  EXPECT_EQ("carol@example.com", std::get<1>(res_data[1]));
  conn.Close();
}

TEST(Executor, Update) {
  trpc::mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<trpc::mysql::OnlyExec> res;
  conn.Connect();
  conn.Execute(res,
               "update users set username = \
                      \"tom\", email = \"tom@abc.com\" where username = ?",
               "bob");
  EXPECT_EQ(1, res.GetAffectedRowNum());
  conn.Execute(res,
               "update users set username = \
                      \"bob\", email = \"bob@abc.com\" where username = ?",
               "tom");
  EXPECT_EQ(1, res.GetAffectedRowNum());
  conn.Close();
}

TEST(Executor, Insert) {
  trpc::mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<trpc::mysql::OnlyExec> res;
  mysql::MysqlTime mtime, mtime2;
  mtime.SetYear(2024).SetMonth(10).SetDay(10).SetHour(22);

  conn.Connect();
  conn.Execute(res,
               "insert into users (username, email, created_at) \
                                   values (\"jack\", \"jack@abc.com\", ?)",
               mtime);
  EXPECT_EQ(1, res.GetAffectedRowNum());
  conn.Close();
}

TEST(Executor, Delete) {
  trpc::mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<trpc::mysql::OnlyExec> res;
  conn.Connect();
  conn.Execute(res, "delete from users where username = \"jack\"");
  EXPECT_EQ(1, res.GetAffectedRowNum());
  conn.Close();
}

TEST(Executor, SynaxError) {
  trpc::mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<trpc::mysql::OnlyExec> res;
  conn.Connect();
  conn.Execute(res, "delete users where username = \"jack\"");
  EXPECT_EQ(false, res.OK());
  std::cout << res.GetErrorMessage() << "\n";
  conn.Close();
}

TEST(Executor, OutputArgsError) {
  mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  mysql::MysqlResults<int, std::string> res;
  conn.Connect();
  conn.QueryAll(res, "select * from users");
  EXPECT_EQ(false, res.OK());
  std::cout << res.GetErrorMessage() << "\n";
  conn.Close();
}

TEST(Executor, BLOB) {
  mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  mysql::MysqlResults<mysql::OnlyExec> exec_res;
  mysql::MysqlResults<mysql::MysqlBlob> special_res;
  mysql::MysqlResults<mysql::NativeString> str_res;
  mysql::MysqlBlob blob(GenRandomBlob(1024));
  conn.Connect();

  // mysql::MysqlBlob
  conn.Execute(exec_res,
               "insert into users (username, email, meta)"
               "values (\"jack\", \"jack@abc.com\", ?)",
               blob);
  EXPECT_EQ(1, exec_res.GetAffectedRowNum());

  conn.QueryAll(special_res, "select meta from users where username = ?", "jack");
  auto& meta_res = special_res.ResultSet();
  EXPECT_EQ(std::get<0>(meta_res[0]), blob);

  conn.QueryAll(str_res, "select meta from users where username = ?", "jack");
  auto& meta_res2 = special_res.ResultSet();
  EXPECT_EQ(std::get<0>(meta_res2[0]), blob);


  conn.Execute(exec_res, "delete from users where username = ?", "jack");
  EXPECT_GE(1, exec_res.GetAffectedRowNum());

  conn.Close();
}

TEST(Executor, TimeType) {
  mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  mysql::MysqlResults<int, mysql::MysqlTime, mysql::MysqlTime, mysql::MysqlTime, mysql::MysqlTime, mysql::MysqlTime,
                      mysql::MysqlTime>
      res;
  mysql::MysqlResults<mysql::NativeString> res2;

  conn.Connect();
  conn.QueryAll(res, "select * from time_example");
  conn.QueryAll(res2, "select * from time_example");


  mysql::MysqlTime target_time;
  target_time.SetYear(2024).SetMonth(4).SetDay(22);
  conn.QueryAll(res2, "select * from time_example where event_date = ?", target_time);

  conn.Close();
}

TEST(Executor, StringType) {
  mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  mysql::MysqlResults<int, std::string, std::string, std::string, std::string, std::string, std::string> res;
  mysql::MysqlResults<mysql::NativeString> res2;
  mysql::MysqlResults<mysql::OnlyExec> res4;

  conn.Connect();
  conn.QueryAll(res, "select * from string_example");
  conn.QueryAll(res2, "select * from string_example");
  conn.Execute(res4, "insert into string_example (description, json_data) values (?, ?)", res2.ResultSet()[0][2],
               res2.ResultSet()[0][4]);
  //  PrintResultTable(res3);

  conn.Close();
}

TEST(Executor, Transaction) {
  mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  mysql::MysqlResults<mysql::OnlyExec> exec_res;

  conn.Connect();
  conn.Execute(exec_res, "begin");
  EXPECT_EQ(true, exec_res.OK());
  conn.Execute(exec_res, "update users set email = ? where username = ?", "rose@abc.com", "rose");
  EXPECT_EQ(1, exec_res.GetAffectedRowNum());
  conn.Execute(exec_res, "rollback");
  EXPECT_EQ(true, exec_res.OK());

  conn.Close();
}

TEST(Executor, GetResultSet) {
  mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);

  mysql::MysqlResults<int, std::string, trpc::mysql::MysqlTime> res;
  mysql::MysqlResults<mysql::NativeString> res2;
  conn.Connect();
  conn.QueryAll(res, "select id, email, created_at from users where id = ? or username = ?", 1, "carol");
  conn.QueryAll(res2, "select id, email, created_at from users where id = ? or username = ?", 1, "carol");

  conn.Close();

  decltype(res)::ResultSetType res_vec;
  std::vector<std::vector<std::string>> res2_vec;
  EXPECT_EQ(true, res.GetResultSet(res_vec));
  EXPECT_EQ(true, res2.GetResultSet(res2_vec));
}

}  // namespace trpc::testing