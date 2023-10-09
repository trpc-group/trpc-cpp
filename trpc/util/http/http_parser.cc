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

#include "trpc/util/http/http_parser.h"

#include "picohttpparser.h"

namespace trpc::http {

namespace {
constexpr int kLessBuffer = -2;
constexpr int kErrBuffer = -1;
}  // namespace

bool HeaderEqual(const phr_header& header, const char* check_name, size_t check_len) {
  return (header.name_len == check_len) && (strncasecmp(header.name, check_name, check_len) == 0);
}

int ParseHead(const char* buf, size_t size, trpc::http::HttpResponse* out) {
  int idx_already_parse{0};
  if (size < http::kMinResponseBytes) {
    return idx_already_parse;
  }

  int minor_version{0};
  int status{200};
  const char* msg{nullptr};
  size_t msg_len;
  struct phr_header headers[http::kMaxHeaderNum];
  size_t num_headers = http::kMaxHeaderNum;

  int nparse = phr_parse_response(buf, size, &minor_version, &status, &msg, &msg_len, headers, &num_headers, 0);
  if (nparse == kErrBuffer) {
    // Bad message.
    return kErrBuffer;
  }

  if (nparse == kLessBuffer) {
    // Incomplete message.
    return idx_already_parse;
  }

  // Parses headers, and set `Content-Length` or `Chunked`.
  bool got_content_length{false};
  bool is_chunked{false};

  for (size_t i = 0; i < num_headers; ++i) {
    if (!is_chunked && HeaderEqual(headers[i], http::kHeaderTransferEncoding, http::kHeaderTransferEncodingLen)) {
      is_chunked = true;
      // Removes `Transfer-Encoding: chunked` if `Chunked` is set.
      continue;
    }
    if (!got_content_length && HeaderEqual(headers[i], http::kHeaderContentLength, http::kHeaderContentLengthLen)) {
      got_content_length = true;
    }
    out->AddHeader(std::string{headers[i].name, headers[i].name_len},
                   std::string{headers[i].value, headers[i].value_len});
  }

  out->SetVersion(minor_version == 0 ? http::kVersion10 : http::kVersion11);
  out->SetStatus(http::HttpResponse::StatusCode(status));
  out->SetReasonPhrase(std::string{msg, msg_len});
  // If it is not chunk encoding and there is no content-length field, explicitly specify it as 0.
  if (!is_chunked) {
    out->SetHeaderIfNotPresent(std::string{http::kHeaderContentLength}, "0");
  }
  return nparse;
}

int ParseHead(const std::string& buf, trpc::http::HttpResponse* out) { return ParseHead(buf.c_str(), buf.size(), out); }

int ParseHead(const char* buf, size_t size, HttpRequest* out) {
  int idx_already_parse{0};
  if (size - idx_already_parse < http::kMinRequestBytes) {
    return idx_already_parse;
  }

  const char* method{nullptr};
  size_t method_len{0};
  const char* path{nullptr};
  size_t path_len{0};
  int minor_version{0};
  struct phr_header headers[http::kMaxHeaderNum];
  size_t num_headers = http::kMaxHeaderNum;

  int nparse =
      phr_parse_request(buf, size, &method, &method_len, &path, &path_len, &minor_version, headers, &num_headers, 0);
  if (nparse == kErrBuffer) {
    // Bad message.
    return kErrBuffer;
  }

  if (nparse == kLessBuffer) {
    // Incomplete message.
    return idx_already_parse;
  }

  bool is_chunked{false};
  bool got_content_length{false};
  bool got_content_type{false};

  for (size_t i = 0; i < num_headers; ++i) {
    if (!is_chunked && HeaderEqual(headers[i], http::kHeaderTransferEncoding, http::kHeaderTransferEncodingLen)) {
      is_chunked = true;
      // Removes `Transfer-Encoding: chunked` if `Chunked` is set.
      continue;
    }

    if (!got_content_length && HeaderEqual(headers[i], http::kHeaderContentLength, http::kHeaderContentLengthLen)) {
      got_content_length = true;
    }

    out->AddHeader(std::string{headers[i].name, headers[i].name_len},
                   std::string{headers[i].value, headers[i].value_len});

    if (!got_content_type && HeaderEqual(headers[i], http::kHeaderContentType, http::kHeaderContentTypeLen)) {
      got_content_type = true;
    }
  }

  if (minor_version == 0) {
    out->SetVersion(std::string{http::kVersion10});
  } else if (minor_version == 1) {
    out->SetVersion(std::string{http::kVersion11});
  }

  // If it is not chunk encoding and there is no content-length field, explicitly specify it as 0.
  if (!is_chunked) {
    out->SetHeaderIfNotPresent(http::kHeaderContentLength, "0");
  }

  out->SetMethodType(StringToType(std::string_view{method, method_len}));
  out->SetUrl(std::string{path, path_len});
  return nparse;
}

int ParseHead(const std::string& buf, HttpRequest* out) { return ParseHead(buf.c_str(), buf.size(), out); }

int Parse(const char* buf, size_t size, trpc::http::HttpResponse* out) {
  int idx_already_parse{0};
  HttpResponse rsp;
  int nparse = ParseHead(buf, size, &rsp);

  if (nparse <= 0) {
    // Bad message or incomplete message.
    return nparse;
  }

  bool is_chunked = !rsp.HasHeader(http::kHeaderContentLength);
  // Use Content-Length to mark the end.
  if (!is_chunked) {
    size_t content_len = std::stoi(rsp.GetHeader(http::kHeaderContentLength));
    if (static_cast<size_t>(nparse + content_len) > size) {
      return idx_already_parse;
    }
    rsp.SetContent(std::string(buf + nparse, content_len));
    idx_already_parse += nparse + content_len;
  } else {
    // For chunk encoding, parse the body in chunks and add Content-Length header.
    struct phr_chunked_decoder decoder {};
    decoder.consume_trailer = 1;
    std::string all_buf(buf + nparse, size - nparse);
    size_t rsize{all_buf.size()};
    // When parsing succeeds, left is the number of bytes in all_buf that have not been parsed.
    ssize_t left = phr_decode_chunked(&decoder, const_cast<char*>(all_buf.c_str()), &rsize);

    if (left == kErrBuffer) {
      return kErrBuffer;
    }

    if (left == kLessBuffer) {
      return idx_already_parse;
    }

    rsp.SetContent(all_buf.substr(0, rsize));
    rsp.SetHeader(http::kHeaderContentLength, std::to_string(rsp.GetContent().size()));
    idx_already_parse += nparse + all_buf.size() - left;
  }

  *out = std::move(rsp);

  return idx_already_parse;
}

int Parse(const char* buf, size_t size, std::deque<HttpResponse>* out) {
  int idx_already_parse{0};
  HttpResponse rsp;
  for (;;) {
    int ret = Parse(buf + idx_already_parse, size - idx_already_parse, &rsp);
    if (ret < 0) {
      return ret;
    } else if (ret == 0) {
      return idx_already_parse;
    } else {
      idx_already_parse += ret;
      out->emplace_back(std::move(rsp));
    }
  }
}

int Parse(const std::string& buf, std::deque<HttpResponse>* out) { return Parse(buf.c_str(), buf.size(), out); }

int Parse(const std::string& buf, HttpResponse* out) { return Parse(buf.c_str(), buf.size(), out); }

int Parse(const char* buf, size_t size, HttpRequest* out) {
  int idx_already_parse{0};
  HttpRequest req;
  int nparse = ParseHead(buf, size, &req);

  if (nparse <= 0) {
    // Bad message or incomplete message.
    return nparse;
  }
  bool is_chunked = !req.HasHeader(http::kHeaderContentLength);
  if (!is_chunked) {
    size_t content_len = std::stoi(req.GetHeader(http::kHeaderContentLength));
    if (static_cast<size_t>(nparse + content_len) > size) {
      return idx_already_parse;
    }
    req.SetContent(std::string(buf + idx_already_parse + nparse, content_len));
    idx_already_parse += nparse + content_len;
  } else {
    struct phr_chunked_decoder decoder {};
    decoder.consume_trailer = 1;
    std::string all_buf(buf + nparse, size - nparse);
    size_t rsize{all_buf.size()};
    // When parsing succeeds, left is the number of bytes in all_buf that have not been parsed.
    int left = phr_decode_chunked(&decoder, const_cast<char*>(all_buf.c_str()), &rsize);

    if (left == kErrBuffer) {
      return kErrBuffer;
    }

    if (left == kLessBuffer) {
      return idx_already_parse;
    }

    req.SetContent(all_buf.substr(0, rsize));
    req.SetHeader(http::kHeaderContentLength, std::to_string(req.GetContent().size()));
    idx_already_parse += nparse + all_buf.size() - left;
  }

  *out = std::move(req);

  return idx_already_parse;
}

int Parse(const char* buf, size_t size, std::deque<HttpRequest>* out) {
  int idx_already_parse{0};
  HttpRequest req;
  for (;;) {
    int ret = Parse(buf + idx_already_parse, size - idx_already_parse, &req);
    if (ret < 0) {
      return ret;
    } else if (ret == 0) {
      return idx_already_parse;
    } else {
      idx_already_parse += ret;
      out->emplace_back(std::move(req));
    }
  }
}

int Parse(const std::string& buf, std::deque<HttpRequest>* out) { return Parse(buf.c_str(), buf.size(), out); }

int Parse(const std::string& buf, HttpRequest* out) { return Parse(buf.c_str(), buf.size(), out); }

}  // namespace trpc::http
