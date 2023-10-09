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

#ifdef TRPC_PROTO_USE_ARENA

#include "trpc/server/method_handler.h"

#include <utility>

namespace trpc {

google::protobuf::ArenaOptions& GetGlobalArenaOptions() {
  // use default option
  static google::protobuf::ArenaOptions options;
  return options;
}

google::protobuf::Arena GeneratePbArena() { return google::protobuf::Arena(GetGlobalArenaOptions()); }

void SetGlobalArenaOptions(const google::protobuf::ArenaOptions& options) { GetGlobalArenaOptions() = options; }

google::protobuf::Arena* GeneratePbArenaPtr() { return new google::protobuf::Arena(GetGlobalArenaOptions()); }

}  // namespace trpc

#endif
