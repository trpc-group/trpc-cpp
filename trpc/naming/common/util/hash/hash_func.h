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

static constexpr int HASH_NODES_MAX_INDEX=5;

static const std::string MD5 = "md5";
static const std::string BKDR = "bkdr";
static const std::string FNV1A = "fnv1a";
static const std::string MURMUR3 = "murmur3";
static const std::string CITY = "city";

static const std::unordered_map<std::string, int> HashFuncTable = {
    {MD5, 0}, {BKDR, 1}, {FNV1A, 2}, {MURMUR3, 3}, {CITY, 4},
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