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

#include "trpc/naming/common/util/hash/hash_func.h"

#include <bits/stdint-uintn.h>
#include <string>

#include "trpc/naming/common/util/hash/city.h"
#include "trpc/naming/common/util/hash/md5.h"
#include "trpc/naming/common/util/hash/murmurhash3.h"

namespace trpc {

std::uint64_t Md5Hash(const std::string& input) {
  const uint32_t seed = 131;
  uint64_t hash = Md5Hash_32(input.c_str(), input.size(), seed);
  return hash;
}
std::uint64_t BkdrHash(const std::string& input) {
  const uint64_t seed = 131;
  uint64_t hash = 0;
  for (char ch : input) {
    hash = hash * seed + static_cast<size_t>(ch);
  }
  return hash;
}
std::uint64_t Fnv1aHash(const std::string& input) {
  const uint64_t fnv_prime = 0x811C9DC5;
  size_t hash = 0;
  for (char ch : input) {
    hash ^= static_cast<size_t>(ch);
    hash *= fnv_prime;
  }
  return hash;
}

std::uint64_t MurmurHash3(const std::string& input) {
  uint32_t seed = 42;
  uint64_t hash[2];
  MurmurHash3_x64_128(input.c_str(), input.size(), seed, hash);
  return hash[0];
}

std::uint64_t CityHash(const std::string& input) {
  uint64_t seed = 42;
  return CityHash64WithSeed(input.c_str(), input.size(), seed);
}

std::uint64_t GetHash(const std::string& input, const HashFuncName& hash_func) {
  uint64_t hash = 0;
  switch (hash_func) {
    case HashFuncName::kMd5:
      hash = Md5Hash(input);
      break;
    case HashFuncName::kBkdr:
      hash = BkdrHash(input);
      break;
    case HashFuncName::kFnv1a:
      hash = Fnv1aHash(input);
      break;
    case HashFuncName::kMurmur3:
      hash = MurmurHash3(input);
      break;
    case HashFuncName::kCity:
      hash = CityHash(input);
      break;
    default:
      hash = MurmurHash3(input);
      break;
  }
  return hash;
}

std::uint64_t Hash(const std::string& input, const std::string& hash_func) {
  HashFuncName func =
      kHashFuncTable.find(hash_func) == kHashFuncTable.end() ? HashFuncName::kDefault : kHashFuncTable.at(hash_func);
  return GetHash(input, func);
}

std::uint64_t Hash(const std::string& input, const HashFuncName& hash_func) { return GetHash(input, hash_func); }

std::uint64_t Hash(const std::string& input, const std::string& hash_func, uint64_t num) {
  HashFuncName func =
      kHashFuncTable.find(hash_func) == kHashFuncTable.end() ? HashFuncName::kDefault : kHashFuncTable.at(hash_func);
  return GetHash(input, func) % num;
}

std::uint64_t Hash(const std::string& input, const HashFuncName& hash_func, uint64_t num) {
  return GetHash(input, hash_func) % num;
}

}  // namespace trpc