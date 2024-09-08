#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include <mysql/mysql.h>

#define private public

#include "mysql_executor.h"
#include "mysql_statement.h"
namespace trpc::testing {

/*
mysql> select * from users;
+----+----------+-------------------+---------------------+
| id | username | email             | created_at          |
+----+----------+-------------------+---------------------+
|  1 | alice    | alice@example.com | 2024-08-17 21:45:57 |
|  2 | bob      | bob@abc.com       | 2024-08-17 21:45:57 |
|  3 | carol    | carol@example.com | 2024-08-17 21:45:57 |
| 19 | rose     | NULL              | 2024-09-01 12:33:24 |
+----+----------+-------------------+---------------------+
*/

TEST(Connection, Init) {
  trpc::mysql::MysqlExecutor conn("localhost", "kosmos", "12345678", "test_db");
  EXPECT_NE(nullptr, conn.mysql_);
  conn.Close();
  EXPECT_EQ(nullptr, conn.mysql_);
}

TEST(Connection, InputBind) {
  trpc::mysql::MysqlExecutor conn("localhost", "kosmos", "12345678", "test_db");
  
  std::vector<MYSQL_BIND> binds;
  float float_num = 3.14f;
  double double_num = 3.14;
  int num = 123;
  unsigned long num2 = 333;

  std::string hello_s("hello");
  conn.BindInputArgs(binds, float_num, double_num, num, num2, hello_s, "hello");

  EXPECT_EQ(6, binds.size());
  EXPECT_FLOAT_EQ(3.14, *static_cast<float*>(binds[0].buffer));
  EXPECT_FLOAT_EQ(3.14, *static_cast<double*>(binds[1].buffer));
  EXPECT_EQ(123, *static_cast<int*>(binds[2].buffer));
  EXPECT_EQ(333, *static_cast<unsigned long*>(binds[3].buffer));
  EXPECT_STREQ(static_cast<char*>(binds[4].buffer), static_cast<char*>(binds[5].buffer));


  conn.Close();
}


TEST(Statement, Prepare) {
  trpc::mysql::MysqlExecutor conn("localhost", "kosmos", "12345678", "test_db");
  trpc::mysql::MysqlStatement prepare("select * from users", conn.mysql_);
  EXPECT_EQ(prepare.GetFieldCount(), 4);

}

TEST(Connection, QueryNoArgs) {
  trpc::mysql::MysqlExecutor conn("localhost", "kosmos", "12345678", "test_db");
  trpc::mysql::MysqlResults<int, std::string, trpc::mysql::mysql_time> res;
  conn.QueryAll(res, "select id, username, created_at from users");

  auto& res_data = res.GetResultSet();
  EXPECT_EQ(4, res_data.size());
  EXPECT_EQ(2, std::get<0>(res_data[1]));
  EXPECT_EQ("alice", std::get<1>(res_data[0]));
  EXPECT_EQ(2024, std::get<2>(res_data[2]).mt.year);
}


TEST(Connection, QueryString) {
  trpc::mysql::MysqlExecutor conn("localhost", "kosmos", "12345678", "test_db");
  trpc::mysql::MysqlResults<trpc::mysql::NativeString> res;
  conn.QueryAll(res, "select id, username, created_at from users");

  // Not Complete yet
  // auto& res_data = res.GetResultSet();
  // EXPECT_EQ("alice", res_data[0][1]);
}

TEST(Connection, QueryNull) {
  trpc::mysql::MysqlExecutor conn("localhost", "kosmos", "12345678", "test_db");
  trpc::mysql::MysqlResults<std::string, std::string> res;
  conn.QueryAll(res, "select username, email from users");
  EXPECT_TRUE(res.null_flags[0][0] == 0);
  EXPECT_TRUE(res.null_flags[0][1] == 0);
  EXPECT_TRUE(res.null_flags[3][0] == 0);
  EXPECT_TRUE(res.null_flags[3][1] != 0);
}

TEST(Connection, QueryArgs) {
  trpc::mysql::MysqlExecutor conn("localhost", "kosmos", "12345678", "test_db");
  trpc::mysql::MysqlResults<int, std::string, trpc::mysql::mysql_time> res;
  conn.QueryAll(res,
    "select id, email, created_at from users where id = ? or username = ?", 1, "carol");
  
  auto& res_data = res.GetResultSet();
  EXPECT_EQ(2, res_data.size());
  EXPECT_EQ("alice@example.com", std::get<1>(res_data[0]));
  EXPECT_EQ("carol@example.com", std::get<1>(res_data[1]));
}

TEST(Connection, Update) {
  trpc::mysql::MysqlExecutor conn("localhost", "kosmos", "12345678", "test_db");
  trpc::mysql::MysqlResults<trpc::mysql::OnlyExec> res;
  conn.Execute(res, "update users set username = \
                      \"tom\", email = \"tom@abc.com\" where username = ?", "bob");
  EXPECT_EQ(1, res.affected_rows);
  res.affected_rows = 0;
  conn.Execute(res, "update users set username = \
                      \"bob\", email = \"bob@abc.com\" where username = ?", "tom");
  EXPECT_EQ(1, res.affected_rows);
}


TEST(Connection, Insert) {
  trpc::mysql::MysqlExecutor conn("localhost", "kosmos", "12345678", "test_db");
  trpc::mysql::MysqlResults<trpc::mysql::OnlyExec> res;
  conn.Execute(res, "insert into users (username, email) \
                                   values (\"jack\", \"jack@abc.com\")");
  EXPECT_EQ(1, res.affected_rows);
}

TEST(Connection, Delete) {
  trpc::mysql::MysqlExecutor conn("localhost", "kosmos", "12345678", "test_db");
  trpc::mysql::MysqlResults<trpc::mysql::OnlyExec> res;
  conn.Execute(res, "delete from users where username = \"jack\"");
  EXPECT_EQ(1, res.affected_rows);
}




}