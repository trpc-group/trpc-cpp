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

#include "trpc/codec/codec_helper.h"

#include "trpc/util/log/logging.h"

namespace trpc::codec {

int GetDefaultServerRetCode(ServerRetCode server_ret_code) {
  auto ret_code = ToUType(server_ret_code);
  if (ret_code < 0 || ret_code >= static_cast<int>(kDefaultServerRetcodes.size())) {
    TRPC_LOG_ERROR(
        "this ret_code is not find in  server general retCode"
        ", please check the setting retcode logic");
    return ret_code;
  }
  return kDefaultServerRetcodes[ret_code];
}

int GetDefaultClientRetCode(ClientRetCode client_ret_code) {
  auto ret_code = ToUType(client_ret_code);
  if (ret_code < 0 || ret_code >= static_cast<int>(kDefaultClientRetcodes.size())) {
    TRPC_LOG_ERROR(
        "this ret_code is not find in  server general retCode"
        ", please check the setting retcode logic");
    return ret_code;
  }
  return kDefaultClientRetcodes[ret_code];
}

}  // namespace trpc::codec
