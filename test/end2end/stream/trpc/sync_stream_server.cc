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

#include "test/end2end/stream/trpc/sync_stream_server.h"

#include "test/end2end/common/util.h"
#include "test/end2end/stream/trpc/stream.trpc.pb.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/util/time.h"

namespace trpc::testing {

using namespace trpc;
using namespace trpc::stream;
using namespace trpc::testing::stream_test;

class SyncStreamService : public TestStreamService {
 public:
  std::map<std::string, std::string> stream_conn_close_status;
  Status StreamAtConnCloseStatus(ServerContextPtr context, const TestRequest* request,
                                 TestResponse* response) override {
    auto it = stream_conn_close_status.find(request->msg());
    if (it != stream_conn_close_status.end()) {
      response->set_msg(it->second);
    } else {
      response->set_msg("failed");
    }
    return kDefaultStatus;
  }

  Status ServerStream(const ServerContextPtr& context, const TestRequest& req,
                      StreamWriter<TestResponse>* writer) override {
    if (req.msg() != "req-msg") {
      return kUnknownErrorStatus;
    }
    Status status;
    for (int i = 0; i < 10; ++i) {
      TestResponse rsp;
      rsp.set_msg(std::string("rsp-msg") + std::to_string(i + 1));
      status = writer->Write(rsp);
      if (status.OK()) {
        continue;
      }
      break;
    }
    return status;
  }

  Status ServerStreamReadTimeout(const ServerContextPtr& context, const TestRequest& req,
                                 StreamWriter<TestResponse>* writer) override {
    if (req.msg() != "req-msg") {
      return kUnknownErrorStatus;
    }
    Status status;
    for (int i = 0; i < 10; ++i) {
      TestResponse rsp;
      rsp.set_msg(std::string("rsp-msg") + std::to_string(i + 1));
      FiberSleepFor(std::chrono::milliseconds(100));
      status = writer->Write(rsp);
      if (status.OK()) {
        continue;
      }
      break;
    }
    return status;
  }

  Status ServerStreamNoData(const ServerContextPtr& context, const TestRequest& req,
                            StreamWriter<TestResponse>* writer) override {
    if (req.msg() != "") {
      return kUnknownErrorStatus;
    }
    return kSuccStatus;
  }

  Status ServerStreamFinishTimeout(const ServerContextPtr& context, const TestRequest& req,
                                   StreamWriter<TestResponse>* writer) override {
    FiberSleepFor(std::chrono::milliseconds(3500));
    return kSuccStatus;
  }

  Status ServerStreamCloseConnectionAtServer(const ServerContextPtr& context, const TestRequest& req,
                                             StreamWriter<TestResponse>* writer) override {
    Status status;
    for (int i = 0; i < 10; ++i) {
      if (i == 5) {
        // Trigger the server's idle connection cleanup.
        FiberSleepFor(std::chrono::milliseconds(4000));
      }
      TestResponse rsp;
      rsp.set_msg(std::string("rsp-msg") + std::to_string(i + 1));
      status = writer->Write(rsp);
      if (!status.OK()) {
        stream_conn_close_status["server_close_conn_server_stream"] = std::to_string(status.GetFrameworkRetCode());
        break;
      }
    }
    return status;
  }

  Status ServerStreamCloseConnectionAtClient(const ServerContextPtr& context, const TestRequest& req,
                                             StreamWriter<TestResponse>* writer) override {
    Status status;
    for (int i = 0; i < 10; ++i) {
      if (i == 5) {
        // Trigger the client's idle connection cleanup.
        FiberSleepFor(std::chrono::milliseconds(11000));
      }
      TestResponse rsp;
      rsp.set_msg(std::string("rsp-msg") + std::to_string(i + 1));
      status = writer->Write(rsp);
      if (!status.OK()) {
        stream_conn_close_status["client_close_conn_server_stream"] = std::to_string(status.GetFrameworkRetCode());
        break;
      }
    }
    return status;
  }

  Status ServerStreamFlowControl(const ServerContextPtr& context, const TestRequest& req,
                                 StreamWriter<TestResponse>* writer) override {
    Status status;
    uint64_t flow_control_trigger_timestamp = 0;
    for (int i = 0; i < 10; ++i) {
      TestResponse rsp;
      rsp.set_msg(std::string(10000, 's'));
      uint64_t begin = trpc::time::GetMilliSeconds();
      status = writer->Write(rsp);
      uint64_t end = trpc::time::GetMilliSeconds();
      if (end - begin > 100) {
        flow_control_trigger_timestamp = end;
      }
      if (status.OK()) {
        continue;
      }
      break;
    }
    TestResponse rsp;
    rsp.set_msg(std::to_string(flow_control_trigger_timestamp));
    status = writer->Write(rsp);
    return status;
  }

