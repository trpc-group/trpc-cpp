#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mysql_service_proxy.h"

#include "trpc/client/make_client_context.h"
#include "trpc/client/service_proxy_option_setter.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/future/future_utility.h"

namespace trpc::testing {

using trpc::mysql::OnlyExec;
using trpc::mysql::NativeString;
using trpc::mysql::MysqlResults;
using trpc::mysql::MysqlTime;
using trpc::mysql::TransactionHandle;

 class MockMysqlServiceProxy : public mysql::MysqlServiceProxy {
 public:
  void SetMockServiceProxyOption(const std::shared_ptr<ServiceProxyOption>& option) {
    SetServiceProxyOptionInner(option);
  }

  void SetMockCodec(ClientCodecPtr&& codec) { codec_ = codec; }

   void PrintResultTable(const mysql::MysqlResults<mysql::NativeString>& res) {

     std::vector<std::string> fields_name = res.GetFieldsName();
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
    option_->target = "localhost:3306";
    option_->selector_name = "direct";
    option_->mysql_conf.dbname = "test";
    option_->mysql_conf.password = "abc123";
    option_->mysql_conf.user_name = "root";
    option_->mysql_conf.enable = true;
    option_->max_conn_num = 12;
  }

  static void TearDownTestCase() { TrpcPlugin::GetInstance()->UnregisterPlugins(); }

