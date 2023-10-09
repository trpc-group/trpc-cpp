// Copyright (c) 2023, Tencent Inc.
// All rights reserved.

#include <chrono>

#include "test/end2end/stream/trpc/stream_test.h"
#include "trpc/common/async_timer.h"
#include "trpc/common/future/future_utility.h"
#include "trpc/util/thread/latch.h"

namespace trpc::testing {

using namespace trpc::stream;
using namespace trpc::testing::stream_test;

using namespace std::chrono_literals;

class AsyncStreamTest : public StreamTest {
 protected:
  void SetUp() override {
    // Run before each test case
    to_sync_service_proxy_ = ::trpc::GetTrpcClient()->GetProxy<AsyncTestStreamServiceServiceProxy>(
        "trpc.testing.stream_test.SyncStreamService");
    to_async_service_proxy_ = ::trpc::GetTrpcClient()->GetProxy<AsyncTestStreamServiceServiceProxy>(
        "trpc.testing.stream_test.AsyncStreamService");
    to_sync_idle_service_proxy_ = ::trpc::GetTrpcClient()->GetProxy<AsyncTestStreamServiceServiceProxy>(
        "trpc.testing.stream_test.SyncStreamServiceConnIdle");
    to_async_idle_service_proxy_ = ::trpc::GetTrpcClient()->GetProxy<AsyncTestStreamServiceServiceProxy>(
        "trpc.testing.stream_test.AsyncStreamServiceConnIdle");
    to_sync_idle_service_idle_proxy_ = ::trpc::GetTrpcClient()->GetProxy<AsyncTestStreamServiceServiceProxy>(
        "trpc.testing.stream_test.SyncStreamServiceConnIdleAtClient");
    to_async_idle_service_idle_proxy_ = ::trpc::GetTrpcClient()->GetProxy<AsyncTestStreamServiceServiceProxy>(
        "trpc.testing.stream_test.AsyncStreamServiceConnIdleAtClient");
  }

  void TearDown() override {
    // Run after each test case
  }

  void DoTest(std::function<void(std::shared_ptr<AsyncTestStreamServiceServiceProxy>)>&& test_fn) {
    std::cout << "==================== Test to sync servie ====================" << std::endl;
    test_fn(to_sync_service_proxy_);
    std::cout << "==================== Test to async servie ====================" << std::endl;
    test_fn(to_async_service_proxy_);
  }

  void DoTestServerCloseConn(std::function<void(std::shared_ptr<AsyncTestStreamServiceServiceProxy>)>&& test_fn) {
    std::cout << "==================== Test to sync servie(DoTestServerCloseConn) ====================" << std::endl;
    test_fn(to_sync_idle_service_proxy_);
    std::cout << "==================== Test to async servie(DoTestServerCloseConn) ====================" << std::endl;
    test_fn(to_async_idle_service_proxy_);
  }

  void DoTestClientCloseConn(std::function<void(std::shared_ptr<AsyncTestStreamServiceServiceProxy>)>&& test_fn) {
    std::cout << "==================== Test to sync servie(DoTestClientCloseConn) ====================" << std::endl;
    test_fn(to_sync_idle_service_idle_proxy_);
    std::cout << "==================== Test to async servie(DoTestClientCloseConn) ====================" << std::endl;
    test_fn(to_async_idle_service_idle_proxy_);
  }

  void CheckConnClose(std::string type, std::string check, bool client_idle = false) {
    RunByEnv([=]() {
      std::cout << "==================== Check conn close for sync servie ====================" << std::endl;
      std::shared_ptr<AsyncTestStreamServiceServiceProxy> sync_proxy =
          client_idle ? to_sync_idle_service_idle_proxy_ : to_sync_idle_service_proxy_;
      ClientContextPtr ctx = MakeClientContext(sync_proxy);
      TestRequest req_sync;
      req_sync.set_msg(type);
      TestResponse rsp_sync;
      Latch latch(1);
      sync_proxy->StreamAtConnCloseStatus(ctx, req_sync).Then([&](Future<TestResponse>&& fut_rsp) {
        ASSERT_TRUE(fut_rsp.IsReady());
        rsp_sync = fut_rsp.GetValue0();
        latch.count_down();
      });
      latch.wait();
      ASSERT_EQ(rsp_sync.msg(), check);
      std::cout << "==================== Check conn close for async servie ====================" << std::endl;
      std::shared_ptr<AsyncTestStreamServiceServiceProxy> async_proxy =
          client_idle ? to_async_idle_service_idle_proxy_ : to_async_idle_service_proxy_;
      ctx = MakeClientContext(async_proxy);
      TestRequest req_async;
      req_async.set_msg(type);
      TestResponse rsp_async;
      Latch latch2(1);
      async_proxy->StreamAtConnCloseStatus(ctx, req_async).Then([&](Future<TestResponse>&& fut_rsp) {
        ASSERT_TRUE(fut_rsp.IsReady());
        rsp_async = fut_rsp.GetValue0();
        latch2.count_down();
      });
      latch2.wait();
      ASSERT_EQ(rsp_async.msg(), check);
    });
  }

