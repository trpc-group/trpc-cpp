// Copyright (c) 2023, Tencent Inc.
// All rights reserved.

#include "test/end2end/stream/trpc/async_stream_server.h"

#include "test/end2end/common/util.h"
#include "test/end2end/stream/trpc/stream.trpc.pb.h"
#include "trpc/common/async_timer.h"
#include "trpc/common/future/future_utility.h"

namespace trpc::testing {

using namespace trpc;
using namespace trpc::stream;
using namespace trpc::testing::stream_test;

using namespace std::chrono_literals;

class AsyncStreamService : public AsyncTestStreamService {
 public:
  std::map<std::string, std::string> stream_conn_close_status;
  Future<TestResponse> StreamAtConnCloseStatus(const ServerContextPtr& context, const TestRequest* request) override {
    TestResponse rsp;
    auto it = stream_conn_close_status.find(request->msg());
    if (it != stream_conn_close_status.end()) {
      rsp.set_msg(it->second);
    } else {
      rsp.set_msg("failed");
    }
    return MakeReadyFuture<TestResponse>(std::move(rsp));
  }

  Future<> ServerStream(const ServerContextPtr& context, TestRequest&& req,
                        const AsyncWriterPtr<TestResponse>& writer) override {
    if (req.msg() != "req-msg") {
      return MakeExceptionFuture<>(StreamError(kUnknownErrorStatus));
    }
    return DoFor(10, [writer, req = std::move(req)](std::size_t i) {
      TestResponse rsp;
      rsp.set_msg(std::string("rsp-msg") + std::to_string(i + 1));
      return writer->Write(std::move(rsp));
    });
  }

  Future<> ServerStreamReadTimeout(const ServerContextPtr& context, TestRequest&& req,
                                   const AsyncWriterPtr<TestResponse>& writer) override {
    if (req.msg() != "req-msg") {
      return MakeExceptionFuture<>(StreamError(kUnknownErrorStatus));
    }
    return DoFor(10, [writer, req = std::move(req)](std::size_t i) {
      return AsyncTimer(false).After(100).Then([writer, i]() {
        TestResponse rsp;
        rsp.set_msg(std::string("rsp-msg") + std::to_string(i + 1));
        return writer->Write(std::move(rsp));
      });
    });
  }

  Future<> ServerStreamNoData(const ServerContextPtr& context, TestRequest&& req,
                              const AsyncWriterPtr<TestResponse>& writer) override {
    if (req.msg() != "") {
      return MakeExceptionFuture<>(StreamError(kUnknownErrorStatus));
    }
    return MakeReadyFuture<>();
  }

  Future<> ServerStreamFinishTimeout(const ServerContextPtr& context, TestRequest&& req,
                                     const AsyncWriterPtr<TestResponse>& writer) override {
    return AsyncTimer(false).After(3500).Then([]() {
      //
      return MakeReadyFuture<>();
    });
  }

  Future<> ServerStreamCloseConnectionAtServer(const ServerContextPtr& context, TestRequest&& req,
                                               const AsyncWriterPtr<TestResponse>& writer) override {
    return DoFor(10, [&, context, writer, req = std::move(req)](std::size_t i) {
      Future fut = MakeReadyFuture<>();
      if (i == 5) {
        fut = AsyncTimer(false).After(3000).Then([]() { return MakeReadyFuture<>(); });
      }
      return fut.Then([&, writer, i]() {
        TestResponse rsp;
        rsp.set_msg(std::string("rsp-msg") + std::to_string(i + 1));
        return writer->Write(std::move(rsp)).Then([&](Future<>&& ft) {
          if (ft.IsReady()) {
            TRPC_FMT_INFO("ServerStreamCloseConnectionAtServer, write ok");
            return MakeReadyFuture<>();
          }
          auto ex = ft.GetException();
          TRPC_FMT_ERROR("ServerStreamCloseConnectionAtServer, write error: {}", ex.what());
          stream_conn_close_status["server_close_conn_server_stream"] = std::to_string(ex.GetExceptionCode());
          return MakeExceptionFuture<>(ex);
        });
      });
    });
  }

