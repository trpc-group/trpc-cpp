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

#include "trpc/codec/grpc/http2/server_session.h"

#include <string>
#include <utility>

#include "trpc/codec/grpc/http2/http2.h"
#include "trpc/codec/grpc/http2/request.h"
#include "trpc/codec/grpc/http2/response.h"
#include "trpc/util/deferred.h"
#include "trpc/util/http/util.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc::http2 {

namespace {
// @brief Starts receiving request header callback.
int OnBeginHeadersCallback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data);

// @brief Receives request header. This method will be called for each key-value pair.
int OnHeaderCallback(nghttp2_session* session, const nghttp2_frame* frame, const uint8_t* name, size_t name_len,
                     const uint8_t* value, size_t value_len, uint8_t flags, void* user_data);

// @brief Receives protocol frame callback.
int OnFrameRecvCallback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data);

// @brief Callback for receiving request data blocks.
int OnDataChunkRecvCallback(nghttp2_session* session, uint8_t flags, int32_t stream_id, const uint8_t* data, size_t len,
                            void* user_data);

// @brief Stream close callback.
int OnStreamCloseCallback(nghttp2_session* session, int32_t stream_id, uint32_t error_code, void* user_data);

// @brief Callback for unsent non-data frames due to errors.
int OnFrameNotSendCallback(nghttp2_session* session, const nghttp2_frame* frame, int error_code, void* user_data);
}  // namespace

ServerSession::ServerSession(Session::Options&& options) : Session(std::move(options)) {
  TRPC_ASSERT(Init() && "http2 server session init failed");
}

ServerSession::~ServerSession() {
  for (auto& stream : streams_) {
    auto& response = stream.second->GetResponse();
    if (response) {
      response->OnClose(0);
    }
  }
  streams_.clear();
}

bool ServerSession::Init() { return InitSession(); }

int ServerSession::SignalRead(NoncontiguousBuffer* in) {
  // Try to read preface message received from client.
  if (TRPC_UNLIKELY(!good_client_magic_)) {
    if (!HandleClientMagic(in)) {
      return -1;
    }
    return 0;
  }

  int read_status = Session::SignalRead(in);
  // Updates the remote processing window size after each read.
  // It is not possible to obtain updates with stream_id=0 using nghttp2_session_callbacks_set_on_frame_recv_callback.
  OnWindowUpdate();
  return read_status;
}

bool ServerSession::HandleClientMagic(NoncontiguousBuffer* in) {
  // An empty packet was read, indicating that the remote connection has been closed, and the connection needs to be
  // disconnected.
  if (in->Empty()) {
    return false;
  }

  BufferView client_magic = in->FirstContiguous();
  // The message is not long enough; wait until next time to handle it.
  if (client_magic.size() < NGHTTP2_CLIENT_MAGIC_LEN) {
    return true;
  }

  // Magic does not match.
  if (!http::StringEqualsLiterals(NGHTTP2_CLIENT_MAGIC, client_magic.data(), NGHTTP2_CLIENT_MAGIC_LEN)) {
    return false;
  }

  // Reads magic error.
  if (Session::SignalRead(in) != 0) {
    return false;
  }

  good_client_magic_ = true;
  return true;
}

int ServerSession::SubmitResponse(const ResponsePtr& response) {
  auto stream_id = response->GetStreamId();
  auto stream = FindStream(stream_id);
  if (!stream) {
    TRPC_LOG_ERROR("stream not found, stream id: " << stream_id);
    return -1;
  }

  stream->SetResponse(response);

  // Builds response headers.
  std::vector<nghttp2_nv> nva{};
  nva.reserve(2 + response->GetHeader().Size());
  std::string status = std::to_string(response->GetStatus());
  nva.push_back(nghttp2::CreatePair(kHttp2HeaderStatusName, status));
  std::string date = TimeStringHelper::ConvertEpochToHttpDate(GetNowAsTimeT());
  nva.push_back(nghttp2::CreatePair(kHttp2HeaderDateName, date));
  response->RangeHeader([&nva](std::string_view key, std::string_view value) {
    nva.emplace_back(nghttp2::CreatePair(key, value, true));
    return true;
  });

  // Set response content.
  nghttp2_data_provider content_provider{};
  content_provider.source.ptr = stream.get();
  content_provider.read_callback = [](nghttp2_session* session, int32_t stream_id, uint8_t* buf, size_t length,
                                      uint32_t* data_flags, nghttp2_data_source* source, void* user_data) -> ssize_t {
    auto stream = static_cast<Stream*>(source->ptr);
    ssize_t nread = stream->GetResponse()->ReadContent(buf, length);
    // EOF
    if (nread == 0) {
      *data_flags |= NGHTTP2_DATA_FLAG_EOF;

      // Handles trailers.
      const auto& trailer = stream->GetResponse()->GetTrailer();
      if (!trailer.Empty()) {
        *data_flags |= NGHTTP2_DATA_FLAG_NO_END_STREAM;
        auto server_session = static_cast<ServerSession*>(user_data);
        return server_session->SubmitTrailer(stream_id, trailer);
      }
    }
    return nread;
  };

  // Submits the response.
  int submit_ok = nghttp2_submit_response(session_, stream_id, nva.data(), nva.size(), &content_provider);
  if (submit_ok != 0) {
    TRPC_LOG_ERROR("nghttp2_submit_response failed, error: " << nghttp2_strerror(submit_ok));
    return -1;
  }

  return 0;
}

