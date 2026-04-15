//trpc/stream/http/http_sse_stream.cc
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

#include "trpc/stream/http/http_sse_stream.h"

#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"
#include "trpc/util/http/util.h"
#include "trpc/codec/http_sse/http_sse_server_codec.h"
#include "trpc/codec/http_sse/http_sse_protocol.h"
#include "trpc/util/http/sse/sse_event.h"       // SseEvent
namespace trpc::stream {

namespace {
Status ContextStatusToStreamStatus(Status&& status, const std::function<void()>& succ_callback) {
  if (TRPC_LIKELY(status.OK())) {
    succ_callback();
  } else if (status.GetFrameworkRetCode() == -1) {
    status = kStreamStatusServerNetworkError;
  } else if (status.GetFrameworkRetCode() == -2) {
    status = kStreamStatusServerWriteTimeout;
  }
  return std::move(status);
}

Connection* GetConnectionFromContext(ServerContext* context) {
  return static_cast<Connection*>(context->GetReserved());
}

}  // namespace

// Simple SSE stream writer for server side.
// Usage:
//   SseStreamWriter writer(context_ptr);
//   writer.WriteHeader();            // optional - WriteEvent/WriteBuffer will auto-call WriteHeader()
//   writer.WriteEvent(sse_event);    // send one SSE event (wrapped as chunked piece)
//   writer.WriteBuffer(buf);         // send pre-serialized bytes (business custom buffer)
//   writer.WriteDone();              // finish chunked response
//   writer.Close();                  // close connection

  // Send SSE response header (Content-Type: text/event-stream, Cache-Control: no-cache, chunked)
  Status SseStreamWriter::WriteHeader() {
    if (state_ & kHeaderWritten) return kSuccStatus;

    NoncontiguousBufferBuilder builder;
    http::Response sse_rsp;
    sse_rsp.SetStatus(http::Response::StatusCode::kOk);
    // use chunked transfer for SSE long polling/streaming
    // use SSE-specific headers 
    sse_rsp.SetMimeType("text/event-stream"); 
    sse_rsp.SetHeader("Cache-Control", "no-cache");
    // Set Connection to keep-alive for streaming
    sse_rsp.SetHeader("Connection", "keep-alive");
  
    // Set Access-Control-Allow-Origin for CORS (if needed)
    sse_rsp.SetHeader("Access-Control-Allow-Origin", "*");
  
    // Set Access-Control-Allow-Headers for CORS
    sse_rsp.SetHeader("Access-Control-Allow-Headers", "Cache-Control");
    sse_rsp.SetHeader(http::kHeaderTransferEncoding, http::kTransferEncodingChunked);

    sse_rsp.SerializeHeaderToString(builder);

    // Send header and set context to streaming mode (SetResponse(false))
    return ContextStatusToStreamStatus(context_->SendResponse(builder.DestructiveGet()), [&]() {
      state_ |= kHeaderWritten;
      context_->SetResponse(false);  // prevent framework from auto-sending a final reply
    });
  }


  Status SseStreamWriter::WriteEvent(const http::sse::SseEvent& ev) {
   // ensure header is written
   if (!(state_ & kHeaderWritten)) {
     Status s = WriteHeader();
     if (!s.OK()) return s;
   }

   // use HttpSseResponseProtocol to serialize the SSE event
   HttpSseResponseProtocol proto;
   proto.SetSseEvent(ev);  // this sets response.content = ev.ToString(), and MIME type

   const std::string& payload = proto.response.GetContent();
 
   // wrap payload as HTTP chunk
   NoncontiguousBufferBuilder builder;
   builder.Append(HttpChunkHeader(payload.size()));
   builder.Append(CreateBufferSlow(payload));
   builder.Append(http::kEndOfChunkMarker);

   // send chunk
   return ContextStatusToStreamStatus(context_->SendResponse(builder.DestructiveGet()), [&]() {
     // success callback if needed
  });
}


  // Directly send a pre-constructed buffer (business may have serialized SSE text already).
  // We will wrap it as a chunked piece.
  Status SseStreamWriter::WriteBuffer(NoncontiguousBuffer&& buf) {
    if (!(state_ & kHeaderWritten)) {
      Status s = WriteHeader();
      if (!s.OK()) return s;
    }

    size_t payload_size = buf.ByteSize();
    NoncontiguousBufferBuilder builder;
    builder.Append(HttpChunkHeader(payload_size));
    builder.Append(std::move(buf));
    builder.Append(http::kEndOfChunkMarker);

    return ContextStatusToStreamStatus(context_->SendResponse(builder.DestructiveGet()), [&]() {});
  }

  // Send chunked end (end-of-chunked-response marker)
  Status SseStreamWriter::WriteDone() {
    if (state_ & kWriteDone) return kSuccStatus;
    return ContextStatusToStreamStatus(context_->SendResponse(CreateBufferSlow(http::kEndOfChunkedResponseMarker)),
                                       [&]() { state_ |= kWriteDone; });
  }

  // Close: best-effort WriteDone then close underlying connection
  void SseStreamWriter::Close() {
    try {
      WriteDone();
    } catch (...) {
    }
    if (context_) {
      context_->CloseConnection();
    }
  }

  // capacity helpers (reuse existing Connection helpers)
  size_t SseStreamWriter::Capacity() const { return GetConnectionFromContext(context_)->GetSendQueueCapacity(); }
  void SseStreamWriter::SetCapacity(size_t capacity) { GetConnectionFromContext(context_)->SetSendQueueCapacity(capacity); }

// --- END: SseStreamWriter ---
}  // namespace trpc::stream
