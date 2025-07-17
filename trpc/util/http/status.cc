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

#include "trpc/util/http/status.h"

#include <algorithm>
#include <mutex>
#include <vector>

namespace trpc::http {

namespace {
struct StatusPair {
  int code;
  std::string_view text;
};

constexpr std::string_view kEmptyText{""};
std::vector<std::string_view> kReasonPhrases{};

const std::vector<StatusPair> kStatusPairs{
    // 1xx
    {ResponseStatus::kContinue, "Continue"},
    {ResponseStatus::kSwitchingProtocols, "Switching Protocols"},
    {ResponseStatus::kProcessing, "Processing"},
    {ResponseStatus::kEarlyHints, "Early Hints"},

    // 2xx
    {ResponseStatus::kOk, "OK"},
    {ResponseStatus::kCreated, "Created"},
    {ResponseStatus::kAccepted, "Accepted"},
    {ResponseStatus::kNonAuthoritativeInfo, "Non-Authoritative Information"},
    {ResponseStatus::kNoContent, "No Content"},
    {ResponseStatus::kResetContent, "Reset Content"},
    {ResponseStatus::kPartialContent, "Partial Content"},
    {ResponseStatus::kMultiStatus, "Multi-Status"},
    {ResponseStatus::kAlreadyReported, "Already Reported"},
    {ResponseStatus::kIAmUsed, "I'm Used"},

    // 3xx
    {ResponseStatus::kMultipleChoices, "Multiple Choices"},
    {ResponseStatus::kMovedPermanently, "Moved Permanently"},
    {ResponseStatus::kFound, "Found"},
    {ResponseStatus::kSeeOther, "See Other"},
    {ResponseStatus::kNotModified, "Not Modified"},
    {ResponseStatus::kUseProxy, "Use Proxy"},
    {ResponseStatus::_Unused306, "Unused306"},
    {ResponseStatus::kTemporaryRedirect, "Temporary Redirect"},
    {ResponseStatus::kPermanentRedirect, "Permanent Redirect"},

    // 4xx
    {ResponseStatus::kBadRequest, "Bad Request"},
    {ResponseStatus::kUnauthorized, "Unauthorized"},
    {ResponseStatus::kPaymentRequired, "Payment Required"},
    {ResponseStatus::kForbidden, "Forbidden"},
    {ResponseStatus::kNotFound, "Not Found"},
    {ResponseStatus::kMethodNotAllowed, "Method Not Allowed"},
    {ResponseStatus::kNotAcceptable, "Not Acceptable"},
    {ResponseStatus::kProxyAuthRequired, "Proxy Authentication Required"},
    {ResponseStatus::kRequestTimeout, "Request Timeout"},
    {ResponseStatus::kConflict, "Conflict"},
    {ResponseStatus::kGone, "Gone"},
    {ResponseStatus::kLengthRequired, "Length Required"},
    {ResponseStatus::kPreconditionFailed, "Precondition Failed"},
    {ResponseStatus::kRequestEntityTooLarge, "Request Entity Too Large"},
    {ResponseStatus::kRequestUriTooLong, "Request URI Too Long"},
    {ResponseStatus::kUnsupportedMediaType, "Unsupported Media Type"},
    {ResponseStatus::kRequestedRangeNotSatisfiable, "Requested Range Not Satisfiable"},
    {ResponseStatus::kExpectationFailed, "Expectation Failed"},
    {ResponseStatus::kIAmATeapot, "I'm a teapot"},
    {ResponseStatus::kMisdirectedRequest, "Misdirected Request"},
    {ResponseStatus::kUnprocessableEntity, "Unprocessable Entity"},
    {ResponseStatus::kLocked, "Locked"},
    {ResponseStatus::kFailedDependency, "Failed Dependency"},
    {ResponseStatus::kTooEarly, "Too Early"},
    {ResponseStatus::kUpgradeRequired, "Upgrade Required"},
    {ResponseStatus::kPreconditionRequired, "Precondition Required"},
    {ResponseStatus::kTooManyRequests, "Too Many Requests"},
    {ResponseStatus::kRequestHeaderFieldsTooLarge, "Request Header Fields Too Large"},
    {ResponseStatus::kUnavailableForLegalReasons, "Unavailable For Legal Reasons"},
    {ResponseStatus::kClientClosedRequest, "Client Closed Request"},

    // 5xx
    {ResponseStatus::kInternalServerError, "Internal Server Error"},
    {ResponseStatus::kNotImplemented, "Not Implemented"},
    {ResponseStatus::kBadGateway, "Bad Gateway"},
    {ResponseStatus::kServiceUnavailable, "Service Unavailable"},
    {ResponseStatus::kGatewayTimeout, "Gateway Timeout"},
    {ResponseStatus::kHttpVersionNotSupported, "HTTP Version Not Supported"},
    {ResponseStatus::kVariantAlsoNegotiates, "Variant Also Negotiates"},
    {ResponseStatus::kInsufficientStorage, "Insufficient Storage"},
    {ResponseStatus::kLoopDetected, "Loop Detected"},
    {ResponseStatus::kNotExtended, "Not Extended"},
    {ResponseStatus::kNetworkAuthenticationRequired, "Network Authentication Required"},
};

void InitReasonPhrases() {
  auto reason_phrases_size = []() -> int {
    auto max =
        std::max_element(kStatusPairs.begin(), kStatusPairs.end(),
                         [](const StatusPair& lhs, const StatusPair& rhs) -> bool { return lhs.code < rhs.code; });
    return max->code + 1;
  }();

  kReasonPhrases.reserve(reason_phrases_size);
  for (auto i = 0; i < reason_phrases_size; i++) {
    kReasonPhrases.push_back("");
  }

  for (size_t i = 0; i < kStatusPairs.size(); i++) {
    const auto& status = kStatusPairs[i];
    if (status.code >= 0 && size_t(status.code) < kReasonPhrases.size()) {
      kReasonPhrases[status.code] = status.text;
    }
  }
}
}  // namespace

std::string_view StatusReasonPhrase(int code) {
  // Initialize reason phrases(thread safe).
  static std::once_flag kReasonPhrasesInitOnce;
  std::call_once(kReasonPhrasesInitOnce, InitReasonPhrases);
  if (code >= 0 && size_t(code) < kReasonPhrases.size()) {
    return kReasonPhrases[code];
  }
  return kEmptyText;
}

}  // namespace trpc::http
