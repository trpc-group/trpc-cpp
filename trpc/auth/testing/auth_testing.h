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

#pragma once

#include <any>
#include <memory>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/auth/auth.h"

namespace trpc::testing {

class TestAuth : public Auth {
 public:
  std::string Name() const override { return "TestAuth"; }
  AuthStatusCode Authenticate(std::any& args) override { return AuthStatusCode::kSuccess; }
  AuthStatusCode GetCredential(std::any& args) override { return AuthStatusCode::kSuccess; }
};

class MockAuth : public Auth {
 public:
  std::string Name() const override { return "MockAuth"; }
  MOCK_METHOD(AuthStatusCode, Authenticate, (std::any & args), (override));
  MOCK_METHOD(AuthStatusCode, GetCredential, (std::any& args), (override));
};

class TestAuthCenterFollower : public AuthCenterFollower {
 public:
  std::string Name() const override { return "TestAuthCenterFollower"; }
  int Expect(const std::any& args) override { return 0; }
  std::any GetAuthOptions(const std::any& args) const override { return std::make_any<int>(0); }
};

class MockAuthCenterFollower : public AuthCenterFollower {
 public:
  std::string Name() const override { return "MockAuthCenterFollower"; }
  MOCK_METHOD(int, Expect, (const std::any& args), (override));
  MOCK_METHOD(std::any, GetAuthOptions, (const std::any& args), (const override));
};

class TestAuthCenterObserver : public AuthCenterObserver {
 public:
  int UpdateCredential(const std::any& args) override { return 0; }
  int UpdateAcl(const std::any& args) override { return 0; }
};

class MockAuthCenterObserver : public AuthCenterObserver {
 public:
  MOCK_METHOD(int, UpdateCredential, (const std::any& args), (override));
  MOCK_METHOD(int, UpdateAcl, (const std::any& args), (override));
};

}  // namespace trpc::testing
