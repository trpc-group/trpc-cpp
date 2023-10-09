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

#include "trpc/codec/grpc/http2/session.h"

#include <memory>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/codec/grpc/http2/client_session.h"
#include "trpc/codec/grpc/http2/request.h"
#include "trpc/codec/grpc/http2/response.h"
#include "trpc/codec/grpc/http2/server_session.h"
#include "trpc/util/time.h"

namespace trpc::testing {

namespace {
class MockSession : public http2::Session {
 public:
  explicit MockSession(http2::Session::Options&& options) : http2::Session(std::move(options)) {}

  ~MockSession() override { Destroy(); }

  bool Init() override { return true; }
};
}  // namespace

class Http2SessionTest : public ::testing::Test {
 protected:
  void SetUp() override { session_ = std::make_unique<MockSession>(http2::Session::Options()); }

  void TearDown() override {}

 protected:
  std::unique_ptr<http2::Session> session_{nullptr};
};

TEST_F(Http2SessionTest, InitOK) { ASSERT_TRUE(session_->Init()); }

TEST_F(Http2SessionTest, CreateStreamOk) {
  auto stream = session_->CreateStream();
  ASSERT_NE(nullptr, stream);
}

TEST_F(Http2SessionTest, InsertStreamOk) {
  auto stream = session_->CreateStream();
  uint32_t stream_id = 1;

  // stream identified by `0` not exists.
  ASSERT_EQ(nullptr, session_->FindStream(0));

  stream->SetStreamId(stream_id);

  session_->AddStream(std::move(stream));
  ASSERT_NE(nullptr, session_->FindStream(stream_id));

  session_->DeleteStream(stream_id);
  ASSERT_EQ(nullptr, session_->FindStream(stream_id));
}

TEST_F(Http2SessionTest, OnMessageFailed) { ASSERT_EQ(-1, session_->OnMessage(nullptr, nullptr)); }

TEST_F(Http2SessionTest, SubmitRequestFailed) { ASSERT_EQ(-1, session_->SubmitRequest(nullptr)); }

TEST_F(Http2SessionTest, SubmitResponseFailed) { ASSERT_EQ(-1, session_->SubmitResponse(nullptr)); }

TEST_F(Http2SessionTest, SubmitHeaderFailed) {
  uint32_t stream_id = 123;
  http2::HeaderPairs header;
  ASSERT_EQ(-1, session_->SubmitHeader(stream_id, header));
}

TEST_F(Http2SessionTest, SubmitDataFailed) {
  uint32_t stream_id = 123;
  NoncontiguousBuffer data;
  ASSERT_EQ(-1, session_->SubmitData(stream_id, std::move(data)));
}

TEST_F(Http2SessionTest, SubmitTrailerFailed) {
  uint32_t stream_id = 123;
  http2::TrailerPairs trailer;
  ASSERT_EQ(-1, session_->SubmitTrailer(stream_id, trailer));
}

TEST_F(Http2SessionTest, SubmitResetFailed) {
  uint32_t stream_id = 123;
  uint32_t error_code = 1;
  ASSERT_EQ(-1, session_->SubmitReset(stream_id, error_code));
}

class Http2SessionImplTest : public ::testing::Test {
 protected:
  void SetUp() override {
    server_session_ = std::make_unique<http2::ServerSession>(http2::Session::Options());
    server_session_->SetOnEofRecvCallback(
        [this](http2::RequestPtr& request) { request_recv_queue_.emplace_back(request); });
    server_session_->SetOnHeaderRecvCallback(
        [this](http2::RequestPtr& request) { server_recv_header_queue_.push_back(request->GetHeader()); });
    server_session_->SetOnDataRecvCallback([this](http2::RequestPtr& request) {
      server_recv_data_queue_.push_back(request->GetNonContiguousBufferContent());
    });
    server_session_->SetOnEofRecvCallback([this](http2::RequestPtr& request) {
      server_recv_eof_count_ += 1;
      request_recv_queue_.emplace_back(request);
    });

    client_session_ = std::make_unique<http2::ClientSession>(http2::Session::Options());
    client_session_->SetOnResponseCallback(
        [this](http2::ResponsePtr&& response) { response_recv_queue_.emplace_back(std::move(response)); });
  }

