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
#include "trpc/util/string_util.h"
#include "trpc/client/make_client_context.h"
#include "trpc/client/mysql/mysql_service_proxy.h"
#include "trpc/client/service_proxy.h"
#include "trpc/client/trpc_client.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/util/random.h"
#include "trpc/log/trpc_log.h"
#include "trpc/coroutine/future.h"
#include "trpc/coroutine/fiber_latch.h"


using trpc::mysql::OnlyExec;
using trpc::mysql::NativeString;
using trpc::mysql::MysqlResults;
using trpc::mysql::MysqlTime;
using trpc::mysql::TransactionHandle;
using trpc::mysql::MysqlBlob;


DEFINE_string(client_config, "fiber_client_client_config.yaml", "trpc cpp framework client_config file");


#define MYSQL_ERROR_CHECK(status, res)  if(!s.OK()) {   \
                                          TRPC_FMT_ERROR("status: {}", s.ToString()); \
                                          return;  \
                                        } else if(!res.OK()) {  \
                                          TRPC_FMT_ERROR("MySQL error: {}", res.GetErrorMessage()); \
                                          return;  \
                                        }  \

MysqlBlob GenRandomBlob(std::size_t length) {
  std::string random_data;
  random_data.reserve(length);

  for (std::size_t i = 0; i < length; ++i) {
    char random_byte = static_cast<char>(trpc::Random<uint8_t>(0, 255));
    random_data.push_back(random_byte);
  }

  return MysqlBlob(std::move(random_data));
}

void PrintResultTable(const MysqlResults<NativeString>& res) {
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

void TestQuery(std::shared_ptr<trpc::mysql::MysqlServiceProxy>& proxy) {
  std::cout << "TestQuery\n";
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  MysqlResults<int, std::string> res;
  trpc::Status s = proxy->Query(ctx, res, "select id, username from users where id = ? and username = ?", 3, "carol");

  MYSQL_ERROR_CHECK(s, res);
  // const std::vector<std::tuple<int, std::string>>& res_set
  auto& res_set = res.ResultSet();
  int id = std::get<0>(res_set[0]);
  std::string username = std::get<1>(res_set[0]);
  std::cout << "id: " << id << ", username: " << username << std::endl;

  MysqlResults<NativeString> res2;
  s = proxy->Query(ctx, res2, "select * from users");

  MYSQL_ERROR_CHECK(s, res2);

  int col_index = 0;
  int row_index = 0;
  for(auto& row : res2.ResultSet()) {
    col_index = 0;
    for(auto field : row) {
      std::cout << (res2.IsValueNull(row_index, col_index) ? "null" : field) << "    ";
      col_index++;
    }
    std::cout << std::endl;
    row_index++;
  }

  std::cout << "\n\n\n";
  PrintResultTable(res2);
}



void TestUpdate(std::shared_ptr<trpc::mysql::MysqlServiceProxy>& proxy) {
  std::cout << "\n\nTestUpdate\n";
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  MysqlResults<OnlyExec> exec_res;
  MysqlResults<std::string, MysqlTime> query_res;
  MysqlTime mtime;
  mtime.SetYear(2024).SetMonth(9).SetDay(10);

  trpc::Status s = proxy->Execute(ctx, exec_res,
                             "insert into users (username, email, created_at)"
                             "values (\"jack\", \"jack@abc.com\", ?)",
                             mtime);
  MYSQL_ERROR_CHECK(s, exec_res);

  if(1 == exec_res.GetAffectedRowNum())
    std::cout << "Insert one\n";

  ctx = trpc::MakeClientContext(proxy);
  s = proxy->Execute(ctx, query_res, "select email, created_at from users where username = ?",
                                     "jack");
  MYSQL_ERROR_CHECK(s, query_res);
  auto& res_vec = query_res.ResultSet();
  std::cout << std::get<0>(res_vec[0]) << std::endl;

  ctx = trpc::MakeClientContext(proxy);
  s = proxy->Execute(ctx, exec_res, "delete from users where username = \"jack\"");
  MYSQL_ERROR_CHECK(s, exec_res);
  if(1 == exec_res.GetAffectedRowNum())
    std::cout << "Delete one\n";

  ctx = trpc::MakeClientContext(proxy);
  s = proxy->Execute(ctx, query_res, "select email, created_at from users where username = ?",
                                     "jack");
  MYSQL_ERROR_CHECK(s, query_res);

  if(query_res.ResultSet().empty())
    std::cout << R"(No user "jack" in users)" << std::endl;
}


void TestTime(std::shared_ptr<trpc::mysql::MysqlServiceProxy>& proxy) {
  std::cout << "\nTestTime\n";
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);

  // Use string
  MysqlResults<NativeString> str_res;

  // Use MysqlTime
  MysqlResults<MysqlTime> time_res;

  trpc::Status s;
  s = proxy->Query(ctx, str_res, "select created_at from users");
  MYSQL_ERROR_CHECK(s, str_res);


  s = proxy->Query(ctx, time_res, "select created_at from users");
  MYSQL_ERROR_CHECK(s, time_res);

  std::string_view str_time = str_res.ResultSet()[0][0];
  MysqlTime my_time = std::get<0>(time_res.ResultSet()[0]);

  std::cout << "NativeString: " << str_time << std::endl;

  std::string fmt_str_time = trpc::util::FormatString("{}-{}-{} {}:{}:{}",
                                                      my_time.GetYear(),
                                                      my_time.GetMonth(),
                                                      my_time.GetDay(),
                                                      my_time.GetHour(),
                                                      my_time.GetMinute(),
                                                      my_time.GetSecond());

  std::cout << "MysqlTime: " << fmt_str_time << std::endl;

  // Or use ToString
  std::cout << "MysqlTime: " << my_time.ToString() << std::endl;


  // Build a MysqlTime
  MysqlTime new_time;
  new_time.SetYear(2024).SetMonth(9).SetDay(10).SetHour(21);
  std::cout << new_time.ToString() << std::endl;

  MysqlTime new_time_from_str;
  new_time_from_str.FromString(new_time.ToString());
  std::cout << new_time_from_str.ToString() << std::endl;
}

