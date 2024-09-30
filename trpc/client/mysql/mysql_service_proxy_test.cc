#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mysql_service_proxy.h"

#include "trpc/client/make_client_context.h"
#include "trpc/future/future_utility.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/client/service_proxy_option_setter.h"

namespace trpc::testing {


class MockMysqlServiceProxy : public mysql::MysqlServiceProxy {
 public:
  void SetMockServiceProxyOption(const std::shared_ptr<ServiceProxyOption> &option) {
    SetServiceProxyOptionInner(option);
  }

  void InitMockOtherMembers() {
    InitOtherMembers();
  }

  void SetMockCodec(ClientCodecPtr &&codec) { codec_ = codec; }

  void PrintResultTable(const mysql::MysqlResults <mysql::IterMode> &res) {
    std::vector<std::string> fields_name;
    bool flag = false;
    std::vector<size_t> column_widths;
    for (auto row: res) {
      if (!flag) {
        fields_name = row.GetFieldsName();
        column_widths.resize(fields_name.size(), 0);
        for (size_t i = 0; i < fields_name.size(); ++i)
          column_widths[i] = std::max(column_widths[i], fields_name[i].length());
        flag = true;
      }

      size_t i = 0;
      for (auto field: row) {
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

    for (auto row: res) {
      int i = 0;
      for (auto field_itr = row.begin(); field_itr != row.end(); ++field_itr) {
        std::cout << std::left
                  << std::setw(column_widths[i] + 2)
                  << (field_itr.IsNull() ? "null" : *field_itr);
        ++i;
      }
      std::cout << std::endl;
    }
  }
};

using MysqlServiceProxyPtr = std::shared_ptr<MockMysqlServiceProxy>;

class MysqlServiceProxyTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    TrpcPlugin::GetInstance()->RegisterPlugins();
    trpc::detail::SetDefaultOption(option_);
    option_->name = "default_mysql_service";
    option_->caller_name = "";
    option_->codec_name = "mysql";
    option_->conn_type = "long";
    option_->network = "tcp";
    option_->timeout = 1000;
    option_->target = "127.0.0.1:3306";
    option_->selector_name = "direct";
    option_->mysql_conf.dbname = "test";
    option_->mysql_conf.password = "abc123";
    option_->mysql_conf.user_name = "root";
    option_->mysql_conf.enable = true;
    option_->mysql_conf.min_size = 6;
  }

  static void TearDownTestCase() { TrpcPlugin::GetInstance()->UnregisterPlugins(); }

 protected:
  void SetUp() override {
    mock_mysql_service_proxy_ = std::make_shared<MockMysqlServiceProxy>();
    mock_mysql_service_proxy_->SetMockServiceProxyOption(option_);
    mock_mysql_service_proxy_->InitMockOtherMembers();
  }

  void TearDown() {
    mock_mysql_service_proxy_->Stop();
    mock_mysql_service_proxy_->Destroy();
  }

  ClientContextPtr GetClientContext() {
    auto ctx = MakeClientContext(mock_mysql_service_proxy_);
    ctx->SetTimeout(option_->timeout);
    ctx->SetAddr("127.0.0.1", 3306);
    return ctx;
  }

