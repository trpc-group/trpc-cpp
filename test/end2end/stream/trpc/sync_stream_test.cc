// Copyright (c) 2023, Tencent Inc.
// All rights reserved.

#include "test/end2end/stream/trpc/stream_test.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/util/string_helper.h"
#include "trpc/util/time.h"

namespace trpc::testing {

using namespace trpc::stream;
using namespace trpc::testing::stream_test;

class SyncStreamTest : public StreamTest {
 protected:
  void SetUp() override {
    // Run before each test case
    to_sync_service_proxy_ =
        ::trpc::GetTrpcClient()->GetProxy<TestStreamServiceServiceProxy>("trpc.testing.stream_test.SyncStreamService");
    to_async_service_proxy_ =
        ::trpc::GetTrpcClient()->GetProxy<TestStreamServiceServiceProxy>("trpc.testing.stream_test.AsyncStreamService");
    to_sync_idle_service_proxy_ = ::trpc::GetTrpcClient()->GetProxy<TestStreamServiceServiceProxy>(
        "trpc.testing.stream_test.SyncStreamServiceConnIdle");
    to_async_idle_service_proxy_ = ::trpc::GetTrpcClient()->GetProxy<TestStreamServiceServiceProxy>(
        "trpc.testing.stream_test.AsyncStreamServiceConnIdle");
    to_sync_idle_service_idle_proxy_ = ::trpc::GetTrpcClient()->GetProxy<TestStreamServiceServiceProxy>(
        "trpc.testing.stream_test.SyncStreamServiceConnIdleAtClient");
    to_async_idle_service_idle_proxy_ = ::trpc::GetTrpcClient()->GetProxy<TestStreamServiceServiceProxy>(
        "trpc.testing.stream_test.AsyncStreamServiceConnIdleAtClient");

    to_sync_no_flow_control_service_proxy_ = ::trpc::GetTrpcClient()->GetProxy<TestStreamServiceServiceProxy>(
        "trpc.testing.stream_test.SyncStreamServiceNoFlowControl");
    to_sync_service_no_flow_control_proxy_ = ::trpc::GetTrpcClient()->GetProxy<TestStreamServiceServiceProxy>(
        "trpc.testing.stream_test.SyncStreamServiceNoFlowControlAtClient");
  }

  void TearDown() override {
    // Run after each test case
  }

  enum TestFlowControlType {
    kFlowControlBoth,
    kFlowControlOnlyServer,
    kFlowControlOnlyClient,
  };
  void DoTestFlowControl(TestFlowControlType type,
                         std::function<void(std::shared_ptr<TestStreamServiceServiceProxy>)>&& test_fn) {
    std::cout << "==================== Test to sync service ====================" << std::endl;
    std::shared_ptr<TestStreamServiceServiceProxy> proxy(nullptr);
    switch (type) {
      case TestFlowControlType::kFlowControlBoth:
        proxy = to_sync_service_proxy_;
        break;
      case TestFlowControlType::kFlowControlOnlyServer:
        proxy = to_sync_service_no_flow_control_proxy_;
        break;
      case TestFlowControlType::kFlowControlOnlyClient:
        proxy = to_sync_no_flow_control_service_proxy_;
      default:
        break;
    }
    test_fn(proxy);
  }

  void DoTest(std::function<void(std::shared_ptr<TestStreamServiceServiceProxy>)>&& test_fn) {
    std::cout << "==================== Test to sync service ====================" << std::endl;
    test_fn(to_sync_service_proxy_);
    std::cout << "==================== Test to async service ====================" << std::endl;
    test_fn(to_async_service_proxy_);
  }

  void DoTestServerCloseConn(std::function<void(std::shared_ptr<TestStreamServiceServiceProxy>)>&& test_fn) {
    std::cout << "==================== Test to sync service(DoTestServerCloseConn) ====================" << std::endl;
    test_fn(to_sync_idle_service_proxy_);
    std::cout << "==================== Test to async service(DoTestServerCloseConn) ====================" << std::endl;
    test_fn(to_async_idle_service_proxy_);
  }