void TestCommit(std::shared_ptr<trpc::mysql::MysqlServiceProxy>& proxy) {

  std::cout << "\nTestCommit\n";
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  MysqlResults<trpc::mysql::OnlyExec> res;
  TransactionHandle handle;
  MysqlResults<OnlyExec> exec_res;
  MysqlResults<NativeString> query_res;
  MysqlTime mtime;
  mtime.FromString("2024-09-10");

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
  if(!s.OK() || (exec_res.GetAffectedRowNum() != 1) || !exec_res.OK()) {
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
  mtime.SetYear(2024).SetMonth(9).SetDay(10);


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
  if(!s.OK() || (exec_res.GetAffectedRowNum() != 1) || !exec_res.OK()) {
    TRPC_FMT_ERROR("status: {}, res error: {}", s.ToString(), exec_res.GetErrorMessage());
    return;
  }


  // Query the new row
  s = proxy->Query(ctx, handle, query_res, "select * from users where username = ?", "jack");
  if(!s.OK() || (exec_res.GetAffectedRowNum() != 1) || !exec_res.OK()) {
    TRPC_FMT_ERROR("status: {}, res error: {}", s.ToString(), exec_res.GetErrorMessage());
    return;
  } else if(query_res.ResultSet().size() != 1) {
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
  if(!s.OK() || !exec_res.OK()) {
    TRPC_FMT_ERROR("status: {}, res error: {}", s.ToString(), exec_res.GetErrorMessage());
    return;
  } else if(!query_res.ResultSet().empty()) {
    TRPC_FMT_ERROR("Unexpected.");
    return;
  }

  std::cout << "Rollback transaction end." << std::endl;
}


void TestError(std::shared_ptr<trpc::mysql::MysqlServiceProxy>& proxy) {
  std::cout << "\nTestError\n";
  MysqlResults<int> res;
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);

  // Make context timeout
  ctx->SetTimeout(0);
  trpc::Status s = proxy->Query(ctx, res, "select id from users where username = ?", "alice");
  if(!s.OK()) {
    std::cout << s.ToString() << std::endl;
    TRPC_FMT_ERROR("Status:{}", s.ToString());
  }

  ctx = trpc::MakeClientContext(proxy);
  s = proxy->Query(ctx, res, "select id from users where usernames = ?", "alice");
  if(!s.OK()) {
    TRPC_FMT_ERROR("Status:{}", s.ToString());
    return;
  }
  if(!res.OK()) {
    std::cout << res.GetErrorMessage() << std::endl;
    TRPC_FMT_ERROR("MySQL error:{}", res.GetErrorMessage());
  }
}


void TestFiberAsync(std::shared_ptr<trpc::mysql::MysqlServiceProxy>& proxy) {
  std::cout << "\nTestFiberAsync\n";
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);

  trpc::FiberLatch latch(1);
  auto future = proxy->AsyncQuery<NativeString>(ctx, "select * from users")
          .Then([&latch](trpc::Future<MysqlResults<NativeString>>&& f){
            latch.Wait();
            if(f.IsReady()) {
              auto res = f.GetValue0();
              PrintResultTable(res);
              return trpc::MakeReadyFuture();
            }
            return trpc::MakeExceptionFuture<>(f.GetException());
          });
  std::cout << "do something\n";
  latch.CountDown();
  auto ret_future = trpc::fiber::BlockingGet(std::move(future));

  if(ret_future.IsFailed()) {
    TRPC_FMT_ERROR(ret_future.GetException().what());
    std::cerr << ret_future.GetException().what() << std::endl;
    return;
  }
}




