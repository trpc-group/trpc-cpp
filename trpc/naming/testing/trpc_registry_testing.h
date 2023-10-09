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

#include "trpc/naming/registry.h"

namespace trpc::testing {

class TestTrpcRegistry : public Registry {
 public:
  TestTrpcRegistry() = default;
  ~TestTrpcRegistry() override = default;

  std::string Name() const override { return "test_trpc_registry"; }

  std::string Version() const { return "0.0.1"; }

  int Init() noexcept override { return 0; }

  void Start() noexcept override {}

  void Stop() noexcept override {}

  void Destroy() noexcept override {}

  int Register(const RegistryInfo* info) override {
    register_flag = true;
    return 0;
  }

  int Unregister(const RegistryInfo* info) override {
    unregister_flag = true;
    return 0;
  }

  int HeartBeat(const RegistryInfo* info) override {
    heartbeat_flag = true;
    return 0;
  }

  Future<> AsyncHeartBeat(const RegistryInfo* info) override { return MakeReadyFuture<>(); }

  bool GetRegisterFlag() const { return register_flag; }

  bool GetUnRegisterFlag() const { return unregister_flag; }

  bool GetHeartBeatFlag() const { return heartbeat_flag; }

 private:
  bool register_flag{false};
  bool unregister_flag{false};
  bool heartbeat_flag{false};
};

}  // namespace trpc::testing
