#pragma once

#include <iostream>
#include <string>
#include <unordered_map>

#include "common/common_codec.h"
#include "yaml-cpp/yaml.h"

namespace trpc::config {

class YamlCodec : public Codec {
 public:
  std::string Name() const override { return "yaml"; }

  bool Decode(const std::string& data, std::unordered_map<std::string, std::any>& out) const override {
    if (data.empty()) {
      TRPC_FMT_WARN("The file is empty! codec: yaml");
      return false;
    }
    try {
      YAML::Node yaml = YAML::Load(data);
      codec::ProcessNode(yaml, "", out);
      return true;
    } catch (const YAML::Exception& e) {
      TRPC_FMT_ERROR("Failed to unmarshal YAML data: {}", e.what());
      return false;
    }
  }
};

using YamlCodecPtr = RefPtr<YamlCodec>;

}  // namespace trpc::config