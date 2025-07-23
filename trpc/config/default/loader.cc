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

#include <unordered_map>

#include "trpc/config/default/loader.h"
#include "trpc/util/log/logging.h"

namespace trpc::config {

// provider_instance_map is a hashmap that stores the association
// between the instance_name (key) and the provider instance (value).
std::unordered_map<std::string, ProviderPtr> provider_instance_map;

namespace detail {

DefaultConfigPtr Load(const std::string& path, std::vector<LoadOptions>&& opts) {
  DefaultConfigPtr cfg = MakeRefCounted<DefaultConfig>();
  // Set the codec and provider members of config
  for (const LoadOptions& opt : opts) {
    opt.Apply(cfg);
  }

  // Read raw data from the data provider
  config::ProviderPtr provider = cfg->GetProvider();

  if (!provider) {
    TRPC_FMT_ERROR("The provider is missing and cannot be loaded!");
    return nullptr;
  }

  // Depends on specific data sources for raw data
  std::string raw_data = provider->Read(path);

  // Decode the original data using the codec
  config::CodecPtr codec = cfg->GetCodec();

  if (!codec) {
    // No codec configuration is allowed, but the "decode_data" member is empty
    TRPC_FMT_INFO("The codec is missing, make sure you are not required");
    return cfg;
  }

  // Depends on the specific codec to parse the raw data
  std::unordered_map<std::string, std::any> decode_data;
  if (codec->Decode(raw_data, decode_data)) {
    // Merge the decoded data into the DefaultConfig object
    cfg->SetDecodeData(decode_data);
  } else {
    TRPC_FMT_ERROR("Decode raw data err! codec: {}", codec->Name());
  }

  return cfg;
}

void RegisterProvider(std::string instance_name, ProviderPtr provider) {
  provider_instance_map[std::move(instance_name)] = std::move(provider);
}

ProviderPtr GetProvider(const std::string& instance_name) {
  auto it = provider_instance_map.find(instance_name);
  if (it != provider_instance_map.end()) {
    return it->second;
  }
  return nullptr;
}

}  // namespace detail

LoadOptions WithCodec(const std::string& name) {
  return LoadOptions([name](DefaultConfigPtr& config) {
    config::CodecPtr codec = CodecFactory::GetInstance()->Get(name);
    TRPC_ASSERT(codec != nullptr && "Codec not found!");
    config->SetCodec(codec);
  });
}

LoadOptions WithProvider(const std::string& name) {
  return LoadOptions([name](DefaultConfigPtr& config) {
    config::ProviderPtr provider = ProviderFactory::GetInstance()->Get(name);
    TRPC_ASSERT(provider != nullptr && "Provider not found!");
    config->SetProvider(provider);
  });
}

}  // namespace trpc::config
