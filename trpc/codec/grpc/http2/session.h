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

#pragma once

#include <any>
#include <deque>
#include <memory>
#include <queue>
#include <unordered_map>
#include <utility>

#include "nghttp2/nghttp2.h"

#include "trpc/codec/grpc/http2/http2.h"
#include "trpc/transport/common/transport_message.h"
#include "trpc/util/buffer/buffer.h"
#include "trpc/util/function.h"

namespace trpc::http2 {

class Request;
using RequestPtr = std::shared_ptr<Request>;

class Response;
using ResponsePtr = std::shared_ptr<Response>;

class Session;
using SessionPtr = std::shared_ptr<Session>;

// @brief HTTP2 stream object.
class Stream {
 public:
  void SetStreamId(uint32_t stream_id) { stream_id_ = stream_id; }
  uint32_t GetStreamId() const { return stream_id_; }

  void SetSession(Session* session) { session_ = session; }

  void SetRequest(const RequestPtr& request) { request_ = request; }
  const RequestPtr& GetRequest() const { return request_; }
  RequestPtr GetMutableRequest() { return std::move(request_); }

  void SetResponse(const ResponsePtr& response) { response_ = response; }
  const ResponsePtr& GetResponse() const { return response_; }
  ResponsePtr GetMutableResponse() { return std::move(response_); }

 private:
  // Stream identifier.
  uint32_t stream_id_{0};
  // Session it belongs to.
  Session* session_{nullptr};
  // Request.
  RequestPtr request_{nullptr};
  // Response.
  ResponsePtr response_{nullptr};
};
using StreamPtr = std::shared_ptr<Stream>;

// @brief HTTP2 Session.
class Session {
  // @brief Response ready callback method (used on the client)
  // @param request HTTP2 response message.
  using OnResponseCallback = Function<void(ResponsePtr&& response)>;

  using SendIoFunction = Function<int(NoncontiguousBuffer&&)>;

  using OnHeaderRecvCallback = Function<void(RequestPtr& request)>;

  using OnDataRecvCallback = Function<void(RequestPtr& request)>;

  using OnEofRecvCallback = Function<void(RequestPtr& request)>;

  using OnRstRecvCallback = Function<void(RequestPtr& request, uint32_t error_code)>;

  using OnWindowUpdateRecvCallback = Function<void(void)>;

 public:
  struct Http2Settings {
    // Maximum concurrent streams.
    uint32_t max_concurrent_streams{100};
    // Initial size of flow control window.
    uint32_t initial_window_size{250000000};
    // Indicates whether to enable PUSH.
    bool enable_push{false};
  };

  struct Options {
    // Callback for Response is ready.
    OnResponseCallback on_response_cb{nullptr};
    // HTTP2 settings.
    Http2Settings http2_settings;
    // Method for sending network messages.
    SendIoFunction send_io_msg{nullptr};
    // Callback for receiving HTTP2 HEADERs. Receiving the first HTTP2 DATA frame indicates that the reception is
    // complete.
    OnHeaderRecvCallback on_header_cb{nullptr};
    // Callback for decoding multiple DATA frames in a single IO.
    OnDataRecvCallback on_data_cb{nullptr};
    // Callback for EOF received.
    OnEofRecvCallback on_eof_cb{nullptr};
    // Callback for RESET received.
    OnRstRecvCallback on_rst_cb{nullptr};
    // Callback for window-updating (Currently only used for connection-level flow control).
    OnWindowUpdateRecvCallback on_window_update_cb{nullptr};
  };

 public:
  explicit Session(Options&& options);
  virtual ~Session();

  // @brief Initializes the session.
  virtual bool Init() = 0;

  // @brief Destroy the session.
  virtual void Destroy() {}

  // @brief Creates a stream.
  virtual StreamPtr CreateStream();

  // @brief Finds a stream by stream ID.
  virtual StreamPtr FindStream(uint32_t stream_id);

