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

#include <memory>
#include <type_traits>
#include <utility>

#include "google/protobuf/message.h"
#include "rapidjson/document.h"

#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/common/status.h"
#include "trpc/stream/stream_provider.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/flatbuffers/message_fbs.h"
#include "trpc/util/log/logging.h"

namespace trpc {

/// @brief Namespace of steam sub-module.
namespace stream {

/// @brief The implementation of synchronous stream reader.
/// @private For internal use purpose only.
template <typename R>
class StreamReaderImpl : public RefCounted<StreamReaderImpl<R>> {
 public:
  StreamReaderImpl(StreamReaderWriterProviderPtr provider, const MessageContentCodecOptions& content_codec)
      : provider_(std::move(provider)), content_codec_(content_codec) {}

  ~StreamReaderImpl() = default;

  /// @brief Reads a message from the stream with optional timeout.
  ///
  /// @param msg is a pointer to the streaming message, which will be updated to the message read.
  /// @param timeout is the timeout for waiting, the function will block until a message is ready
  ///   - -1: wait indefinitely, >=0: wait for timeout in milliseconds.
  /// @return Returns status of the reading operation.
  Status Read(R* msg, int timeout = -1) {
    NoncontiguousBuffer msg_buff;
    auto status = provider_->Read(&msg_buff, timeout);
    if (!status.OK()) {
      return status;
    }
    if (!Deserialize(&msg_buff, static_cast<void*>(msg))) {
      status.SetFrameworkRetCode(provider_->GetDecodeErrorCode());
      status.SetErrorMessage("deserialize message failed");
      return status;
    }
    return status;
  }

  /// @brief Returns the final RPC execution result.
  Status Finish() { return provider_->Finish(); }

  /// @brief Returns the inner status of stream. If you want to known the details of error occurred in the stream,
  /// it will help.
  Status GetStatus() const { return provider_->GetStatus(); }

 private:
  inline bool Deserialize(NoncontiguousBuffer* buffer, void* msg) {
    serialization::DataType type;
    if constexpr (std::is_convertible_v<R*, google::protobuf::MessageLite*>) {
      type = serialization::kPbMessage;
    } else if constexpr (std::is_convertible_v<R*, rapidjson::Document*>) {
      type = serialization::kRapidJson;
    } else if constexpr (std::is_convertible_v<R*, flatbuffers::trpc::MessageFbs*>) {
      type = serialization::kFlatBuffers;
    } else if constexpr (std::is_convertible_v<R*, NoncontiguousBuffer*>) {
      type = serialization::kNonContiguousBufferNoop;
    } else if constexpr (std::is_convertible_v<R*, std::string*>) {
      type = serialization::kStringNoop;
    } else {
      TRPC_FMT_ERROR("unsupported serialization type");
      return false;
    }
    return content_codec_.serialization->Deserialize(buffer, type, msg);
  }

 private:
  StreamReaderWriterProviderPtr provider_;
  MessageContentCodecOptions content_codec_;
};

/// @brief The implementation of synchronous stream writer.
/// @private For internal use purpose only.
template <typename W>
class StreamWriterImpl : public RefCounted<StreamWriterImpl<W>> {
 public:
  StreamWriterImpl(StreamReaderWriterProviderPtr provider, const MessageContentCodecOptions& content_codec)
      : provider_(std::move(provider)), content_codec_(content_codec) {}

  ~StreamWriterImpl() = default;

  /// @brief Writes a message to the stream.
  ///
  /// @param msg is a message to be sent.
  /// @return Return a status of writing operation.
  Status Write(const W& msg) {
    NoncontiguousBuffer msg_buff;
    if (!Serialize(const_cast<void*>(static_cast<const void*>(&msg)), &msg_buff)) {
      Status status;
      status.SetFrameworkRetCode(provider_->GetEncodeErrorCode());
      status.SetErrorMessage("serialize message failed");
      return status;
    }
    return provider_->Write(std::move(msg_buff));
  }

  /// @brief It indicates all messages were written. It will send EOF message to the remote peer.
  Status WriteDone() { return provider_->WriteDone(); }

  /// @brief Finishes the stream, and returns the final RPC execution result.
  Status Finish() { return provider_->Finish(); }

  /// @brief Returns the inner status of stream. If you want to known the details of error occurred in the stream,
  /// it will help.
  Status GetStatus() { return provider_->GetStatus(); }

 private:
  inline bool Serialize(void* msg, NoncontiguousBuffer* buffer) {
    serialization::DataType type;
    if constexpr (std::is_convertible_v<W*, google::protobuf::MessageLite*>) {
      type = serialization::kPbMessage;
    } else if constexpr (std::is_convertible_v<W*, rapidjson::Document*>) {
      type = serialization::kRapidJson;
    } else if constexpr (std::is_convertible_v<W*, flatbuffers::trpc::MessageFbs*>) {
      type = serialization::kFlatBuffers;
    } else if constexpr (std::is_convertible_v<W*, NoncontiguousBuffer*>) {
      type = serialization::kNonContiguousBufferNoop;
    } else if constexpr (std::is_convertible_v<W*, std::string*>) {
      type = serialization::kStringNoop;
    } else {
      TRPC_FMT_ERROR("unsupported serialization type");
      return false;
    }
    return content_codec_.serialization->Serialize(type, msg, buffer);
  }

