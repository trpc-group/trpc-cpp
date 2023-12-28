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

#define TRPC_CPP_MAJOR_VERSION 1
#define TRPC_CPP_MINOR_VERSION 1
#define TRPC_CPP_PATCH_VERSION 0

// (e.g. "-alpha", "-alpha.1", "-beta", "-rc", "-rc.1")
#define TRPC_VERSION_SUFFIX ""

#define TRPC_CPP_STR_HELPER(x) #x
#define TRPC_CPP_STR(x) TRPC_CPP_STR_HELPER(x)

#define TRPC_CPP_VERSION_STRING                                                                    \
  (TRPC_CPP_STR(TRPC_CPP_MAJOR_VERSION) "." TRPC_CPP_STR(TRPC_CPP_MINOR_VERSION) "." TRPC_CPP_STR( \
      TRPC_CPP_PATCH_VERSION) TRPC_VERSION_SUFFIX)

extern "C" {
const char* TRPC_Cpp_Version();
}
