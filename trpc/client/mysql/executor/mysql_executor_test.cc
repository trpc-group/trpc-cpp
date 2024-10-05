#include <utility>
#include <vector>

#include <mysql/mysql.h>
//#include "include/mysql.h"
#include "include/gtest/gtest.h"

#define private public

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


void PrintResultTable(mysql::MysqlResults<mysql::IterMode>& res) {
  // Step 1: 确定每一列的宽度
  std::vector<std::string> fields_name;
  bool flag = false;
  std::vector<size_t> column_widths;

  for(auto row : res) {
    if(!flag) {
      fields_name = row.GetFieldsName();
      column_widths.resize(fields_name.size(), 0);
      for (size_t i = 0; i < fields_name.size(); ++i)
        column_widths[i] = std::max(column_widths[i], fields_name[i].length());
      flag = true;
    }

    size_t i = 0;
    for(auto field : row) {
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
  std::cout << std::setfill(' '); // 重置填充字符

  for (auto row : res) {
    int i = 0;
    for(auto field_itr = row.begin(); field_itr != row.end(); ++field_itr) {
      std::cout << std::left
                << std::setw(column_widths[i] + 2)
                << (field_itr.IsNull() ? "null" : *field_itr);
      ++i;
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
  conn.QueryAll(res, "select id, username, created_at from users");

  auto& res_data = res.GetResultSet();
  ASSERT_EQ(true, res.IsSuccess());
  EXPECT_EQ(4, res_data.size());
  EXPECT_EQ(2, std::get<0>(res_data[1]));
  EXPECT_EQ("alice", std::get<1>(res_data[0]));
  EXPECT_EQ(2024, std::get<2>(res_data[2]).mt.year);
}

TEST(Executor, QueryString) {
  trpc::mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<trpc::mysql::NativeString> res;
  conn.QueryAll(res, "select * from users where id > ? or username = ?", 1, "alice");

  auto& res_data = res.GetResultSet();
  EXPECT_EQ("alice", res_data[0][1]);
}
//
TEST(Executor, QueryNull) {
  trpc::mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<std::string, std::string> res;
  conn.QueryAll(res, "select username, email from users");
  EXPECT_TRUE(res.GetNullFlag()[0][0] == 0);
  EXPECT_TRUE(res.GetNullFlag()[0][1] == 0);
  EXPECT_TRUE(res.GetNullFlag()[3][0] == 0);
  EXPECT_TRUE(res.GetNullFlag()[3][1] != 0);
}

TEST(Executor, QueryArgs) {
  trpc::mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<int, std::string, trpc::mysql::MysqlTime> res;
  conn.QueryAll(res, "select id, email, created_at from users where id = ? or username = ?", 1, "carol");

  auto& res_data = res.GetResultSet();
  EXPECT_EQ(2, res_data.size());
  EXPECT_EQ("alice@example.com", std::get<1>(res_data[0]));
  EXPECT_EQ("carol@example.com", std::get<1>(res_data[1]));
}


TEST(Executor, Update) {
  trpc::mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<trpc::mysql::OnlyExec> res;
  conn.Execute(res,
               "update users set username = \
                      \"tom\", email = \"tom@abc.com\" where username = ?",
               "bob");
  EXPECT_EQ(1, res.GetAffectedRows());
  res.affected_rows = 0;
  conn.Execute(res,
               "update users set username = \
                      \"bob\", email = \"bob@abc.com\" where username = ?",
               "tom");
  EXPECT_EQ(1, res.GetAffectedRows());
}

TEST(Executor, Insert) {
  trpc::mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<trpc::mysql::OnlyExec> res;
  mysql::MysqlTime mtime;
  mtime.mt.year = 2024;
  mtime.mt.month = 10;
  mtime.mt.day = 10;

  conn.Execute(res,
               "insert into users (username, email, created_at) \
                                   values (\"jack\", \"jack@abc.com\", ?)", mtime);
  EXPECT_EQ(1, res.GetAffectedRows());
}

TEST(Executor, Delete) {
  trpc::mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<trpc::mysql::OnlyExec> res;
  conn.Execute(res, "delete from users where username = \"jack\"");
  EXPECT_EQ(1, res.GetAffectedRows());
}



TEST(Executor, SynaxError) {
  trpc::mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  trpc::mysql::MysqlResults<trpc::mysql::OnlyExec> res;
  conn.Execute(res, "delete users where username = \"jack\"");
  EXPECT_EQ(false, res.IsSuccess());
  std::cout << res.error_message << "\n";
}

TEST(Executor, OutputArgsError) {
  mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  mysql::MysqlResults<int, std::string> res;
  conn.QueryAll(res, "select * from users");
  EXPECT_EQ(false, res.IsSuccess());
  std::cout << res.error_message << "\n";
}



TEST(Executor, IterMode) {
  mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  mysql::MysqlResults<mysql::IterMode> res;
  conn.QueryAll(res, "select * from users where id > ? or username = ?", 1, "alice");
  std::vector<std::string> fields_name = res.GetFieldsName();

  EXPECT_EQ("id", fields_name[0]);
  EXPECT_EQ("username", fields_name[1]);
  EXPECT_EQ("email", fields_name[2]);
  EXPECT_EQ("created_at", fields_name[3]);
  EXPECT_EQ("meta", fields_name[4]);

  PrintResultTable(res);

  std::cout << "\n--------------------------------------------------------------\n" << std::endl;

  for(auto row : res) {
    for(unsigned int i = 0; i < row.NumFields(); i++)
      std::cout << row.GetFieldData(i) << ", ";
    std::cout << std::endl;
  }

  std::cout << "\n---------------------------------------------------------------\n" << std::endl;

  for(auto row : res) {
    for(auto field : row)
      std::cout << field << ", ";
    std::cout << std::endl;
  }

  std::cout << std::endl;
}


TEST(Executor, BLOB) {
  mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  mysql::MysqlResults<mysql::OnlyExec> exec_res;
  mysql::MysqlResults<mysql::MysqlBlob> special_res;
  mysql::MysqlResults<mysql::NativeString> str_res;
  mysql::MysqlResults<mysql::IterMode> itr_res;
  mysql::MysqlBlob blob(GenRandomBlob(1024));


  // mysql::MysqlBlob
  conn.Execute(exec_res,
               "insert into users (username, email, meta)"
               "values (\"jack\", \"jack@abc.com\", ?)", blob);
  EXPECT_EQ(1, exec_res.GetAffectedRows());

  conn.QueryAll(special_res, "select meta from users where username = ?", "jack");
  auto& meta_res = special_res.GetResultSet();
  EXPECT_EQ(std::get<0>(meta_res[0]), blob);


  conn.QueryAll(str_res, "select meta from users where username = ?", "jack");
  auto& meta_res2 = special_res.GetResultSet();
  EXPECT_EQ(std::get<0>(meta_res2[0]), blob);

  conn.QueryAll(itr_res, "select meta from users where username = ?", "jack");
  for(auto row : itr_res) {
    mysql::MysqlBlob data(std::string(row.GetFieldData(0)));
    EXPECT_EQ(data, blob);
  }


  conn.Execute(exec_res, "delete from users where username = ?", "jack");
  EXPECT_GE(1, exec_res.GetAffectedRows());

}

TEST(Executor, TimeType) {
  mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  mysql::MysqlResults<int, mysql::MysqlTime, mysql::MysqlTime, mysql::MysqlTime, mysql::MysqlTime, mysql::MysqlTime, mysql::MysqlTime> res;
  mysql::MysqlResults<mysql::NativeString> res2;
  mysql::MysqlResults<mysql::IterMode> res3;

  conn.QueryAll(res, "select * from time_example");
  conn.QueryAll(res2, "select * from time_example");
  conn.QueryAll(res3, "select * from time_example");
//  PrintResultTable(res3);
}

TEST(Executor, StringType) {
  mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  mysql::MysqlResults<int, std::string, std::string, std::string, std::string, std::string, std::string> res;
  mysql::MysqlResults<mysql::NativeString> res2;
  mysql::MysqlResults<mysql::IterMode> res3;
  mysql::MysqlResults<mysql::OnlyExec> res4;



  conn.QueryAll(res, "select * from string_example");
  conn.QueryAll(res2, "select * from string_example");
  conn.Execute(res4, "insert into string_example (description, json_data) values (?, ?)",
               res2.GetResultSet()[0][2], res2.GetResultSet()[0][4]);
  conn.QueryAll(res3, "select * from string_example");
//  PrintResultTable(res3);

}


TEST(Executor, Transaction) {
  mysql::MysqlExecutor conn(db_ip, "root", "abc123", "test", 3306);
  mysql::MysqlResults<mysql::OnlyExec> exec_res;
  mysql::MysqlResults<mysql::IterMode> itr_res;
  conn.Execute(exec_res, "begin");
  EXPECT_EQ(true, exec_res.IsSuccess());
  conn.Execute(exec_res, "update users set email = ? where username = ?", "rose@abc.com", "rose");
  EXPECT_EQ(1, exec_res.GetAffectedRows());
  conn.Execute(exec_res, "rollback");
  EXPECT_EQ(true, exec_res.IsSuccess());

  conn.QueryAll(itr_res, "select * from users where username = ?", "rose");
  for(auto row : itr_res) {
//    for(auto citr = row.begin(); citr != row.end(); ++citr)
//      if(citr.GetFieldName() == "email")
//        EXPECT_EQ(true, citr.IsNull());
    EXPECT_EQ("rose", row.GetFieldData(1));
    EXPECT_EQ(true, row.IsFieldNull(2));
  }
}


}  // namespace trpc::testing