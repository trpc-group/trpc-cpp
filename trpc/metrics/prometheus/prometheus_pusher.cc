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

#include "trpc/metrics/prometheus/prometheus_pusher.h"
#include "trpc/util/log/logging.h"
#include <sstream>

namespace trpc {

PrometheusPusher::PrometheusPusher(const std::string& gateway_url, const std::string& job_name)
    : job_name_(job_name) {
    gateway_ = std::make_unique<::prometheus::Gateway>("localhost","9091", job_name);
}

PrometheusPusher::~PrometheusPusher() = default;



bool PrometheusPusher::Push(const std::vector<::prometheus::MetricFamily>& metrics)
 {
    int push_result = gateway_->Push();
    if (push_result != 200) {
        std::ostringstream oss;
        oss << "Failed to push metrics to Pushgateway. Error code: " << push_result;
        TRPC_LOG_ERROR(oss.str());
        return false;
    }
    return true;
}

bool PrometheusPusher::AsyncPush(const std::vector<::prometheus::MetricFamily>& metrics)
 {
    std::future<int> push_future = gateway_->AsyncPush();
    
    // 等待异步操作完成并获取结果
    try {
        int push_result = push_future.get();
        if (push_result != 200) {
            std::ostringstream oss;
            oss << "Failed to push metrics to Pushgateway. Error code: " << push_result;
            TRPC_LOG_ERROR(oss.str());
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        TRPC_LOG_ERROR("Exception occurred while pushing metrics: " + std::string(e.what()));
        return false;
    }
}

} // namespace trpc