 protected:
  std::shared_ptr<AsyncTestStreamServiceServiceProxy> to_sync_service_proxy_;
  std::shared_ptr<AsyncTestStreamServiceServiceProxy> to_sync_idle_service_proxy_;
  std::shared_ptr<AsyncTestStreamServiceServiceProxy> to_sync_idle_service_idle_proxy_;
  std::shared_ptr<AsyncTestStreamServiceServiceProxy> to_async_service_proxy_;
  std::shared_ptr<AsyncTestStreamServiceServiceProxy> to_async_idle_service_proxy_;
  std::shared_ptr<AsyncTestStreamServiceServiceProxy> to_async_idle_service_idle_proxy_;
};

// Test Case:
// The server writes to the stream 10 times, each time with "rsp-msgx", where x increases from 1 to 10.
// The client sends a message "req-msg" and reads from the stream 10 times.
// Expected:
// The server writes successfully each time.
// The client checks the messages 1st to the 10th time are "rsp-msg1" to "rsp-msg10" respectively.
TEST_F(AsyncStreamTest, TestServerStreamOk) {
  DoTest([](std::shared_ptr<AsyncTestStreamServiceServiceProxy> proxy) {
    int count = 0;
    auto fut = RunAsMerge([&](PromPtr prom) {
      ClientContextPtr ctx = MakeClientContext(proxy);
      TestRequest req;
      req.set_msg("req-msg");
      return proxy->ServerStream(ctx, std::move(req))
          .Then([&, prom](stream::AsyncReaderPtr<TestResponse>&& reader) {
            return DoUntil([&, reader, prom]() {
              return reader->Read().Then([&, prom](Future<std::optional<TestResponse>>&& ft) {
                if (ft.IsFailed()) {
                  auto&& ex = ft.GetException();
                  std::cerr << "stream got error: " << ex.what() << std::endl;
                  return MakeExceptionFuture<bool>(std::move(ex));
                }
                const auto& rsp = ft.GetValue0();
                if (rsp) {
                  ++count;
                  EXPECT_EQ(rsp->msg(), std::string("rsp-msg") + std::to_string(count));
                  return MakeReadyFuture<bool>(true);
                } else {
                  prom->SetValue();
                  return MakeReadyFuture<bool>(false);
                }
              });
            });
          });
    });
    future::BlockingGet(std::move(fut));
    EXPECT_EQ(count, 10);
  });
}

// Test Case:
// The server writes to the stream 10 times, each time with "rsp-msgx", where x increases from 1 to 10. Before each
// write, sleep for 100ms.
// The client sends a message "req-msg" from the stream until EOF, reading messages with a timeout of 90ms each time.
// Expected:
// The server should be able to write successfully each time.
// The client:
// 1. Should read the messages "rsp-msg1"~"rsp-msg10" in order for the first 10 reads.
// 2. Should read time out accumulated 10 times.
TEST_F(AsyncStreamTest, TestServerStreamReadTimeout) {
  DoTest([](std::shared_ptr<AsyncTestStreamServiceServiceProxy> proxy) {
    int count = 0;
    int timer_count = 0;
    auto fut = RunAsMerge([&](PromPtr prom) {
      ClientContextPtr ctx = MakeClientContext(proxy);
      TestRequest req;
      req.set_msg("req-msg");
      return proxy->ServerStreamReadTimeout(ctx, std::move(req))
          .Then([&, prom](stream::AsyncReaderPtr<TestResponse>&& reader) {
            return DoUntil([&, reader, prom]() {
              return reader->Read(90ms).Then([&, prom](Future<std::optional<TestResponse>>&& ft) {
                if (ft.IsFailed()) {
                  auto&& ex = ft.GetException();
                  if (ex.is<stream::Timeout>()) {
                    ++timer_count;
                    std::cout << "read timeout" << std::endl;
                    return MakeReadyFuture<bool>(true);
                  }
                  std::cerr << "stream got error: " << ex.what() << std::endl;
                  return MakeExceptionFuture<bool>(std::move(ex));
                }
                const auto& rsp = ft.GetValue0();
                if (rsp) {
                  ++count;
                  std::cout << "read data: " << rsp->msg() << std::endl;
                  EXPECT_EQ(rsp->msg(), std::string("rsp-msg") + std::to_string(count));
                  return MakeReadyFuture<bool>(true);
                } else {
                  prom->SetValue();
                  std::cout << "read eof" << std::endl;
                  return MakeReadyFuture<bool>(false);
                }
              });
            });
          });
    });
    future::BlockingGet(std::move(fut));
    ASSERT_EQ(count, 10);
    ASSERT_EQ(timer_count, 10);
  });
}

// Test Case:
// The client sends an empty message string and immediately ends the communication without reading from the stream.
// The server does not write anything to the stream.
// Expected:
// the client will continue to read eof after reading it first.
TEST_F(AsyncStreamTest, TestServerStreamNoData) {
  DoTest([](std::shared_ptr<AsyncTestStreamServiceServiceProxy> proxy) {
    auto fut = RunAsMerge([&](PromPtr prom) {
      ClientContextPtr ctx = MakeClientContext(proxy);
      TestRequest req;
      req.set_msg("");
      return proxy->ServerStreamNoData(ctx, std::move(req))
          .Then([&, prom](stream::AsyncReaderPtr<TestResponse>&& reader) {
            return reader->Read().Then([&, prom, reader](Future<std::optional<TestResponse>>&& ft) {
              EXPECT_TRUE(ft.IsReady());
              EXPECT_TRUE(!ft.GetValue0().has_value());
              return reader->Read().Then([&, prom](Future<std::optional<TestResponse>>&& ft) {
                EXPECT_TRUE(ft.IsReady());
                EXPECT_TRUE(!ft.GetValue0().has_value());
                prom->SetValue();
              });
            });
          });
    });
    future::BlockingGet(std::move(fut));
  });
}

// Test Case:
// The client initiates a message with an empty string, then calls `Finish` until timeout
// The server does not write to the stream and waits for 3.5 seconds before ending.
// Expected:
// After the client calling Finish until timeout and get timeout error, then it can call `Finish` again and get the same
// error immediately without block.
TEST_F(AsyncStreamTest, TestServerStreamFinishTimeout) {
  // async stream `Finish` does not support timeout yet.
}

// Test Case:
// After the server writes to the stream 5 times, it will disconnect.(depending on the server's idle connection cleanup)
// Expected:
// The client is expected to read 5 messages before the connection is disconnected.
// The server writes to stream but got connection closed error.
TEST_F(AsyncStreamTest, TestServerStreamCloseConnectionAtServer) {
  DoTestServerCloseConn([](std::shared_ptr<AsyncTestStreamServiceServiceProxy> proxy) {
    int count = 0;
    auto fut = RunAsMerge([&](PromPtr prom) {
      ClientContextPtr ctx = MakeClientContext(proxy);
      return proxy->ServerStreamCloseConnectionAtServer(ctx, TestRequest{})
          .Then([&, prom](stream::AsyncReaderPtr<TestResponse>&& reader) {
            return DoUntil([&, reader, prom]() {
              return reader->Read().Then([&, prom](Future<std::optional<TestResponse>>&& ft) {
                std::cout << "client, i: " << count << std::endl;
                if (count == 5) {
                  EXPECT_EQ(ft.GetException().GetExceptionCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
                  prom->SetValue();
                  return MakeReadyFuture<bool>(false);
                }
                const auto& rsp = ft.GetValue0();
                ++count;
                EXPECT_EQ(rsp->msg(), std::string("rsp-msg") + std::to_string(count));
                return MakeReadyFuture<bool>(true);
              });
            });
          });
    });
    future::BlockingGet(std::move(fut));
    EXPECT_EQ(count, 5);
  });
  // Wait a moment, let the server handle the disconnection so that it can be easily verified.
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  CheckConnClose("server_close_conn_server_stream", std::to_string(TRPC_STREAM_SERVER_NETWORK_ERR));
}

// Test Case:
// After the server writes to the stream 5 times, it will disconnect.(depending on the client's idle connection cleanup)
// Expected:
// The client is expected to read 5 messages before the connection is disconnected.
// The server writes to stream but got connection closed error.
TEST_F(AsyncStreamTest, TestServerStreamCloseConnectionAtClient) {
  DoTestClientCloseConn([](std::shared_ptr<AsyncTestStreamServiceServiceProxy> proxy) {
    int count = 0;
    auto fut = RunAsMerge([&](PromPtr prom) {
      ClientContextPtr ctx = MakeClientContext(proxy);
      return proxy->ServerStreamCloseConnectionAtClient(ctx, TestRequest{})
          .Then([&, prom](stream::AsyncReaderPtr<TestResponse>&& reader) {
            return DoUntil([&, reader, prom]() {
              return reader->Read().Then([&, prom](Future<std::optional<TestResponse>>&& ft) {
                if (count == 5) {
                  EXPECT_EQ(ft.GetException().GetExceptionCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
                  prom->SetValue();
                  return MakeReadyFuture<bool>(false);
                }
                const auto& rsp = ft.GetValue0();
                ++count;
                EXPECT_EQ(rsp->msg(), std::string("rsp-msg") + std::to_string(count));
                return MakeReadyFuture<bool>(true);
              });
            });
          });
    });
    future::BlockingGet(std::move(fut));
    EXPECT_EQ(count, 5);
  });
  // Wait a moment, let the server handle the disconnection so that it can be easily verified.
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  CheckConnClose("client_close_conn_server_stream", std::to_string(TRPC_STREAM_SERVER_NETWORK_ERR), true);
}

// Test Case:
// After stream established(with flow control window size of 65535), the server starts writing 10 responses, each with
// 10000 bytes. The client reads every 100ms. In the 11th response, the server includes
// which write triggers flow control and the timestamp when it was released.
// The flow control window is set to 0 in the client's YAML file, disabling flow control.
// Expected:
// The server will not trigger flow control.
TEST_F(AsyncStreamTest, TestServerStreamFlowControlOnlyServer) {
  // async stream does not support flow control yet.
}

// Test Case:
// After stream established(with flow control window size of 65535), the server starts writing 10 responses, each with
// 10000 bytes. The client reads every 100ms. In the 11th response, the server includes
// which write triggers flow control and the timestamp when it was released.
// Expected:
// The server triggers flow control while writing the 8th response, until the client reads the 2nd request. After
// receiving the 11th response from the server, the client verifies the result and checks that the flow control was
// triggered for the 3rd time and that the timestamp of when it was released is close to when the client read the 3rd
// request.
TEST_F(AsyncStreamTest, TestServerStreamFlowControl) {
  // async stream does not support flow control yet.
}

// Test case:
// The client writes to the stream 10 times, with each message being "req-msgx", where x increases from 1 to 10. The
// server reads from the stream 10 times, checks each message, concatenates them, and returns the concatenated message.
// The client should be able to write successfully each time.
// Expected:
// The server checks the messages read from the 1st to the 10th time, and they should be "req-msg1" to "req-msg10"
// respectively.
// The client should be able to write successfully each time, and the concatenated message from server should match the
// expected result.
TEST_F(AsyncStreamTest, TestClientStreamOk) {
  DoTest([](std::shared_ptr<AsyncTestStreamServiceServiceProxy> proxy) {
    ClientContextPtr ctx = MakeClientContext(proxy);
    std::string check_msg;
    std::string rsp_msg;
    auto fut = RunAsMerge([&](PromPtr prom) {
      return proxy->ClientStream(ctx)
          .Then([&, prom](std::pair<stream::AsyncWriterPtr<TestRequest>, Future<TestResponse>>&& p) {
            auto writer = p.first;
            return DoFor(10,
                         [&, writer](std::size_t i) {
                           std::string write_msg = std::string("req-msg") + std::to_string(i + 1);
                           check_msg += write_msg;
                           TestRequest req;
                           req.set_msg(std::move(write_msg));
                           return writer->Write(req);
                         })
                .Then([&, writer, rsp = std::move(p.second)]() mutable {
                  writer->Finish();
                  return rsp.Then([&](TestResponse&& rsp) {
                    rsp_msg = rsp.msg();
                    return MakeReadyFuture<>();
                  });
                })
                .Then([prom](Future<>&& ft) {
                  if (ft.IsFailed()) {
                    std::cerr << "stream write error: " << ft.GetException().what() << std::endl;
                  }
                  prom->SetValue();
                  return MakeReadyFuture<>();
                });
          });
    });
    future::BlockingGet(std::move(fut));
    ASSERT_FALSE(check_msg.empty());
    ASSERT_FALSE(rsp_msg.empty());
    ASSERT_EQ(check_msg, rsp_msg);
  });
}

// Test case:
// The client writes to the stream 10 times, with each message being "rsp-msgx", where x increases from 1 to 10. Before
// each write, the client sleeps for 100ms. The server reads from the stream 10 times, checks each message, concatenates
// them, and returns the concatenated message. The server reads each message with a timeout of 90ms and records the
// number of timeouts.
// Expected:
// The client should be able to write successfully each time.
// The server should return the concatenated message as "req-msg1" to "req-msg10" followed by the string
// "timeout_count:10".
TEST_F(AsyncStreamTest, TestClientStreamReadTimeout) {
  DoTest([](std::shared_ptr<AsyncTestStreamServiceServiceProxy> proxy) {
    ClientContextPtr ctx = MakeClientContext(proxy);
    std::string check_msg;
    std::string rsp_msg;
    auto fut = RunAsMerge([&](PromPtr prom) {
      return proxy->ClientStreamReadTimeout(ctx)
          .Then([&, prom](std::pair<stream::AsyncWriterPtr<TestRequest>, Future<TestResponse>>&& p) {
            auto writer = p.first;
            return DoFor(10,
                         [&, writer](std::size_t i) {
                           return AsyncTimer(false).After(100).Then([&, writer, i]() {
                             std::string write_msg = std::string("req-msg") + std::to_string(i + 1);
                             check_msg += write_msg;
                             TestRequest req;
                             req.set_msg(std::move(write_msg));
                             return writer->Write(req);
                           });
                         })
                .Then([&, writer, rsp = std::move(p.second)]() mutable {
                  writer->Finish();
                  return rsp.Then([&](TestResponse&& rsp) {
                    rsp_msg = rsp.msg();
                    return MakeReadyFuture<>();
                  });
                })
                .Then([prom](Future<>&& ft) {
                  if (ft.IsFailed()) {
                    std::cerr << "stream write error: " << ft.GetException().what() << std::endl;
                  }
                  prom->SetValue();
                  return MakeReadyFuture<>();
                });
          });
    });
    future::BlockingGet(std::move(fut));
    ASSERT_FALSE(check_msg.empty());
    ASSERT_FALSE(rsp_msg.empty());
    ASSERT_EQ(check_msg + "timeout_count:10", rsp_msg);
  });
}

// Test case:
// The client does not write to the stream.
// The server does not read from the stream and ends directly, returning an empty string as the message.
// Expected:
// The server should detect the end of the stream (EOF) and any further read attempts should also return EOF.
// The client should receive an empty response.
TEST_F(AsyncStreamTest, TestClientStreamNoData) {
  DoTest([](std::shared_ptr<AsyncTestStreamServiceServiceProxy> proxy) {
    auto fut = RunAsMerge([&](PromPtr prom) {
      ClientContextPtr ctx = MakeClientContext(proxy);
      return proxy->ClientStreamNoData(ctx)
          .Then([&, prom](std::pair<stream::AsyncWriterPtr<TestRequest>, Future<TestResponse>>&& p) {
            auto writer = p.first;
            return writer->Finish().Then([rsp = std::move(p.second), prom](Future<>&& ft) mutable {
              EXPECT_TRUE(ft.IsReady());
              return rsp.Then([prom](Future<TestResponse> rsp_fut) {
                EXPECT_TRUE(rsp_fut.IsReady());
                EXPECT_TRUE(rsp_fut.GetValue0().msg().empty());
                prom->SetValue();
                return MakeReadyFuture<>();
              });
            });
          });
    });
    future::BlockingGet(std::move(fut));
  });
}

// Test case:
// The client does not write to the stream.
// The server does not read from the stream and waits for 6.5 seconds before ending.
// Expected:
// The client should time out when calling 'WriteDone' to wait for a response.
// The client should also time out when calling 'Finish'.
// When the client calls 'Finish' again, it should immediately receive an error.
TEST_F(AsyncStreamTest, TestClientStreamFinishTimeout) {
  // async stream does not support flow control yet.
}

// Test case:
// The client writes to the stream 10 times, and the server disconnects after the 5th write.
// Expected result:
// The 6th message written by the client will fail with a network error, and calling `WriteDone` and `Finish` will also
// result in a network error.
TEST_F(AsyncStreamTest, TestClientStreamCloseConnectionAtServer) {
  DoTestServerCloseConn([](std::shared_ptr<AsyncTestStreamServiceServiceProxy> proxy) {
    int counter = 0;
    auto fut = RunAsMerge([&](PromPtr prom) {
      ClientContextPtr ctx = MakeClientContext(proxy);
      return proxy->ClientStreamCloseConnectionAtServer(ctx)
          .Then([&, prom](std::pair<stream::AsyncWriterPtr<TestRequest>, Future<TestResponse>>&& p) {
            auto writer = p.first;
            return DoFor(10,
                         [&, writer](std::size_t i) {
                           Future<> ft = MakeReadyFuture<>();
                           if (i == 5) {
                             // Trigger the server's idle connection cleanup.
                             ft = AsyncTimer(false).After(3000);
                           }
                           return ft.Then([&, writer]() {
                             std::string write_msg = std::string("req-msg") + std::to_string(i + 1);
                             TestRequest req;
                             req.set_msg(std::move(write_msg));
                             return writer->Write(req).Then([&]() {
                               ++counter;
                               return MakeReadyFuture<>();
                             });
                           });
                         })
                .Then([prom, writer](Future<>&& ft) {
                  EXPECT_EQ(ft.GetException().GetExceptionCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
                  return writer->Finish().Then([prom](Future<>&& ft) {
                    prom->SetValue();
                    return MakeReadyFuture<>();
                  });
                });
          });
    });
    future::BlockingGet(std::move(fut));
    ASSERT_EQ(counter, 5);
  });
  // Wait a moment, let the server handle the disconnection so that it can be easily verified.
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  CheckConnClose("server_close_conn_client_stream", "5-" + std::to_string(TRPC_STREAM_SERVER_NETWORK_ERR));
}

// Test case:
// The client writes to the stream 10 times, and the server disconnects after the 5th write (depending on the client's
// idle connection cleanup).
// Expected:
// The 6th message written by the client will fail with a network error, and calling 'WriteDone' and 'Finish' will also
// result in a network error.
TEST_F(AsyncStreamTest, TestClientStreamCloseConnectionAtClient) {
  DoTestClientCloseConn([](std::shared_ptr<AsyncTestStreamServiceServiceProxy> proxy) {
    int counter = 0;
    auto fut = RunAsMerge([&](PromPtr prom) {
      ClientContextPtr ctx = MakeClientContext(proxy);
      return proxy->ClientStreamCloseConnectionAtClient(ctx)
          .Then([&, prom](std::pair<stream::AsyncWriterPtr<TestRequest>, Future<TestResponse>>&& p) {
            auto writer = p.first;
            return DoFor(10,
                         [&, writer](std::size_t i) {
                           Future<> ft = MakeReadyFuture<>();
                           if (i == 5) {
                             // Trigger the client's idle connection cleanup.
                             ft = AsyncTimer(false).After(11000);
                           }
                           return ft.Then([&, writer]() {
                             std::string write_msg = std::string("req-msg") + std::to_string(i + 1);
                             TestRequest req;
                             req.set_msg(std::move(write_msg));
                             return writer->Write(req).Then([&]() {
                               ++counter;
                               return MakeReadyFuture<>();
                             });
                           });
                         })
                .Then([prom, writer](Future<>&& ft) {
                  EXPECT_EQ(ft.GetException().GetExceptionCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
                  return writer->Finish().Then([prom](Future<>&& ft) {
                    prom->SetValue();
                    return MakeReadyFuture<>();
                  });
                });
          });
    });
    future::BlockingGet(std::move(fut));
    ASSERT_EQ(counter, 5);
  });
  // Wait a moment, let the server handle the disconnection so that it can be easily verified.
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  CheckConnClose("client_close_conn_client_stream", "5-" + std::to_string(TRPC_STREAM_SERVER_NETWORK_ERR), true);
}

// Test Case:
// After establishing a stream (with a flow control window size of 65535), the client starts writing 10 requests, each
// with 10000 bytes, and the server reads a request every 100ms.
// Expected:
// The client triggers flow control when writing the 8th request, and the flow control is released after the server
// reads the 2nd request. The server returns the timestamp of when it read the 3rd request, and the client verifies that
// this timestamp is close to the timestamp when the flow control was released.
TEST_F(AsyncStreamTest, TestClientStreamFlowControl) {
  // async stream does not support flow control yet.
}

// Test case:
// After establishing a stream (with a flow control window size of 65535), the client starts writing 10 requests, each
// with 10000 bytes, and the server reads a request every 100ms.
// Set the flow control window to 0 in the server's YAML file to disable flow control.
// Expected:
// Flow control will not be triggered.
TEST_F(AsyncStreamTest, TestClientStreamFlowControlOnlyClient) {
  // async stream does not support flow control yet.
}

// Test Case:
// The client and server both read and write to the stream 10 times, each time with "msgx", where x increases from 1
// to 10.
// Expected:
// The client and server both read and write to the stream 10 times. Check each message read, and the server and client
// should read messages "msg1" to "msg10" in sequence.
TEST_F(AsyncStreamTest, TestBidiStreamOk) {
  DoTest([](std::shared_ptr<AsyncTestStreamServiceServiceProxy> proxy) {
    std::string check_msg;
    std::string rsp_msg;
    auto fut = RunAsMerge([&](PromPtr prom) {
      ClientContextPtr ctx = MakeClientContext(proxy);
      return proxy->BidiStream(ctx)
          .Then([prom](stream::AsyncReaderWriterPtr<TestResponse, TestRequest>&& rw) {
            return DoFor(10,
                         [rw](std::size_t i) {
                           TestRequest req;
                           req.set_msg(std::string("msg") + std::to_string(i + 1));
                           return rw->Write(std::move(req)).Then([i, rw]() {
                             return rw->Read().Then([i](std::optional<TestResponse>&& rsp) {
                               EXPECT_EQ(rsp->msg(), std::string("msg") + std::to_string(i + 1));
                               return MakeReadyFuture<bool>(true);
                             });
                           });
                         })
                .Then([rw, prom]() {
                  prom->SetValue();
                  return rw->Finish();
                });  // future discarded
          })
          .Then([prom](Future<>&& ft) {
            if (ft.IsFailed()) {
              std::cerr << "stream got error: " << ft.GetException().what() << std::endl;
            }
            prom->SetValue();
            return MakeReadyFuture<>();
          });
    });
    future::BlockingGet(std::move(fut));
  });
}

// Test Case:
// Both the client and server read and write to the stream 10 times with the message "msgx", where x increments from 1
// to 10. Before each write, sleep for 100ms. The timeout for each read is 90ms. The client writes first, and the server
// responds with a message to the client.
// Expected:
// Both ends can write successfully.
// The first read on both ends will timeout, and the second will succeed.
// The client checks the server's result for "msgx".
TEST_F(AsyncStreamTest, TestBidiStreamReadTimeout) {
  DoTest([](std::shared_ptr<AsyncTestStreamServiceServiceProxy> proxy) {
    std::string check_msg;
    std::string rsp_msg;
    auto fut = RunAsMerge([&](PromPtr prom) {
      ClientContextPtr ctx = MakeClientContext(proxy);
      return proxy->BidiStream(ctx)
          .Then([prom](stream::AsyncReaderWriterPtr<TestResponse, TestRequest>&& rw) {
            return DoFor(10,
                         [rw](std::size_t i) {
                           // Sleep 100ms before writing.
                           return AsyncTimer(100).After(100).Then([rw, i] {
                             TestRequest req;
                             req.set_msg(std::string("msg") + std::to_string(i + 1));
                             return rw->Write(std::move(req)).Then([i, rw]() {
                               // Read timed when first reading.
                               return rw->Read(90ms).Then([i, rw](Future<std::optional<TestResponse>>&& rsp_fut) {
                                 EXPECT_TRUE(rsp_fut.GetException().is<stream::Timeout>());
                                 return rw->Read(90ms).Then([i](Future<std::optional<TestResponse>>&& rsp_fut) {
                                   EXPECT_TRUE(rsp_fut.IsReady());
                                   auto rsp = rsp_fut.GetValue0();
                                   EXPECT_EQ(rsp->msg(), std::string("msg") + std::to_string(i + 1));
                                   return MakeReadyFuture<bool>(true);
                                 });
                               });
                             });
                           });
                         })
                .Then([rw, prom]() {
                  prom->SetValue();
                  return rw->Finish();
                });  // future discarded
          })
          .Then([prom](Future<>&& ft) {
            if (ft.IsFailed()) {
              std::cerr << "stream got error: " << ft.GetException().what() << std::endl;
            }
            prom->SetValue();
            return MakeReadyFuture<>();
          });
    });
    future::BlockingGet(std::move(fut));
  });
}

// Test Case:
// Neither the client nor the server writes to the stream.
// The server ends the stream handle directly after receiving it.
// The client sends `WriteDone` directly and calls `Finish`.
// Expected:
// The server reads EOF, and the client reads EOF.
// Both ends will read EOF again.
TEST_F(AsyncStreamTest, TestBidiStreamNoData) {
  DoTest([](std::shared_ptr<AsyncTestStreamServiceServiceProxy> proxy) {
    std::string check_msg;
    std::string rsp_msg;
    auto fut = RunAsMerge([&](PromPtr prom) {
      ClientContextPtr ctx = MakeClientContext(proxy);
      return proxy->BidiStream(ctx).Then([prom](stream::AsyncReaderWriterPtr<TestResponse, TestRequest>&& rw) {
        return rw->Finish().Then([prom, rw](Future<>&& ft) {
          return rw->Read().Then([&, prom, rw](Future<std::optional<TestResponse>>&& ft) {
            EXPECT_TRUE(ft.IsReady());
            EXPECT_TRUE(!ft.GetValue0().has_value());
            return rw->Read().Then([&, prom](Future<std::optional<TestResponse>>&& ft) {
              EXPECT_TRUE(ft.IsReady());
              EXPECT_TRUE(!ft.GetValue0().has_value());
              prom->SetValue();
              return MakeReadyFuture<>();
            });
          });
        });
      });
    });
    future::BlockingGet(std::move(fut));
  });
}

// Test Case:
// The client does not write to the stream.
// The server does not read from the stream and ends after waiting for 6.5 seconds.
// Expected:
// The client calling `WriteDone` will not wait for beyond 3 seconds.
// The call to `Finish` will timeout, and another call to `Finish` will immediately return a timeout error.
TEST_F(AsyncStreamTest, TestBidiStreamFinishTimeout) {
  // async stream does not support flow control yet.
}

// Test Case:
// The client writes to the stream 10 times, and the server disconnects after the 5th write.(Depending on the server's
// idle connection clean up)
// Expected:
// The client's 6th message will write incorrectly, and the read will also be incorrect. The error will be a network
// error.
TEST_F(AsyncStreamTest, TestBidiStreamCloseConnectionAtServer) {
  DoTestServerCloseConn([](std::shared_ptr<AsyncTestStreamServiceServiceProxy> proxy) {
    auto fut = RunAsMerge([&](PromPtr prom) {
      ClientContextPtr ctx = MakeClientContext(proxy);
      return proxy->BidiStreamCloseConnectionAtServer(ctx).Then(
          [&, prom](stream::AsyncReaderWriterPtr<TestResponse, TestRequest>&& rw) {
            return DoFor(10, [rw, prom](std::size_t i) {
              Future<> fut = MakeReadyFuture<>();
              if (i == 5) {
                // Trigger the server's idle connection cleanup.
                fut = AsyncTimer(false).After(3000);
              }
              return fut.Then([=]() {
                TestRequest req;
                req.set_msg(std::string("msg") + std::to_string(i + 1));
                return rw->Write(std::move(req))
                    .Then([i, rw]() {
                      return rw->Read().Then(
                          [i](std::optional<TestResponse>&& rsp) { return MakeReadyFuture<bool>(true); });
                    })
                    .Then([rw, i, prom](Future<bool>&& ft) {
                      if (ft.IsReady()) {
                        return MakeReadyFuture<bool>(true);
                      }
                      EXPECT_EQ(i, 5);
                      // Write to stream error
                      EXPECT_EQ(ft.GetException().GetExceptionCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
                      // Read from stream error
                      return rw->Read().Then([rw, prom](Future<std::optional<TestResponse>>&& ft) {
                        EXPECT_EQ(ft.GetException().GetExceptionCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
                        // `Finish` can normally done
                        return rw->Finish().Then([prom]() {
                          prom->SetValue();
                          return MakeExceptionFuture<bool>(StreamError(kUnknownErrorStatus));
                        });
                      });
                    });
              });
            });
          });
    });
    future::BlockingGet(std::move(fut));
  });
  // Wait a moment, let the server handle the disconnection so that it can be easily verified.
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  CheckConnClose("server_close_conn_bidi_stream", "5-" + std::to_string(TRPC_STREAM_SERVER_NETWORK_ERR));
}

// Test Case:
// The client writes to the stream 10 times, and the client disconnects after the 5th write (depending on the client's
// idle connection cleanup).
// Expected:
// The client's 6th message will write failed, and the read will also be failed.
// The error will be a network error.
TEST_F(AsyncStreamTest, TestBidiStreamCloseConnectionAtClient) {
  DoTestClientCloseConn([](std::shared_ptr<AsyncTestStreamServiceServiceProxy> proxy) {
    auto fut = RunAsMerge([&](PromPtr prom) {
      ClientContextPtr ctx = MakeClientContext(proxy);
      return proxy->BidiStreamCloseConnectionAtClient(ctx).Then(
          [&, prom](stream::AsyncReaderWriterPtr<TestResponse, TestRequest>&& rw) {
            return DoFor(10, [rw, prom](std::size_t i) {
              Future<> fut = MakeReadyFuture<>();
              if (i == 5) {
                // Trigger the client's idle connection cleanup.
                fut = AsyncTimer(false).After(11000);
              }
              return fut.Then([=]() {
                TestRequest req;
                req.set_msg(std::string("msg") + std::to_string(i + 1));
                return rw->Write(std::move(req))
                    .Then([i, rw]() {
                      return rw->Read().Then(
                          [i](std::optional<TestResponse>&& rsp) { return MakeReadyFuture<bool>(true); });
                    })
                    .Then([rw, i, prom](Future<bool>&& ft) {
                      if (ft.IsReady()) {
                        return MakeReadyFuture<bool>(true);
                      }
                      EXPECT_EQ(i, 5);
                      // Write to stream error
                      EXPECT_EQ(ft.GetException().GetExceptionCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
                      // Read from stream error
                      return rw->Read().Then([rw, prom](Future<std::optional<TestResponse>>&& ft) {
                        EXPECT_EQ(ft.GetException().GetExceptionCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
                        // `Finish` can normally done
                        return rw->Finish().Then([prom]() {
                          prom->SetValue();
                          return MakeExceptionFuture<bool>(StreamError(kUnknownErrorStatus));
                        });
                      });
                    });
              });
            });
          });
    });
    future::BlockingGet(std::move(fut));
  });
  // Wait a moment, let the server handle the disconnection so that it can be easily verified.
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  CheckConnClose("client_close_conn_bidi_stream", "5-" + std::to_string(TRPC_STREAM_SERVER_NETWORK_ERR), true);
}

}  // namespace trpc::testing

int main(int argc, char** argv) {
  int test_argc;
  char **client_argv, **server1_argv, **server2_argv;
  if (!::trpc::testing::ExtractServerAndClientArgs(argc, argv, &test_argc, &client_argv, &server1_argv)) {
    exit(EXIT_FAILURE);
  }
  if (!::trpc::testing::ExtractServerAndClientArgs(test_argc, client_argv, &test_argc, &client_argv, &server2_argv)) {
    exit(EXIT_FAILURE);
  }

  trpc::testing::AsyncStreamTest::SetArgv(test_argc, client_argv, server1_argv, server2_argv);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