  void DoTestClientCloseConn(std::function<void(std::shared_ptr<TestStreamServiceServiceProxy>)>&& test_fn) {
    std::cout << "==================== Test to sync service(DoTestClientCloseConn) ====================" << std::endl;
    test_fn(to_sync_idle_service_idle_proxy_);
    std::cout << "==================== Test to async service(DoTestClientCloseConn) ====================" << std::endl;
    test_fn(to_async_idle_service_idle_proxy_);
  }

  void CheckConnClose(std::string type, std::string check, bool client_idle = false) {
    RunByEnv([=]() {
      std::cout << "==================== Check conn close for sync service ====================" << std::endl;
      std::shared_ptr<TestStreamServiceServiceProxy> sync_proxy =
          client_idle ? to_sync_idle_service_idle_proxy_ : to_sync_idle_service_proxy_;
      ClientContextPtr ctx = MakeClientContext(sync_proxy);
      TestRequest req_sync;
      req_sync.set_msg(type);
      TestResponse rsp_sync;
      Status status = sync_proxy->StreamAtConnCloseStatus(ctx, req_sync, &rsp_sync);
      ASSERT_TRUE(status.OK());
      ASSERT_EQ(rsp_sync.msg(), check);
            std::cout << "==================== Check conn close for async service ====================" << std::endl;
            std::shared_ptr<TestStreamServiceServiceProxy> async_proxy =
                client_idle ? to_async_idle_service_idle_proxy_ : to_async_idle_service_proxy_;
            ctx = MakeClientContext(async_proxy);
            TestRequest req_async;
            req_async.set_msg(type);
            TestResponse rsp_async;
            status = async_proxy->StreamAtConnCloseStatus(ctx, req_async, &rsp_async);
            ASSERT_TRUE(status.OK());
            ASSERT_EQ(rsp_async.msg(), check);
    });
  }

  // Used to trigger client idle connection cleanup at fiber mode
  void DoEmtpyCall(std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    ClientContextPtr ctx = MakeClientContext(proxy);
    TestRequest req_sync;
    req_sync.set_msg("empty call");
    TestResponse rsp_sync;
    Status status = proxy->StreamAtConnCloseStatus(ctx, req_sync, &rsp_sync);
  }

 protected:
  std::shared_ptr<TestStreamServiceServiceProxy> to_sync_service_proxy_;
  std::shared_ptr<TestStreamServiceServiceProxy> to_sync_idle_service_proxy_;
  std::shared_ptr<TestStreamServiceServiceProxy> to_sync_idle_service_idle_proxy_;
  std::shared_ptr<TestStreamServiceServiceProxy> to_async_service_proxy_;
  std::shared_ptr<TestStreamServiceServiceProxy> to_async_idle_service_proxy_;
  std::shared_ptr<TestStreamServiceServiceProxy> to_async_idle_service_idle_proxy_;

  std::shared_ptr<TestStreamServiceServiceProxy> to_sync_no_flow_control_service_proxy_;
  std::shared_ptr<TestStreamServiceServiceProxy> to_sync_service_no_flow_control_proxy_;
};

// Test Case:
// The server writes to the stream 10 times, each time with "rsp-msgx", where x increases from 1 to 10.
// The client sends a message "req-msg" and reads from the stream 10 times.
// Expected:
// The server writes successfully each time.
// The client checks the messages 1st to the 10th time are "rsp-msg1" to "rsp-msg10" respectively.
TEST_F(SyncStreamTest, TestServerStreamOk) {
  DoTest([](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      TestRequest req;
      req.set_msg("req-msg");
      StreamReader<TestResponse> reader = proxy->ServerStream(ctx, req);
      int count = 0;
      for (;;) {
        TestResponse rsp;
        Status status = reader.Read(&rsp);
        if (status.StreamEof()) {
          break;
        }
        ASSERT_TRUE(status.OK());
        ++count;
        ASSERT_EQ(rsp.msg(), std::string("rsp-msg") + std::to_string(count));
      }
      ASSERT_EQ(count, 10);
    });
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
TEST_F(SyncStreamTest, TestServerStreamReadTimeout) {
  DoTest([](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      TestRequest req;
      req.set_msg("req-msg");
      StreamReader<TestResponse> reader = proxy->ServerStreamReadTimeout(ctx, req);
      int count = 0;
      int timeout_count = 0;
      for (;;) {
        TestResponse rsp;
        Status status = reader.Read(&rsp, 90);
        if (status.StreamEof()) {
          break;
        }
        if (status.GetFrameworkRetCode() == TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR) {
          ++timeout_count;
          continue;
        }
        ASSERT_TRUE(status.OK());
        ++count;
        ASSERT_EQ(rsp.msg(), std::string("rsp-msg") + std::to_string(count));
      }
      ASSERT_EQ(count, 10);
      ASSERT_EQ(timeout_count, 10);
    });
  });
}

