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

#include "trpc/codec/grpc/http2/session.h"

namespace trpc::http2 {

Session::Session(Options&& options) : options_(std::move(options)), session_(nullptr) {}

Session::~Session() {
  if (session_) {
    nghttp2_session_del(session_);
    session_ = nullptr;
  }
}

StreamPtr Session::CreateStream() { return std::make_shared<Stream>(); }

StreamPtr Session::FindStream(uint32_t stream_id) {
  auto found = streams_.find(stream_id);
  if (found != streams_.end()) {
    return found->second;
  }
  return nullptr;
}

void Session::AddStream(StreamPtr&& stream) { streams_.emplace(stream->GetStreamId(), std::move(stream)); }

void Session::DeleteStream(uint32_t stream_id) {
  auto found = streams_.find(stream_id);
  if (found != streams_.end()) {
    streams_.erase(found);
  }
}

int Session::SignalRead(NoncontiguousBuffer* in) {
  while (!in->Empty()) {
    auto first_buffer = in->FirstContiguous();
    auto data = reinterpret_cast<uint8_t*>(const_cast<char*>(first_buffer.data()));
    auto size = first_buffer.size();
    ssize_t pos = 0;

    while (size > 0) {
      ssize_t nread = nghttp2_session_mem_recv(session_, data + pos, size);
      if (nread > 0) {
        in->Skip(nread);
        pos += nread;
        size -= nread;
        continue;
      }
      // Error occurred.
      TRPC_LOG_ERROR("nghttp2_session_mem_recv() failed, error: " << nread << ", " << nghttp2_strerror(nread));
      return -1;
    }
  }
  return 0;
}

int Session::SignalWrite(NoncontiguousBuffer* out) {
  NoncontiguousBufferBuilder buffer_builder;
  for (;;) {
    const uint8_t* buffer{nullptr};
    ssize_t nwrite = nghttp2_session_mem_send(session_, &buffer);

    // Error occurred.
    if (nwrite < 0) {
      TRPC_LOG_ERROR("nghttp2_session_mem_send() failed, error: " << nghttp2_strerror(nwrite));
      return -1;
    }
    // EOF.
    if (nwrite == 0) {
      break;
    }

    buffer_builder.Append(buffer, nwrite);
  }
  *out = buffer_builder.DestructiveGet();
  return 0;
}

int Session::SubmitHeader(uint32_t stream_id, const HeaderPairs& pairs) {
  auto stream = FindStream(stream_id);
  if (!stream) {
    TRPC_LOG_ERROR("stream not found, stream id: " << stream_id);
    return -1;
  }

  std::vector<nghttp2_nv> nva{};
  nva.reserve(pairs.Size());
  pairs.Range([&nva](std::string_view key, std::string_view value) {
    nva.push_back(nghttp2::CreatePair(key, value, true));
    return true;
  });

  int submit_ok =
      nghttp2_submit_headers(session_, NGHTTP2_FLAG_NONE, stream_id, nullptr, nva.data(), nva.size(), nullptr);
  if (submit_ok != 0) {
    TRPC_LOG_ERROR("nghttp2_submit_headers failed, error: " << nghttp2_strerror(submit_ok));
    return -1;
  }
  return 0;
}

int Session::SubmitData(uint32_t stream_id, NoncontiguousBuffer&& data) {
  auto stream = FindStream(stream_id);
  if (!stream) {
    TRPC_LOG_ERROR("stream not found, stream id: " << stream_id);
    return -1;
  }

  stream_data_queue_.push(std::move(data));

  nghttp2_data_provider content_provider{};
  content_provider.source.ptr = &stream_data_queue_;
  content_provider.read_callback = [](nghttp2_session* session, int32_t stream_id, uint8_t* buf, size_t length,
                                      uint32_t* data_flags, nghttp2_data_source* source, void* user_data) -> ssize_t {
    auto stream_data_queue_ = static_cast<std::queue<NoncontiguousBuffer>*>(source->ptr);

    // The queue content is empty, and encoding is not required.
    if (stream_data_queue_->empty()) {
      *data_flags |= NGHTTP2_DATA_FLAG_EOF | NGHTTP2_DATA_FLAG_NO_END_STREAM;
      return 0;
    }

    // There are messages in the queue, process the front message until it is encoded.
    auto& content_buffer = stream_data_queue_->front();
    auto content_buffer_size = content_buffer.ByteSize();
    // The message content is empty and encoding is not required. Pop the first element from the queue.
    if (content_buffer_size == 0) {
      *data_flags |= NGHTTP2_DATA_FLAG_EOF | NGHTTP2_DATA_FLAG_NO_END_STREAM;
      stream_data_queue_->pop();
      return 0;
    }

    size_t expect_bytes = std::min(length, content_buffer_size);
    // Attempt to read the first block.
    if (content_buffer.FirstContiguous().size() >= expect_bytes) {
      memcpy(buf, content_buffer.FirstContiguous().data(), expect_bytes);
    } else {
      FlattenToSlow(content_buffer, static_cast<void*>(buf), expect_bytes);
    }

    content_buffer.Skip(expect_bytes);
    return expect_bytes;
  };

  int submit_ok = nghttp2_submit_data(session_, NGHTTP2_FLAG_NONE, stream_id, &content_provider);
  if (submit_ok != 0) {
    TRPC_LOG_ERROR("nghttp2_submit_data failed, stream id: " << stream_id << ",error: " << nghttp2_strerror(submit_ok));
    return -1;
  }
  return 0;
}

int Session::SubmitTrailer(uint32_t stream_id, const TrailerPairs& pairs) {
  auto stream = FindStream(stream_id);
  if (!stream) {
    TRPC_LOG_ERROR("stream not found, stream id: " << stream_id);
    return -1;
  }

  std::vector<nghttp2_nv> nva{};
  nva.reserve(pairs.Size());
  pairs.Range([&nva](std::string_view key, std::string_view value) {
    nva.push_back(nghttp2::CreatePair(key, value, true));
    return true;
  });

  int submit_ok = nghttp2_submit_trailer(session_, stream_id, nva.data(), nva.size());
  if (submit_ok != 0) {
    TRPC_LOG_ERROR("nghttp2_submit_trailer failed, error: " << nghttp2_strerror(submit_ok));
    return -1;
  }
  return 0;
}

int Session::SubmitReset(uint32_t stream_id, uint32_t error_code) {
  auto stream = FindStream(stream_id);
  if (!stream) {
    TRPC_LOG_ERROR("stream not found, stream id: " << stream_id);
    return -1;
  }

  int submit_ok = nghttp2_submit_rst_stream(session_, NGHTTP2_FLAG_NONE, stream_id, error_code);
  if (submit_ok != 0) {
    TRPC_LOG_ERROR("nghttp2_submit_rst_stream failed, error: " << nghttp2_strerror(submit_ok));
    return -1;
  }
  return 0;
}

}  // namespace trpc::http2
