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
#include "trpc/naming/common/util/hash/hash_func.h"

namespace trpc{

std::uint64_t MD5Hash(const std::string& input) {
  std::uint8_t hash[MD5_DIGEST_LENGTH];

  MD5(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);

  std::uint64_t res = 0;
  for (int i = 0; i < 8; ++i) {
    res = (res << 8) | hash[i];
  }

  return res;
}
std::uint64_t BKDRHash(const std::string& input) {
  const uint64_t seed = 131;
  uint64_t hash = 0;
  for (char ch : input) {
    hash = hash * seed + static_cast<size_t>(ch);
  }
  return hash;
}
std::uint64_t FNV1aHash(const std::string& input) {
  const uint64_t fnv_prime = 0x811C9DC5;
  size_t hash = 0;
  for (char ch : input) {
    hash ^= static_cast<size_t>(ch);
    hash *= fnv_prime;
  }
  return hash;
}

std::uint64_t MurMurHash3(const std::string& input) {
  uint32_t seed = 42;
  uint64_t hash[2];
  MurmurHash3_x64_128(input.c_str(), input.size(), seed, hash);
  return hash[0];
}

std::uint64_t CityHash(const std::string& input) {
  uint64_t seed = 42;
  return CityHash64WithSeed(input.c_str(), input.size(), seed);
}

std::uint64_t GetHash(const std::string& input, const std::string& hash_func) {
  if (hash_func == MD5HASH) {
    return MD5Hash(input);
  } else if (hash_func == BKDRHASH) {
    return BKDRHash(input);
  } else if (hash_func == CITYHASH) {
    return FNV1aHash(input);
  } else if (hash_func == FNV1AHASH) {
    return FNV1aHash(input);
  } else {
    return MurMurHash3(input);
  }
}

std::uint64_t Hash(const std::string& input, const std::string& hash_func) { return GetHash(input, hash_func); }

std::uint64_t Hash(const std::string& input, const std::string& hash_func, uint64_t num) {
  return GetHash(input, hash_func) % num;
}

} // namespace trpc