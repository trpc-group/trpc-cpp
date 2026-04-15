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

#include "trpc/codec/http/http_stream_frame.h"
#include "trpc/stream/stream_handler.h"
#include "trpc/stream/stream_message.h"
#include "trpc/stream/stream_provider.h"
#include "trpc/util/http/sse/sse_event.h"

namespace trpc::stream {

/// @brief The basic class for http stream. It Implements the common logic of http stream, includes the definition of
///        status, as well as the processing for header, body, and trailer frame.
class HttpCommonStream : public StreamReaderWriterProvider {
 public:
  explicit HttpCommonStream(StreamOptions&& options) : options_(std::move(options)) {}

  virtual ~HttpCommonStream();

  Status GetStatus() const override { return status_; }

  StreamOptions* GetMutableStreamOptions() override { return &options_; }

  /// @brief Pushes the received stream protocol frame message.
  virtual void PushRecvMessage(HttpStreamFramePtr&& msg) = 0;

  /// @brief Determines whether the http packet is completely received at one time when reading data.
  /// @note It can be used to optimize the performance of unary scenarios.
  bool HasFullMessage() { return has_full_message_; }

  /// @brief Determines if a stream has been terminated.
  bool IsStateTerminate();

  /// @brief The handling function called when a connection is disconnected.
  /// @note It will push the error of connection disconnected, stop and remove the stream.
  void OnConnectionClosed();

  /// @brief Stops and removes the stream.
  void Stop();

  /// @brief Closes the connection in exceptional cases.
  void Reset(Status status = kUnknownErrorStatus) override { TRPC_ASSERT(false && "Should implement"); }



 protected:
  /// @brief The status of the stream
  /// @note In the kHalfClosed state, only trailers will be processed/sent.
  enum class State { kIdle, kInit, kOpen, kClosed, kHalfClosed };

  /// @brief Types of behaviors for sending and processing frame messages in streams.
  enum class Action {
    /// Sending start/status line
    kSendStartLine,
    /// Sending header
    kSendHeader,
    /// Sending data
    kSendData,
    /// Sending eof
    kSendEof,
    /// Sending trailer
    kSendTrailer,
    /// Handling start/status line
    kHandleStartLine,
    /// Handling header
    kHandleHeader,
    /// Handling data
    kHandleData,
    /// Handing eof
    kHandleEof,
    /// Handling trailer
    kHandleTrailer,
  };

  /// @brief Read and write modes of streams.
  enum class DataMode {
    /// No `content-length` or `transfer-encoding: chunked` in the header
    kNoData,
    /// Chunked
    kChunked,
    /// Fixed length
    kContentLength,
    /// Unknown type
    kUnknown,
  };

 protected:
  /// @brief Gets the read timeout error code
  virtual int GetReadTimeoutErrorCode() { return TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR; }

  /// @brief Gets the write timeout error code
  virtual int GetWriteTimeoutErrorCode() { return TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR; }

  /// @brief Gets the network error code
  virtual int GetNetWorkErrorCode() { return TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR; }

  /// @brief The handling function called when the header arrives.
  virtual void OnHeader(http::HttpHeader* header) = 0;

  /// @brief The handling function called when the data arrives.
  virtual void OnData(NoncontiguousBuffer* data) = 0;

  /// @brief The handling function called when the eof arrives.
  virtual void OnEof() = 0;

  /// @brief The handling function called when an error occurs.
  virtual void OnError(Status status) = 0;

  /// @brief The handling function called when the trailer arrives.
  virtual void OnTrailer(http::HttpHeader* header) { TRPC_FMT_WARN("Not Implmented"); }

  /// @brief Handles the received stream frames.
  /// @param msg the stream frame received
  /// @return handled result
  /// @note It handles the msg types that are common to both requests and responses, and subclasses need to implement
  ///       the handling of their own specific message types, such as kRequestLine or kStatusLine types.
  virtual RetCode HandleRecvMessage(HttpStreamFramePtr&& msg);

  /// @brief Transforms state to desc string
  std::string_view StreamStateToString(State state);

  /// @brief Transforms action to desc string
  std::string_view StreamActionToString(Action action);

  /// @brief Handles the stream frames to be sent.
  /// @param msg the stream frame to send
  /// @param out the final encode msg
  /// @return handled result
  /// @note It handles the msg types that are common to both requests and responses, and subclasses need to implement
  ///       the handling of their own specific message types, such as kRequestLine or kStatusLine types.
  virtual RetCode HandleSendMessage(HttpStreamFramePtr&& msg, NoncontiguousBuffer* out);

  void SetStopped(bool stopped) { is_stop_ = stopped; }

 protected:
  /// @brief The validity of the stream
  Status status_{kDefaultStatus};
  /// @brief The status of the read/write stream
  State read_state_{State::kIdle}, write_state_{State::kIdle};
  /// @brief The modes of read/write stream data
  DataMode read_mode_{DataMode::kUnknown}, write_mode_{DataMode::kUnknown};

  /// @brief The number of bytes that have already been written
  std::size_t write_bytes_{0};
  /// @brief The flag that indicating whether the stream frame obtained by a single check constitute a complete message,
  ///        used to optimize the performance of unary scenarios.
  bool has_full_message_{false};
  /// @brief Whether the recv packet has trailer
  bool read_has_trailer_{false};
  /// @brief The trailer to write
  std::unordered_set<std::string> write_trailers_;
  /// @brief The content length to write, used in kContentLength DataMode
  std::size_t write_content_length_{0};



 private:
  // Handles the header received
  RetCode HandleHeader(http::HttpHeader* header, HttpStreamHeader::MetaData* meta);

  // Handles the data received
  RetCode HandleData(NoncontiguousBuffer* data);

  // Handles eof received
  RetCode HandleEof();

  // Handles trailer received
  RetCode HandleTrailer(http::HttpHeader* trailer);

  // Performs before sending the header.
  RetCode PreSendHeader(http::HttpHeader* header, NoncontiguousBuffer* out);

  // Performs before sending the data.
  RetCode PreSendData(NoncontiguousBuffer* data, NoncontiguousBuffer* out);

  // Performs before sending eof.
  RetCode PreSendEof(NoncontiguousBuffer* out);

  // Performs before sending the trailer.
  RetCode PreSendTrailer(http::HttpHeader* header, NoncontiguousBuffer* out);

  // Checks the state of the stream before each operation.
  bool CheckState(State state, Action action);

 private:
  StreamOptions options_;

  // Whether the stream already stopped
  bool is_stop_{false};
};

using HttpCommonStreamPtr = RefPtr<HttpCommonStream>;

}  // namespace trpc::stream
