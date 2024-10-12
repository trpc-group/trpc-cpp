//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include <gflags/gflags.h>

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <iostream>
#include "trpc/client/client_context.h"
#include "trpc/client/make_client_context.h"
#include "trpc/client/mysql/mysql_service_proxy.h"
#include "trpc/client/service_proxy.h"
#include "trpc/client/trpc_client.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/log/trpc_log.h"
#include "trpc/util/latch.h"
#include "trpc/future/future_utility.h"

using trpc::mysql::OnlyExec;
using trpc::mysql::IterMode;
using trpc::mysql::NativeString;
using trpc::mysql::MysqlResults;
using trpc::mysql::MysqlTime;
using trpc::mysql::TransactionHandle;
using trpc::Future;

DEFINE_string(client_config, "fiber_client_client_config.yaml", "trpc cpp framework client_config file");

void printResult(const std::vector<std::tuple<int, std::string>>& res_data) {
  for (const auto& tuple : res_data) {
    int id = std::get<0>(tuple);
    std::string username = std::get<1>(tuple);
    std::cout << "ID: " << id << ", Username: " << username << std::endl;
  }
}

void PrintResultTable(const MysqlResults<IterMode>& res) {
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
      std::cout << std::left << std::setw(column_widths[i] + 2) << (field_itr.IsNull() ? "null" : *field_itr);
      ++i;
    }
    std::cout << std::endl;
  }
}


void TestAsyncQuery(std::shared_ptr<trpc::mysql::MysqlServiceProxy>& proxy) {
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  MysqlResults<int, std::string> res;

  using ResultType = MysqlResults<int, std::string>;
  auto future = proxy->AsyncQuery<int, std::string>(ctx, "select id, username from users where id = ?", 3)
                        .Then([](trpc::Future<ResultType>&& f){
                          if(f.IsReady()) {
                            auto res = f.GetValue0();
                            printResult(res.GetResultSet());
                            return trpc::MakeReadyFuture();
                          }
                          return trpc::MakeExceptionFuture<>(f.GetException());
                        });
  std::cout << "do something\n";
  trpc::future::BlockingGet(std::move(future));
}


