#include <chrono>
#include <thread>
#include "trpc/metrics/prometheus/prometheus_metrics_api.h"
#include "trpc/log/trpc_log.h"



int main(int argc, char** argv) {
 
   while (true) {
        if (::trpc::prometheus::PushMetricsInfo())
            {
                std::cout << "Successfully pushed metrics to Pushgateway" << std::endl;
            } else {
                std::cerr << "Failed to push metrics to Pushgateway" << std::endl;
            }

        std::this_thread::sleep_for(std::chrono::seconds(5)); // 每60秒推送一次
    }

    return 0;
}