// Test Case:
// The client sends an empty message string and immediately ends the communication without reading from the stream.
// The server does not write anything to the stream.
// Expected:
// The client will continue to read eof after reading it first.
TEST_F(SyncStreamTest, TestServerStreamNoData) {
  DoTest([](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      TestRequest req;
      req.set_msg("");
      StreamReader<TestResponse> reader = proxy->ServerStreamNoData(ctx, req);
      TestResponse rsp;
      ASSERT_TRUE(reader.Read(&rsp).StreamEof());
      // After reading EOF, reading again will still result in EOF.
      ASSERT_TRUE(reader.Read(&rsp).StreamEof());
    });
  });
}

// Test Case:
// The client initiates a message with an empty string, then calls `Finish` until timeout
// The server does not write to the stream and waits for 3.5 seconds before ending.
// Expected:
// After the client calling Finish until timeout and get timeout error, then it can call `Finish` again and get the same
// error immediately without block.
TEST_F(SyncStreamTest, TestServerStreamFinishTimeout) {
  DoTest([](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      StreamReader<TestResponse> reader = proxy->ServerStreamFinishTimeout(ctx, TestRequest());
      // First time reading, timeout waiting for 3 seconds, treated as a stream error.
      uint64_t begin = trpc::time::GetMilliSeconds();
      ASSERT_EQ(reader.Finish().GetFrameworkRetCode(), TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR);
      ASSERT_GT(trpc::time::GetMilliSeconds() - begin, 1000);
      // Second time reading, the error can be directly obtained.
      begin = trpc::time::GetMilliSeconds();
      ASSERT_EQ(reader.Finish().GetFrameworkRetCode(), TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR);
      ASSERT_LT(trpc::time::GetMilliSeconds() - begin, 1000);
    });
  });
}

// Test Case:
// After the server writes to the stream 5 times, it will disconnect.(depending on the server's idle connection cleanup)
// Expected:
// The client is expected to read 5 messages before the connection is disconnected.
// The server writes to stream but got connection closed error.
TEST_F(SyncStreamTest, TestServerStreamCloseConnectionAtServer) {
  DoTestServerCloseConn([](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      TestRequest req;
      StreamReader<TestResponse> reader = proxy->ServerStreamCloseConnectionAtServer(ctx, req);
      int count = 0;
      for (;;) {
        TestResponse rsp;
        Status status = reader.Read(&rsp);
        if (!status.OK()) {
          ASSERT_EQ(status.GetFrameworkRetCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
          break;
        }
        ++count;
        ASSERT_EQ(rsp.msg(), std::string("rsp-msg") + std::to_string(count));
      }
      ASSERT_EQ(count, 5);
    });
  });
  // Wait a moment, let the server handle the disconnection so that it can be easily verified.
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  CheckConnClose("server_close_conn_server_stream", std::to_string(TRPC_STREAM_SERVER_NETWORK_ERR));
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}

