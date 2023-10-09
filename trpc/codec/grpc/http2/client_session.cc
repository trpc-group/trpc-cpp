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

#include "trpc/codec/grpc/http2/client_session.h"

#include <string>
#include <utility>

#include "trpc/codec/grpc/http2/http2.h"
#include "trpc/codec/grpc/http2/request.h"
#include "trpc/codec/grpc/http2/response.h"
#include "trpc/util/deferred.h"
#include "trpc/util/http/util.h"
#include "trpc/util/log/logging.h"

namespace trpc::http2 {

namespace {
// @brief Starts receiving response header callback.
int OnBeginHeadersCallback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data);

// @brief Receives response header. This method will be called for each key-value pair.
int OnHeaderCallback(nghttp2_session* session, const nghttp2_frame* frame, const uint8_t* name, size_t name_len,
                     const uint8_t* value, size_t value_len, uint8_t flags, void* user_data);

// @brief Receives protocol frame callback.
int OnFrameRecvCallback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data);

// @brief Callback for receiving response data blocks.
int OnDataChunkRecvCallback(nghttp2_session* session, uint8_t flags, int32_t stream_id, const uint8_t* data, size_t len,
                            void* user_data);

// @brief Stream close callback.
int OnStreamCloseCallback(nghttp2_session* session, int32_t stream_id, uint32_t error_code, void* user_data);

// @brief Callback for unsent non-data frames due to errors.
int OnFrameNotSendCallback(nghttp2_session* session, const nghttp2_frame* frame, int error_code, void* user_data);
}  // namespace

ClientSession::ClientSession(Session::Options&& options) : Session(std::move(options)) {
  TRPC_ASSERT(Init() && "http2 client session init failed");
}

ClientSession::~ClientSession() {
  for (auto& stream : streams_) {
    auto& response = stream.second->GetResponse();
    if (response) {
      response->OnClose(0);
    }
  }
  streams_.clear();
}

bool ClientSession::Init() { return InitSession(); }

int ClientSession::SubmitRequest(const RequestPtr& request) {
  // Builds request headers.
  std::vector<nghttp2_nv> nva{};
  nva.reserve(request->GetHeader().Size() + 4);
  nva.emplace_back(nghttp2::CreatePair(":method", request->GetMethod(), true));
  nva.emplace_back(nghttp2::CreatePair(":scheme", request->GetUriRef().Scheme(), true));
  nva.emplace_back(nghttp2::CreatePair(":path", request->GetUriRef().Path(), true));
  nva.emplace_back(nghttp2::CreatePair(":authority", request->GetUriRef().Host(), true));
  request->RangeHeader([&nva](std::string_view key, std::string_view value) {
    nva.emplace_back(nghttp2::CreatePair(key, value, true));
    return true;
  });

  // Creates a stream.
  StreamPtr stream = CreateStream();

  // Sets request content.
  nghttp2_data_provider provider{};
  provider.source.ptr = stream.get();
  provider.read_callback = [](nghttp2_session* session, int32_t stream_id, uint8_t* buf, size_t length,
                              uint32_t* data_flags, nghttp2_data_source* source, void* user_data) -> ssize_t {
    auto stream = static_cast<Stream*>(source->ptr);
    ssize_t nread = stream->GetRequest()->ReadContent(buf, length);
    // EOF
    if (nread == 0) {
      *data_flags |= NGHTTP2_DATA_FLAG_EOF;
    }
    return nread;
  };

  // Submits the request.
  auto stream_id = nghttp2_submit_request(session_, nullptr, nva.data(), nva.size(), &provider, stream.get());

  TRPC_LOG_TRACE("nghttp2_submit_request"
                 << ", stream id: " << stream_id << ", request id: " << request->GetContentSequenceId());

  if (stream_id < 0) {
    TRPC_LOG_ERROR("nghttp2_submit_request failed, error: " << nghttp2_strerror(stream_id));
    return -1;
  }

  request->SetStreamId(stream_id);

  // Creates a response object here for easy correlation with the request.
  ResponsePtr response = CreateResponse();
  response->SetContentSequenceId(request->GetContentSequenceId());
  response->SetStreamId(stream_id);

  stream->SetStreamId(stream_id);
  stream->SetSession(this);
  stream->SetRequest(request);
  stream->SetResponse(response);

  AddStream(std::move(stream));
  return 0;
}

void ClientSession::OnResponse(StreamPtr& stream) {
  if (options_.on_response_cb) {
    options_.on_response_cb(stream->GetMutableResponse());
  }
}

bool ClientSession::InitSession() {
  nghttp2_session_callbacks* callbacks{nullptr};
  nghttp2_session_callbacks_new(&callbacks);

  auto callback_defer_deleter = Deferred([callbacks]() { nghttp2_session_callbacks_del(callbacks); });

  // Setups callbacks.
  nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, OnBeginHeadersCallback);
  nghttp2_session_callbacks_set_on_header_callback(callbacks, OnHeaderCallback);
  nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, OnFrameRecvCallback);
  nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, OnDataChunkRecvCallback);
  nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, OnStreamCloseCallback);
  nghttp2_session_callbacks_set_on_frame_not_send_callback(callbacks, OnFrameNotSendCallback);

  auto new_ok = nghttp2_session_client_new(&session_, callbacks, this);
  if (new_ok != 0) {
    TRPC_LOG_ERROR("create client session failed, error: " << new_ok);
    return false;
  }

  // HTTP2 settings: set the maximum concurrent stream count, default window size, etc.
  if (SubmitSettings() != 0) {
    return false;
  }
  return true;
}

