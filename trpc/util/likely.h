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

#if __GNUC__
#define TRPC_LIKELY(expr) (__builtin_expect(!!(expr), 1))
#define TRPC_UNLIKELY(expr) (__builtin_expect(!!(expr), 0))
#else
#define TRPC_LIKELY(expr) (expr)
#define TRPC_UNLIKELY(expr) (expr)
#endif