// Test Case:
// After the server writes to the stream 5 times, it will disconnect.(depending on the client's idle connection cleanup)
// Expected:
// The client is expected to read 5 messages before the connection is disconnected.
// The server writes to stream but got connection closed error.
TEST_F(SyncStreamTest, TestServerStreamCloseConnectionAtClient) {
  DoTestClientCloseConn([&](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    // When at fiber mode, to trigger idle connection cleanup, it needs to obtain a connector.
    StartFiberDetached([&]() {
      FiberSleepFor(std::chrono::milliseconds(1000));
      DoEmtpyCall(proxy);
    });
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      TestRequest req;
      StreamReader<TestResponse> reader = proxy->ServerStreamCloseConnectionAtClient(ctx, req);
      int count = 0;
      for (;;) {
        TestResponse rsp;
        Status status = reader.Read(&rsp);
        if (!status.OK()) {
          ASSERT_EQ(status.GetFrameworkRetCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
          break;
        }
        ++count;
        ASSERT_EQ(rsp.msg(), std::string("rsp-msg") + std::to_string(count));
      }
      ASSERT_EQ(count, 5);
    });
  });
  // Wait a moment, let the server handle the disconnection so that it can be easily verified.
  std::this_thread::sleep_for(std::chrono::milliseconds(14000));
  CheckConnClose("client_close_conn_server_stream", std::to_string(TRPC_STREAM_SERVER_NETWORK_ERR), true);
}

// Test Case:
// After stream established(with flow control window size of 65535), the server starts writing 10 responses, each with
// 10000 bytes. The client reads every 100ms. In the 11th response, the server includes
// which write triggers flow control and the timestamp when it was released.
// The flow control window is set to 0 in the client's YAML file, disabling flow control.
// Expected:
// The server will not trigger flow control.
TEST_F(SyncStreamTest, TestServerStreamFlowControlOnlyServer) {
  DoTestFlowControl(TestFlowControlType::kFlowControlOnlyServer,
                    [](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
                      RunByEnv([&]() {
                        ClientContextPtr ctx = MakeClientContext(proxy);
                        TestRequest req;
                        StreamReader<TestResponse> reader = proxy->ServerStreamFlowControl(ctx, req);
                        for (int i = 0; i < 10; ++i) {
                          FiberSleepFor(std::chrono::milliseconds(100));
                          TestResponse rsp;
                          Status status = reader.Read(&rsp);
                          ASSERT_TRUE(status.OK());
                        }
                        TestResponse rsp;
                        Status status = reader.Read(&rsp);
                        ASSERT_TRUE(status.OK());
                        uint64_t timestamp = TryParse<uint64_t>(rsp.msg()).value_or(9999);
                        ASSERT_EQ(timestamp, 0);
                        status = reader.Read(&rsp);
                        ASSERT_TRUE(status.StreamEof());
                      });
                    });
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
TEST_F(SyncStreamTest, TestServerStreamFlowControl) {
  DoTestFlowControl(TestFlowControlType::kFlowControlBoth, [](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      TestRequest req;
      StreamReader<TestResponse> reader = proxy->ServerStreamFlowControl(ctx, req);
      uint64_t flow_control_trigger_timestamp = 0;
      for (int i = 0; i < 10; ++i) {
        FiberSleepFor(std::chrono::milliseconds(100));
        TestResponse rsp;
        Status status = reader.Read(&rsp);
        if (i == 1) {
          flow_control_trigger_timestamp = trpc::time::GetMilliSeconds();
        }
        ASSERT_TRUE(status.OK());
      }
      TestResponse rsp;
      Status status = reader.Read(&rsp);
      ASSERT_TRUE(status.OK());
      uint64_t server_flow_control_trigger_timestamp = TryParse<uint64_t>(rsp.msg()).value_or(9999);
      uint64_t diff = server_flow_control_trigger_timestamp > flow_control_trigger_timestamp
                          ? server_flow_control_trigger_timestamp - flow_control_trigger_timestamp
                          : flow_control_trigger_timestamp - server_flow_control_trigger_timestamp;
      ASSERT_LE(diff, 100);
      status = reader.Read(&rsp);
      ASSERT_TRUE(status.StreamEof());
    });
  });
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
TEST_F(SyncStreamTest, TestClientStreamOk) {
  DoTest([](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      TestResponse rsp;
      StreamWriter<TestRequest> writer = proxy->ClientStream(ctx, &rsp);
      std::string check_msg;
      for (int i = 0; i < 10; ++i) {
        TestRequest req;
        std::string write_msg = std::string("req-msg") + std::to_string(i + 1);
        check_msg += write_msg;
        req.set_msg(write_msg);
        Status status = writer.Write(req);
        ASSERT_TRUE(status.OK());
      }
      writer.WriteDone();
      ASSERT_EQ(check_msg, rsp.msg());
    });
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
TEST_F(SyncStreamTest, TestClientStreamReadTimeout) {
  DoTest([](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      TestResponse rsp;
      StreamWriter<TestRequest> writer = proxy->ClientStreamReadTimeout(ctx, &rsp);
      std::string check_msg;
      for (int i = 0; i < 10; ++i) {
        TestRequest req;
        std::string write_msg = std::string("req-msg") + std::to_string(i + 1);
        check_msg += write_msg;
        req.set_msg(write_msg);
        FiberSleepFor(std::chrono::milliseconds(100));
        Status status = writer.Write(req);
        ASSERT_TRUE(status.OK());
      }
      writer.WriteDone();
      ASSERT_EQ(check_msg + "timeout_count:10", rsp.msg());
    });
  });
}

