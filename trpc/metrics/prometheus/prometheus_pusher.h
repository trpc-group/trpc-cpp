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

#include <string>
#include <memory>
#include "prometheus/registry.h"
#include "prometheus/gateway.h"

namespace trpc {

class PrometheusPusher {
public:
    PrometheusPusher(const std::string& gateway_url, const std::string& job_name);
    ~PrometheusPusher();

    bool Push(const std::vector<::prometheus::MetricFamily>& metrics);
    bool AsyncPush(const std::vector<::prometheus::MetricFamily>& metrics);

    void registerCollectable(const std::weak_ptr<::prometheus::Collectable>& collectable) {
            gateway_->RegisterCollectable(collectable);
}
private:
    std::unique_ptr<::prometheus::Gateway> gateway_;
    std::string job_name_;
};

} // namespace trpc