void TestAsyncTx(std::shared_ptr<trpc::mysql::MysqlServiceProxy>& proxy) {
  MysqlResults<OnlyExec> exec_res;
  MysqlResults<IterMode> query_res;
  TransactionHandle handle;
  int table_rows = 0;

  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  proxy->Query(ctx, query_res, "select * from users");
  table_rows = query_res.GetResultSet().size();

  // Do two query separately in the same one transaction and the handle will be moved to handle2
  auto fu = proxy->AsyncBegin(ctx)
          .Then([&handle](Future<TransactionHandle>&& f) mutable {
            if(f.IsFailed())
              return trpc::MakeExceptionFuture<>(f.GetException());
            handle = f.GetValue0();
            return trpc::MakeReadyFuture<>();
          });

  trpc::future::BlockingGet(std::move(fu));

  auto fu2 = proxy
          ->AsyncQuery<IterMode>(ctx, std::move(handle), "select username from users where username = ?", "alice")
          .Then([](Future<TransactionHandle, MysqlResults<IterMode>>&& f) mutable {
            if(f.IsFailed())
              return trpc::MakeExceptionFuture<TransactionHandle>(f.GetException());
            auto t = f.GetValue();
            auto res = std::move(std::get<1>(t));

            std::cout << "\n>>> select username from users where username = alice\n";
            PrintResultTable(res);
            return trpc::MakeReadyFuture<TransactionHandle>(std::move(std::get<0>(t)));
          });

  auto fu3 = trpc::future::BlockingGet(std::move(fu2));
  TransactionHandle handle2(fu3.GetValue0());

  // Do query in "Then Chain" and rollback
  MysqlTime mtime;
  mtime.mt.year = 2024;
  mtime.mt.month = 9;
  mtime.mt.day = 10;
  auto fu4 = proxy
          ->AsyncExecute<OnlyExec>(ctx, std::move(handle2),
                                   "insert into users (username, email, created_at)"
                                   "values (\"jack\", \"jack@abc.com\", ?)", mtime)
          .Then([proxy, ctx](Future<TransactionHandle, MysqlResults<OnlyExec>>&& f) {
            if(f.IsFailed())
              return trpc::MakeExceptionFuture<TransactionHandle, MysqlResults<OnlyExec>>(f.GetException());
            auto t = f.GetValue();
            auto res = std::move(std::get<1>(t));
            std::cout << "\n>>> "
                      << "insert into users (username, email, created_at)\n"
                      << "values (\"jack\", \"jack@abc.com\", \"2024-9-10\")\n\n"
                      << "affected rows: "
                      << res.GetAffectedRows()
                      << "\n";

            return proxy
                    ->AsyncQuery<OnlyExec>(ctx, std::move(std::get<0>(t)),
                                           "update users set email = ? where username = ? ", "jack@gmail.com", "jack");
          })
          .Then([proxy, ctx](Future<TransactionHandle, MysqlResults<OnlyExec>>&& f) {
            if(f.IsFailed())
              return trpc::MakeExceptionFuture<TransactionHandle, MysqlResults<IterMode>>(f.GetException());
            auto t = f.GetValue();
            auto res = std::move(std::get<1>(t));
            std::cout << "\n>>> "
                      << "update users set email = \"jack@gmail.com\" where username = \"jack\"\n\n"
                      << "affected rows: "
                      << res.GetAffectedRows()
                      << "\n";

            return proxy->AsyncQuery<IterMode>(ctx, std::move(std::get<0>(t)), "select * from users");
          })
          .Then([proxy, ctx](Future<TransactionHandle, MysqlResults<IterMode>>&& f){
            if(f.IsFailed())
              return trpc::MakeExceptionFuture<TransactionHandle>(f.GetException());
            auto t = f.GetValue();
            auto res = std::move(std::get<1>(t));

            std::cout << "\n>>> select * from users\n";
            PrintResultTable(res);
            std::cout << "\n\n";

            return proxy
                    ->AsyncRollback(ctx, std::move(std::get<0>(t)));
          })
          .Then([proxy, ctx](Future<TransactionHandle>&& f){
            if(f.IsFailed())
              return trpc::MakeExceptionFuture<>(f.GetException());

            std::cout << "\n>>> rollback\n"
                      << "transaction end\n"
                      << "\n>>> select * from users\n";
            MysqlResults<IterMode> query_res;
            proxy->Query(ctx, query_res, "select * from users");
            PrintResultTable(query_res);

            return trpc::MakeReadyFuture<>();
          });

  trpc::future::BlockingGet(std::move(fu4));

}

int Run() {
  auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::mysql::MysqlServiceProxy>("mysql_server");
  TestAsyncQuery(proxy);
  TestAsyncTx(proxy);
  return 0;
}

void ParseClientConfig(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::CommandLineFlagInfo info;
  if (GetCommandLineFlagInfo("client_config", &info) && info.is_default) {
    std::cerr << "start client with client_config, for example: " << argv[0]
              << " --client_config=/client/client_config/filepath" << std::endl;
    exit(-1);
  }
  std::cout << "FLAGS_client_config:" << FLAGS_client_config << std::endl;

  int ret = ::trpc::TrpcConfig::GetInstance()->Init(FLAGS_client_config);
  if (ret != 0) {
    std::cerr << "load client_config failed." << std::endl;
    exit(-1);
  }
}

int main(int argc, char* argv[]) {
  ParseClientConfig(argc, argv);

  // If the business code is running in trpc pure client mode,
  // the business code needs to be running in the `RunInTrpcRuntime` function
  return ::trpc::RunInTrpcRuntime([]() { return Run(); });
}