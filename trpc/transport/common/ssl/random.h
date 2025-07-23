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

#pragma once

#include <random>

namespace trpc::ssl {

class Random {
 public:
  static int RandomInt(int begin, int end) {
    static std::random_device rd;
    static std::default_random_engine re(rd());
    std::uniform_int_distribution<int> dist(begin, end);

    return dist(re);
  }
};

}  // namespace trpc::ssl
