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
#include <functional>
#include <memory>
#include <string>

#include "trpc/common/status.h"
#include "trpc/runtime/iomodel/reactor/common/io_message.h"
#include "trpc/stream/stream_message.h"
#include "trpc/stream/stream_provider.h"
#include "trpc/transport/server/server_transport_def.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc::stream {

/// @brief Callback method for stream initialization readiness.
///
/// @param stream_id is the stream identifier.
using OnInitCallback = std::function<void(uint32_t stream_id)>;

/// @brief Callback method for data frame arrival on stream.
///
/// @param data is the received application data.
using OnDataCallback = std::function<void(std::any&& data)>;

/// @brief Callback method for stream closure.
///
/// @param reason indicates what kind of error occurred (if any).
using OnCloseCallback = std::function<void(int reason)>;

/// @brief Callback method for the end of a stream RPC.
using OnFinishCallback = std::function<void(Status status)>;

/// @brief Callback method for stream error.
///
/// @param status is error status.
using OnErrorCallback = std::function<void(Status status)>;

/// @brief Callback method for flow control.
///
/// @param feedback is the flow control related parameters, such as send window.
using OnFeedbackCallback = std::function<void(std::any feedback)>;

/// @brief The method for sending network data.
///
/// @param message contains data to be sent and metadata.
using SendFunction = std::function<int(IoMessage&& message)>;

/// @brief Callbacks of stream.
struct StreamCallbacks {
  OnInitCallback on_init_cb{nullptr};
  OnDataCallback on_data_cb{nullptr};
  OnFinishCallback on_finish_cb{nullptr};
  OnCloseCallback on_close_cb{nullptr};
  OnFeedbackCallback on_feedback_cb{nullptr};
  OnErrorCallback on_error_cb{nullptr};
  HandleStreamingRpcFunction handle_streaming_rpc_cb{nullptr};
};

struct StreamOptions;

/// @brief Handles stream message, specifically responsible for interacting with the transport and managing
/// streams on the connection.
class StreamHandler : public RefCounted<StreamHandler> {
 public:
  virtual ~StreamHandler() = default;

  /// @brief Initializes the stream handler.
  virtual bool Init() { return false; }

  /// @brief Stops the stream coroutine.
  virtual void Stop() {}

  /// @brief Waits for the stream to finish.
  virtual void Join() {}

  /// @brief Creates a stream, which means creating stream entity information, such as stream identifier, setting
  /// stream context, and other configuration options.
  ///
  /// @param options is stream configuration options.
  /// @return Returns nullptr indicates failure to create.
  virtual StreamReaderWriterProviderPtr CreateStream(StreamOptions&& options) = 0;

  /// @brief Removes a stream.
  /// @param stream_id is the stream identifier.
  /// @return Returns the status of removing operation. 0 indicates success, other values indicate errors.
  virtual int RemoveStream(uint32_t stream_id) = 0;

  /// @brief Reports whether a stream is a newly arrived stream (for the receiver, such as the server) or a newly
  /// created stream (for the sender, such as the client).
  virtual bool IsNewStream(uint32_t stream_id, uint32_t frame_type) = 0;

  /// @brief Pushes a stream message to the received message queue.
  /// @param msg is the received stream message.
  /// @param metadata is the stream message metadata.
  virtual void PushMessage(std::any&& msg, ProtocolMessageMetadata&& metadata) = 0;

  /// @brief Send a message to the network.
  /// @param send_data is the binary data to be sent.
  virtual int SendMessage(const std::any& msg, NoncontiguousBuffer&& send_data) = 0;

  /// @brief Parses the network data received after the connection is established, used for scenarios where the
  /// codec is stateful, such as HTTP2 and gRPC.
  ///
  /// @param in is the received network data buffer.
  /// @param out is the parsed result.
  /// @return Returns 0 on success, -1 indicates an error.
  virtual int ParseMessage(NoncontiguousBuffer* in, std::deque<std::any>* out) { return -1; }

  /// @brief Encoding of Unary response/request for streaming protocols at the transport layer, used for
  /// scenarios where the codec is stateful, such as HTTP2 and gRPC.
  virtual int EncodeTransportMessage(IoMessage* msg) { return -1; }

  /// @brief Gets the mutable stream options.
  virtual StreamOptions* GetMutableStreamOptions() { return nullptr; }
};
using StreamHandlerPtr = RefPtr<StreamHandler>;
using StreamHandlerCreator = std::function<StreamHandlerPtr(StreamOptions&& options)>;

/// @brief Stream context.
struct StreamContext {
  // The RPC context, which may contain different content in the client or server side.
  std::any context{nullptr};
};

/// @brief Stream options, including stream context, stream callbacks, steam handler, stream identifier.
struct StreamOptions {
  StreamContext context{};
  StreamCallbacks callbacks{};
  StreamHandlerPtr stream_handler{nullptr};
  uint64_t connection_id{0};
  // stream_id is the stream identifier.
  uint32_t stream_id{0};
  // Read stream message timout (ms).
  // Fixme: Gets timeout option from configure file.
  int stream_read_timeout{3000};
  // Indicates whether a stream was created on server side.
  bool server_mode{false};
  // Indicates whether thread model is 'Fiber'.
  bool fiber_mode{false};
  // Streaming flow control window size, in bytes. The default value is 65535. A value of 0 means that flow control
  // is disabled (currently effective for tRPC streaming), and represents the receiving window size on the local end.
  uint32_t stream_max_window_size = 65535;
  // Pointer to the RPC response message, which will be updated with the response from the server in the future.
  // This is mainly used in the Client-Stream scenario.
  void* rpc_reply_msg{nullptr};
  SendFunction send{nullptr};
  // This is used for some streaming protocols to determine whether it is a Unary or a stream frame during the
  // check (such as gRPC).
  CheckStreamRpcFunction check_stream_rpc_function{nullptr};
};

}  // namespace trpc::stream
