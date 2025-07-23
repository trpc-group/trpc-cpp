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

#include "trpc/tracing/tracing.h"

namespace trpc::tracing {

/// @brief Initialize tracing plugins
/// @return Returns 0 on success
bool Init();

/// @brief Stop tracing plugins, e.g. stop the threads running inside the plugins.
void Stop();

/// @brief Destroy all tracing plugins
void Destroy();

};  // namespace trpc::tracing