  // @brief Adds a new stream.
  virtual void AddStream(StreamPtr&& stream);

  // @brief Deletes a stream by stream ID.
  virtual void DeleteStream(uint32_t stream_id);

  // @brief Parses network message into stream messages.
  virtual int OnMessage(NoncontiguousBuffer* in, std::deque<std::any>* out) { return -1; }

  // @brief Submits a HTTP2 request.
  virtual int SubmitRequest(const RequestPtr& request) { return -1; }

  // @brief Submits a HTTP2 response.
  virtual int SubmitResponse(const ResponsePtr& response) { return -1; }

  // @brief Submits HTTP2 headers.
  virtual int SubmitHeader(uint32_t stream_id, const HeaderPairs& pairs);

  // @brief Submits HTTP2 data (Request or response content).
  virtual int SubmitData(uint32_t stream_id, NoncontiguousBuffer&& data);

  // @brief Submits HTTP2 trailers.
  virtual int SubmitTrailer(uint32_t stream_id, const TrailerPairs& pairs);

  // @brief Submits HTTP2 RESET.
  virtual int SubmitReset(uint32_t stream_id, uint32_t error_code);

  // @brief Notifies reading the frame message, read the message in the in buffer, and parse the received frame message.
  // @param in is streaming message received from the network.
  // @return 0: success; -1: error.
  virtual int SignalRead(NoncontiguousBuffer* in);

  // @brief Notifies writing frame message, write the message to the out buffer, and package the frame message to be
  // sent to the network and place it in the out buffer.
  //
  // @param out Message stream to be sent to the network.
  // @return 0: success; -1: error.
  virtual int SignalWrite(NoncontiguousBuffer* out);

  // @brief Decreases the available window size for connection-level flow control.
  // @param occupy_window_size is the size of the window to be occupied (only actually occupied after submission).
  // @return true if the reduction is successful; false if the window size is insufficient.
  virtual bool DecreaseRemoteWindowSize(int32_t occupy_window_size) { return false; }

  void SetOnResponseCallback(OnResponseCallback&& on_response_cb) {
    options_.on_response_cb = std::move(on_response_cb);
  }

  void SetOnHeaderRecvCallback(OnHeaderRecvCallback&& on_header_cb) { options_.on_header_cb = std::move(on_header_cb); }

  void SetOnDataRecvCallback(OnDataRecvCallback&& on_data_cb) { options_.on_data_cb = std::move(on_data_cb); }

  void SetOnEofRecvCallback(OnEofRecvCallback&& on_eof_cb) { options_.on_eof_cb = std::move(on_eof_cb); }

  void SetOnRstRecvCallback(OnRstRecvCallback&& on_rst_cb) { options_.on_rst_cb = std::move(on_rst_cb); }

  void SetOnWindowUpdateRecvCallback(OnWindowUpdateRecvCallback&& on_window_update_cb) {
    options_.on_window_update_cb = std::move(on_window_update_cb);
  }

 protected:
  // Session options.
  Options options_;
  // HTTP2 session manager implemented using nghttp2, with some constraints; please refer to the following
  // documentation for details.
  // @sa: https://nghttp2.org/documentation/programmers-guide.html
  nghttp2_session* session_{nullptr};
  // Mark processing of client MAGIC message as successful.
  bool client_magic_ok_{false};
  // Streams.
  std::unordered_map<uint32_t, StreamPtr> streams_;
  // Stream data storage queue. The timing of consuming stream messages when submitting them is determined by
  // nghttp2-sdk and may be later than expected. Stream messages cannot be `new`'d and passed to the SDK because they
  // will not be released when the connection closes. Use this queue to prevent memory leaks.
  std::queue<NoncontiguousBuffer> stream_data_queue_;
};
using SessionPtr = std::shared_ptr<Session>;
using SessionOptions = Session::Options;

}  // namespace trpc::http2
