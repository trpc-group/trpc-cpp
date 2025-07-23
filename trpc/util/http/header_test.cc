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

#include "trpc/util/http/header.h"

#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(Header, VersionSetOrGet) {
  struct testing_args_t {
    std::string version;
    std::string expect;
    int major;
    int minor;
    std::string full_version;
    bool less_than_http1_1;
    bool is_http2;
  };

  std::vector<testing_args_t> testings{
      {"1.1", "1.1", 1, 1, "HTTP/1.1", false, false},
      {"1.0", "1.0", 1, 0, "HTTP/1.0", true, false},
      {"2.0", "2.0", 2, 0, "HTTP/2.0", false, true},
      {"0.9", "0.9", 0, 9, "HTTP/0.9", true, false},
      {"not found", "1.1", 1, 1, "HTTP/1.1", false, false},
  };

  for (const auto& t : testings) {
    trpc::http::Header h;
    h.SetVersion(std::string{t.version});
    ASSERT_EQ(t.expect, h.GetVersion());
    ASSERT_EQ(t.major, h.GetVersionMajor()) << t.version;
    ASSERT_EQ(t.minor, h.GetVersionMinor());
    ASSERT_EQ(t.full_version, h.FullVersion());
    ASSERT_EQ(t.less_than_http1_1, h.LessThanHttp1_1()) << t.version;
    ASSERT_EQ(t.is_http2, h.IsHttp2());
  }
}

}  // namespace trpc::testing
