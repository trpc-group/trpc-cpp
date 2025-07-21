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

#include <chrono>
#include <limits>

#include "trpc/stream/stream_provider.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::stream {

/// @brief Asynchronous stream reader.
///
/// @tparam R Output packet type.
template <typename R>
class AsyncReader final : public RefCounted<AsyncReader<R>> {
 public:
  /// @brief User usually don't need to construct one by themself. When user gets one, it
  /// also implies the stream is started.
  ///
  /// @param provider The stream on which operates.
  /// @param content_codec Information about packet (de)serialization.
  AsyncReader(StreamReaderWriterProviderPtr provider, const MessageContentCodecOptions& content_codec)
      : provider_(std::move(provider)), content_codec_(content_codec) {}

  /// @brief Reads a packet from the stream. A reading should be started after the previous reading(if any) is done,
  /// i.e. at most one pending reading is allowed at a time, and should not be called anymore after the stream EOF read,
  /// otherwise it's undefined behavior.
  ///
  /// @param timeout Reading timeout, never timeout by default.
  /// @return A future which becomes ready while one packet got, with an empty optional value to indicate a stream
  /// EOF. If timeout occurred, future fails with an `Timeout` exception, however the stream operates normally. If any
  /// other error occurred, future fails with an `StreamError` exception  with the stream status carried in most cases,
  /// then the stream stops as well, only `Finish` can be called.
  Future<std::optional<R>> Read(std::chrono::milliseconds timeout = std::chrono::milliseconds::max()) {
    int ms = timeout.count() >= std::numeric_limits<int>::max() ? -1 : timeout.count();
    return provider_->AsyncRead(ms).Then(
        [this, holder = RefPtr(ref_ptr, this)](std::optional<NoncontiguousBuffer>&& buf) {
          if (!buf) {
            return MakeReadyFuture<std::optional<R>>(std::optional<R>());
          }
        // PB Arena is unsupported by streaming RPC.
        // Ref: https://developers.google.com/protocol-buffers/docs/reference/arenas
#ifdef TRPC_PROTO_USE_ARENA
          TRPC_FMT_WARN_ONCE("stream rpc does NOT support PB arena yet.");
#endif

          R r;
          if (!Deserialize(&buf.value(), &r)) {
            Status status(provider_->GetDecodeErrorCode(), 0, "deserialize message failed");
            return MakeExceptionFuture<std::optional<R>>(StreamError(std::move(status)));
          }
          return MakeReadyFuture<std::optional<R>>(std::make_optional(std::move(r)));
        });
  }

  /// @brief Finishes the stream. For client, a EOF will be sent to server automatically, something like WriteDone in
  /// other APIs. For server, nothing happens. It's recommended to call this for client, but optional for server
  /// because server will close the stream automatically after the returned future of user's service function becomes
  /// resolved. It makes no difference whether it's called from the reader, writer or readerwriter for specified stream.
  /// For one stream, it should be called at most once at each side, and it is also regarded as a writing operation,
  /// hence, it should be called after the previous writing(if any) is done, otherwise it's undefined behavior.
  ///
  /// @return A future which becomes ready while the stream is closed. For the current implementation, it never fails
  /// even stream error occurred.
  Future<> Finish() { return provider_->AsyncFinish(); }

  // @brief User should NOT call this.
  AsyncReader() = default;

 private:
  inline bool Deserialize(NoncontiguousBuffer* buffer, void* msg) {
    return content_codec_.serialization->Deserialize(buffer, content_codec_.content_type, msg);
  }

 private:
  StreamReaderWriterProviderPtr provider_;

  MessageContentCodecOptions content_codec_;
};

/// @brief Pointer(RefPtr) to asynchronous stream reader.
///
/// @tparam R Output packet type.
template <typename R>
using AsyncReaderPtr = RefPtr<AsyncReader<R>>;

/// @brief Asynchronous stream writer.
///
/// @tparam W Output packet type.
template <typename W>
class AsyncWriter final : public RefCounted<AsyncWriter<W>> {
 public:
  /// @brief User usually don't need to construct one by themself. When user gets one, it
  /// also implies the stream is started.
  ///
  /// @param provider The stream on which operates.
  /// @param content_codec Information about packet (de)serialization.
  AsyncWriter(StreamReaderWriterProviderPtr provider, const MessageContentCodecOptions& content_codec)
      : provider_(std::move(provider)), content_codec_(content_codec) {}

  /// @brief Writes a packet to the stream. A writing should be started after the previous writing(if any) is done, i.e.
  /// at most one pending writing is allowed at a time, otherwise it's undefined behavior. A stream is a full-duplex
  /// communication channel, hence reading and writing can operate concurrently.
  ///
  /// @param msg Message to be sent.
  /// @return A future which becomes ready while the next writing is able to be started, which does NOT mean the packet
  /// is already sent on the wire. If any error occurred, future fails with an exception which is `StreamError` type
  /// with stream status carried at most cases, but other exception types are possible.
  Future<> Write(const W& msg) {
    NoncontiguousBuffer msg_buff;
    if (!Serialize(const_cast<void*>(static_cast<const void*>(&msg)), &msg_buff)) {
      Status status;
      status.SetFrameworkRetCode(provider_->GetEncodeErrorCode());
      status.SetErrorMessage("serialize message failed");
      return MakeExceptionFuture<>(StreamError(std::move(status)));
    }

    return provider_->AsyncWrite(std::move(msg_buff));
  }

