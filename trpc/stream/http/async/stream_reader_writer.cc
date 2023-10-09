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

#include "trpc/stream/http/async/stream_reader_writer.h"

#include "trpc/util/http/util.h"
#include "trpc/util/string_helper.h"

namespace trpc::stream {

HttpAsyncStreamReader::HttpAsyncStreamReader(HttpAsyncStreamPtr stream) : stream_(stream) {
  TRPC_ASSERT(stream_ && "HttpAsyncStreamPtr can't be nullptr");
}

Future<http::HttpHeader> HttpAsyncStreamReader::ReadHeader(int timeout) { return stream_->AsyncReadHeader(timeout); }

Future<NoncontiguousBuffer> HttpAsyncStreamReader::ReadChunk(int timeout) { return stream_->AsyncReadChunk(timeout); }

Future<NoncontiguousBuffer> HttpAsyncStreamReader::ReadAtMost(uint64_t len, int timeout) {
  return stream_->AsyncReadAtMost(len, timeout);
}

Future<NoncontiguousBuffer> HttpAsyncStreamReader::ReadExactly(uint64_t len, int timeout) {
  return stream_->AsyncReadExactly(len, timeout);
}

HttpServerAsyncStreamReader::HttpServerAsyncStreamReader(HttpServerAsyncStreamPtr stream)
    : HttpAsyncStreamReader(stream), stream_(stream) {
  TRPC_ASSERT(stream_ && "HttpServerAsyncStreamPtr can't be nullptr");
}

const HttpRequestLine& HttpServerAsyncStreamReader::GetRequestLine() { return stream_->GetRequestLine(); }

HttpAsyncStreamWriter::HttpAsyncStreamWriter(HttpAsyncStreamPtr stream) : stream_(stream) {
  TRPC_ASSERT(stream_ && "HttpAsyncStreamPtr can't be nullptr");
}

Future<> HttpAsyncStreamWriter::WriteHeader(http::HttpHeader&& header, bool is_trailer) {
  if (!is_trailer) {
    auto frame = MakeRefCounted<HttpStreamHeader>();
    *frame->GetMutableHeader() = std::move(header);
    return stream_->PushSendMessage(std::move(frame));
  } else {
    auto frame = MakeRefCounted<HttpStreamTrailer>();
    *frame->GetMutableHeader() = std::move(header);
    return stream_->PushSendMessage(std::move(frame));
  }
}

Future<> HttpAsyncStreamWriter::WriteData(NoncontiguousBuffer&& data) {
  // Writing empty data is not allowed. In chunked mode, empty data represents the end of the chunked data, so WriteDone
  // should be explicitly called to send it.
  if (data.Empty()) {
    Status status{TRPC_STREAM_UNKNOWN_ERR, 0, "Empty data not allowed"};
    TRPC_FMT_DEBUG(status.ErrorMessage());
    return MakeExceptionFuture<>(StreamError(status));
  }
  auto frame = MakeRefCounted<HttpStreamData>();
  *frame->GetMutableData() = std::move(data);
  return stream_->PushSendMessage(std::move(frame));
}

Future<> HttpAsyncStreamWriter::WriteDone() { return stream_->PushSendMessage(MakeRefCounted<HttpStreamEof>()); }

HttpServerAsyncStreamWriter::HttpServerAsyncStreamWriter(HttpServerAsyncStreamPtr stream)
    : HttpAsyncStreamWriter(stream), stream_(stream) {
  TRPC_ASSERT(stream_ && "HttpServerAsyncStreamPtr can't be nullptr");
}

Future<> HttpServerAsyncStreamWriter::WriteStatusLine(HttpStatusLine&& rsp_line) {
  auto frame = MakeRefCounted<HttpStreamStatusLine>();
  *frame->GetMutableHttpStatusLine() = std::move(rsp_line);
  return stream_->PushSendMessage(std::move(frame));
}

HttpAsyncStreamReaderWriter::HttpAsyncStreamReaderWriter(HttpAsyncStreamPtr stream) : stream_(stream) {
  TRPC_ASSERT(stream_ && "HttpAsyncStreamPtr can't be nullptr");
}

Future<http::HttpHeader> HttpAsyncStreamReaderWriter::ReadHeader(int timeout) { return reader_->ReadHeader(timeout); }

Future<NoncontiguousBuffer> HttpAsyncStreamReaderWriter::ReadChunk(int timeout) { return reader_->ReadChunk(timeout); }

Future<NoncontiguousBuffer> HttpAsyncStreamReaderWriter::ReadAtMost(uint64_t len, int timeout) {
  return reader_->ReadAtMost(len, timeout);
}

Future<NoncontiguousBuffer> HttpAsyncStreamReaderWriter::ReadExactly(uint64_t len, int timeout) {
  return reader_->ReadExactly(len, timeout);
}

Future<> HttpAsyncStreamReaderWriter::WriteHeader(http::HttpHeader&& header) {
  return writer_->WriteHeader(std::move(header));
}

Future<> HttpAsyncStreamReaderWriter::WriteData(NoncontiguousBuffer&& data) {
  return writer_->WriteData(std::move(data));
}

Future<> HttpAsyncStreamReaderWriter::WriteDone() { return writer_->WriteDone(); }

void HttpAsyncStreamReaderWriter::SetReaderWriter(HttpAsyncStreamReaderPtr reader, HttpAsyncStreamWriterPtr writer) {
  reader_ = reader;
  writer_ = writer;
}

HttpServerAsyncStreamReaderWriter::HttpServerAsyncStreamReaderWriter(HttpServerAsyncStreamPtr stream)
    : HttpAsyncStreamReaderWriter(stream),
      reader_(MakeRefCounted<HttpServerAsyncStreamReader>(stream)),
      writer_(MakeRefCounted<HttpServerAsyncStreamWriter>(stream)) {
  SetReaderWriter(reader_, writer_);
}

const HttpRequestLine& HttpServerAsyncStreamReaderWriter::GetRequestLine() { return reader_->GetRequestLine(); }

Future<> HttpServerAsyncStreamReaderWriter::WriteStatusLine(HttpStatusLine&& rsp_line) {
  return writer_->WriteStatusLine(std::move(rsp_line));
}

namespace {

template <typename T>
Future<> WriteFullResponseImpl(T rw, http::HttpResponse&& rsp) {
  HttpStatusLine start_line;
  start_line.status_code = rsp.GetStatus();
  start_line.status_text =
      rsp.GetReasonPhrase().empty() ? http::StatusReasonPhrase(start_line.status_code) : rsp.GetReasonPhrase();
  start_line.version = rsp.GetVersion();
  http::HttpHeader header = std::move(*(rsp.GetMutableHeader()));
  NoncontiguousBufferBuilder builder;
  if (rsp.GetNonContiguousBufferContent().Empty()) {
    builder.Append(std::move(*(rsp.GetMutableContent())));
  } else {
    builder.Append(std::move(*(rsp.GetMutableNonContiguousBufferContent())));
  }
  NoncontiguousBuffer data = builder.DestructiveGet();
  return rw
      ->WriteStatusLine(std::move(start_line))                                    // writes status line
      .Then([rw, header = std::move(header), data = std::move(data)]() mutable {  // writes header
        bool has_data = !data.Empty();
        if (has_data) {
          header.SetIfNotPresent("Content-Length", std::to_string(data.ByteSize()));
        } else {
          header.SetIfNotPresent("Content-Length", "0");
        }
        Future<> ft = rw->WriteHeader(std::move(header));
        if (has_data) {  // writes data
          ft = ft.Then([rw, data = std::move(data)]() mutable {
            return rw->WriteData(std::move(data)).Then([rw]() { return rw->WriteDone(); });
          });
        }
        return ft;
      });
}

http::HttpRequestPtr ConstructHttpRequest(HttpRequestLine&& start_line, trpc::http::Parameters&& param,
                                          http::HttpHeader&& header, NoncontiguousBuffer&& data) {
  http::HttpRequestPtr request = std::make_shared<http::HttpRequest>();
  request->SetMethod(start_line.method);
  request->SetVersion(start_line.version);
  request->SetUrl(start_line.request_uri);
  request->SetParameters(std::move(param));
  request->InitQueryParameters();
  *(request->GetMutableHeader()) = std::move(header);
  request->SetNonContiguousBufferContent(std::move(data));
  return request;
}

template <typename T>
Future<http::HttpRequestPtr> ReadFullRequestImpl(T rw, int timeout) {
  HttpRequestLine start_line = rw->GetRequestLine();  // reads request line
  http::PathParameters param = rw->GetParameters();
  // Reads header first, then read the body.
  return rw->ReadHeader(timeout).Then(
      [rw, start_line = std::move(start_line), param = std::move(param)](trpc::http::HttpHeader&& header) mutable {
        // Read full request body if it has any.
        bool has_data = false;
        uint64_t read_bytes = std::numeric_limits<uint64_t>::max();
        if (header.Get(http::kHeaderTransferEncoding) == http::kTransferEncodingChunked) {
          has_data = true;
        }
        if (const std::string& len = header.Get(http::kHeaderContentLength); !len.empty()) {
          has_data = true;
          // The |len| here is valid(a failure will occur if it has bad length).
          read_bytes = trpc::TryParse<uint64_t>(len).value();
        }
        if (has_data) {
          return rw->ReadExactly(read_bytes)
              .Then([start_line = std::move(start_line), param = std::move(param),
                     header = std::move(header)](NoncontiguousBuffer&& data) mutable {
                return MakeReadyFuture<trpc::http::HttpRequestPtr>(
                    ConstructHttpRequest(std::move(start_line), std::move(param), std::move(header), std::move(data)));
              });
        }
        return MakeReadyFuture<trpc::http::HttpRequestPtr>(
            ConstructHttpRequest(std::move(start_line), std::move(param), std::move(header), NoncontiguousBuffer{}));
      });
}

}  // namespace

Future<> WriteFullResponse(HttpServerAsyncStreamReaderWriterPtr rw, http::HttpResponse&& rsp) {
  return WriteFullResponseImpl(rw, std::move(rsp));
}

Future<> WriteFullResponse(HttpServerAsyncStreamWriterPtr rw, http::HttpResponse&& rsp) {
  return WriteFullResponseImpl(rw, std::move(rsp));
}

Future<http::HttpRequestPtr> ReadFullRequest(HttpServerAsyncStreamReaderWriterPtr rw, int timeout) {
  return ReadFullRequestImpl(rw, timeout);
}

Future<http::HttpRequestPtr> ReadFullRequest(HttpServerAsyncStreamReaderPtr rw, int timeout) {
  return ReadFullRequestImpl(rw, timeout);
}

namespace {
template <typename T>
Future<> WriteFullRequestImpl(T rw, trpc::http::HttpRequest&& req) {
  HttpRequestLine start_line;
  start_line.method = req.GetMethod();
  start_line.request_uri = req.GetUrl();
  start_line.version = req.GetVersion();
  trpc::http::HttpHeader header = std::move(*(req.GetMutableHeader()));
  NoncontiguousBufferBuilder builder;
  if (req.GetNonContiguousBufferContent().Empty()) {
    builder.Append(std::move(*(req.GetMutableContent())));
  } else {
    builder.Append(std::move(*(req.GetMutableNonContiguousBufferContent())));
  }
  NoncontiguousBuffer data = builder.DestructiveGet();
  // Sends request line.
  return rw
      ->WriteRequestLine(std::move(start_line))
      // Sends headers.
      .Then([rw, header = std::move(header), data = std::move(data)]() mutable {
        bool has_data = !data.Empty();
        if (has_data) {
          header.SetIfNotPresent("Content-Length", std::to_string(data.ByteSize()));
        } else {
          header.SetIfNotPresent("Content-Length", "0");
        }
        Future<> ft = rw->WriteHeader(std::move(header));
        // Sends content if it has.
        if (has_data) {
          ft = ft.Then([rw, data = std::move(data)]() mutable {
            return rw->WriteData(std::move(data)).Then([rw]() { return rw->WriteDone(); });
          });
        }
        return ft;
      });
}
}  // namespace

Future<> WriteFullRequest(HttpClientAsyncStreamReaderWriterPtr rw, trpc::http::HttpRequest&& req) {
  return WriteFullRequestImpl(rw, std::move(req));
}
Future<> WriteFullRequest(HttpClientAsyncStreamWriterPtr rw, trpc::http::HttpRequest&& req) {
  return WriteFullRequestImpl(rw, std::move(req));
}

namespace {
trpc::http::HttpResponsePtr ConstructHttpResponse(HttpStatusLine&& staus_line, http::HttpHeader&& header,
                                                  NoncontiguousBuffer&& data) {
  trpc::http::HttpResponsePtr response = std::make_shared<trpc::http::HttpResponse>();
  response->SetVersion(staus_line.version);
  response->SetStatus(staus_line.status_code);
  response->SetReasonPhrase(staus_line.status_text);
  *(response->GetMutableHeader()) = std::move(header);
  response->SetNonContiguousBufferContent(std::move(data));
  return response;
}

template <typename T>
Future<trpc::http::HttpResponsePtr> ReadFullResponseImpl(T rw, int timeout) {
  return rw->ReadStatusLine().Then([rw](HttpStatusLine&& start_line) {
    return rw->ReadHeader().Then([rw, start_line = std::move(start_line)](trpc::http::HttpHeader&& header) mutable {
      // Reads content if it has.
      bool has_data = false;
      uint64_t read_bytes = std::numeric_limits<uint64_t>::max();
      if (header.Get(http::kHeaderTransferEncoding) == http::kTransferEncodingChunked) {
        has_data = true;
      }
      if (const std::string& len = header.Get(http::kHeaderContentLength); !len.empty()) {
        has_data = true;
        // `len` is valid here. An invalid value for `len` can cause the stream to fail.
        read_bytes = trpc::TryParse<uint64_t>(len).value();
      }
      if (has_data) {
        return rw->ReadExactly(read_bytes)
            .Then([start_line = std::move(start_line), header = std::move(header)](NoncontiguousBuffer&& data) mutable {
              return MakeReadyFuture<trpc::http::HttpResponsePtr>(
                  ConstructHttpResponse(std::move(start_line), std::move(header), std::move(data)));
            });
      }
      return MakeReadyFuture<trpc::http::HttpResponsePtr>(
          ConstructHttpResponse(std::move(start_line), std::move(header), NoncontiguousBuffer{}));
    });
  });
}
}  // namespace

Future<trpc::http::HttpResponsePtr> ReadFullResponse(HttpClientAsyncStreamReaderWriterPtr rw, int timeout) {
  return ReadFullResponseImpl(rw, timeout);
}
Future<trpc::http::HttpResponsePtr> ReadFullResponse(HttpClientAsyncStreamReaderPtr rw, int timeout) {
  return ReadFullResponseImpl(rw, timeout);
}

}  // namespace trpc::stream
