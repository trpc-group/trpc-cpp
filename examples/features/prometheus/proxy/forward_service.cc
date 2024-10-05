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

#include "examples/features/prometheus/proxy/forward_service.h"

#include <random>

#include "trpc/client/client_context.h"
#include "trpc/client/make_client_context.h"
#include "trpc/client/trpc_client.h"
#include "trpc/log/trpc_log.h"
#include "trpc/metrics/prometheus/prometheus_metrics_api.h"

namespace examples::forward {

ForwardServiceImpl::ForwardServiceImpl() {
  greeter_proxy_ =
      ::trpc::GetTrpcClient()->GetProxy<::trpc::test::helloworld::GreeterServiceProxy>("trpc.test.helloworld.Greeter");
}

::trpc::Status ForwardServiceImpl::Route(::trpc::ServerContextPtr context,
                                         const ::trpc::test::helloworld::HelloRequest* request,
                                         ::trpc::test::helloworld::HelloReply* reply) {
  TRPC_FMT_INFO("Forward request:{}, req id:{}", request->msg(), context->GetRequestId());

#ifdef TRPC_BUILD_INCLUDE_PROMETHEUS
  // 1. uses the Prometheus metrics interfaces
  std::random_device rd;
  std::mt19937 eng(rd());
  std::uniform_int_distribution<> distr(0, 99);
  int random_num = distr(eng);
  TRPC_FMT_INFO("Add random number {} to Prometheus metrics", random_num);

  ::trpc::prometheus::ReportSumMetricsInfo({{"sum_test_key1", "sum_test_value1"}, {"sum_test_key2", "sum_test_value2"}},
                                           random_num);
  ::trpc::prometheus::ReportSetMetricsInfo({{"set_test_key1", "set_test_value1"}, {"set_test_key2", "set_test_value2"}},
                                           random_num);
  ::trpc::prometheus::ReportMidMetricsInfo({{"mid_test_key1", "mid_test_value1"}, {"mid_test_key2", "mid_test_value2"}},
                                           random_num);
  ::trpc::prometheus::ReportQuantilesMetricsInfo(
      {{"quantiles_test_key1", "quantiles_test_value1"}, {"quantiles_test_key2", "quantiles_test_value2"}},
      {{0.5, 0.05}, {0.1, 0.05}}, random_num);
  ::trpc::prometheus::ReportHistogramMetricsInfo(
      {{"histogram_test_key1", "histogram_test_value1"}, {"histogram_test_key2", "histogram_test_value2"}},
      {10, 20, 50}, random_num);

  // 2. uses the common metrics interfaces. (these interfaces are cumbersome to use, it is not recommended to use them.)
  // Here we take the reporting of SUM type data as an example.
  ::trpc::TrpcSingleAttrMetricsInfo single_metrics_info;
  single_metrics_info.plugin_name = ::trpc::prometheus::kPrometheusMetricsName;
  single_metrics_info.single_attr_info.policy = ::trpc::MetricsPolicy::SUM;
  single_metrics_info.single_attr_info.name = "single_test_key";
  single_metrics_info.single_attr_info.dimension = "single_test_value";
  single_metrics_info.single_attr_info.value = random_num;
  ::trpc::metrics::SingleAttrReport(std::move(single_metrics_info));

  ::trpc::TrpcMultiAttrMetricsInfo multi_metrics_info;
  multi_metrics_info.plugin_name = ::trpc::prometheus::kPrometheusMetricsName;
  multi_metrics_info.multi_attr_info.values = {{::trpc::MetricsPolicy::SUM, random_num}};
  multi_metrics_info.multi_attr_info.tags = {{"multi_test_key1", "multi_test_value1"},
                                             {"multi_test_key2", "multi_test_value2"}};
  ::trpc::metrics::MultiAttrReport(std::move(multi_metrics_info));

  // 3. creates a custom metrics item and report it through Prometheus native API.
  // For example, creating and using a counter type metrics item:
  ::prometheus::Family<::prometheus::Counter>* counter_family = ::trpc::prometheus::GetCounterFamily(
      "counter_name", "counter_desc", {{"const_counter_key", "const_counter_value"}});
  ::prometheus::Counter& counter = counter_family->Add({{"counter_key", "counter_value"}});
  counter.Increment(random_num);
  
  // if (::trpc::prometheus::PushMetricsInfo()) {
  //       TRPC_FMT_INFO("Successfully pushed metrics to Pushgateway");
  //   } else {
  //       TRPC_FMT_ERROR("Failed to push metrics to Pushgateway");
  //   }
#endif

  auto client_context = ::trpc::MakeClientContext(context, greeter_proxy_);

  ::trpc::test::helloworld::HelloRequest route_request;
  route_request.set_msg(request->msg());
  ::trpc::test::helloworld::HelloReply route_reply;
  // block current fiber, not block current fiber worker thread
  ::trpc::Status status = greeter_proxy_->SayHello(client_context, route_request, &route_reply);

  TRPC_FMT_INFO("Forward status:{}, route_reply:{}", status.ToString(), route_reply.msg());

  reply->set_msg(route_reply.msg());

  return status;
}

}  // namespace examples::forward