void ServerSession::OnHeader(RequestPtr& request) {
  if (options_.on_header_cb) {
    options_.on_header_cb(request);
  }
}

void ServerSession::OnData(RequestPtr& request, const uint8_t* data, size_t len) {
  request->OnData(data, len);
  if (options_.on_data_cb) {
    options_.on_data_cb(request);
  }
}

void ServerSession::OnEof(RequestPtr& request) {
  if (options_.on_eof_cb) {
    options_.on_eof_cb(request);
  }
}

void ServerSession::OnRst(RequestPtr& request, uint32_t error_code) {
  if (options_.on_rst_cb) {
    options_.on_rst_cb(request, error_code);
  }
}

void ServerSession::OnWindowUpdate() {
  if (options_.on_window_update_cb) {
    int32_t new_remote_window_size = nghttp2_session_get_remote_window_size(session_);
    TRPC_LOG_TRACE("remote window size, update from origin: " << remote_window_size_
                                                              << " to: " << new_remote_window_size);
    remote_window_size_ = new_remote_window_size;
    options_.on_window_update_cb();
  }
}

bool ServerSession::DecreaseRemoteWindowSize(int32_t occupy_window_size) {
  if (!options_.on_window_update_cb) {
    return false;
  }
  TRPC_LOG_TRACE("remote window size, origin: " << remote_window_size_ << ", decrease: " << occupy_window_size);
  // Insufficient window size.
  if (remote_window_size_ - occupy_window_size < 0) {
    return false;
  }
  remote_window_size_ -= occupy_window_size;
  return true;
}

bool ServerSession::InitSession() {
  nghttp2_session_callbacks* callbacks{nullptr};
  int new_ok = nghttp2_session_callbacks_new(&callbacks);
  if (new_ok != 0) {
    TRPC_LOG_ERROR("new session callbacks failed, error: " << new_ok);
    return false;
  }

  auto callback_defer_deleter = Deferred([callbacks]() { nghttp2_session_callbacks_del(callbacks); });

  // Setups callbacks.
  nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, OnBeginHeadersCallback);
  nghttp2_session_callbacks_set_on_header_callback(callbacks, OnHeaderCallback);
  nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, OnFrameRecvCallback);
  nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, OnDataChunkRecvCallback);
  nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, OnStreamCloseCallback);
  nghttp2_session_callbacks_set_on_frame_not_send_callback(callbacks, OnFrameNotSendCallback);

  new_ok = nghttp2_session_server_new(&session_, callbacks, this);
  if (new_ok != 0) {
    TRPC_LOG_ERROR("create server session failed, error: " << new_ok);
    return false;
  }

  // HTTP2 settings: set the maximum concurrent stream count, default window size, etc.
  if (SubmitSettings() != 0) {
    return false;
  }

  // Get the window size for the remote connection-level flow control.
  remote_window_size_ = nghttp2_session_get_remote_window_size(session_);

  return true;
}

int ServerSession::SubmitSettings() {
  std::vector<nghttp2_settings_entry> settings{
      // The maximum concurrent stream count.
      {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, options_.http2_settings.max_concurrent_streams},
      // Default window size.
      {NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, options_.http2_settings.initial_window_size},
  };

  int set_ok = nghttp2_submit_settings(session_, NGHTTP2_FLAG_NONE, settings.data(), settings.size());
  if (set_ok != 0) {
    TRPC_LOG_ERROR("http2 submit settings failed, error: " << set_ok);
  } else {
    nghttp2_session_set_local_window_size(session_, NGHTTP2_FLAG_NONE, 0, options_.http2_settings.initial_window_size);
  }

  return set_ok;
}