  void TearDown() override {}

  http2::RequestPtr CreateHttp2Request() {
    auto request = http2::CreateRequest();
    request->SetMethod("POST");
    request->SetPath("/index.html");
    request->SetScheme("http");
    request->SetAuthority("xx.example.com");
    request->AddHeader("content-type", "application/json");
    request->AddHeader("te", "trailers");
    request->AddHeader("x-user-defined-01", "Tom");
    request->AddHeader("x-user-defined-02", "Jerry");

    NoncontiguousBufferBuilder buffer_builder{};
    buffer_builder.Append(R"({"greetings": "hello world!"})");
    request->SetNonContiguousBufferContent(std::move(buffer_builder.DestructiveGet()));
    return request;
  }

  http2::ResponsePtr CreateHttp2Response() {
    auto response = http2::CreateResponse();
    response->SetStatus(200);
    response->AddHeader("content-type", "application/json");
    //    response->AddHeader("te", "trailers");
    response->AddHeader("x-user-defined-01", "Tom");
    response->AddHeader("x-user-defined-02", "Jerry");

    response->GetMutableTrailer()->Add("x-user-defined-03", "Cat");
    response->GetMutableTrailer()->Add("x-user-defined-04", "Mouse");

    NoncontiguousBufferBuilder buffer_builder{};
    buffer_builder.Append(R"({"greetings": "hello world!"})");
    response->SetNonContiguousBufferContent(buffer_builder.DestructiveGet());
    return response;
  }

  bool SubmitRequest(const http2::RequestPtr& request) {
    if (client_session_->SubmitRequest(request) != 0) {
      return false;
    }

    // Packs HTTP2 request.
    NoncontiguousBuffer send_buffer;
    if (client_session_->SignalWrite(&send_buffer) != 0) {
      return false;
    }

    // Skips network transporting.
    NoncontiguousBuffer recv_buffer = std::move(send_buffer);

    // Unpacks HTTP2 Request.
    if (server_session_->SignalRead(&recv_buffer) != 0) {
      return false;
    }

    return true;
  }

  bool SubmitResponse(const http2::ResponsePtr& response) {
    if (server_session_->SubmitResponse(response) != 0) {
      return false;
    }

    NoncontiguousBuffer send_buffer;
    if (server_session_->SignalWrite(&send_buffer) != 0) {
      return false;
    }

    std::cout << "recv response buffer size: " << send_buffer.ByteSize() << std::endl;

    NoncontiguousBuffer recv_buffer = std::move(send_buffer);
    if (client_session_->SignalRead(&recv_buffer) != 0) {
      return false;
    }

    return true;
  }

  bool SubmitResponseInStreamWay(const http2::ResponsePtr& response) {
    uint32_t stream_id = response->GetStreamId();

    NoncontiguousBuffer send_buffer;

    http2::HeaderPairs header;
    header.Add(http2::kHttp2HeaderStatusName, http2::kHttp2HeaderStatusOk);
    std::string date = TimeStringHelper::ConvertEpochToHttpDate(GetNowAsTimeT());
    header.Add(http2::kHttp2HeaderDateName, date);
    for (const auto& it : response->GetHeaderPairs()) {
      header.Add(it.first, it.second);
    }
    if (server_session_->SubmitHeader(stream_id, header) != 0) {
      return false;
    }
    NoncontiguousBuffer header_buffer;
    if (server_session_->SignalWrite(&header_buffer) != 0) {
      return false;
    }
    send_buffer.Append(std::move(header_buffer));

    if (server_session_->SubmitData(stream_id, std::move(*response->GetMutableNonContiguousBufferContent())) != 0) {
      return false;
    }
    NoncontiguousBuffer data_buffer;
    if (server_session_->SignalWrite(&data_buffer) != 0) {
      return false;
    }
    send_buffer.Append(std::move(data_buffer));

    if (server_session_->SubmitTrailer(stream_id, response->GetTrailer()) != 0) {
      return false;
    }
    NoncontiguousBuffer trailer_buffer;
    if (server_session_->SignalWrite(&trailer_buffer) != 0) {
      return false;
    }
    send_buffer.Append(std::move(trailer_buffer));

    std::cout << "recv response buffer size: " << send_buffer.ByteSize() << std::endl;
    NoncontiguousBuffer recv_buffer = std::move(send_buffer);
    if (client_session_->SignalRead(&recv_buffer) != 0) {
      return false;
    }

    return true;
  }