 protected:
  static std::shared_ptr<ServiceProxyOption> option_;
  MysqlServiceProxyPtr mock_mysql_service_proxy_{nullptr};
};

std::shared_ptr<ServiceProxyOption> MysqlServiceProxyTest::option_ = std::make_shared<ServiceProxyOption>();

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

TEST_F(MysqlServiceProxyTest, Query) {
  auto client_context = GetClientContext();
  trpc::mysql::MysqlResults<int, std::string> res;
  mock_mysql_service_proxy_->Query(client_context, res, "select id, username from users where id = ?", 1);
  auto &res_vec = res.GetResultSet();
  EXPECT_EQ(true, res.IsSuccess());
  EXPECT_EQ("alice", std::get<1>(res_vec[0]));
}


TEST_F(MysqlServiceProxyTest, AsyncQuery) {
  // trpc::mysql::MysqlServiceProxy proxy;
  auto client_context = GetClientContext();
  auto res = mock_mysql_service_proxy_->AsyncQuery<mysql::IterMode>(client_context, "select * from users where id >= ?",
                                                                    1)
          .Then([this](trpc::Future<mysql::MysqlResults<mysql::IterMode>> &&f) {
            if (f.IsReady()) {
              auto row_iter = f.GetConstValue().begin();
              EXPECT_EQ(5, (*row_iter).NumFields());
              this->mock_mysql_service_proxy_->PrintResultTable(f.GetConstValue());
            }
            return trpc::MakeReadyFuture<>();
          });

  std::cout << "ret\n";

  ::trpc::future::BlockingGet(std::move(res));
}

TEST_F(MysqlServiceProxyTest, Execute) {
  auto client_context = GetClientContext();
  mysql::MysqlResults<mysql::OnlyExec> exec_res;
  mysql::MysqlTime mtime;
  mtime.mt.year = 2024;
  mtime.mt.month = 9;
  mtime.mt.day = 10;

  mock_mysql_service_proxy_->Execute(client_context, exec_res,
                                     "insert into users (username, email, created_at) \
                                   values (\"jack\", \"jack@abc.com\", ?)", mtime);
  EXPECT_EQ(1, exec_res.GetAffectedRows());


  client_context = GetClientContext();
  trpc::mysql::MysqlResults<std::string, mysql::MysqlTime> res;
  mock_mysql_service_proxy_->Execute(client_context, res, "select email, created_at from users where username = ?",
                                     "jack");
  auto &res_vec = res.GetResultSet();
  EXPECT_EQ(true, res.IsSuccess());
  EXPECT_EQ("jack@abc.com", std::get<0>(res_vec[0]));

  client_context = GetClientContext();
  mock_mysql_service_proxy_->Execute(client_context, exec_res, "delete from users where username = \"jack\"");
  EXPECT_EQ(1, exec_res.GetAffectedRows());

  client_context = GetClientContext();
  mock_mysql_service_proxy_->Execute(client_context, res, "select email, created_at from users where username = ?",
                                     "jack");
  EXPECT_EQ(true, res.IsSuccess());
  EXPECT_EQ(0, res.GetResultSet().size());
}


TEST_F(MysqlServiceProxyTest, AsyncExecute) {
  auto client_context = GetClientContext();
  mysql::MysqlTime mtime;
  mtime.mt.year = 2024;
  mtime.mt.month = 9;
  mtime.mt.day = 10;

  auto res = mock_mysql_service_proxy_->AsyncExecute<mysql::OnlyExec>(client_context,
                                                                      "insert into users (username, email, created_at) \
                                   values (\"jack\", \"jack@abc.com\", ?)", mtime)
          .Then([](trpc::Future<mysql::MysqlResults<mysql::OnlyExec>> &&f) {
            if (f.IsReady()) {
              EXPECT_EQ(1, f.GetConstValue().GetAffectedRows());
            }
            return trpc::MakeReadyFuture<>();
          });
  ::trpc::future::BlockingGet(std::move(res));

  auto res2 = mock_mysql_service_proxy_->AsyncExecute<std::string, mysql::MysqlTime>(client_context,
                                                                                     "select email, created_at from users where username = ?",
                                                                                     "jack")
          .Then([](trpc::Future<mysql::MysqlResults<std::string, mysql::MysqlTime>> &&f) {
            if (f.IsReady()) {
              auto &res_vec = f.GetValue0().GetResultSet();
              EXPECT_EQ("jack@abc.com", std::get<0>(res_vec[0]));
            }
            return trpc::MakeReadyFuture<>();
          });
  ::trpc::future::BlockingGet(std::move(res2));

  auto res3 = mock_mysql_service_proxy_->AsyncExecute<mysql::OnlyExec>(client_context,
                                                                       "delete from users where username = \"jack\"")
          .Then([](trpc::Future<mysql::MysqlResults<mysql::OnlyExec>> &&f) {
            if (f.IsReady()) {
              EXPECT_EQ(1, f.GetConstValue().GetAffectedRows());
            }
            return trpc::MakeReadyFuture<>();
          });
  ::trpc::future::BlockingGet(std::move(res3));

  auto res4 = mock_mysql_service_proxy_->AsyncExecute<std::string, mysql::MysqlTime>(client_context,
                                                                                     "select email, created_at from users where username = ?",
                                                                                     "jack")
          .Then([](trpc::Future<mysql::MysqlResults<std::string, mysql::MysqlTime>> &&f) {
            if (f.IsReady()) {
              EXPECT_EQ(0, f.GetValue0().GetResultSet().size());
            }
            return trpc::MakeReadyFuture<>();
          });
  ::trpc::future::BlockingGet(std::move(res4));


}

TEST_F(MysqlServiceProxyTest, AsyncException) {
  std::string error_msg = "You have an error in your SQL syntax; "
                          "check the manual that corresponds to your MySQL server version "
                          "for the right syntax to use near '' at line 1";
  auto client_context = GetClientContext();
  auto res = mock_mysql_service_proxy_->AsyncQuery<mysql::IterMode>(client_context, "select * from users where")
          .Then([&error_msg](trpc::Future<mysql::MysqlResults<mysql::IterMode>> &&f) {
            EXPECT_EQ(true, f.IsFailed());
            auto e = f.GetException();
            EXPECT_EQ(error_msg, e.what());
            return trpc::MakeExceptionFuture<>(e);
          });

  std::cout << "async call\n";
  ::trpc::future::BlockingGet(std::move(res));

}
}  // namespace trpc::testing