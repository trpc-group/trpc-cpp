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

#include <map>
#include <string>
#include <unordered_map>

namespace trpc {

static constexpr int HASH_NODES_MAX_INDEX = 5;

enum HashFuncName {
  DEFAULT,
  MD5,
  BKDR,
  FNV1A,
  MURMUR3,
  CITY,
};

static const std::unordered_map<std::string, HashFuncName> HashFuncTable = {
    {"md5", HashFuncName::MD5},         {"bkdr", HashFuncName::BKDR}, {"fnv1a", HashFuncName::FNV1A},
    {"murmur3", HashFuncName::MURMUR3}, {"city", HashFuncName::CITY},
};

std::uint64_t MD5Hash(const std::string& input);

std::uint64_t BKDRHash(const std::string& input);

std::uint64_t FNV1aHash(const std::string& input);

std::uint64_t MurMurHash3(const std::string& input);

std::uint64_t CityHash(const std::string& input);

std::uint64_t GetHash(const std::string& input, const std::string& hash_func);

std::uint64_t Hash(const std::string& input, const std::string& hash_func);

std::uint64_t Hash(const std::string& input, const std::string& hash_func, uint64_t num);

}  // namespace trpc