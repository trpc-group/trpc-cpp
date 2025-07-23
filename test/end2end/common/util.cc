//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "test/end2end/common/util.h"

#include <cstring>
#include <functional>
#include <iostream>

#include "gflags/gflags.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/common/future/future.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/runtime.h"
#include "trpc/runtime/threadmodel/merge/merge_thread_model.h"
#include "trpc/util/thread/latch.h"

DEFINE_string(client_config, "xxx.yaml", "trpc cpp framework config file");

namespace trpc::testing {

bool ExtractServerAndClientArgs(int argc, char** argv, int* test_argc, char*** client_argv, char*** server_argv) {
  if (argc < 3) {
    std::cerr << "./xxx_test --config=client_conf.yaml --config=server_conf.yaml" << std::endl;
    return false;
  }

  Function<void(char*&, char*)> copy_fn = [](char*& dst, char* src) {
    int src_len = strlen(src);
    dst = new char[src_len + 1];
    strcpy(dst, src);
    dst[src_len] = 0;
  };

  *test_argc = argc - 1;

  *client_argv = new char*[*test_argc];
  *server_argv = new char*[*test_argc];

  int idx_client = 0;
  int idx_server = 0;
  for (int i = 0; i < argc; ++i) {
    if (i == 1) {
      // client configuration
      copy_fn((*client_argv)[idx_client++], argv[i]);
      continue;
    } else if (i == 2) {
      // server configuration
      copy_fn((*server_argv)[idx_server++], argv[i]);
      continue;
    }
    // Copy all other parameters.
    copy_fn((*client_argv)[idx_client++], argv[i]);
    copy_fn((*server_argv)[idx_server++], argv[i]);
  }

  return true;
}

namespace {

bool IsFiberEnv() { return ::trpc::runtime::GetRuntimeType() == ::trpc::runtime::kFiberRuntime; }

}  // namespace

void RunByEnv(Function<void(void)>&& fn) {
  if (IsFiberEnv()) {
    // run as fiber
    ::trpc::Latch l(1);
    ::trpc::StartFiberDetached([&] {
      fn();
      l.count_down();
    });
    l.wait();
  } else {
    // run as thread
    fn();
  }
}

Future<> RunAsMerge(Function<void(PromPtr)>&& fn) {
  auto prom = std::make_shared<Promise<>>();
  //  always select the same reactor
  auto* reactor = trpc::merge::GetReactor(merge::RandomGetMergeThreadModel(), 1);
  reactor->SubmitTask([prom, fn = std::move(fn)]() {
    fn(prom);
  });
  return prom->GetFuture();
}

void InitializeRuntimeEnv(int argc, char** argv) {
  ::google::ParseCommandLineFlags(&argc, &argv, false);

  if (::trpc::TrpcConfig::GetInstance()->Init(FLAGS_client_config) != 0) {
    std::cerr << "load config failed." << std::endl;
    exit(EXIT_FAILURE);
  }

  ::trpc::InitFrameworkRuntime();

  RunByEnv([]() { ::trpc::TrpcPlugin::GetInstance()->RegisterPlugins(); });
}

void DestroyRuntimeEnv() {
  RunByEnv([]() { ::trpc::TrpcPlugin::GetInstance()->UnregisterPlugins(); });

  ::trpc::DestroyFrameworkRuntime();
}

bool CheckJsonKV(const ::rapidjson::Document& data, const char* key, const char* value) {
  if (!data.HasMember(key)) {
    std::cerr << "CheckJsonKV: has no such member->" << key << std::endl;
    return false;
  }
  std::cout << "CheckJsonKv: compare " << data[key].GetString() << "[json]<->" << value << "[check]" << std::endl;
  return std::strcmp(data[key].GetString(), value) == 0;
}

std::string ConstructJsonStr(const std::map<std::string, std::string>& data) {
  ::rapidjson::Document d;
  d.SetObject();
  for (const auto& [k, v] : data) {
    d.AddMember(::rapidjson::StringRef(k.c_str()), ::rapidjson::StringRef(v.c_str()), d.GetAllocator());
  }
  ::rapidjson::StringBuffer buffer;
  ::rapidjson::Writer<::rapidjson::StringBuffer> writer(buffer);
  d.Accept(writer);
  return buffer.GetString();
}

#if __GNUC__ >= 11
  extern "C" void __gcov_dump();
#else
  extern "C" void __gcov_flush();
#endif

void GcovFlush() {
#if __GNUC__ >= 11
  __gcov_dump();
#else
  __gcov_flush();
#endif
}

}  // namespace trpc::testing
