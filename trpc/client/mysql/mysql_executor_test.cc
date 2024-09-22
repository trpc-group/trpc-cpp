#include <utility>
#include <vector>

#include <mysql/mysql.h>
#include "gtest/gtest.h"

#define private public

#include "mysql_executor.h"
#include "mysql_statement.h"
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



TEST(Executor, QueryNoArgs) {
  trpc::mysql::MysqlExecutor conn("localhost", "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<int, std::string, trpc::mysql::MysqlTime> res;
  conn.QueryAll(res, "select id, username, created_at from users");

  auto& res_data = res.GetResultSet();
  ASSERT_EQ(true, res.IsSuccess());
  EXPECT_EQ(4, res_data.size());
  EXPECT_EQ(2, std::get<0>(res_data[1]));
  EXPECT_EQ("alice", std::get<1>(res_data[0]));
  EXPECT_EQ(2024, std::get<2>(res_data[2]).mt.year);
}

TEST(Executor, QueryString) {
  trpc::mysql::MysqlExecutor conn("localhost", "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<trpc::mysql::NativeString> res;
  conn.QueryAll(res, "select * from users where id > ? or username = ?", 1, "alice");

   auto& res_data = res.GetResultSet();
   EXPECT_EQ("alice", res_data[0][1]);
}
//
TEST(Executor, QueryNull) {
  trpc::mysql::MysqlExecutor conn("localhost", "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<std::string, std::string> res;
  conn.QueryAll(res, "select username, email from users");
  EXPECT_TRUE(res.null_flags[0][0] == 0);
  EXPECT_TRUE(res.null_flags[0][1] == 0);
  EXPECT_TRUE(res.null_flags[3][0] == 0);
  EXPECT_TRUE(res.null_flags[3][1] != 0);
}
//
TEST(Executor, QueryArgs) {
  trpc::mysql::MysqlExecutor conn("localhost", "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<int, std::string, trpc::mysql::MysqlTime> res;
  conn.QueryAll(res, "select id, email, created_at from users where id = ? or username = ?", 1, "carol");

  auto& res_data = res.GetResultSet();
  EXPECT_EQ(2, res_data.size());
  EXPECT_EQ("alice@example.com", std::get<1>(res_data[0]));
  EXPECT_EQ("carol@example.com", std::get<1>(res_data[1]));
}

TEST(Executor, Update) {
  trpc::mysql::MysqlExecutor conn("localhost", "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<trpc::mysql::OnlyExec> res;
  conn.Execute(res,
               "update users set username = \
                      \"tom\", email = \"tom@abc.com\" where username = ?",
               "bob");
  EXPECT_EQ(1, res.affected_rows);
  res.affected_rows = 0;
  conn.Execute(res,
               "update users set username = \
                      \"bob\", email = \"bob@abc.com\" where username = ?",
               "tom");
  EXPECT_EQ(1, res.affected_rows);
}

TEST(Executor, Insert) {
  trpc::mysql::MysqlExecutor conn("localhost", "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<trpc::mysql::OnlyExec> res;
  mysql::MysqlTime mtime;
  mtime.mt.year = 2024;
  mtime.mt.month = 9;
  mtime.mt.day = 10;

  conn.Execute(res,
               "insert into users (username, email, created_at) \
                                   values (\"jack\", \"jack@abc.com\", ?)", mtime);
  EXPECT_EQ(1, res.affected_rows);
}

TEST(Executor, Delete) {
  trpc::mysql::MysqlExecutor conn("localhost", "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<trpc::mysql::OnlyExec> res;
  conn.Execute(res, "delete from users where username = \"jack\"");
  EXPECT_EQ(1, res.affected_rows);
}



TEST(Executor, SynaxError) {
  trpc::mysql::MysqlExecutor conn("localhost", "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<trpc::mysql::OnlyExec> res;
  conn.Execute(res, "delete users where username = \"jack\"");
  EXPECT_EQ(false, res.IsSuccess());
  std::cout << res.error_message << "\n";
}

TEST(Executor, OutputArgsError) {
  mysql::MysqlExecutor conn("localhost", "root", "abc123", "test", 3306);
  mysql::MysqlResults<int, std::string> res;
  conn.QueryAll(res, "select * from users");
  EXPECT_EQ(false, res.IsSuccess());
  std::cout << res.error_message << "\n";
}

TEST(Executor, BLOB) {
  mysql::MysqlExecutor conn("localhost", "root", "abc123", "test", 3306);
  mysql::MysqlResults<mysql::OnlyExec> res;
  mysql::MysqlResults<mysql::MysqlBlob> res2;
  mysql::MysqlBlob blob(GenRandomBlob(1024));

  conn.Execute(res,
               "insert into users (username, email, meta) \
                                   values (\"jack\", \"jack@abc.com\", ?)", blob);
  EXPECT_EQ(1, res.affected_rows);

  conn.QueryAll(res2, "select meta from users where username = ?", "jack");
  auto& meta_res = res2.GetResultSet();
  EXPECT_EQ(std::get<0>(meta_res[0]), blob);

  conn.Execute(res, "delete from users where username = ?", "jack");
  EXPECT_GE(1, res.affected_rows);
}


}  // namespace trpc::testing