  Status ClientStream(const ServerContextPtr& context, const StreamReader<TestRequest>& reader,
                      TestResponse* response) override {
    std::string rsp_msg;
    for (;;) {
      TestRequest req;
      Status status = reader.Read(&req);
      if (status.StreamEof()) {
        break;
      }
      if (!status.OK()) {
        return status;
      }
      rsp_msg += req.msg();
    }
    response->set_msg(rsp_msg);
    return kSuccStatus;
  }

  Status ClientStreamReadTimeout(const ServerContextPtr& context, const StreamReader<TestRequest>& reader,
                                 TestResponse* response) override {
    std::string rsp_msg;
    int timeout_count = 0;
    for (;;) {
      TestRequest req;
      Status status = reader.Read(&req, 90);
      if (status.StreamEof()) {
        break;
      }
      if (status.GetFrameworkRetCode() == TRPC_STREAM_SERVER_READ_TIMEOUT_ERR) {
        ++timeout_count;
        continue;
      }
      if (!status.OK()) {
        return status;
      }
      rsp_msg += req.msg();
    }
    rsp_msg += std::string("timeout_count:") + std::to_string(timeout_count);
    response->set_msg(rsp_msg);
    return kSuccStatus;
  }

  Status ClientStreamNoData(const ServerContextPtr& context, const StreamReader<TestRequest>& reader,
                            TestResponse* response) override {
    TestRequest req;
    Status status = reader.Read(&req);
    if (status.StreamEof()) {
      status = reader.Read(&req);
      if (status.StreamEof()) {
        return kSuccStatus;
      }
    }
    response->set_msg("failed");
    return kSuccStatus;
  }

  Status ClientStreamFinishTimeout(const ServerContextPtr& context, const StreamReader<TestRequest>& reader,
                                   TestResponse* response) override {
    FiberSleepFor(std::chrono::milliseconds(6500));
    return kSuccStatus;
  }

  Status ClientStreamCloseConnectionAtServer(const ServerContextPtr& context, const StreamReader<TestRequest>& reader,
                                             TestResponse* response) override {
    std::string rsp_msg;
    int count = 0;
    Status status;
    for (;;) {
      TestRequest req;
      status = reader.Read(&req);
      if (!status.OK()) {
        stream_conn_close_status["server_close_conn_client_stream"] =
            std::to_string(count) + "-" + std::to_string(status.GetFrameworkRetCode());
        break;
      }
      ++count;
    }
    return status;
  }

  Status ClientStreamCloseConnectionAtClient(const ServerContextPtr& context, const StreamReader<TestRequest>& reader,
                                             TestResponse* response) override {
    std::string rsp_msg;
    int count = 0;
    Status status;
    for (;;) {
      TestRequest req;
      status = reader.Read(&req);
      if (!status.OK()) {
        stream_conn_close_status["client_close_conn_client_stream"] =
            std::to_string(count) + "-" + std::to_string(status.GetFrameworkRetCode());
        break;
      }
      ++count;
    }
    return status;
  }

  Status ClientStreamFlowControl(const ServerContextPtr& context, const StreamReader<TestRequest>& reader,
                                 TestResponse* response) override {
    std::string rsp_msg;
    int count = 0;
    for (;; ++count) {
      TestRequest req;
      FiberSleepFor(std::chrono::milliseconds(100));
      Status status = reader.Read(&req);
      if (count == 1) {
        response->set_msg(std::to_string(trpc::time::GetMilliSeconds()));
      }
      if (status.StreamEof()) {
        break;
      }
      if (!status.OK()) {
        TRPC_LOG_ERROR(status.ErrorMessage());
        return status;
      }
    }
    return kSuccStatus;
  }

