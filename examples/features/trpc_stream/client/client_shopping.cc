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

#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "gflags/gflags.h"

#include "trpc/client/make_client_context.h"
#include "trpc/client/trpc_client.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/common/status.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/util/log/logging.h"

#include "examples/features/trpc_stream/server/stream.trpc.pb.h"

DEFINE_string(service_name, "trpc.test.shopping.StreamShopping", "callee service name");
DEFINE_string(client_config, "trpc_cpp_fiber.yaml", "");
DEFINE_string(addr, "127.0.0.1:24757", "ip:port");
DEFINE_string(rpc_method, "ClientStreamShopping", "RPC method name");
DEFINE_int32(request_count, 3, "count of request");

namespace test::shopping {

using StreamShoppingServiceProxy = ::trpc::test::shopping::StreamShoppingServiceProxy;
using StreamShoppingServiceProxyPtr = std::shared_ptr<StreamShoppingServiceProxy>;

int GetRequestCount(int request_count) {
  if (request_count > 0) {
    return std::min(100, FLAGS_request_count);
  }
  return std::numeric_limits<int>::max();
}

bool CallClientStreamShopping(const StreamShoppingServiceProxyPtr& proxy, int request_count) {
  auto context = ::trpc::MakeClientContext(proxy);
  ::trpc::test::shopping::ShoppingReply reply;
  int send_count{0};
  int send_bytes{0};
  ::trpc::Status status{0, 0, "OK"};
  bool ok{true};
  do {
    auto stream = proxy->ClientStreamShopping(context, &reply);
    if (!stream.GetStatus().OK()) {
      std::cerr << "stream error:" << stream.GetStatus().ToString() << std::endl;
      ok = false;
      break;
    }

    for (int i = 0; i < request_count; ++i) {
      ::trpc::test::shopping::ShoppingRequest request;
      request.set_msg("ClientStreamShopping#" + std::to_string(i + 1));
      status = stream.Write(request);
      if (status.OK()) {
        ++send_count;
        send_bytes += request.msg().size();
        std::cout << "send request msg:" << request.msg() << std::endl;
        continue;
      }
      break;
    }

    // Check: last write is ok.
    if (status.OK()) {
      status = stream.WriteDone();
      if (status.OK()) {
        // Waits the final status of the RPC calling.
        status = stream.Finish();
      } else {
        std::cerr << "write done error: " << status.ToString() << std::endl;
        ok = false;
      }
    } else {
      std::cerr << "write error: " << status.ToString() << std::endl;
      ok = false;
    }

    if (status.OK()) {
      std::cout << "stream rpc succeed, send count: " << send_count << ", send bytes: " << send_bytes
                << ", reply: " << reply.msg() << std::endl;
    } else {
      std::cerr << "stream rpc failed:" << status.ToString() << std::endl;
      ok = false;
    }
  } while (0);
  return ok;
}

bool CallServerStreamShopping(const StreamShoppingServiceProxyPtr& proxy) {
  auto context = ::trpc::MakeClientContext(proxy);
  ::trpc::test::shopping::ShoppingRequest request;
  request.set_msg("ServerStreamShopping");
  auto stream = proxy->ServerStreamShopping(context, request);
  ::trpc::Status status{0, 0, "OK"};
  bool ok{true};
  do {
    if (!stream.GetStatus().OK()) {
      std::cout << "stream error:" << stream.GetStatus().ToString() << std::endl;
      ok = false;
      break;
    }

    for (;;) {
      ::trpc::test::shopping::ShoppingReply reply;
      status = stream.Read(&reply, 2000);
      if (status.OK()) {
        std::cout << "stream got reply:" << reply.msg() << std::endl;
        continue;
      }
      if (status.StreamEof()) {
        std::cout << "got EOF" << std::endl;
        // Waits the final status of the RPC calling.
        status = stream.Finish();
      }
      break;
    }

    if (status.OK()) {
      std::cout << "stream rpc succeed" << std::endl;
    } else {
      std::cerr << "stream rpc failed:" << status.ToString() << std::endl;
      ok = false;
    }
  } while (0);
  return ok;
}

bool CallBidiStreamShopping(const StreamShoppingServiceProxyPtr& proxy, int request_count) {
  auto context = ::trpc::MakeClientContext(proxy);
  auto stream = proxy->BidiStreamShopping(context);
  ::trpc::Status status{0, 0, "OK"};
  bool ok{true};
  do {
    if (!stream.GetStatus().OK()) {
      std::cout << "stream error:" << stream.GetStatus().ToString() << std::endl;
      ok = false;
      break;
    }

    for (int i = 0; i < request_count; ++i) {
      std::stringstream request_msg;
      request_msg << "BidiStreamShopping, " << (i + 1);
      ::trpc::test::shopping::ShoppingRequest request;
      request.set_msg(request_msg.str());
      status = stream.Write(request);
      if (status.OK()) {
        continue;
      }
      break;
    }

    if (!status.OK()) {
      std::cerr << "write error: " << status.ToString() << std::endl;
      ok = false;
      break;
    }

    status = stream.WriteDone();
    if (!status.OK()) {
      std::cerr << "write done error: " << status.ToString() << std::endl;
      ok = false;
      break;
    }

    ::trpc::test::shopping::ShoppingReply reply;
    for (;;) {
      status = stream.Read(&reply, 2000);
      if (status.OK()) {
        std::stringstream reply_msg;
        reply_msg << "stream got reply:" << reply.msg();
        std::cout << reply_msg.str() << std::endl;
        continue;
      }
      if (status.StreamEof()) {
        std::cout << "got EOF" << std::endl;
        // Waits the final status of the RPC calling.
        status = stream.Finish();
      }
      break;
    }

    if (status.OK()) {
      std::cout << "stream rpc succeed" << std::endl;
    } else {
      std::cerr << "stream rpc failed:" << status.ToString() << std::endl;
      ok = false;
    }
  } while (0);
  return ok;
}

namespace test::shopping {

bool CallPurchase(const StreamShoppingServiceProxyPtr& proxy, int purchase_count) {
  auto context = ::trpc::MakeClientContext(proxy);
  ::trpc::test::shopping::ShoppingRequest request;
  ::trpc::test::shopping::ShoppingReply reply;
  request.set_purchase_count(purchase_count);

  ::trpc::Status status = proxy->Purchase(context, request, &reply);
  if (status.OK()) {
    std::cout << "抢购结果: " << (reply.success() ? "成功" : "失败") << std::endl;
    std::cout << "消息: " << reply.msg() << std::endl;
    std::cout << "剩余库存: " << reply.remaining_stock() << std::endl;
    return reply.success();
  } else {
    std::cerr << "调用失败: " << status.ToString() << std::endl;
    return false;
  }
}

int Run() {
  bool final_ok{true};

  struct calling_args_t {
    std::string calling_name;
    std::function<bool()> calling_executor;
    bool ok;
  };
  std::vector<calling_args_t> callings{};

  int request_count = GetRequestCount(FLAGS_request_count);
  std::string rpc_method = FLAGS_rpc_method;

  ::trpc::ServiceProxyOption option;
  option.name = FLAGS_service_name;
  option.codec_name = "trpc";
  option.network = "tcp";
  option.conn_type = "long";
  option.timeout = 1000;
  option.selector_name = "direct";
  option.target = FLAGS_addr;

  auto stream_shopping_proxy =
      ::trpc::GetTrpcClient()->GetProxy<StreamShoppingServiceProxy>(FLAGS_service_name, option);

  std::string calling_name{""};
  std::function<bool()> calling_executor{nullptr};
  if (rpc_method == "ClientStreamShopping") {
    calling_name = "Streaming RPC, ClientStreamShopping";
    calling_executor = [&stream_shopping_proxy, request_count]() {
      return CallClientStreamShopping(stream_shopping_proxy, request_count);
    };
  } else if (rpc_method == "ServerStreamShopping") {
    calling_name = "Streaming RPC, ServerStreamShopping";
    calling_executor = [&stream_shopping_proxy]() { return CallServerStreamShopping(stream_shopping_proxy); };
  } else if (rpc_method == "BidiStreamShopping") {
    calling_name = "Streaming RPC, BidiStreamShopping";
    calling_executor = [&stream_shopping_proxy, request_count]() {
      return CallBidiStreamShopping(stream_shopping_proxy, request_count);
    };
  } else if (rpc_method == "Purchase") {
    calling_name = "RPC, Purchase";
    calling_executor = [&stream_shopping_proxy, request_count]() {
      return CallPurchase(stream_shopping_proxy, request_count);
    };
  } else {
    std::cout << "RPC method is empty, nothing todo" << std::endl;
    return 0;
  }
  // Executing multiple cases is to send concurrent requests.
  for (int i = 0; i < 8; i++) {
    callings.push_back({calling_name + std::to_string(i + 1), calling_executor, false});
  }

  auto latch_count = static_cast<std::ptrdiff_t>(callings.size());
  ::trpc::FiberLatch callings_latch{latch_count};

  for (auto& c : callings) {
    ::trpc::StartFiberDetached([&callings_latch, &c]() {
      c.ok = c.calling_executor();
      callings_latch.CountDown();
    });
  }
  callings_latch.Wait();

  for (const auto& c : callings) {
    final_ok &= c.ok;
    std::cout << "name: " << c.calling_name << ", ok: " << c.ok << std::endl;
  }

  std::cout << "final result of streaming RPC calling: " << final_ok << std::endl;
  return final_ok ? 0 : -1;
}

}  // namespace test::shopping

bool ParseClientConfig(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::CommandLineFlagInfo info;
  if (GetCommandLineFlagInfo("client_config", &info) && info.is_default) {
    std::cerr << "start client with client_config, for example: " << argv[0]
              << " --client_config=/client/client_config/filepath" << std::endl;
    return false;
  }
  std::cout << "FLAGS_service_name: " << FLAGS_service_name << std::endl;
  std::cout << "FLAGS_client_config: " << FLAGS_client_config << std::endl;
  std::cout << "FLAGS_addr: " << FLAGS_addr << std::endl;
  std::cout << "FLAGS_rpc_method: " << FLAGS_rpc_method << std::endl;
  std::cout << "FLAGS_request_count: " << FLAGS_request_count << std::endl;
  return true;
}

int main(int argc, char* argv[]) {
  if (!ParseClientConfig(argc, argv)) {
    exit(-1);
  }

  if (::trpc::TrpcConfig::GetInstance()->Init(FLAGS_client_config) != 0) {
    std::cerr << "load client_config failed." << std::endl;
    exit(-1);
  }

  // If the business code is running in trpc pure client mode, the business code needs to be running in the
  // `RunInTrpcRuntime` function
  return ::trpc::RunInTrpcRuntime([]() { return test::helloworld::Run(); });
}