  /// @brief Finishes the stream. For client, a EOF will be sent to server automatically, something like WriteDone in
  /// other APIs. For server, nothing happens. It's recommended to call this for client, but optional for server
  /// because server will close the stream automatically after the returned future of user's service function becomes
  /// resolved. It makes no difference whether it's called from the reader, writer or readerwriter for specified stream.
  /// For one stream, it should be called at most once at each side, and it is also regarded as a writing operation,
  /// hence, it should be called after the previous writing(if any) is done, otherwise it's undefined behavior.
  ///
  /// @return A future which becomes ready while the stream is closed. For the current implementation, it never fails
  /// even stream error occurred.
  Future<> Finish() { return provider_->AsyncFinish(); }

  // @brief User should NOT call this.
  AsyncWriter() = default;

 private:
  inline bool Serialize(void* msg, NoncontiguousBuffer* buffer) {
    return content_codec_.serialization->Serialize(content_codec_.content_type, msg, buffer);
  }

 private:
  StreamReaderWriterProviderPtr provider_;

  MessageContentCodecOptions content_codec_;
};

/// @brief Pointer(RefPtr) to asynchronous stream writer.
///
/// @tparam R Input packet type.
template <typename W>
using AsyncWriterPtr = RefPtr<AsyncWriter<W>>;

/// @brief Asynchronous stream reader/writer.
///
/// @tparam R Input packet type.
/// @tparam W Output packet type.
template <typename R, typename W>
class AsyncReaderWriter final : public RefCounted<AsyncReaderWriter<R, W>> {
 public:
  /// @brief User usually don't need to construct one by themself. When user gets one, it also implies the stream is
  /// started already.
  ///
  /// @param provider The stream on which operates.
  /// @param content_codec Information about packet (de)serialization.
  AsyncReaderWriter(StreamReaderWriterProviderPtr provider, const MessageContentCodecOptions& content_codec)
      : provider_(provider),
        reader_(MakeRefCounted<AsyncReader<R>>(provider, content_codec)),
        writer_(MakeRefCounted<AsyncWriter<W>>(provider, content_codec)) {}

  /// @brief Reads a packet from the stream. A reading should be started after the previous reading(if any) is done, i.e.
  /// at most one pending reading is allowed at a time, and should not be called anymore after the stream EOF read,
  /// otherwise it's undefined behavior.
  ///
  /// @param timeout Reading timeout, never timeout by default.
  /// @return A future which becomes ready while one packet got, with an empty optional value to indicate a stream
  /// EOF. If timeout occurred, future fails with an `Timeout` exception, however the stream operates normally. If any
  /// other error occurred, future fails with an `StreamError` exception  with the stream status carried in most cases,
  /// then the stream stops as well, only `Finish` can be called.
  Future<std::optional<R>> Read(std::chrono::milliseconds timeout = std::chrono::milliseconds::max()) {
    return reader_->Read(timeout);
  }

  /// @brief Writes a packet to the stream. A writing should be started after the previous writing(if any) is done, i.e.
  /// at most one pending writing is allowed at a time, otherwise it's undefined behavior. A stream is a full-duplex
  /// communication channel, hence reading and writing can operate concurrently.
  ///
  /// @param msg Message to be sent.
  /// @return A future which becomes ready while the next writing is able to be started, which does NOT mean the packet
  /// is already sent on the wire. If any error occurred, future fails with an exception which is `StreamError` type
  /// with stream status carried at most cases, but other exception types are possible.
  Future<> Write(const W& msg) { return writer_->Write(msg); }

  /// @brief Finishes the stream. For client, a EOF will be sent to server automatically, something like WriteDone in
  /// other APIs. For server, nothing happens. It's recommended to call this for client, but optional for server
  /// because server will close the stream automatically after the returned future of user's service function becomes
  /// resolved. It makes no difference whether it's called from the reader, writer or readerwriter for specified stream.
  /// For one stream, it should be called at most once at each side, and it is also regarded as a writing operation,
  /// hence, it should be called after the previous writing(if any) is done, otherwise it's undefined behavior.
  ///
  /// @return A future which becomes ready while the stream is closed. For the current implementation, it never fails
  /// even stream error occurred.
  Future<> Finish() { return provider_->AsyncFinish(); }

  /// @brief Gets the reader inside, which provides limited interfaces.
  ///
  /// @return A smart pointer to reader which operates on the same stream.
  AsyncReaderPtr<R> Reader() const { return reader_; }

  /// @brief Gets the writer inside, which provides limited interfaces.
  ///
  /// @return A smart pointer to writer which operates on the same stream.
  AsyncWriterPtr<W> Writer() const { return writer_; }

  // @brief User should NOT call this.
  AsyncReaderWriter() = default;

 private:
  StreamReaderWriterProviderPtr provider_;

  AsyncReaderPtr<R> reader_;

  AsyncWriterPtr<W> writer_;
};

/// @brief Pointer(RefPtr) to asynchronous stream reader/writer.
///
/// @tparam R Input packet type.
/// @rparam W Output packet type.
template <typename R, typename W>
using AsyncReaderWriterPtr = RefPtr<AsyncReaderWriter<R, W>>;

}  // namespace trpc::stream
