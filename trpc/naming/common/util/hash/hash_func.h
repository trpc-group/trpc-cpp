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

#pragma once

#include <openssl/md5.h>
#include <atomic>
#include <memory>
#include <set>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "trpc/naming/common/util/hash/City.h"
#include "trpc/naming/common/util/hash/MurmurHash3.h"

namespace trpc {
static const std::string MD5HASH = "md5";
static const std::string BKDRHASH = "bkdr";
static const std::string FNV1AHASH = "fnv1a";
static const std::string MURMURHASH3 = "murmur3";
static const std::string CITYHASH = "city";

std::uint64_t MD5Hash(const std::string& input);
std::uint64_t BKDRHash(const std::string& input);
std::uint64_t FNV1aHash(const std::string& input);

std::uint64_t MurMurHash3(const std::string& input) ;
 

std::uint64_t CityHash(const std::string& input);

std::uint64_t GetHash(const std::string& input, const std::string& hash_func);


std::uint64_t Hash(const std::string& input, const std::string& hash_func);

std::uint64_t Hash(const std::string& input, const std::string& hash_func, uint64_t num) ;


}  // namespace trpc