 private:
  StreamReaderWriterProviderPtr provider_;
  MessageContentCodecOptions content_codec_;
};

template <typename R>
using StreamReaderImplPtr = RefPtr<StreamReaderImpl<R>>;

template <typename W>
using StreamWriterImplPtr = RefPtr<StreamWriterImpl<W>>;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/// @brief The synchronous stream reader. It provides users with streaming programing. We can read messages from
/// the stream.
///
/// \tparam R is the type of message to be read.
template <typename R>
class StreamReader {
 public:
  explicit StreamReader(const StreamReaderImplPtr<R>& reader) : reader_impl_(reader) {}

  ~StreamReader() = default;

  /// @brief Reads a message from the stream with optional timeout, the order in which messages are read is consistent
  /// with the order in which they were sent by the remote peer. A reading should starts before the previous reading
  /// (if any) to complete, i.e. at most one reading operation can be performed at the same time, otherwise it's
  /// undefined behavior. A stream is a full-duplex communication channel, hence reading and writing can be performed
  /// concurrently.
  ///
  /// @param msg is a pointer to the streaming message, which will be updated to the message read successfully from
  /// the stream.
  /// @param timeout is the timeout for waiting, the function will block until a message is ready
  ///   - -1: wait indefinitely, >=0: wait for timeout in milliseconds.
  ///
  /// @return Returns status of the reading operation. An ok status will be returned if good message was read
  /// successfully. If EOF is read, it indicates the end of the stream. If a timeout is reached, it means that the
  /// remote peer did not send any more messages within the timeout period. There may be other errors than occur,
  /// such as network errors.
  Status Read(R* msg, int timeout = -1) const { return reader_impl_->Read(msg, timeout); }

  /// @brief Finishes the stream. For the client, an EOF will be sent to the server automatically, and the final RPC
  /// execution result will be waited. For the server, nothing happens. It's recommended to call this method on the
  /// client side, but it is optional for the server side, because the stream will be closed automatically when user's
  /// function of service returned.
  /// It makes no difference in the final result whether it's called from the "StreamReader", "StreamWriter" or
  /// "StreamReaderWriter" specified stream. For one stream, it should be called at most once at each side, it is also
  /// regarded as a writing operation, hence, it should be called after the previous write or read operation (if any)
  /// to complete, otherwise it's undefined behavior.
  ///
  /// @return Returns the final RPC execution result.
  Status Finish() const { return reader_impl_->Finish(); }

  /// @brief Returns the inner status of stream. If you want to known the details of error occurred in the stream,
  /// it will help.
  Status GetStatus() const { return reader_impl_->GetStatus(); }

 private:
  StreamReaderImplPtr<R> reader_impl_;
};

/// @brief The synchronous stream writer. It provides users with streaming programing. We can write messages to
/// the stream.
///
/// \tparam W is the type of message to be written.
template <typename W>
class StreamWriter {
 public:
  explicit StreamWriter(const StreamWriterImplPtr<W>& writer) : writer_impl_(writer) {}

  ~StreamWriter() = default;

  /// @brief Writes a message to the stream. A writing operation should start before the previous writing (if any)
  /// to complete, i.e. at most one reading operation can be performed at the same time, otherwise it's undefined
  /// behavior. A stream is a full-duplex communication channel, hence reading and writing can be performed
  /// concurrently.
  ///
  /// @param msg is a message to be sent.
  /// @return Return a status of writing operation. An ok status will be returned if no error occurred, but it does NOT
  /// mean the message has already bee sent on the wire. Error status Otherwise.
  Status Write(const W& msg) const { return writer_impl_->Write(msg); }

  /// @brief It indicates all messages were written. It will send EOF message to the remote peer.
  Status WriteDone() const { return writer_impl_->WriteDone(); }

  /// @brief Finishes the stream. For the client, an EOF will be sent to the server automatically, and the final RPC
  /// execution result will be waited. For the server, nothing happens. It's recommended to call this method on the
  /// client side, but it is optional for the server side, because the stream will be closed automatically when user's
  /// function of service returned.
  /// It makes no difference in the final result whether it's called from the "StreamReader", "StreamWriter" or
  /// "StreamReaderWriter" specified stream. For one stream, it should be called at most once at each side, it is also
  /// regarded as a writing operation, hence, it should be called after the previous write or read operation (if any)
  /// to complete, otherwise it's undefined behavior.
  ///
  /// @return Returns the final RPC execution result.
  Status Finish() const { return writer_impl_->Finish(); }

