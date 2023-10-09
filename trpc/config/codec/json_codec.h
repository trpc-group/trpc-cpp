#pragma once

#include <iostream>
#include <string>
#include <unordered_map>

#include "common/common_codec.h"
#include "json/json.h"

#include "trpc/util/log/logging.h"

namespace trpc::config {

/// @brief JSON codec implementation.
/// This class provides an implementation of the JSON which be to encode decode data in JSON format.
class JsonCodec : public Codec {
 public:
  /// @brief Get the name of the JSON codec.
  /// @return The name of the JSON codec.
  std::string Name() const override { return "json"; }

  /// @brief Decode the given JSON data and store the result in the given map.
  /// @param data The JSON data to decode.
  /// @param out The map to store the decoded result.
  /// @return True if the decoding is successful, false otherwise.
  bool Decode(const std::string& data, std::unordered_map<std::string, std::any>& out) const override {
    if (data.empty()) {
      TRPC_FMT_WARN("The file is empty! codec: json");
      return false;
    }
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errors;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (!reader->parse(data.c_str(), data.c_str() + data.size(), &root, &errors)) {
      TRPC_FMT_ERROR("Failed to unmarshal JSON data: {}", errors);
      return false;
    }

    codec::ProcessNode(root, "", out);
    return true;
  }
};

using JsonCodecPtr = RefPtr<JsonCodec>;

}  // namespace trpc::config