 protected:
  void SetUp() override {
    mock_mysql_service_proxy_ = std::make_shared<MockMysqlServiceProxy>();
    mock_mysql_service_proxy_->SetMockServiceProxyOption(option_);
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
  MysqlResults<int, std::string> res;
  mock_mysql_service_proxy_->Query(client_context, res, "select id, username from users where id = ?", 1);
  auto& res_vec = res.ResultSet();
  EXPECT_EQ(true, res.OK());
  EXPECT_EQ("alice", std::get<1>(res_vec[0]));
}

TEST_F(MysqlServiceProxyTest, AsyncQuery) {
  // trpc::MysqlServiceProxy proxy;
  auto client_context = GetClientContext();
  auto res =
      mock_mysql_service_proxy_->AsyncQuery<NativeString>(client_context, "select * from users where id >= ?", 1)
          .Then([this](trpc::Future<MysqlResults<NativeString>>&& f) {
            if (f.IsReady()) {
              const auto& res_value = f.GetConstValue();
              EXPECT_EQ(5, res_value.GetFieldsName().size());
              this->mock_mysql_service_proxy_->PrintResultTable(res_value);
            }
            return trpc::MakeReadyFuture<>();
          });

  std::cout << "ret\n";

  ::trpc::future::BlockingGet(std::move(res));
}

TEST_F(MysqlServiceProxyTest, Execute) {
  auto client_context = GetClientContext();
  MysqlResults<OnlyExec> exec_res;
  MysqlTime mtime;
  mtime.SetYear(2024).SetMonth(9).SetDay(10);

  mock_mysql_service_proxy_->Execute(client_context, exec_res,
                                     "insert into users (username, email, created_at) \
                                   values (\"jack\", \"jack@abc.com\", ?)",
                                     mtime);
  EXPECT_EQ(1, exec_res.GetAffectedRowNum());

  client_context = GetClientContext();
  MysqlResults<std::string, MysqlTime> res;
  mock_mysql_service_proxy_->Execute(client_context, res, "select email, created_at from users where username = ?",
                                     "jack");
  auto& res_vec = res.ResultSet();
  EXPECT_EQ(true, res.OK());
  EXPECT_EQ("jack@abc.com", std::get<0>(res_vec[0]));

  client_context = GetClientContext();
  mock_mysql_service_proxy_->Execute(client_context, exec_res, "delete from users where username = \"jack\"");
  EXPECT_EQ(1, exec_res.GetAffectedRowNum());

  client_context = GetClientContext();
  mock_mysql_service_proxy_->Execute(client_context, res, "select email, created_at from users where username = ?",
                                     "jack");
  EXPECT_EQ(true, res.OK());
  EXPECT_EQ(0, res.ResultSet().size());
}

TEST_F(MysqlServiceProxyTest, AsyncExecute) {
  auto client_context = GetClientContext();
  MysqlTime mtime;
  mtime.SetYear(2024).SetMonth(9).SetDay(10);


  auto res = mock_mysql_service_proxy_
                 ->AsyncExecute<OnlyExec>(client_context,
                                                 "insert into users (username, email, created_at) \
                                   values (\"jack\", \"jack@abc.com\", ?)",
                                                 mtime)
                 .Then([](trpc::Future<MysqlResults<OnlyExec>>&& f) {
                   if (f.IsReady()) {
                     EXPECT_EQ(1, f.GetConstValue().GetAffectedRowNum());
                   }
                   return trpc::MakeReadyFuture<>();
                 });
  ::trpc::future::BlockingGet(std::move(res));

  auto res2 = mock_mysql_service_proxy_
                  ->AsyncExecute<std::string, MysqlTime>(
                      client_context, "select email, created_at from users where username = ?", "jack")
                  .Then([](trpc::Future<MysqlResults<std::string, MysqlTime>>&& f) {
                    if (f.IsReady()) {
                      auto& res_vec = f.GetValue0().ResultSet();
                      EXPECT_EQ("jack@abc.com", std::get<0>(res_vec[0]));
                    }
                    return trpc::MakeReadyFuture<>();
                  });
  ::trpc::future::BlockingGet(std::move(res2));

  auto res3 = mock_mysql_service_proxy_
                  ->AsyncExecute<OnlyExec>(client_context, "delete from users where username = \"jack\"")
                  .Then([](trpc::Future<MysqlResults<OnlyExec>>&& f) {
                    if (f.IsReady()) {
                      EXPECT_EQ(1, f.GetConstValue().GetAffectedRowNum());
                    }
                    return trpc::MakeReadyFuture<>();
                  });
  ::trpc::future::BlockingGet(std::move(res3));

  auto res4 = mock_mysql_service_proxy_
                  ->AsyncExecute<std::string, MysqlTime>(
                      client_context, "select email, created_at from users where username = ?", "jack")
                  .Then([](trpc::Future<MysqlResults<std::string, MysqlTime>>&& f) {
                    if (f.IsReady()) {
                      EXPECT_EQ(0, f.GetValue0().ResultSet().size());
                    }
                    return trpc::MakeReadyFuture<>();
                  });
  ::trpc::future::BlockingGet(std::move(res4));
}

TEST_F(MysqlServiceProxyTest, AsyncException) {
  std::string error_msg =
      "You have an error in your SQL syntax; "
      "check the manual that corresponds to your MySQL server version "
      "for the right syntax to use near '' at line 1";
  auto client_context = GetClientContext();
  auto res = mock_mysql_service_proxy_->AsyncQuery<NativeString>(client_context, "select * from users where")
                 .Then([&error_msg](trpc::Future<MysqlResults<NativeString>>&& f) {
                   EXPECT_EQ(true, f.IsFailed());
                   auto e = f.GetException();
                   EXPECT_EQ(error_msg, e.what());
                   return trpc::MakeExceptionFuture<>(e);
                 });

  std::cout << "async call\n";
  ::trpc::future::BlockingGet(std::move(res));
}

TEST_F(MysqlServiceProxyTest, AsyncQueryRepeat) {
  std::vector<trpc::Future<>> futures;
  for (int i = 0; i < 8; i++) {
    auto client_context = GetClientContext();
    auto f = mock_mysql_service_proxy_
                 ->AsyncQuery<NativeString>(client_context, "select * from users where id > ? or username = ?", i,
                                               "alice")
                 .Then([i](trpc::Future<MysqlResults<NativeString>>&& f) {
                   EXPECT_EQ(true, f.IsReady());
                   EXPECT_EQ(true, f.GetValue0().OK());
                   return trpc::MakeReadyFuture<>();
                 });
    futures.push_back(std::move(f));
  }

  for (auto& future : futures) {
    ::trpc::future::BlockingGet(std::move(future));
  }
}


TEST_F(MysqlServiceProxyTest, QueryRepeat) {
  auto client_context = GetClientContext();
  MysqlResults<NativeString> res;
  for (int i = 0; i < 8; i++) {
    mock_mysql_service_proxy_->Query(client_context, res, "select * from users where id > ? or username = ?", i,
                                     "alice");
    EXPECT_EQ(true, res.OK());
  }
}

TEST_F(MysqlServiceProxyTest, ConcurrentQuery) {
  const int kThreadCount = 10;
  std::vector<std::thread> threads;
  std::mutex result_mutex;
  bool all_success = true;

  for (int i = 0; i < kThreadCount; ++i) {
    threads.emplace_back([this, &result_mutex, &all_success, i]() {
      auto client_context = GetClientContext();
      MysqlResults<int, std::string> res;
      mock_mysql_service_proxy_->Query(client_context, res, "select id, username from users where id = ?", i + 1);

      std::lock_guard<std::mutex> lock(result_mutex);
      if (!res.OK()) {
        all_success = false;
      } else {
        auto& res_vec = res.ResultSet();
        if (!res_vec.empty()) {
          std::cout << "Thread " << i << " retrieved username: " << std::get<1>(res_vec[0]) << std::endl;
        }
      }
    });
  }

  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  EXPECT_EQ(true, all_success);
}

TEST_F(MysqlServiceProxyTest, ConcurrentAsyncQueryWithFutures) {
  const int kThreadCount = 10;
  std::vector<trpc::Future<>> futures;
  std::mutex result_mutex;
  bool all_success = true;

  for (int i = 0; i < kThreadCount; ++i) {
    auto client_context = GetClientContext();
    auto future = mock_mysql_service_proxy_
                      ->AsyncQuery<NativeString>(client_context, "select * from users where id >= ?", i + 1)
                      .Then([&result_mutex, &all_success, i](trpc::Future<MysqlResults<NativeString>>&& f) {
                        if (f.IsReady()) {
                          auto& result = f.GetConstValue();
                          std::lock_guard<std::mutex> lock(result_mutex);
                          if (!result.OK()) {
                            all_success = false;
                          } else {
                            std::cout << "Thread " << i << " completed async query successfully" << std::endl;
                          }
                        }
                        return trpc::MakeReadyFuture<>();
                      });

    futures.push_back(std::move(future));
  }

  for (auto& future : futures) {
    ::trpc::future::BlockingGet(std::move(future));
  }

  EXPECT_EQ(true, all_success);
}


TEST_F(MysqlServiceProxyTest, Transaction) {
  auto client_context = GetClientContext();
  TransactionHandle handle;
  MysqlResults<OnlyExec> exec_res;
  MysqlResults<NativeString> query_res;
  MysqlTime mtime;
  mtime.SetYear(2024).SetMonth(9).SetDay(10);

  Status s = mock_mysql_service_proxy_->Begin(client_context, handle);
  EXPECT_EQ(s.OK(), true);
  mock_mysql_service_proxy_->Execute(client_context, handle, exec_res,
                                     "insert into users (username, email, created_at)"
                                     "values (\"jack\", \"jack@abc.com\", ?)", mtime);
  EXPECT_EQ(1, exec_res.GetAffectedRowNum());

  mock_mysql_service_proxy_->Query(client_context, handle, query_res, "select * from users where username = ?", "jack");
  EXPECT_EQ(1, query_res.ResultSet().size());
  mock_mysql_service_proxy_->Rollback(client_context, handle);

  mock_mysql_service_proxy_->Query(client_context, query_res, "select * from users where username = ?", "jack");
  EXPECT_EQ(0, query_res.ResultSet().size());
}

TEST_F(MysqlServiceProxyTest, TransactionNoCommit) {
  auto client_context = GetClientContext();
  TransactionHandle handle;
  MysqlResults<OnlyExec> exec_res;
  MysqlResults<NativeString> query_res;
  MysqlTime mtime;
  mtime.SetYear(2024).SetMonth(9).SetDay(10);

  Status s = mock_mysql_service_proxy_->Begin(client_context, handle);
  EXPECT_EQ(s.OK(), true);
  mock_mysql_service_proxy_->Execute(client_context, handle, exec_res,
                                     "insert into users (username, email, created_at)"
                                     "values (\"jack\", \"jack@abc.com\", ?)", mtime);
  EXPECT_EQ(1, exec_res.GetAffectedRowNum());

  mock_mysql_service_proxy_->Query(client_context, handle, query_res, "select * from users where username = ?", "jack");
  EXPECT_EQ(1, query_res.ResultSet().size());


  handle.GetExecutor()->Close();

  mock_mysql_service_proxy_->Query(client_context, query_res, "select * from users where username = ?", "jack");
  EXPECT_EQ(0, query_res.ResultSet().size());
}

TEST_F(MysqlServiceProxyTest, AsyncTransaction) {
  auto client_context = GetClientContext();
  MysqlResults<OnlyExec> exec_res;
  MysqlResults<NativeString> query_res;
  TransactionHandle handle;
  int table_rows = 0;

  //
  mock_mysql_service_proxy_->Query(client_context, query_res, "select * from users");
  table_rows = query_res.ResultSet().size();

  // Do two query separately in the same one transaction and the handle will be moved to handle2
  auto fu = mock_mysql_service_proxy_->AsyncBegin(client_context)
          .Then([&handle](Future<TransactionHandle>&& f) mutable {
            if(f.IsFailed())
              return MakeExceptionFuture<>(f.GetException());
            handle = f.GetValue0();
            return MakeReadyFuture<>();
          });

  future::BlockingGet(std::move(fu));

  auto fu2 = mock_mysql_service_proxy_
          ->AsyncQuery<NativeString>(client_context, std::move(handle), "select username from users where username = ?", "alice")
          .Then([](Future<TransactionHandle, MysqlResults<NativeString>>&& f) mutable {
            if(f.IsFailed())
              return MakeExceptionFuture<TransactionHandle>(f.GetException());
            auto t = f.GetValue();
            auto res = std::move(std::get<1>(t));
            EXPECT_EQ("alice", res.ResultSet()[0][0]);
            return MakeReadyFuture<TransactionHandle>(std::move(std::get<0>(t)));
          });

  auto fu3 = future::BlockingGet(std::move(fu2));
  TransactionHandle handle2(fu3.GetValue0());
  EXPECT_EQ(TransactionHandle::TxState::kStart, handle2.GetState());

  // Do query in "Then Chain" and rollback
  MysqlTime mtime;
  mtime.SetYear(2024).SetMonth(9).SetDay(10);

  auto fu4 = mock_mysql_service_proxy_
          ->AsyncExecute<OnlyExec>(client_context, std::move(handle2),
                                   "insert into users (username, email, created_at)"
                                   "values (\"jack\", \"jack@abc.com\", ?)", mtime)
          .Then([this, client_context](Future<TransactionHandle, MysqlResults<OnlyExec>>&& f) {
            if(f.IsFailed())
              return MakeExceptionFuture<TransactionHandle, MysqlResults<NativeString>>(f.GetException());
            auto t = f.GetValue();
            auto res = std::move(std::get<1>(t));
            EXPECT_EQ(1, res.GetAffectedRowNum());
            return mock_mysql_service_proxy_->AsyncQuery<NativeString>(client_context, std::move(std::get<0>(t)),
                                                                       "select username from users where username = ?", "jack");
          })
          .Then([this, client_context](Future<TransactionHandle, MysqlResults<NativeString>>&& f){
            if(f.IsFailed())
              return MakeExceptionFuture<TransactionHandle, MysqlResults<OnlyExec>>(f.GetException());
            auto t = f.GetValue();
            auto res = std::move(std::get<1>(t));
            EXPECT_EQ("jack", res.ResultSet()[0][0]);
            return mock_mysql_service_proxy_
                    ->AsyncQuery<OnlyExec>(client_context, std::move(std::get<0>(t)),
                                           "update users set email = ? where username = ? ", "jack@gmail.com", "jack");
          })
          .Then([this, client_context](Future<TransactionHandle, MysqlResults<OnlyExec>>&& f) {
            if(f.IsFailed())
              return MakeExceptionFuture<TransactionHandle>(f.GetException());
            auto t = f.GetValue();
            auto res = std::move(std::get<1>(t));
            EXPECT_EQ(1, res.GetAffectedRowNum());
            return mock_mysql_service_proxy_
                    ->AsyncRollback(client_context, std::move(std::get<0>(t)));
          })
          .Then([](Future<TransactionHandle>&& f){
            if(f.IsFailed())
              return MakeExceptionFuture<>(f.GetException());
            auto handle =f.GetValue0();
            EXPECT_EQ(TransactionHandle::TxState::kEnd, handle.GetState());
            return MakeReadyFuture<>();
          });

  trpc::future::BlockingGet(std::move(fu4));

  // Check rollback
  mock_mysql_service_proxy_->Query(client_context, query_res, "select * from users");
  EXPECT_EQ(table_rows, query_res.ResultSet().size());
  mock_mysql_service_proxy_->Query(client_context, query_res, "select * from users where username = ?", "jack");
  EXPECT_EQ(true, query_res.ResultSet().empty());

}


}  // namespace trpc::testing