// Test case:
// The client does not write to the stream.
// The server does not read from the stream and ends directly, returning an empty string as the message.
// Expected:
// The server should detect the end of the stream (EOF) and any further read attempts should also return EOF.
// The client should receive an empty response.
TEST_F(SyncStreamTest, TestClientStreamNoData) {
  DoTest([](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      TestResponse rsp;
      StreamWriter<TestRequest> writer = proxy->ClientStreamNoData(ctx, &rsp);
      writer.WriteDone();
      ASSERT_TRUE(writer.Finish().OK());
      ASSERT_EQ(rsp.msg(), "");
    });
  });
}

// Test case:
// The client does not write to the stream.
// The server does not read from the stream and waits for 6.5 seconds before ending.
// Expected:
// The client should time out when calling 'WriteDone' to wait for a response.
// The client should also time out when calling 'Finish'.
// When the client calls 'Finish' again, it should immediately receive an error.
TEST_F(SyncStreamTest, TestClientStreamFinishTimeout) {
  DoTest([](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      TestResponse rsp;
      StreamWriter<TestRequest> writer = proxy->ClientStreamFinishTimeout(ctx, &rsp);
      uint64_t begin = trpc::time::GetMilliSeconds();
      // 'WriteDone' will wait for a response for 3 seconds.
      ASSERT_EQ(writer.WriteDone().GetFrameworkRetCode(), TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR);
      ASSERT_GT(trpc::time::GetMilliSeconds() - begin, 1000);
      // The first read will time out after 3 seconds and will be considered a stream timeout error.
      begin = trpc::time::GetMilliSeconds();
      ASSERT_EQ(writer.Finish().GetFrameworkRetCode(), TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR);
      ASSERT_GT(trpc::time::GetMilliSeconds() - begin, 1000);
      // When calling the function again for the second time, it will immediately receive the same timeout error.
      begin = trpc::time::GetMilliSeconds();
      ASSERT_EQ(writer.Finish().GetFrameworkRetCode(), TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR);
      ASSERT_LT(trpc::time::GetMilliSeconds() - begin, 1000);
    });
  });
}