  Status BidiStream(const ServerContextPtr& context, const StreamReader<TestRequest>& reader,
                    StreamWriter<TestResponse>* writer) override {
    int count = 0;
    Status status = kSuccStatus;
    for (;;) {
      TestRequest req;
      status = reader.Read(&req);
      if (!status.OK()) {
        break;
      }
      TestResponse rsp;
      rsp.set_msg(req.msg() == std::string("msg") + std::to_string(count + 1) ? req.msg() : "failed");
      status = writer->Write(rsp);
      if (!status.OK()) {
        break;
      }
      ++count;
    }
    return status.StreamEof() ? kSuccStatus : status;
  }

  Status BidiStreamReadTimeout(const ServerContextPtr& context, const StreamReader<TestRequest>& reader,
                               StreamWriter<TestResponse>* writer) override {
    int count = 0;
    Status status = kSuccStatus;
    for (;;) {
      TestRequest req;
      // The first read will timeout.
      status = reader.Read(&req, 90);
      if (status.StreamEof()) {
        break;
      }
      if (status.GetFrameworkRetCode() != TRPC_STREAM_SERVER_READ_TIMEOUT_ERR) {
        return status;
      }
      // The second read will succeed.
      status = reader.Read(&req, 90);
      TestResponse rsp;
      rsp.set_msg(req.msg() == std::string("msg") + std::to_string(count + 1) ? req.msg() : "failed");
      // sleep 100ms before write
      FiberSleepFor(std::chrono::milliseconds(100));
      status = writer->Write(rsp);
      if (!status.OK()) {
        return status;
      }
      ++count;
    }
    return status.StreamEof() ? kSuccStatus : status;
  }

  Status BidiStreamNoData(const ServerContextPtr& context, const StreamReader<TestRequest>& reader,
                          StreamWriter<TestResponse>* writer) override {
    TestRequest req;
    Status status;
    if (!reader.Read(&req).StreamEof()) {
      TestResponse rsp;
      rsp.set_msg("failed");
      status = writer->Write(rsp);
    }
    return status;
  }

  Status BidiStreamFinishTimeout(const ServerContextPtr& context, const StreamReader<TestRequest>& reader,
                                 StreamWriter<TestResponse>* writer) override {
    FiberSleepFor(std::chrono::milliseconds(6500));
    return kSuccStatus;
  }

  Status BidiStreamCloseConnectionAtServer(const ServerContextPtr& context, const StreamReader<TestRequest>& reader,
                                           StreamWriter<TestResponse>* writer) override {
    int count = 0;
    Status status;
    for (;;) {
      TestRequest req;
      status = reader.Read(&req);
      bool check_read_ret = status.GetFrameworkRetCode() == TRPC_STREAM_SERVER_NETWORK_ERR;
      TestResponse rsp;
      rsp.set_msg(req.msg() == std::string("msg") + std::to_string(count + 1) ? req.msg() : "failed");
      status = writer->Write(rsp);
      if (!status.OK()) {
        if (check_read_ret) {
          stream_conn_close_status["server_close_conn_bidi_stream"] =
              std::to_string(count) + "-" + std::to_string(status.GetFrameworkRetCode());
        }
        break;
      }
      ++count;
    }
    return status;
  }

  Status BidiStreamCloseConnectionAtClient(const ServerContextPtr& context, const StreamReader<TestRequest>& reader,
                                           StreamWriter<TestResponse>* writer) override {
    int count = 0;
    Status status;
    for (;;) {
      TestRequest req;
      status = reader.Read(&req);
      bool check_read_ret = status.GetFrameworkRetCode() == TRPC_STREAM_SERVER_NETWORK_ERR;
      TestResponse rsp;
      rsp.set_msg(req.msg() == std::string("msg") + std::to_string(count + 1) ? req.msg() : "failed");
      status = writer->Write(rsp);
      if (!status.OK()) {
        if (check_read_ret) {
          stream_conn_close_status["client_close_conn_bidi_stream"] =
              std::to_string(count) + "-" + std::to_string(status.GetFrameworkRetCode());
        }
        break;
      }
      ++count;
    }
    return status;
  }
};

int SyncStreamServer::Initialize() {
  RegisterService("trpc.testing.stream_test.SyncStreamService", std::make_shared<SyncStreamService>());
  RegisterService("trpc.testing.stream_test.SyncStreamServiceConnIdle", std::make_shared<SyncStreamService>());
  RegisterService("trpc.testing.stream_test.SyncStreamServiceNoFlowControl", std::make_shared<SyncStreamService>());

  test_signal_->SignalClientToContinue();

  return 0;
}

void SyncStreamServer::Destroy() { GcovFlush(); }

}  // namespace trpc::testing
