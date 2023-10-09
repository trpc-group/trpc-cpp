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

#include "trpc/client/client_context.h"

#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/util/log/logging.h"

namespace trpc {

ClientContext::ClientContext(const ClientCodecPtr& client_codec) : codec_(client_codec) {
  req_msg_ = client_codec->CreateRequestPtr();
  TRPC_ASSERT(req_msg_ && "create request protocol failed");
}

ClientContext::~ClientContext() {
  if (naming_extend_select_info_ != nullptr) {
    object_pool::Delete<NamingExtendSelectInfo>(naming_extend_select_info_);
  }
}

std::string ClientContext::GetTargetMetadata(const std::string& key) const {
  const auto& metadata = GetTargetMetadata();
  auto iter = metadata.find(key);
  if (iter != metadata.end()) {
    return iter->second;
  }

  return "";
}

bool ClientContext::IsDyeingMessage() const {
  return (GetMessageType() & TrpcMessageType::TRPC_DYEING_MESSAGE) != 0;
}

std::string ClientContext::GetDyeingKey(const std::string& key) const {
  if (req_msg_) {
    const auto& tran_info = GetPbReqTransInfo();
    auto it = tran_info.find(key);
    if (it != tran_info.end()) {
      return it->second;
    }
  } else {
    // ...
  }
  return "";
}

std::string ClientContext::GetDyeingKey() const { return GetDyeingKey(TRPC_DYEING_KEY); }

void ClientContext::SetDyeingKey(const std::string& dyeing_key) { SetDyeingKey(TRPC_DYEING_KEY, dyeing_key); }

void ClientContext::SetDyeingKey(const std::string& key, const std::string& dyeing_key) {
  if (dyeing_key.empty()) {
    return;
  }

  AddReqTransInfo(key, dyeing_key);

  SetMessageType(GetMessageType() | TrpcMessageType::TRPC_DYEING_MESSAGE);
}

}  // namespace trpc
