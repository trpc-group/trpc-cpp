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

#include "trpc/config/trpc_conf.h"

#include "trpc/config/codec/json_codec.h"
#include "trpc/config/codec/toml_codec.h"
#include "trpc/config/codec/yaml_codec.h"
#include "trpc/config/config_factory.h"
#include "trpc/config/default/default_config.h"
#include "trpc/config/provider/local_file/local_file_provider.h"

namespace trpc::config {

bool Init() {
  // Register codecs
  CodecFactory::GetInstance()->Register(MakeRefCounted<JsonCodec>());
  CodecFactory::GetInstance()->Register(MakeRefCounted<YamlCodec>());
  CodecFactory::GetInstance()->Register(MakeRefCounted<TomlCodec>());

  // Register local file providers
  std::vector<LocalFileProviderConfig> config_list;
  if (!GetAllLocalFileProviderConfig(config_list)) {
    // No configuration got, skip
    return true;
  }
  for (const auto& config : config_list) {
    ProviderFactory::GetInstance()->Register(MakeRefCounted<LocalFileProvider>(std::move(config)));
  }

  return true;
}

void Stop() {}

void Destroy() {
  ProviderFactory::GetInstance()->Clear();
  CodecFactory::GetInstance()->Clear();
}

}  // namespace trpc::config
