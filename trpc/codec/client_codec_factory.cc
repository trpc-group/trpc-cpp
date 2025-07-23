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

#include "trpc/codec/client_codec_factory.h"

namespace trpc {

bool ClientCodecFactory::Register(const ClientCodecPtr& codec) {
  std::string name = codec->Name();
  auto it = codecs_.find(name);
  if (it == codecs_.end()) {
    codecs_[name] = codec;
    return true;
  }

  return false;
}

ClientCodecPtr ClientCodecFactory::Get(const std::string& codec_name) const {
  auto it = codecs_.find(codec_name);
  if (it != codecs_.end()) {
    return it->second;
  }

  return nullptr;
}

void ClientCodecFactory::Clear() { codecs_.clear(); }

}  // namespace trpc
