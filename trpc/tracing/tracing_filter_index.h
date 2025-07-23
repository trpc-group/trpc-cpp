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

#include <any>

namespace trpc {

/// @brief The tracing-related data that server saves in the context for transmission
struct ServerTracingSpan {
  std::any span;  // server tracing data, eg. server span in OpenTracing or OpenTelemetry
};

/// @brief The tracing-related data that client saves in the context for transmission
struct ClientTracingSpan {
  std::any parent_span;  // the parent span, used for contextual information inheritance
  std::any span;         // client tracing data, eg. client span in OpenTracing or OpenTelemetry
};

}  // namespace trpc
