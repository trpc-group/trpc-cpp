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
#include <string>

#include "gmock/gmock.h"

#include "trpc/compressor/compressor.h"

namespace trpc::compressor::testing {

class MockCompressor : public Compressor {
 public:
  MOCK_METHOD(CompressType, Type, (), (const, override));

  MOCK_METHOD(bool, DoCompress, (const NoncontiguousBuffer& in, NoncontiguousBuffer& out, LevelType level), (override));

  MOCK_METHOD(bool, DoDecompress, (const NoncontiguousBuffer& in, NoncontiguousBuffer& out), (override));
};

// @brief Generates random string.
std::string GenRandomStr(int len) {
  static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";

  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_real_distribution<double> dist(0.0, sizeof(alphanum));

  std::string tmp_str;
  tmp_str.reserve(len);

  for (int i = 0; i < len; ++i) {
    tmp_str += alphanum[static_cast<int>(dist(mt))];
  }
  return tmp_str;
}

}  // namespace trpc::compressor::testing
