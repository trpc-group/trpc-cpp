
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

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#pragma once

#include <functional>
#include <string>

#include "trpc/rpcz/filter/client_filter.h"
#include "trpc/rpcz/filter/server_filter.h"
#include "trpc/rpcz/rpcz.h"
#include "trpc/server/server_context.h"

namespace trpc::rpcz {

/// @brief Let user set their own server sample function.
/// @param sample_function Sample function to set.
/// @note Used in server processing scenario.
void SetServerSampleFunction(const CustomerServerRpczSampleFunction& sample_function);

/// @brief Delete user set server sample funtion, fallback to framework sample function.
/// @note Not thread-safe.
void DelServerSampleFunction();

/// @brief Let user set their own client sample function.
/// @param sample_function Sample function to set.
/// @note Used in client processing scenario.
void SetClientSampleFunction(const CustomerClientRpczSampleFunction& sample_function);

/// @brief Delete user set client sample funtion, fallback to framework sample function.
/// @note Not thread-safe.
void DelClientSampleFunction();

}  // namespace trpc::rpcz

/// @brief Used by user inside callback to trace user logic.
/// @param context Mainly server context.
/// @param formats Format to print log.
#define TRPC_RPCZ_PRINT(context, formats, args...)                                    \
  do {                                                                                \
    trpc::rpcz::RpczPrint(context, __FILE__, __LINE__, fmt::format(formats, ##args)); \
  } while (0)

#endif