void TestBlob(std::shared_ptr<trpc::mysql::MysqlServiceProxy>& proxy) {
  std::cout << "\nTestBlob\n";
  MysqlResults<OnlyExec> exec_res;
  MysqlBlob blob(GenRandomBlob(1024));

  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  // MysqlBlob
  trpc::Status s = proxy->Execute(ctx, exec_res,
                         "insert into users (username, email, meta)"
                         "values (\"jack\", \"jack@abc.com\", ?)",
                         blob);
  if(s.OK() && exec_res.OK())
    std::cout << "blob inserted.\n";
  else
    return;

  // three mode for Blob
  MysqlResults<MysqlBlob> bind_blob_res;
  MysqlResults<std::string> bind_str_res;

  MysqlResults<NativeString> str_res;

  s = proxy->Query(ctx, bind_blob_res, "select meta from users where username = ?", "jack");
  MYSQL_ERROR_CHECK(s, bind_blob_res);
  if(std::get<0>(bind_blob_res.ResultSet()[0]) == blob)
    std::cout << "same blob\n";


  s = proxy->Query(ctx, bind_str_res, "select meta from users where username = ?", "jack");
  MYSQL_ERROR_CHECK(s, bind_str_res);
  if(std::get<0>(bind_str_res.ResultSet()[0]) == blob.AsStringView())
    std::cout << "same blob\n";

  s = proxy->Query(ctx, str_res, "select meta from users where username = ?", "jack");
  MYSQL_ERROR_CHECK(s, str_res);
  auto str_view = str_res.ResultSet()[0][0];
  if(MysqlBlob(std::string(str_view)) == blob)
    std::cout << "same blob\n";

}



int Run() {
  auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::mysql::MysqlServiceProxy>("mysql_server");
  TestQuery(proxy);
  TestUpdate(proxy);
  TestCommit(proxy);
  TestRollback(proxy);
  TestError(proxy);
  TestFiberAsync(proxy);
  TestBlob(proxy);
  TestTime(proxy);
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
  std::cout << "************************************\n"
            << "************fiber_client************\n"
            << "************************************\n\n";
  // If the business code is running in trpc pure client mode,
  // the business code needs to be running in the `RunInTrpcRuntime` function
  return ::trpc::RunInTrpcRuntime([]() { return Run(); });
}