int ClientSession::SubmitSettings() {
  std::vector<nghttp2_settings_entry> iv{
      // The maximum concurrent stream count.
      {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, options_.http2_settings.max_concurrent_streams},
      // Default window size.
      {NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, options_.http2_settings.initial_window_size},
      // Server PUSH.
      {NGHTTP2_SETTINGS_ENABLE_PUSH, (options_.http2_settings.enable_push ? 1U : 0U)},
  };

  int set_ok = nghttp2_submit_settings(session_, NGHTTP2_FLAG_NONE, iv.data(), iv.size());
  if (set_ok != 0) {
    TRPC_LOG_ERROR("http2 submit settings failed, error: " << set_ok);
    return set_ok;
  }

  set_ok = nghttp2_session_set_local_window_size(session_, NGHTTP2_FLAG_NONE, 0,
                                                 options_.http2_settings.initial_window_size);
  if (set_ok != 0) {
    TRPC_LOG_ERROR("http2 set local_window_size failed, error: " << set_ok);
    return set_ok;
  }
  return set_ok;
}

namespace {
int OnBeginHeadersCallback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data) {
  TRPC_LOG_TRACE("stream begin headers, stream id: " << frame->hd.stream_id);
  return 0;
}
}  // namespace

namespace {
int OnHeaderCallback(nghttp2_session* session, const nghttp2_frame* frame, const uint8_t* name, size_t name_len,
                     const uint8_t* value, size_t value_len, uint8_t flags, void* user_data) {
  TRPC_LOG_TRACE("stream on header, stream id: " << frame->hd.stream_id);
  auto client_session = static_cast<ClientSession*>(user_data);
  switch (frame->hd.type) {
    case NGHTTP2_HEADERS: {
      auto stream = client_session->FindStream(frame->hd.stream_id);
      if (!stream) {
        return 0;
      }

      int token = LookupToken(name, name_len);
      auto& response = stream->GetResponse();
      // Received `status`.
      if (token == kHeaderColonStatus) {
        response->SetStatus(http::ParseUint(value, value_len));
      } else {
        response->AddHeader(std::string(name, name + name_len), std::string(value, value + value_len));
      }
      break;
    }
    case NGHTTP2_PUSH_PROMISE: {
      TRPC_LOG_TRACE("http2 push promise not supported now");
      break;
    }
    default:
      return 0;
  }
  return 0;
}
}  // namespace

namespace {
int OnFrameRecvCallback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data) {
  TRPC_LOG_TRACE("stream recv frame, stream id: " << frame->hd.stream_id);
  auto client_session = static_cast<ClientSession*>(user_data);
  auto stream = client_session->FindStream(frame->hd.stream_id);
  if (!stream) {
    return 0;
  }

  switch (frame->hd.type) {
    case NGHTTP2_DATA: {
      // Response content ends.
      if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) {
        stream->GetResponse()->OnData(nullptr, 0);
        client_session->OnResponse(stream);
      }
      break;
    }
    case NGHTTP2_HEADERS: {
      if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) {
        // Response ends.
        stream->GetResponse()->OnData(nullptr, 0);
        client_session->OnResponse(stream);
      }
      break;
    }
    case NGHTTP2_PUSH_PROMISE: {
      TRPC_LOG_ERROR("http2 push promise not supported now");
      break;
    }
  }

  return 0;
}
}  // namespace

namespace {
int OnDataChunkRecvCallback(nghttp2_session* session, uint8_t flags, int32_t stream_id, const uint8_t* data, size_t len,
                            void* user_data) {
  TRPC_LOG_TRACE("stream recv data chunk, stream id: " << stream_id);
  auto client_session = static_cast<ClientSession*>(user_data);
  auto stream = client_session->FindStream(stream_id);
  if (!stream) {
    return 0;
  }
  stream->GetResponse()->OnData(data, len);
  return 0;
}
}  // namespace

namespace {
int OnStreamCloseCallback(nghttp2_session* session, int32_t stream_id, uint32_t error_code, void* user_data) {
  auto client_session = static_cast<ClientSession*>(user_data);
  auto stream = client_session->FindStream(stream_id).get();
  if (!stream) {
    return 0;
  }
  stream->GetResponse()->OnClose(error_code);
  client_session->DeleteStream(stream_id);
  return 0;
}
}  // namespace

namespace {
int OnFrameNotSendCallback(nghttp2_session* session, const nghttp2_frame* frame, int error_code, void* user_data) {
  TRPC_LOG_ERROR("frame not send error: " << nghttp2_strerror(error_code));
  return 0;
}
}  // namespace

}  // namespace trpc::http2
