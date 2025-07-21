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

#include "trpc/util/http/http_cookie.h"

#include "trpc/util/http/util.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc::http {

HttpCookie::HttpCookie()
    : version_(kVersionNetscape), secure_(false), max_age_(-1), http_only_(false), same_site_(kSameSiteNotSpecified) {}

HttpCookie::HttpCookie(const std::string& name, const std::string& value)
    : version_(kVersionNetscape),
      name_(name),
      value_(value),
      secure_(false),
      max_age_(-1),
      http_only_(false),
      same_site_(kSameSiteNotSpecified) {}

HttpCookie::HttpCookie(const std::unordered_map<std::string, std::string>& name_value_pairs)
    : version_(kVersionNetscape), secure_(false), max_age_(-1), http_only_(false), same_site_(kSameSiteNotSpecified) {
  for (const auto& p : name_value_pairs) {
    SetCookieFieldNameValue(p.first, p.second);
  }
}

std::string HttpCookie::ToString() const {
  std::string result{};
  result.reserve(256);
  if (version_ == kVersionNetscape) {
    ToStringVersionZero(&result);
  } else {
    ToStringVersionOne(&result);
  }
  return result;
}

namespace {
void AppendSameSite(int same_site, std::string* result) {
  switch (same_site) {
    case HttpCookie::kSameSiteNone:
      result->append("; SameSite=None");
      break;
    case HttpCookie::kSameSiteLax:
      result->append("; SameSite=Lax");
    case HttpCookie::kSameSiteStrict:
      result->append("; SameSite=Strict");
      break;
    case HttpCookie::kSameSiteNotSpecified:
      break;
    default:
      break;
  }
}
}  // namespace

void HttpCookie::ToStringVersionZero(std::string* result) const {
  // Netscape cookie
  if (name_.empty() || value_.empty()) {
    return;
  }

  result->append(name_);
  result->append("=");
  result->append(value_);

  if (!domain_.empty()) {
    result->append("; domain=");
    result->append(domain_);
  }

  if (!path_.empty()) {
    result->append("; path=");
    result->append(path_);
  }

  if (max_age_ != -1) {
    auto now_time_t = GetNowAsTimeT();
    now_time_t += static_cast<time_t>(max_age_);
    result->append("; expires=");
    result->append(TimeStringHelper::ConvertEpochToHttpDate(now_time_t));
  }

  AppendSameSite(same_site_, result);

  if (secure_) {
    result->append("; secure");
  }

  if (http_only_) {
    result->append("; HttpOnly");
  }
}

void HttpCookie::ToStringVersionOne(std::string* result) const {
  // RFC 2109 cookie.
  if (name_.empty() || value_.empty()) {
    return;
  }

  result->append(name_);
  result->append("=");
  result->append(R"(")");
  result->append(value_);
  result->append(R"(")");

  if (!comment_.empty()) {
    result->append(R"(; Comment=")");
    result->append(comment_);
    result->append(R"(")");
  }

  if (!domain_.empty()) {
    result->append(R"(; Domain=")");
    result->append(domain_);
    result->append(R"(")");
  }

  if (!path_.empty()) {
    result->append(R"(; Path=")");
    result->append(path_);
    result->append(R"(")");
  }

  if (max_age_ != -1) {
    result->append(R"(; Max-Age=")");
    result->append(std::to_string(max_age_));
    result->append(R"(")");
  }

  AppendSameSite(same_site_, result);

  if (secure_) {
    result->append("; Secure");
  }

  if (http_only_) {
    result->append("; HttpOnly");
  }

  result->append(R"(; Version="1")");
}

namespace {
int ParseSameSiteString(const std::string& value) {
  if (StringEqualsLiteralsIgnoreCase("None", value)) {
    return HttpCookie::kSameSiteNone;
  } else if (StringEqualsLiteralsIgnoreCase("Lax", value)) {
    return HttpCookie::kSameSiteLax;
  } else if (StringEqualsLiteralsIgnoreCase("Strict", value)) {
    return HttpCookie::kSameSiteStrict;
  }

  return HttpCookie::kSameSiteNotSpecified;
}
}  // namespace
void HttpCookie::SetCookieFieldNameValue(const std::string& name, const std::string& value) {
  if (StringEqualsLiteralsIgnoreCase("comment", name)) {
    SetComment(value);
  } else if (StringEqualsLiteralsIgnoreCase("domain", name)) {
    SetDomain(value);
  } else if (StringEqualsLiteralsIgnoreCase("path", name)) {
    SetPath(value);
  } else if (StringEqualsLiteralsIgnoreCase("max-age", name)) {
    TRPC_FMT_TRACE("max-age: {}", value);
    SetMaxAge(std::stoi(value));
  } else if (StringEqualsLiteralsIgnoreCase("secure", name)) {
    SetSecure(true);
  } else if (StringEqualsLiteralsIgnoreCase("expires", name)) {
    time_t expires_time = TimeStringHelper::ConvertHttpDateToEpoch(value);
    if (expires_time != -1) {
      int max_age = static_cast<int>(expires_time - GetSteadyMilliSeconds() / 1000);
      SetMaxAge(max_age);
    }
  } else if (StringEqualsLiteralsIgnoreCase("SameSite", name)) {
    SetSameSite(ParseSameSiteString(value));
  } else if (StringEqualsLiteralsIgnoreCase("Version", name)) {
    TRPC_FMT_TRACE("version: {}", value);
    SetVersion(std::stoi(value));
  } else if (StringEqualsLiteralsIgnoreCase("HttpOnly", name)) {
    SetHttpOnly(true);
  } else {
    SetName(name);
    SetValue(value);
  }
}

std::string HttpCookie::Escape(const std::string& str) { return PercentEncode(str); }

std::string HttpCookie::Unescape(const std::string& str) { return PercentDecode(str); }

}  // namespace trpc::http
