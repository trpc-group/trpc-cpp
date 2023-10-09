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

#include <any>
#include <memory>
#include <string>

#include "trpc/common/plugin.h"

namespace trpc {

/// @brief Auth status code.
enum class AuthStatusCode {
  kError = -1,
  kSuccess = 0,
  kInvalidToken = 1,
  kClientAuthVersionOutdated = 2,
  kServerAclVersionOutdated = 3,
  kClientIpIllegal = 4,
  kClientIsUnauthorized = 5,

  kAuthTokenNotFound = 404,
  kAuthedServiceNotFound = 504,
};

/// @brief An auth interface that provides authentication capabilities.
class Auth : public Plugin {
 public:
  /// @brief Returns plugin type.
  PluginType Type() const override { return PluginType::kAuth; }

  /// @brief For server side to authenticate the client's credential.
  virtual AuthStatusCode Authenticate(std::any& args) = 0;

  /// @brief For client side to get credentials, e.g. access token or something other.
  virtual AuthStatusCode GetCredential(std::any& args) = 0;
};
using AuthPtr = std::shared_ptr<Auth>;

/// @brief An observer interface to update credential and ACL.
class AuthCenterObserver {
 public:
  virtual ~AuthCenterObserver() = default;

  /// @brief Updates the credential
  /// @brief Return value 0 indicates failure, and values less than 0 indicate an error.
  virtual int UpdateCredential(const std::any& args) = 0;

  /// @brief Updates the ACL.
  /// @brief Return value 0 indicates failure, and values less than 0 indicate an error.
  virtual int UpdateAcl(const std::any& args) = 0;
};
using AuthCenterObserverPtr = std::shared_ptr<AuthCenterObserver>;

/// @brief An follower interface to execute commands as expected.
class AuthCenterFollower : public Plugin {
 public:
  enum class ExpectCommand {
    kRefreshCredential = 0x01,
    kRefreshAcl = 0x02,
  };

 public:
  /// @brief Plugin type.
  PluginType Type() const override { return PluginType::kAuthCenterFollower; }

  /// @brief Executes the commands specified by the parameter.
  /// @brief Return value 0 indicates failure, and values less than 0 indicate an error.
  virtual int Expect(const std::any& args) = 0;

  /// @brief Returns auth options.
  virtual std::any GetAuthOptions(const std::any& args) const = 0;
};
using AuthExpectCommand = AuthCenterFollower::ExpectCommand;
using AuthCenterFollowerPtr = std::shared_ptr<AuthCenterFollower>;

}  // namespace trpc
