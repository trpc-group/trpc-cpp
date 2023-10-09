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

#include "trpc/telemetry/telemetry.h"

namespace trpc::telemetry {

/// @brief Initializes telemetry plugins
/// @return Returns 0 on success
bool Init();

/// @brief Stops telemetry plugins, e.g. stop the threads running inside the plugins.
void Stop();

/// @brief Destroys all telemetry plugins
void Destroy();

};  // namespace trpc::telemetry
