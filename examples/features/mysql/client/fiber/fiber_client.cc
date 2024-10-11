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

#include <iostream>
#include "trpc/client/make_client_context.h"
#include "trpc/client/mysql/mysql_service_proxy.h"
#include "trpc/client/service_proxy.h"
#include "trpc/client/trpc_client.h"
#include "trpc/common/runtime_manager.h"

#include "trpc/log/trpc_log.h"

using trpc::mysql::OnlyExec;
using trpc::mysql::IterMode;
using trpc::mysql::NativeString;
using trpc::mysql::MysqlResults;
using trpc::mysql::MysqlTime;
using trpc::mysql::TransactionHandle;



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

void TestQuery(std::shared_ptr<trpc::mysql::MysqlServiceProxy>& proxy) {
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  MysqlResults<int, std::string> res;
  proxy->Query(ctx, res, "select id, username from users where id = ? and username = ?", 3, "carol");
  auto& res_data = res.GetResultSet();
  printResult(res_data);

  MysqlResults<IterMode> res2;
  auto s = proxy->Query(ctx, res2, "select * from users");
  for(auto row : res2) {
    int i = 0;
    for(auto field : row) {
      std::cout << (row.IsFieldNull(i++) ? "null" : field) << "    ";
    }
    std::cout << std::endl;
  }

  std::cout << "\n\n\n";
  PrintResultTable(res2);
}

void TestUpdate(std::shared_ptr<trpc::mysql::MysqlServiceProxy>& proxy) {
  std::cout << "\n\n\n";
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  MysqlResults<OnlyExec> exec_res;
  MysqlResults<std::string, MysqlTime> query_res;
  MysqlTime mtime;
  mtime.mt.year = 2024;
  mtime.mt.month = 9;
  mtime.mt.day = 10;

  proxy->Execute(ctx, exec_res,
                             "insert into users (username, email, created_at)"
                             "values (\"jack\", \"jack@abc.com\", ?)",
                             mtime);
  if(1 == exec_res.GetAffectedRows())
    std::cout << "Insert one\n";

  ctx = trpc::MakeClientContext(proxy);
  proxy->Execute(ctx, query_res, "select email, created_at from users where username = ?",
                                     "jack");
  auto& res_vec = query_res.GetResultSet();
  std::cout << std::get<0>(res_vec[0]) << std::endl;

  ctx = trpc::MakeClientContext(proxy);
  proxy->Execute(ctx, exec_res, "delete from users where username = \"jack\"");
  if(1 == exec_res.GetAffectedRows())
    std::cout << "Delete one\n";

  ctx = trpc::MakeClientContext(proxy);
  proxy->Execute(ctx, query_res, "select email, created_at from users where username = ?",
                                     "jack");
  if(query_res.GetResultSet().empty())
    std::cout << R"(No user "jack" in users)" << std::endl;
}

void TestCommit(std::shared_ptr<trpc::mysql::MysqlServiceProxy>& proxy) {

  std::cout << "\nTestCommit\n";
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  MysqlResults<trpc::mysql::OnlyExec> res;
  TransactionHandle handle;
  MysqlResults<OnlyExec> exec_res;
  MysqlResults<IterMode> query_res;
  MysqlTime mtime;
  mtime.mt.year = 2024;
  mtime.mt.month = 9;
  mtime.mt.day = 10;

  trpc::Status s = proxy->Query(ctx, query_res, "select * from users");
  if(!s.OK()) {
    TRPC_FMT_ERROR("status: {}", s.ToString());
    return;
  }

  std::cout << "Before transaction\n\n";
  PrintResultTable(query_res);

  // Begin
  s = proxy->Begin(ctx, handle);
  if(!s.OK()) {
    TRPC_FMT_ERROR("status: {}", s.ToString());
    return;
  }

  // Insert a row
  s = proxy->Execute(ctx, handle, exec_res,
                     "insert into users (username, email, created_at)"
                     "values (\"jack\", \"jack@abc.com\", ?)", mtime);
  if(!s.OK() || (exec_res.GetAffectedRows() != 1) || !exec_res.IsSuccess()) {
    TRPC_FMT_ERROR("status: {}, res error: {}", s.ToString(), exec_res.GetErrorMessage());
    return;
  }

  // Commit
  s = proxy->Commit(ctx, handle);
  if(!s.OK()) {
    TRPC_FMT_ERROR("status: {}", s.ToString());
    return;
  }

  // Print table after commit
  s = proxy->Query(ctx, query_res, "select * from users");
  if(!s.OK()) {
    TRPC_FMT_ERROR("status: {}", s.ToString());
    return;
  }

  std::cout << "\n\nAfter commit\n\n";
  PrintResultTable(query_res);


  // Clean new data
  s = proxy->Execute(ctx, exec_res, "delete from users where username = ?", "jack");
  if(!s.OK()) {
    TRPC_FMT_ERROR("status: {}", s.ToString());
    return;
  }
}

void TestRollback(std::shared_ptr<trpc::mysql::MysqlServiceProxy>& proxy) {
  std::cout << "\n\nTestRollback\n";

  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  MysqlResults<trpc::mysql::OnlyExec> res;
  TransactionHandle handle;
  MysqlResults<OnlyExec> exec_res;
  MysqlResults<NativeString> query_res;
  MysqlTime mtime;
  mtime.mt.year = 2024;
  mtime.mt.month = 9;
  mtime.mt.day = 10;

  // Begin
  trpc::Status s = proxy->Begin(ctx, handle);
  if(!s.OK()) {
    TRPC_FMT_ERROR("status: {}", s.ToString());
    return;
  }

  // Insert a row
  s = proxy->Execute(ctx, handle, exec_res,
                                     "insert into users (username, email, created_at)"
                                     "values (\"jack\", \"jack@abc.com\", ?)", mtime);
  if(!s.OK() || (exec_res.GetAffectedRows() != 1) || !exec_res.IsSuccess()) {
    TRPC_FMT_ERROR("status: {}, res error: {}", s.ToString(), exec_res.GetErrorMessage());
    return;
  }


  // Query the new row
  s = proxy->Query(ctx, handle, query_res, "select * from users where username = ?", "jack");
  if(!s.OK() || (exec_res.GetAffectedRows() != 1) || !exec_res.IsSuccess()) {
    TRPC_FMT_ERROR("status: {}, res error: {}", s.ToString(), exec_res.GetErrorMessage());
    return;
  } else if(query_res.GetResultSet().size() != 1) {
    TRPC_FMT_ERROR("Unexpected.");
    return;
  }


  // Rollback
  s = proxy->Rollback(ctx, handle);
  if(!s.OK()) {
    TRPC_FMT_ERROR("status: {}", s.ToString());
    return;
  }


  // Check Rollback
  s = proxy->Query(ctx, query_res, "select * from users where username = ?", "jack");
  if(!s.OK() || (exec_res.GetAffectedRows() != 1) || !exec_res.IsSuccess()) {
    TRPC_FMT_ERROR("status: {}, res error: {}", s.ToString(), exec_res.GetErrorMessage());
    return;
  } else if(!query_res.GetResultSet().empty()) {
    TRPC_FMT_ERROR("Unexpected.");
    return;
  }

  std::cout << "Rollback transaction end." << std::endl;
}



int Run() {
  auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::mysql::MysqlServiceProxy>("mysql_server");
  TestQuery(proxy);
  TestUpdate(proxy);
  TestCommit(proxy);
  TestRollback(proxy);
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