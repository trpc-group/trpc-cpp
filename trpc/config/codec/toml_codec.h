#pragma once

#include <iostream>
#include <string>
#include <unordered_map>

#include "common/common_codec.h"
#include "toml.hpp"
#include "toml/exception.hpp"

namespace trpc::config {

class TomlCodec : public Codec {
 public:
  std::string Name() const override { return "toml"; }

  bool Decode(const std::string& data, std::unordered_map<std::string, std::any>& out) const override {
    try {
      std::istringstream data_stream(data);
      auto toml_data = toml::parse(data_stream);
      codec::ProcessNode(toml_data, "", out);
    } catch (const toml::syntax_error& e) {
      TRPC_FMT_ERROR("Error: TOML syntax error: {}", e.what());
      return false;
    }

    return true;
  }
};

using TomlCodecPtr = RefPtr<TomlCodec>;

}  // namespace trpc::config