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

#include "trpc/stream/http/common/client/stream_handler.h"

#include "trpc/codec/http/http_stream_parser.h"

namespace trpc::stream {

int HttpClientCommonStreamHandler::ParseMessageImpl(NoncontiguousBuffer* in, std::deque<std::any>* out) {
  pass_state_idle_ = false;
  pass_state_finish_ = false;
  // Parses response header.
  switch (parse_state_) {
    case ParseState::kIdle: {
      // Status line.
      auto frame = MakeRefCounted<HttpStreamStatusLine>();
      int parse_bytes = ParseHttpStatusLine(*in, frame->GetMutableHttpStatusLine());
      if (parse_bytes < 0) {
        return parse_bytes;
      }
      in->Skip(parse_bytes);
      bool is_100_continue = frame->GetMutableHttpStatusLine()->status_code == 100;
      out->push_back(static_pointer_cast<HttpStreamFrame>(frame));
      pass_state_idle_ = true;
      // 100-continue.
      if (is_100_continue) {
        return ParseMessageImpl(in, out);
      }
      parse_state_ = ParseState::kAfterParseStartLine;
      if (in->Empty()) {
        // OK.
        return 0;
      }
    }
    // fallthrough
    default:
      return HttpCommonStreamHandler::ParseMessageImpl(in, out);
  }
}

}  // namespace trpc::stream
