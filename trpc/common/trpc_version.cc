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

#include "trpc/common/trpc_version.h"

const char TRPC_Cpp_Version_String[] = TRPC_CPP_VERSION_STRING;

extern "C" {
const char* TRPC_Cpp_Version() { return TRPC_Cpp_Version_String; }
}
