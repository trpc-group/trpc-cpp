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

#include <string>
#include <vector>

namespace trpc::util {

enum AddrType { ADDR_IP, ADDR_DOMAIN, ADDR_NX };

/// @brief Get the type of addr
AddrType GetAddrType(const std::string& addr);

/// @brief Get addrs of the domain
void GetAddrFromDomain(const std::string& domain, std::vector<std::string>& addrs);

}  // namespace trpc::util