  Future<> ServerStreamCloseConnectionAtClient(const ServerContextPtr& context, TestRequest&& req,
                                               const AsyncWriterPtr<TestResponse>& writer) override {
    return DoFor(10, [&, context, writer, req = std::move(req)](std::size_t i) {
      Future fut = MakeReadyFuture<>();
      if (i == 5) {
        fut = AsyncTimer(false).After(11000).Then([]() { return MakeReadyFuture<>(); });
      }
      return fut.Then([&, writer, i]() {
        TestResponse rsp;
        rsp.set_msg(std::string("rsp-msg") + std::to_string(i + 1));
        return writer->Write(std::move(rsp)).Then([&](Future<>&& ft) {
          if (ft.IsReady()) {
            return MakeReadyFuture<>();
          }
          auto ex = ft.GetException();
          TRPC_FMT_ERROR("ServerStreamCloseConnectionAtServer, write error: {}", ex.what());
          stream_conn_close_status["client_close_conn_server_stream"] = std::to_string(ex.GetExceptionCode());
          return MakeExceptionFuture<>(ex);
        });
      });
    });
  }

  Future<TestResponse> ClientStream(const ServerContextPtr& context,
                                    const AsyncReaderPtr<TestRequest>& reader) override {
    auto rsp_msg = std::make_shared<std::string>();
    return DoUntil([reader, rsp_msg]() {
             return reader->Read().Then([reader, rsp_msg](std::optional<TestRequest>&& req) {
               if (req) {
                 *rsp_msg += req->msg();
                 return MakeReadyFuture<bool>(true);
               } else {
                 return MakeReadyFuture<bool>(false);
               }
             });
           })
        .Then([rsp_msg]() {
          TestResponse rsp;
          rsp.set_msg(*rsp_msg);
          return MakeReadyFuture<TestResponse>(std::move(rsp));
        });
  }

  Future<TestResponse> ClientStreamReadTimeout(const ServerContextPtr& context,
                                               const AsyncReaderPtr<TestRequest>& reader) override {
    auto rsp_msg = std::make_shared<std::string>();
    auto timeout_count = std::make_shared<int>(0);
    return DoUntil([reader, rsp_msg, timeout_count]() {
             return reader->Read(std::chrono::milliseconds(90))
                 .Then([reader, rsp_msg, timeout_count](Future<std::optional<TestRequest>>&& req_fut) {
                   if (req_fut.IsFailed()) {
                     Exception ex = req_fut.GetException();
                     if (ex.is<stream::Timeout>()) {
                       ++(*timeout_count);
                       return MakeReadyFuture<bool>(true);
                     }
                     return MakeExceptionFuture<bool>(ex);
                   }
                   auto req = req_fut.GetValue0();
                   if (req) {
                     *rsp_msg += req->msg();
                     return MakeReadyFuture<bool>(true);
                   } else {
                     return MakeReadyFuture<bool>(false);
                   }
                 });
           })
        .Then([rsp_msg, timeout_count]() {
          TestResponse rsp;
          rsp.set_msg(*rsp_msg + std::string("timeout_count:") + std::to_string(*timeout_count));
          return MakeReadyFuture<TestResponse>(std::move(rsp));
        });
  }

  Future<TestResponse> ClientStreamNoData(const ServerContextPtr& context,
                                          const AsyncReaderPtr<TestRequest>& reader) override {
    auto rsp = std::make_shared<TestResponse>();
    return reader->Read().Then([reader, rsp](std::optional<TestRequest>&& req) {
      if (req) {
        rsp->set_msg("failed");
        return MakeReadyFuture<TestResponse>(std::move(*rsp));
      }
      return reader->Read().Then([reader, rsp](std::optional<TestRequest>&& req) {
        if (req) {
          rsp->set_msg("failed");
        }
        return MakeReadyFuture<TestResponse>(std::move(*rsp));
      });
    });
  }

  Future<TestResponse> ClientStreamFinishTimeout(const ServerContextPtr& context,
                                                 const AsyncReaderPtr<TestRequest>& reader) override {
    return AsyncTimer(false).After(6500).Then([]() {
      //
      return MakeReadyFuture<TestResponse>(TestResponse());
    });
  }