  /// @brief Returns the inner status of stream. If you want to known the details of error occurred in the stream,
  /// it will help.
  Status GetStatus() const { return writer_impl_->GetStatus(); }

 private:
  StreamWriterImplPtr<W> writer_impl_;
};

/// @brief The synchronous stream reader/writer. It provides users with streaming programing.
///
/// \tparam W is the type of message to be written.
/// \tparam R is the type of message to be read.
template <typename W, typename R>
class StreamReaderWriter {
 public:
  StreamReaderWriter(const StreamReaderImplPtr<R>& reader, const StreamWriterImplPtr<W>& writer)
      : reader_impl_(reader), writer_impl_(writer) {}

  ~StreamReaderWriter() = default;

  /// @brief Reads a message from the stream with optional timeout, the order in which messages are read is consistent
  /// with the order in which they were sent by the remote peer. A reading should starts before the previous reading
  /// (if any) to complete, i.e. at most one reading operation can be performed at the same time, otherwise it's
  /// undefined behavior. A stream is a full-duplex communication channel, hence reading and writing can be performed
  /// concurrently.
  ///
  /// @param msg is a pointer to the streaming message, which will be updated to the message read successfully from
  /// the stream.
  /// @param timeout is the timeout for waiting, the function will block until a message is ready
  ///   - -1: wait indefinitely, >=0: wait for timeout in milliseconds.
  ///
  /// @return Returns status of the reading operation. An ok status will be returned if good message was read
  /// successfully. If EOF is read, it indicates the end of the stream. If a timeout is reached, it means that the
  /// remote peer did not send any more messages within the timeout period. There may be other errors than occur,
  /// such as network errors.
  Status Read(R* msg, int timeout = -1) const { return reader_impl_->Read(msg, timeout); }

  /// @brief Writes a message to the stream. A writing operation should start before the previous writing (if any)
  /// to complete, i.e. at most one reading operation can be performed at the same time, otherwise it's undefined
  /// behavior. A stream is a full-duplex communication channel, hence reading and writing can be performed
  /// concurrently.
  ///
  /// @param msg is a message to be sent.
  /// @return Return a status of writing operation. An ok status will be returned if no error occurred, but it does NOT
  /// mean the message has already bee sent on the wire. Error status Otherwise.
  Status Write(const W& msg) const { return writer_impl_->Write(msg); }

  /// @brief It indicates all messages were written. It will send EOF message to the remote peer.
  Status WriteDone() const { return writer_impl_->WriteDone(); }

  /// @brief Finishes the stream. For the client, an EOF will be sent to the server automatically, and the final RPC
  /// execution result will be waited. For the server, nothing happens. It's recommended to call this method on the
  /// client side, but it is optional for the server side, because the stream will be closed automatically when user's
  /// function of service returned.
  /// It makes no difference in the final result whether it's called from the "StreamReader", "StreamWriter" or
  /// "StreamReaderWriter" specified stream. For one stream, it should be called at most once at each side, it is also
  /// regarded as a writing operation, hence, it should be called after the previous write or read operation (if any)
  /// to complete, otherwise it's undefined behavior.
  ///
  /// @return Returns the final RPC execution result.
  Status Finish() const { return writer_impl_->Finish(); }

  /// @brief Returns the inner status of stream. If you want to known the details of error occurred in the stream,
  /// it will help.
  Status GetStatus() const { return writer_impl_->GetStatus(); }

  /// @brief Returns the internal stream reader which provides a limited interfaces.
  StreamReader<R> Reader() const { return StreamReader<R>(reader_impl_); }

  /// @brief Returns the internal stream writer which provides a limited interfaces.
  StreamWriter<W> Writer() const { return StreamWriter<W>(writer_impl_); }

 private:
  StreamReaderImplPtr<R> reader_impl_;
  StreamWriterImplPtr<W> writer_impl_;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/// @brief Creates a stream reader-writer.
/// @private For internal use purpose only.
template <typename W, typename R>
StreamReaderWriter<W, R> Create(const StreamReaderWriterProviderPtr& provider,
                                const MessageContentCodecOptions& content_codec) {
  TRPC_ASSERT(provider != nullptr && "stream reader writer provider is nullptr");
  TRPC_ASSERT(content_codec.serialization != nullptr && "serialization is nullptr");

  auto reader_impl = MakeRefCounted<StreamReaderImpl<R>>(provider, content_codec);
  auto writer_impl = MakeRefCounted<StreamWriterImpl<W>>(provider, content_codec);

  return StreamReaderWriter<W, R>(reader_impl, writer_impl);
}

/// @brief Creates a error stream reader-writer. It is easy for coding.
/// @private For internal use purpose only.
inline StreamReaderWriterProviderPtr CreateErrorStreamProvider(Status status) {
  return MakeRefCounted<ErrorStreamProvider>(std::move(status));
}

}  // namespace stream

}  // namespace trpc
