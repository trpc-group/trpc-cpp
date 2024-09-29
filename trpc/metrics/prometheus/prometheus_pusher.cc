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

} // namespace trpc