  Future<TestResponse> ClientStreamCloseConnectionAtServer(const ServerContextPtr& context,
                                                           const AsyncReaderPtr<TestRequest>& reader) override {
    auto count = std::make_shared<int>(0);
    return DoFor(10,
                 [reader, count](std::size_t i) {
                   *count = i;
                   return reader->Read().Then([reader](std::optional<TestRequest>&& req) {
                     if (req) {
                       return MakeReadyFuture<bool>(true);
                     } else {
                       return MakeReadyFuture<bool>(false);
                     }
                   });
                 })
        .Then([&, count](Future<>&& ft) {
          auto ex = ft.GetException();
          TRPC_FMT_ERROR("ClientStreamCloseConnectionAtServer, write error: {}", ex.what());
          stream_conn_close_status["server_close_conn_client_stream"] =
              std::to_string(*count) + "-" + std::to_string(ex.GetExceptionCode());
          return MakeExceptionFuture<TestResponse>(ex);
        });
  }

  Future<TestResponse> ClientStreamCloseConnectionAtClient(const ServerContextPtr& context,
                                                           const AsyncReaderPtr<TestRequest>& reader) override {
    auto count = std::make_shared<int>(0);
    return DoFor(10,
                 [reader, count](std::size_t i) {
                   *count = i;
                   return reader->Read().Then([reader](std::optional<TestRequest>&& req) {
                     if (req) {
                       return MakeReadyFuture<bool>(true);
                     } else {
                       return MakeReadyFuture<bool>(false);
                     }
                   });
                 })
        .Then([&, count](Future<>&& ft) {
          auto ex = ft.GetException();
          TRPC_FMT_ERROR("ClientStreamCloseConnectionAtClient, write error: {}", ex.what());
          stream_conn_close_status["client_close_conn_client_stream"] =
              std::to_string(*count) + "-" + std::to_string(ex.GetExceptionCode());
          return MakeExceptionFuture<TestResponse>(ex);
        });
  }

  Future<> BidiStream(const ServerContextPtr& context,
                      const AsyncReaderWriterPtr<TestRequest, TestResponse>& rw) override {
    auto count = std::make_shared<int>(0);
    return DoUntil([rw, count]() {
      return rw->Read().Then([rw, count](std::optional<TestRequest>&& req) {
        if (req) {
          TestResponse rsp;
          ++(*count);
          rsp.set_msg(req->msg() == std::string("msg") + std::to_string(*count) ? req->msg() : "failed");
          return rw->Write(std::move(rsp)).Then([]() { return MakeReadyFuture<bool>(true); });
        } else {
          // EOF
          return MakeReadyFuture<bool>(false);
        }
      });
    });
  }

  Future<> BidiStreamReadTimeout(const ServerContextPtr& context,
                                 const AsyncReaderWriterPtr<TestRequest, TestResponse>& rw) override {
    auto count = std::make_shared<int>(0);
    return DoUntil([rw, count]() {
      // The first read will timeout.
      return rw->Read(90ms).Then([rw, count](Future<std::optional<TestRequest>>&& req_fut) {
        if (req_fut.IsFailed()) {
          Exception ex = req_fut.GetException();
          if (!ex.is<stream::Timeout>()) {
            return MakeExceptionFuture<bool>(ex);
          }
        } else {
          auto req = req_fut.GetValue0();
          if (!req) {
            // EOF
            return MakeReadyFuture<bool>(false);
          }
        }
        // The second read will succeed.
        return rw->Read(90ms).Then([rw, count](std::optional<TestRequest>&& req) {
          if (req) {
            // sleep 100ms before write
            return AsyncTimer(false).After(100).Then([rw, count, req]() {
              TestResponse rsp;
              ++(*count);
              rsp.set_msg(req->msg() == std::string("msg") + std::to_string(*count) ? req->msg() : "failed");
              return rw->Write(std::move(rsp)).Then([]() { return MakeReadyFuture<bool>(true); });
            });
          }
          return MakeReadyFuture<bool>(false);
        });
      });
    });
  }

  Future<> BidiStreamNoData(const ServerContextPtr& context,
                            const AsyncReaderWriterPtr<TestRequest, TestResponse>& rw) override {
    auto rsp = std::make_shared<TestResponse>();
    return rw->Read().Then([rw, rsp](std::optional<TestRequest>&& req) {
      if (req) {
        rsp->set_msg("failed");
        return rw->Write(std::move(*rsp));
      }
      // After reading EOF, reading again will also result in EOF.
      return rw->Read().Then([rw, rsp](std::optional<TestRequest>&& req) {
        if (req) {
          rsp->set_msg("failed");
          return rw->Write(std::move(*rsp));
        }
        return MakeReadyFuture<>();
      });
    });
  }

