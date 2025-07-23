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

namespace trpc::ssl {

using u_char = unsigned char;

const int kOk = 0;
const int kError = -1;
const int kAgain = -2;
const int kBusy = -3;
const int kDone = -4;
const int kDeclined = -5;
const int kAbort = -6;
const int kWantRead = -7;
const int kWantWrite = -8;
}  // namespace trpc::ssl
