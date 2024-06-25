// Copyright (c) 2024, Tencent Inc.
// All rights reserved.

#include "trpc/overload_control/trpc_overload_control.h"

#include "trpc/overload_control/server_overload_controller_factory.h"

namespace trpc::overload_control {

bool Init() {
  // Register plugins here
  return true;
}

void Stop() {
  // Stop plugins here
  ServerOverloadControllerFactory::GetInstance()->Stop();
}

void Destroy() {
  // Destroy plugins here
  ServerOverloadControllerFactory::GetInstance()->Destroy();
}

}  // namespace trpc::overload_control
