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

    void registerCollectable(const std::weak_ptr<::prometheus::Collectable>& collectable) {
            gateway_->RegisterCollectable(collectable);
}
private:
    std::unique_ptr<::prometheus::Gateway> gateway_;
    std::string job_name_;
};

} // namespace trpc
