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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. incubator-brpc
// Copyright (C) 2019 The Apache Software Foundation
// incubator-brpc is licensed under the Apache 2.0 License.
//
//

#pragma once

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/bvar/bvar.h

#include "trpc/tvar/basic_ops/passive_status.h"
#include "trpc/tvar/basic_ops/recorder.h"
#include "trpc/tvar/basic_ops/reducer.h"
#include "trpc/tvar/basic_ops/status.h"
#include "trpc/tvar/common/tvar_group.h"
#include "trpc/tvar/compound_ops/latency_recorder.h"
#include "trpc/tvar/compound_ops/window.h"

// End of source codes that are from incubator-brpc.

// This file is to ease header including for users.
