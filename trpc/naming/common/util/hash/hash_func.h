//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2024 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>

namespace trpc {

static constexpr int kHashNodesMaxIndex = 5;

enum HashFuncName {
  kDefault,
  kMd5,
  kBkdr,
  kFnv1a,
  kMurmur3,
  kCity,
};

static const std::unordered_map<std::string, HashFuncName> kHashFuncTable = {
    {"md5", HashFuncName::kMd5},         {"bkdr", HashFuncName::kBkdr}, {"fnv1a", HashFuncName::kFnv1a},
    {"murmur3", HashFuncName::kMurmur3}, {"city", HashFuncName::kCity},
};

std::uint64_t Md5Hash(const std::string& input);

std::uint64_t BkdrHash(const std::string& input);

std::uint64_t Fnv1aHash(const std::string& input);

std::uint64_t MurmurHash3(const std::string& input);

std::uint64_t CityHash(const std::string& input);

std::uint64_t GetHash(const std::string& input, const std::string& hash_func);

std::uint64_t Hash(const std::string& input, const std::string& hash_func);

std::uint64_t Hash(const std::string& input, const HashFuncName& hash_func);

std::uint64_t Hash(const std::string& input, const std::string& hash_func, uint64_t num);

std::uint64_t Hash(const std::string& input, const HashFuncName& hash_func, uint64_t num);

}  // namespace trpc
