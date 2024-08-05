// Copyright (c) 2024, Tencent Inc.
// All rights reserved.

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL
#pragma once

#include <string>
#include <unordered_map>

#include "trpc/overload_control/common/overload_control_factory.h"
#include "server_overload_controller.h"

namespace trpc::overload_control {

using ServerOverloadControllerFactory = OverloadControlFactory<ServerOverloadControllerPtr>;

}  // namespace trpc::overload_control
#endif
