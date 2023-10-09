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

#include "trpc/stream/http/common/stream_handler.h"

#include "trpc/codec/http/http_stream_parser.h"

namespace trpc::stream {

void HttpCommonStreamHandler::Stop() {
  TRPC_FMT_DEBUG("OnCleanup conn_id: {}", options_.connection_id);
  conn_closed_ = true;
  if (stream_) {
    stream_->OnConnectionClosed();
  }
}

bool HttpCommonStreamHandler::IsNewStream(uint32_t stream_id, uint32_t frame_type) { return stream_ == nullptr; }

int HttpCommonStreamHandler::RemoveStream(uint32_t stream_id) {
  if (stream_) {
    stream_ = nullptr;
    parse_state_ = ParseState::kIdle;
    return 0;
  }
  TRPC_FMT_ERROR("stream not found");
  return -1;
}

void HttpCommonStreamHandler::PushMessage(std::any&& message, ProtocolMessageMetadata&& metadata) {
  if (stream_ == nullptr) {
    TRPC_FMT_ERROR("stream not found");
    return;
  }
  stream_->PushRecvMessage(std::any_cast<HttpStreamFramePtr>(message));
}

int HttpCommonStreamHandler::SendMessage(const std::any& msg, NoncontiguousBuffer&& send_data) {
  IoMessage io_msg;
  io_msg.buffer = std::move(send_data);
  io_msg.msg = msg;

  if (int ret = options_.send(std::move(io_msg)); ret != 0) {
    return ret;
  }

  return 0;
}

int HttpCommonStreamHandler::ParseMessage(NoncontiguousBuffer* in, std::deque<std::any>* out) {
  int ret = ParseMessageImpl(in, &cached_out_);
  if (ret == -1) {
    return ret;
  }

  /// If it can get the complete stream frames by once check, we change them to HttpStreamFullMessage to avoid
  /// performance loss caused by too many future-promises
  if (pass_state_idle_ && pass_state_finish_) {
    auto frame = MakeRefCounted<HttpStreamFullMessage>();
    frame->GetMutableFrames()->swap(cached_out_);
    out->push_back(static_pointer_cast<HttpStreamFrame>(frame));
  } else {
    out->swap(cached_out_);
  }
  return ret;
}

int HttpCommonStreamHandler::ParseMessageImpl(NoncontiguousBuffer* in, std::deque<std::any>* out) {
  switch (parse_state_) {
    case ParseState::kIdle: {
      TRPC_ASSERT(false && "Unreachable");
    }
    case ParseState::kAfterParseStartLine: {
      // parses header
      auto frame = MakeRefCounted<HttpStreamHeader>();
      int parse_bytes = ParseHttpHeader(*in, frame->GetMutableHeader(), frame->GetMutableMetaData());
      if (parse_bytes < 0) {
        return parse_bytes;
      }
      is_chunked_ = frame->GetMutableMetaData()->is_chunk;
      has_trailer_ = frame->GetMutableMetaData()->has_trailer;
      expect_bytes_ = frame->GetMutableMetaData()->content_length;
      in->Skip(parse_bytes);
      out->push_back(static_pointer_cast<HttpStreamFrame>(frame));
      // Parsing needs to continue for both chunked mode and non-chunked mode with Content-Length not greater than zero.
      if (is_chunked_ || (!is_chunked_ && expect_bytes_ > 0)) {
        parse_state_ = ParseState::kAfterParseHeader;
        if (in->Empty()) {
          return kParseHttpSucc;
        }
        return HttpCommonStreamHandler::ParseMessageImpl(in, out);
      } else {
        parse_state_ = ParseState::kFinish;
        pass_state_finish_ = true;
        return kParseHttpSucc;
      }
    }
    // fallthru
    case ParseState::kAfterParseHeader: {
      // parses data
      if (is_chunked_) {
        while (true) {
          auto frame = MakeRefCounted<HttpStreamData>();
          int parse_bytes = ParseHttpChunk(in, frame->GetMutableData());
          if (parse_bytes > 0) {
            if (!frame->GetMutableData()->Empty()) {  // there are more chunked blocks to be parsed
              out->push_back(static_pointer_cast<HttpStreamFrame>(frame));
              continue;
            } else {  // reach the end
              out->push_back(static_pointer_cast<HttpStreamFrame>(MakeRefCounted<HttpStreamEof>()));
              parse_state_ = ParseState::kAfterParseData;
              break;
            }
          } else {
            return parse_bytes;
          }
        }
      } else {
        if (expect_bytes_ == 0) {
          // push eof frame
          out->push_back(static_pointer_cast<HttpStreamFrame>(MakeRefCounted<HttpStreamEof>()));
          parse_state_ = ParseState::kFinish;
          pass_state_finish_ = true;
          return kParseHttpSucc;
        }
        if (in->ByteSize() <= expect_bytes_) {
          // There is no data to be checked, return directly.
          if (in->ByteSize() == 0) {
            return kParseHttpNeedMore;
          }
          expect_bytes_ -= in->ByteSize();
          auto frame = MakeRefCounted<HttpStreamData>();
          *frame->GetMutableData() = in->Cut(in->ByteSize());
          out->push_back(static_pointer_cast<HttpStreamFrame>(frame));
          // The body has been fully received, push eof
          if (expect_bytes_ == 0) {
            out->push_back(static_pointer_cast<HttpStreamFrame>(MakeRefCounted<HttpStreamEof>()));
            parse_state_ = ParseState::kFinish;
            pass_state_finish_ = true;
            return kParseHttpSucc;
          }
          return kParseHttpNeedMore;
        } else {
          // The data is too long, there may be more data afterwards.
          // Concurrent packets on the same stream are not supported currently.
          TRPC_FMT_ERROR("http body size exceed Content-Length");
          return kParseHttpError;
        }
      }
    }
    // fallthru
    case ParseState::kAfterParseData: {
      // parses trailer
      TRPC_ASSERT(is_chunked_ && "Must be chunked");
      if (!has_trailer_) {
        if (in->ByteSize() < 2) {
          return kParseHttpNeedMore;
        }
        if (FlattenSlow(*in, 2) != http::kEndOfChunkMarker) {
          return kParseHttpError;
        }
        in->Skip(2);
        parse_state_ = ParseState::kFinish;
        pass_state_finish_ = true;
        return kParseHttpSucc;
      }
      auto frame = MakeRefCounted<HttpStreamTrailer>();
      int parse_bytes = ParseHttpTrailer(*in, frame->GetMutableHeader());
      if (parse_bytes < 0) {
        return parse_bytes;
      }
      if (parse_bytes > 0) {
        out->push_back(static_pointer_cast<HttpStreamFrame>(frame));
      }
      in->Skip(parse_bytes);
      parse_state_ = ParseState::kFinish;
      pass_state_finish_ = true;
      return kParseHttpSucc;
    }
    case ParseState::kFinish: {
      // When the state is kFinish, all the data on the connection has been checked and there will be no new data unless
      // the HTTP interaction on the connection ends.
      TRPC_FMT_ERROR("Unexpected state finish");
      return kParseHttpError;
    }
  }
  TRPC_ASSERT(false && "Unreachable");
}

}  // namespace trpc::stream
