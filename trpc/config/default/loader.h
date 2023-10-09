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

#pragma once

#include <stdexcept>
#include <string>

#include "trpc/config/codec_factory.h"
#include "trpc/config/default/default_config.h"
#include "trpc/config/provider_factory.h"
#include "trpc/util/log/logging.h"

namespace trpc::config {

/// @brief The LoadOption class represents an option for the default loader.
class LoadOptions {
 public:
  /// @brief OptionFunc is a function type that takes a DefaultConfigPtr parameter and returns void.
  LoadOptions(const OptionFunc& func) : func_(func) {}

  /// @brief Applies the option to the given DefaultConfig object.
  /// @param config The DefaultConfig object to apply the option to.
  void Apply(DefaultConfigPtr& config) const { func_(config); }

 private:
  // The OptionFunc stored in the object.
  OptionFunc func_;
};

/// @brief Returns an option that sets the codec's name.
/// @param name The name of the codec to be set.
/// @return A LoadOption object that sets the codec's name.
LoadOptions WithCodec(const std::string& name);

/// @brief Returns an option that sets the provider's name.
/// @param name The name of the provider instance to be set.
/// @return A LoadOption object that sets the provider's name.
LoadOptions WithProvider(const std::string& name);

namespace detail {

/// @brief Load the configuration file at the specified path and configure it according to the given options.
/// This function loads the configuration file at the specified path, and configures it according to the given options.
/// @param path The path of the configuration file to load.
/// @param opts The options to use when configuring the loaded configuration file.
/// @return A pointer to the loaded configuration object, or `nullptr` if the loading fails.
/// @private For internal use purpose only.
DefaultConfigPtr Load(const std::string& path, std::vector<LoadOptions>&& opts);

}  // namespace detail

}  // namespace trpc::config