namespace {
int OnBeginHeadersCallback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data) {
  TRPC_LOG_TRACE("stream begin headers, stream id: " << frame->hd.stream_id);
  if (frame->hd.type != NGHTTP2_HEADERS || frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
    return 0;
  }
  auto server_session = static_cast<ServerSession*>(user_data);
  auto request = CreateRequest();
  request->SetStreamId(frame->hd.stream_id);
  auto stream = server_session->CreateStream();
  stream->SetStreamId(frame->hd.stream_id);
  stream->SetSession(server_session);
  stream->SetRequest(request);
  server_session->AddStream(std::move(stream));
  return 0;
}
}  // namespace

namespace {
int OnHeaderCallback(nghttp2_session* session, const nghttp2_frame* frame, const uint8_t* name, size_t name_len,
                     const uint8_t* value, size_t value_len, uint8_t flags, void* user_data) {
  TRPC_LOG_TRACE("stream on header, stream id: " << frame->hd.stream_id);
  if (frame->hd.type != NGHTTP2_HEADERS || frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
    return 0;
  }

  auto server_session = static_cast<ServerSession*>(user_data);
  auto stream_id = frame->hd.stream_id;
  auto stream = server_session->FindStream(stream_id);
  if (!stream) {
    return 0;
  }

  auto& request = stream->GetRequest();
  auto* uri_ref = request->GetMutableUriRef();
  int token = LookupToken(name, name_len);
  switch (token) {
    case HeaderToken::kHeaderColonMethod: {
      request->SetMethod(std::string{value, value + value_len});
      break;
    }
    case HeaderToken::kHeaderColonScheme: {
      uri_ref->MutableScheme()->assign(value, value + value_len);
      break;
    }
    case HeaderToken::kHeaderColonAuthority: {
      uri_ref->MutableHost()->assign(value, value + value_len);
      break;
    }
    case HeaderToken::kHeaderColonPath: {
      SplitPath(value, value + value_len, uri_ref);
      break;
    }
    case HeaderToken::kHeaderColonHost: {
      if (uri_ref->Host().empty()) {
        uri_ref->MutableHost()->assign(value, value + value_len);
      }
      // fall through
    }
    default: {
      request->AddHeader(std::string{name, name + name_len}, std::string{value, value + value_len});
    }
  }

  return 0;
}
}  // namespace

namespace {
int OnFrameRecvCallback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data) {
  TRPC_LOG_TRACE("stream recv frame, stream id: " << frame->hd.stream_id);
  auto server_session = static_cast<ServerSession*>(user_data);
  auto stream = server_session->FindStream(frame->hd.stream_id);
  if (!stream) {
    return 0;
  }

  auto request = stream->GetRequest();
  switch (frame->hd.type) {
    case NGHTTP2_DATA: {
      // Request content ends.
      if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) {
        server_session->OnEof(request);
      }
      break;
    }
    case NGHTTP2_HEADERS: {
      if (frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
        break;
      }
      if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) {
        server_session->OnEof(request);
      }
      break;
    }
    case NGHTTP2_RST_STREAM: {
      server_session->OnRst(request, frame->rst_stream.error_code);
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
  auto server_session = static_cast<ServerSession*>(user_data);
  auto stream = server_session->FindStream(stream_id);
  if (!stream) {
    return 0;
  }

  // Data in the DATA frame has been received, indicating that the HEADER information has been received.
  auto request = stream->GetRequest();
  if (!request->CheckHeaderRecv()) {
    server_session->OnHeader(request);
    request->SetHeaderRecv(true);
  }

  server_session->OnData(request, data, len);

  return 0;
}
}  // namespace

namespace {
int OnStreamCloseCallback(nghttp2_session* session, int32_t stream_id, uint32_t error_code, void* user_data) {
  TRPC_LOG_TRACE("stream close, stream id: " << stream_id);
  auto server_session = static_cast<ServerSession*>(user_data);
  server_session->DeleteStream(stream_id);
  return 0;
}
}  // namespace

namespace {
int OnFrameNotSendCallback(nghttp2_session* session, const nghttp2_frame* frame, int error_code, void* user_data) {
  TRPC_LOG_ERROR("frame send error: " << nghttp2_strerror(error_code));
  return 0;
}
}  // namespace

}  // namespace trpc::http2