  bool Http2Handshake() {  // http2 client preface
    NoncontiguousBuffer send_buffer;
    if (client_session_->SignalWrite(&send_buffer) != 0) {
      return false;
    }

    NoncontiguousBuffer recv_buffer = std::move(send_buffer);
    if (server_session_->SignalRead(&recv_buffer) != 0) {
      return false;
    }

    // http2 server preface
    if (server_session_->SignalWrite(&send_buffer) != 0) {
      return false;
    }

    recv_buffer = std::move(send_buffer);
    if (client_session_->SignalRead(&recv_buffer) != 0) {
      return false;
    }

    return true;
  }

  void TestSubmitResponseOrInStreamWayOk(bool is_in_stream_way) {
    ASSERT_TRUE(Http2Handshake());

    auto send_request = CreateHttp2Request();
    std::string send_request_str = send_request->ToString();
    ASSERT_TRUE(SubmitRequest(send_request));

    ASSERT_EQ(request_recv_queue_.size(), 1);
    auto recv_request = request_recv_queue_.front();
    request_recv_queue_.pop_front();

    auto send_response = CreateHttp2Response();
    std::string send_response_str = send_response->ToString();
    send_response->SetStreamId(recv_request->GetStreamId());

    if (is_in_stream_way) {
      ASSERT_TRUE(SubmitResponseInStreamWay(send_response));
    } else {
      ASSERT_TRUE(SubmitResponse(send_response));
    }

    ASSERT_EQ(response_recv_queue_.size(), 1);
    auto recv_response = response_recv_queue_.front();
    response_recv_queue_.pop_front();

    auto recv_response_str = recv_response->ToString();
    ASSERT_TRUE(!recv_response_str.empty());
    ASSERT_GE(recv_response_str.size(), send_response_str.size());
  }

 protected:
  std::unique_ptr<http2::ClientSession> client_session_{nullptr};
  std::unique_ptr<http2::ServerSession> server_session_{nullptr};
  std::deque<http2::RequestPtr> request_recv_queue_;
  std::deque<http2::ResponsePtr> response_recv_queue_;
  std::deque<http2::HeaderPairs> server_recv_header_queue_;
  std::deque<NoncontiguousBuffer> server_recv_data_queue_;
  uint32_t server_recv_eof_count_ = 0;
};

TEST_F(Http2SessionImplTest, SubmitRequestOk) {
  ASSERT_TRUE(Http2Handshake());

  auto send_request = CreateHttp2Request();
  std::string send_request_str{send_request->ToString()};
  ASSERT_TRUE(SubmitRequest(send_request));

  ASSERT_EQ(server_recv_header_queue_.size(), 1);
  server_recv_header_queue_.pop_front();

  ASSERT_EQ(server_recv_data_queue_.size(), 1);
  server_recv_data_queue_.pop_front();

  ASSERT_EQ(server_recv_eof_count_, 1);

  ASSERT_EQ(request_recv_queue_.size(), 1);
  auto recv_request = request_recv_queue_.front();
  request_recv_queue_.pop_front();

  ASSERT_EQ(recv_request->ToString(), send_request_str);
}

TEST_F(Http2SessionImplTest, SubmitResponseOk) { TestSubmitResponseOrInStreamWayOk(false); }

TEST_F(Http2SessionImplTest, SubmitResponseInStreamWayOk) { TestSubmitResponseOrInStreamWayOk(true); }

}  // namespace trpc::testing
