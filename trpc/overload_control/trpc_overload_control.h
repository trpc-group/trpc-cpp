// Copyright (c) 2024, Tencent Inc.
// All rights reserved.

#pragma once

namespace trpc::overload_control {

/// @brief Intialize overload plugins
bool Init();

/// @brief Stop inner started thread(maybe) of overload plugins
void Stop();

/// @brief Destroy resource of overload plugins
void Destroy();

}  // namespace trpc::overload_control