// Test case:
// The client writes to the stream 10 times, and the server disconnects after the 5th write.
// Expected result:
// The 6th message written by the client will fail with a network error, and calling `WriteDone` and `Finish` will also
// result in a network error.
TEST_F(SyncStreamTest, TestClientStreamCloseConnectionAtServer) {
  DoTestServerCloseConn([](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      TestResponse rsp;
      StreamWriter<TestRequest> writer = proxy->ClientStreamCloseConnectionAtServer(ctx, &rsp);
      int counter = 0;
      Status status;
      for (int i = 0; i < 10; ++i) {
        TestRequest req;
        std::string write_msg = std::string("req-msg") + std::to_string(i + 1);
        req.set_msg(write_msg);
        status = writer.Write(req);
        if (!status.OK()) {
          ASSERT_EQ(status.GetFrameworkRetCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
          break;
        }
        ++counter;
        if (counter == 5) {
          // Trigger the server's idle connection cleanup.
          std::cout << "sleep to trigger server idle conn clean, " << std::endl;
          FiberSleepFor(std::chrono::milliseconds(3000));
          std::cout << "after sleep: " << std::endl;
        }
      }
      ASSERT_EQ(counter, 5);
      status = writer.WriteDone();
      ASSERT_EQ(status.GetFrameworkRetCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
      status = writer.Finish();
      ASSERT_EQ(status.GetFrameworkRetCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
    });
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
TEST_F(SyncStreamTest, TestClientStreamCloseConnectionAtClient) {
  DoTestClientCloseConn([&](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    // When at fiber mode, to trigger idle connection cleanup, it needs to obtain a connector.
    StartFiberDetached([&]() {
      FiberSleepFor(std::chrono::milliseconds(1000));
      DoEmtpyCall(proxy);
    });
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      TestResponse rsp;
      StreamWriter<TestRequest> writer = proxy->ClientStreamCloseConnectionAtClient(ctx, &rsp);
      int counter = 0;
      Status status;
      for (int i = 0; i < 10; ++i) {
        TestRequest req;
        std::string write_msg = std::string("req-msg") + std::to_string(i + 1);
        req.set_msg(write_msg);
        status = writer.Write(req);
        if (!status.OK()) {
          ASSERT_EQ(status.GetFrameworkRetCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
          break;
        }
        ++counter;
        if (counter == 5) {
          // Trigger the client's idle connection cleanup.
          FiberSleepFor(std::chrono::milliseconds(3000));
        }
      }
      ASSERT_EQ(counter, 5);
      status = writer.WriteDone();
      ASSERT_EQ(status.GetFrameworkRetCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
      status = writer.Finish();
      ASSERT_EQ(status.GetFrameworkRetCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
    });
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
TEST_F(SyncStreamTest, TestClientStreamFlowControl) {
  DoTestFlowControl(TestFlowControlType::kFlowControlBoth, [](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      TestResponse rsp;
      StreamWriter<TestRequest> writer = proxy->ClientStreamFlowControl(ctx, &rsp);
      std::string check_msg;
      uint64_t begin, end;
      for (int i = 0; i < 10; ++i) {
        TestRequest req;
        req.set_msg(std::string(10000, 's'));
        begin = trpc::time::GetMilliSeconds();
        Status status = writer.Write(req);
        end = trpc::time::GetMilliSeconds();
        ASSERT_TRUE(status.OK());
        // If the write is blocked for more than 100ms, it is considered that flow control has been triggered.
        if (end - begin > 100) {
          ASSERT_EQ(i, 7);
          break;
        }
      }
      ASSERT_TRUE(writer.WriteDone().OK());
      ASSERT_TRUE(writer.Finish().OK());
      uint64_t timestamp = TryParse<uint64_t>(rsp.msg()).value_or(9999);
      uint64_t diff = timestamp > end ? timestamp - end : end - timestamp;
      ASSERT_LE(diff, 100);
    });
  });
}

// Test case:
// After establishing a stream (with a flow control window size of 65535), the client starts writing 10 requests, each
// with 10000 bytes, and the server reads a request every 100ms.
// Set the flow control window to 0 in the server's YAML file to disable flow control.
// Expected:
// Flow control will not be triggered.
TEST_F(SyncStreamTest, TestClientStreamFlowControlOnlyClient) {
  DoTestFlowControl(TestFlowControlType::kFlowControlOnlyClient,
                    [](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
                      RunByEnv([&]() {
                        ClientContextPtr ctx = MakeClientContext(proxy);
                        TestResponse rsp;
                        StreamWriter<TestRequest> writer = proxy->ClientStreamFlowControl(ctx, &rsp);
                        std::string check_msg;
                        uint64_t begin, end;
                        for (int i = 0; i < 10; ++i) {
                          TestRequest req;
                          req.set_msg(std::string(10000, 's'));
                          begin = trpc::time::GetMilliSeconds();
                          Status status = writer.Write(req);
                          end = trpc::time::GetMilliSeconds();
                          ASSERT_TRUE(status.OK());
                          // There will be no write blocking.
                          if (end - begin > 100) {
                            ASSERT_FALSE(true);
                            break;
                          }
                        }
                        ASSERT_TRUE(writer.WriteDone().OK());
                        ASSERT_TRUE(writer.Finish().OK());
                      });
                    });
}

// Test Case:
// The client and server both read and write to the stream 10 times, each time with "msgx", where x increases from 1
// to 10.
// Expected:
// The client and server both read and write to the stream 10 times. Check each message read, and the server and client
// should read messages "msg1" to "msg10" in sequence.
TEST_F(SyncStreamTest, TestBidiStreamOk) {
  DoTest([](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      StreamReaderWriter<TestRequest, TestResponse> rw = proxy->BidiStream(ctx);
      for (int i = 0; i < 10; ++i) {
        TestRequest req;
        std::string write_msg = std::string("msg") + std::to_string(i + 1);
        req.set_msg(write_msg);
        Status status = rw.Write(req);
        ASSERT_TRUE(status.OK());
        TestResponse rsp;
        status = rw.Read(&rsp);
        ASSERT_TRUE(status.OK());
        ASSERT_EQ(rsp.msg(), std::string("msg") + std::to_string(i + 1));
      }
      rw.WriteDone();
      Status status = rw.Finish();
      ASSERT_TRUE(status.OK());
    });
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
TEST_F(SyncStreamTest, TestBidiStreamReadTimeout) {
  DoTest([](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      StreamReaderWriter<TestRequest, TestResponse> rw = proxy->BidiStreamReadTimeout(ctx);
      for (int i = 0; i < 10; ++i) {
        TestRequest req;
        std::string write_msg = std::string("msg") + std::to_string(i + 1);
        req.set_msg(write_msg);
        // Sleep for 100ms before writing.
        FiberSleepFor(std::chrono::milliseconds(100));
        Status status = rw.Write(req);
        ASSERT_TRUE(status.OK());
        TestResponse rsp;
        // The first read will timeout.
        status = rw.Read(&rsp, 90);
        ASSERT_EQ(status.GetFrameworkRetCode(), TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR);
        // The second read will succeed.
        status = rw.Read(&rsp, 100);
        if (!status.OK()) {
          std::cout << "got err: " << status.ErrorMessage() << std::endl;
        }
        ASSERT_TRUE(status.OK());
        ASSERT_EQ(rsp.msg(), std::string("msg") + std::to_string(i + 1));
      }
      rw.WriteDone();
      Status status = rw.Finish();
      ASSERT_TRUE(status.OK());
    });
  });
}

// Test Case:
// Neither the client nor the server writes to the stream.
// The server ends the stream handle directly after receiving it.
// The client sends `WriteDone` directly and calls `Finish`.
// Expected:
// The server reads EOF, and the client reads EOF.
// Both ends will read EOF again.
TEST_F(SyncStreamTest, TestBidiStreamNoData) {
  DoTest([](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      StreamReaderWriter<TestRequest, TestResponse> rw = proxy->BidiStreamNoData(ctx);
      ASSERT_TRUE(rw.WriteDone().OK());
      TestResponse rsp;
      ASSERT_TRUE(rw.Read(&rsp).StreamEof());
      ASSERT_TRUE(rw.Read(&rsp).StreamEof());
      ASSERT_TRUE(rw.Finish().OK());
    });
  });
}

// Test Case:
// The client does not write to the stream.
// The server does not read from the stream and ends after waiting for 6.5 seconds.
// Expected:
// The client calling `WriteDone` will not wait for beyond 3 seconds.
// The call to `Finish` will timeout, and another call to `Finish` will immediately return a timeout error.
TEST_F(SyncStreamTest, TestBidiStreamFinishTimeout) {
  DoTest([](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      StreamReaderWriter<TestRequest, TestResponse> rw = proxy->BidiStreamFinishTimeout(ctx);
      // `WriteDone` will not wait for 3 seconds.
      ASSERT_TRUE(rw.WriteDone().OK());
      uint64_t begin = trpc::time::GetMilliSeconds();
      // The first read will timeout after 3 seconds, and it will be treated as a stream error.
      begin = trpc::time::GetMilliSeconds();
      ASSERT_EQ(rw.Finish().GetFrameworkRetCode(), TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR);
      ASSERT_GT(trpc::time::GetMilliSeconds() - begin, 1000);
      // When calling for the second time, the same error will be immediately returned.
      begin = trpc::time::GetMilliSeconds();
      ASSERT_EQ(rw.Finish().GetFrameworkRetCode(), TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR);
      ASSERT_LT(trpc::time::GetMilliSeconds() - begin, 1000);
    });
  });
}

// Test Case:
// The client writes to the stream 10 times, and the server disconnects after the 5th write.(Depending on the server's
// idle connection clean up)
// Expected:
// The client's 6th message will write incorrectly, and the read will also be incorrect. The error will be a network
// error.
TEST_F(SyncStreamTest, TestBidiStreamCloseConnectionAtServer) {
  DoTestServerCloseConn([](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      StreamReaderWriter<TestRequest, TestResponse> rw = proxy->BidiStreamCloseConnectionAtServer(ctx);
      int i = 0;
      Status status;
      for (; i < 10; ++i) {
        TestRequest req;
        std::string write_msg = std::string("msg") + std::to_string(i + 1);
        req.set_msg(write_msg);
        if (i == 5) {
          // Trigger the server's idle connection cleanup.
          FiberSleepFor(std::chrono::milliseconds(3000));
          status = rw.Write(req);
          ASSERT_EQ(status.GetFrameworkRetCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
        } else {
          status = rw.Write(req);
          ASSERT_TRUE(status.OK());
        }
        TestResponse rsp;
        status = rw.Read(&rsp);
        if (i == 5) {
          ASSERT_EQ(status.GetFrameworkRetCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
          break;
        }
        ASSERT_TRUE(status.OK());
      }
      ASSERT_EQ(i, 5);
      ASSERT_EQ(rw.WriteDone().GetFrameworkRetCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
      ASSERT_EQ(rw.Finish().GetFrameworkRetCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
    });
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
TEST_F(SyncStreamTest, TestBidiStreamCloseConnectionAtClient) {
  DoTestClientCloseConn([&](std::shared_ptr<TestStreamServiceServiceProxy> proxy) {
    // When at fiber mode, to trigger idle connection cleanup, it needs to obtain a connector.
    StartFiberDetached([&]() {
      FiberSleepFor(std::chrono::milliseconds(1000));
      DoEmtpyCall(proxy);
    });
    RunByEnv([&]() {
      ClientContextPtr ctx = MakeClientContext(proxy);
      StreamReaderWriter<TestRequest, TestResponse> rw = proxy->BidiStreamCloseConnectionAtClient(ctx);
      int i = 0;
      Status status;
      for (; i < 10; ++i) {
        TestRequest req;
        std::string write_msg = std::string("msg") + std::to_string(i + 1);
        req.set_msg(write_msg);
        if (i == 5) {
          // Trigger the client's idle connection cleanup.
          FiberSleepFor(std::chrono::milliseconds(3000));
          status = rw.Write(req);
          ASSERT_EQ(status.GetFrameworkRetCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
        } else {
          status = rw.Write(req);
          ASSERT_TRUE(status.OK());
        }
        TestResponse rsp;
        status = rw.Read(&rsp);
        if (i == 5) {
          ASSERT_EQ(status.GetFrameworkRetCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
          break;
        }
        ASSERT_TRUE(status.OK());
      }
      ASSERT_EQ(i, 5);
      ASSERT_EQ(rw.WriteDone().GetFrameworkRetCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
      ASSERT_EQ(rw.Finish().GetFrameworkRetCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
    });
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

  trpc::testing::SyncStreamTest::SetArgv(test_argc, client_argv, server1_argv, server2_argv);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
