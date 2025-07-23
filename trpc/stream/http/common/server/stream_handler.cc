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

#include "trpc/stream/http/common/server/stream_handler.h"

#include "trpc/codec/http/http_stream_parser.h"

namespace trpc::stream {

int HttpServerCommonStreamHandler::ParseMessageImpl(NoncontiguousBuffer* in, std::deque<std::any>* out) {
  pass_state_idle_ = false;
  pass_state_finish_ = false;
  switch (parse_state_) {
    case ParseState::kIdle: {
      // parse request line
      auto frame = MakeRefCounted<HttpStreamRequestLine>();
      int parse_bytes = ParseHttpRequestLine(*in, frame->GetMutableHttpRequestLine());
      if (parse_bytes < 0) {
        return parse_bytes;
      }
      in->Skip(parse_bytes);
      out->push_back(static_pointer_cast<HttpStreamFrame>(frame));
      // set pass_state_idle_
      pass_state_idle_ = true;
      parse_state_ = ParseState::kAfterParseStartLine;
      if (in->Empty()) {
        return kParseHttpSucc;
      }
    }
    // fallthru
    default:
      return HttpCommonStreamHandler::ParseMessageImpl(in, out);
  }
}

}  // namespace trpc::stream
