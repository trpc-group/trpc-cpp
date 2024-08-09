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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL
#pragma once

#include <string>
#include <unordered_map>
#include "trpc/overload_control/smooth_filter/server_overload_controller.h"

#include "trpc/overload_control/common/overload_control_factory.h"

namespace trpc::overload_control {

using ServerOverloadControllerFactory = OverloadControlFactory<ServerOverloadControllerPtr>;

}  // namespace trpc::overload_control
#endif
