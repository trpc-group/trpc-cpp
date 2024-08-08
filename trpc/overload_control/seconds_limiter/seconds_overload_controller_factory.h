// Copyright (c) 2024, Tencent Inc.
// All rights reserved.

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL
#pragma once

#include "trpc/overload_control/common/overload_control_factory.h"
#include "trpc/overload_control/seconds_limiter/seconds_overload_controller.h"

namespace trpc::overload_control {

using SecondsOverloadControllerFactory = OverloadControlFactory<SecondsOverloadControllerPtr>;

}  // namespace trpc::overload_control
#endif