  Future<> BidiStreamFinishTimeout(const ServerContextPtr& context,
                                   const AsyncReaderWriterPtr<TestRequest, TestResponse>& rw) override {
    return AsyncTimer(false).After(6500).Then([]() {
      //
      return MakeReadyFuture<>();
    });
  }

  Future<> BidiStreamCloseConnectionAtServer(const ServerContextPtr& context,
                                             const AsyncReaderWriterPtr<TestRequest, TestResponse>& rw) override {
    auto counter = std::make_shared<int>(0);
    auto check_read_ret = std::make_shared<bool>(false);
    return DoFor(
               10,
               [&, rw, counter, check_read_ret](std::size_t i) {
                 return rw->Read().Then([rw, i, counter, check_read_ret](Future<std::optional<TestRequest>>&& fut_req) {
                   if (fut_req.IsFailed()) {
                     // Read and write fails
                     *check_read_ret = fut_req.GetException().GetExceptionCode() == TRPC_STREAM_SERVER_NETWORK_ERR;
                     return rw->Write(TestResponse{}).Then([]() { return MakeReadyFuture<bool>(true); });
                   }
                   std::optional<TestRequest> req = fut_req.GetValue0();
                   return rw->Write(TestResponse{}).Then([i, counter]() {
                     ++(*counter);
                     return MakeReadyFuture<bool>(true);
                   });
                 });
               })
        .Then([&, counter, check_read_ret](Future<>&& fut) {
          // The captured error here is a write error.
          if (fut.IsReady()) {
            return MakeReadyFuture<>();
          }
          auto ex = fut.GetException();
          if (*check_read_ret) {
            TRPC_FMT_ERROR("BidiStreamCloseConnectionAtServer, write error: {}", ex.what());
            stream_conn_close_status["server_close_conn_bidi_stream"] =
                std::to_string(*counter) + "-" + std::to_string(ex.GetExceptionCode());
          }
          return MakeExceptionFuture<>(ex);
        });
  }

  Future<> BidiStreamCloseConnectionAtClient(const ServerContextPtr& context,
                                             const AsyncReaderWriterPtr<TestRequest, TestResponse>& rw) override {
    auto counter = std::make_shared<int>(0);
    auto check_read_ret = std::make_shared<bool>(false);
    return DoFor(
               10,
               [&, rw, counter, check_read_ret](std::size_t i) {
                 return rw->Read().Then([rw, i, counter, check_read_ret](Future<std::optional<TestRequest>>&& fut_req) {
                   if (fut_req.IsFailed()) {
                     // Read and write fails
                     *check_read_ret = fut_req.GetException().GetExceptionCode() == TRPC_STREAM_SERVER_NETWORK_ERR;
                     return rw->Write(TestResponse{}).Then([]() { return MakeReadyFuture<bool>(true); });
                   }
                   std::optional<TestRequest> req = fut_req.GetValue0();
                   return rw->Write(TestResponse{}).Then([i, counter]() {
                     ++(*counter);
                     return MakeReadyFuture<bool>(true);
                   });
                 });
               })
        .Then([&, counter, check_read_ret](Future<>&& fut) {
          // The captured error here is a write error.
          if (fut.IsReady()) {
            return MakeReadyFuture<>();
          }
          auto ex = fut.GetException();
          if (*check_read_ret) {
            TRPC_FMT_ERROR("BidiStreamCloseConnectionAtClient, write error: {}", ex.what());
            stream_conn_close_status["client_close_conn_bidi_stream"] =
                std::to_string(*counter) + "-" + std::to_string(ex.GetExceptionCode());
          }
          return MakeExceptionFuture<>(ex);
        });
  }
};

int AsyncStreamServer::Initialize() {
  RegisterService("trpc.testing.stream_test.AsyncStreamService", std::make_shared<AsyncStreamService>());
  RegisterService("trpc.testing.stream_test.AsyncStreamServiceConnIdle", std::make_shared<AsyncStreamService>());

  test_signal_->SignalClientToContinue();

  return 0;
}

void AsyncStreamServer::Destroy() { GcovFlush(); }

}  // namespace trpc::testing
