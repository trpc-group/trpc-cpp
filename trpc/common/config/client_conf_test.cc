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

#include "trpc/common/config/client_conf.h"

#include "gtest/gtest.h"

#include "trpc/common/config/client_conf_parser.h"
#include "trpc/common/config/retry_conf.h"
#include "trpc/common/config/yaml_parser.h"

namespace trpc::testing {

TEST(ClientConfigTest, All) {
  ClientConfig client_config;

  ServiceProxyConfig proxy_config;
  proxy_config.name = "trpc.test.helloworld.Greeter";
  proxy_config.target = proxy_config.name;
  proxy_config.protocol = "trpc";
  proxy_config.network = "tcp";
  proxy_config.conn_type = "long";
  proxy_config.is_conn_complex = true;
  proxy_config.support_pipeline = false;
  proxy_config.connect_timeout = 100;
  proxy_config.timeout = 10;
  proxy_config.request_timeout_check_interval = 5;
  proxy_config.max_packet_size = 20000000;
  proxy_config.max_conn_num = 128;
  proxy_config.idle_time = 10000;
  proxy_config.is_reconnection = false;
  proxy_config.allow_reconnect = false;
  proxy_config.recv_buffer_size = 4096;
  proxy_config.send_queue_capacity = 20000000;
  proxy_config.send_queue_timeout = 10000;
  proxy_config.threadmodel_instance_name = "fiber_instance";
  proxy_config.selector_name = "poloris";
  proxy_config.namespace_ = "informal";
  proxy_config.load_balance_name = "default";
  proxy_config.load_balance_type = "default";
  proxy_config.disable_servicerouter = false;
  proxy_config.callee_name = proxy_config.name;
  proxy_config.callee_set_name = "a.b.c";
  proxy_config.stream_max_window_size = 10000;

  proxy_config.redis_conf.password = "my_redis";
  proxy_config.redis_conf.enable = true;

  RetryHedgingLimitConfig retry_hedging_config;
  retry_hedging_config.max_tokens = 10;

  proxy_config.service_filter_configs[kRetryHedgingLimitFilter] = retry_hedging_config;

  client_config.filters = {"tpstelemetry"};
  client_config.service_proxy_config.push_back(proxy_config);

  YAML::convert<trpc::ClientConfig> c;

  YAML::Node client_node = c.encode(client_config);

  ClientConfig tmp_client_config;
  ASSERT_EQ(c.decode(client_node, tmp_client_config), true);

  tmp_client_config.Display();

  auto& tmp_proxy_config = tmp_client_config.service_proxy_config.front();

  ASSERT_EQ(proxy_config.name, tmp_proxy_config.name);
  ASSERT_EQ(proxy_config.target, tmp_proxy_config.target);
  ASSERT_EQ(proxy_config.protocol, tmp_proxy_config.protocol);
  ASSERT_EQ(proxy_config.network, tmp_proxy_config.network);
  ASSERT_EQ(proxy_config.conn_type, tmp_proxy_config.conn_type);
  ASSERT_EQ(proxy_config.is_conn_complex, tmp_proxy_config.is_conn_complex);
  ASSERT_EQ(proxy_config.support_pipeline, tmp_proxy_config.support_pipeline);
  ASSERT_EQ(proxy_config.connect_timeout, tmp_proxy_config.connect_timeout);
  ASSERT_EQ(proxy_config.timeout, tmp_proxy_config.timeout);
  ASSERT_EQ(proxy_config.request_timeout_check_interval, tmp_proxy_config.request_timeout_check_interval);
  ASSERT_EQ(proxy_config.max_packet_size, tmp_proxy_config.max_packet_size);
  ASSERT_EQ(proxy_config.max_conn_num, tmp_proxy_config.max_conn_num);
  ASSERT_EQ(proxy_config.idle_time, tmp_proxy_config.idle_time);
  ASSERT_EQ(proxy_config.is_reconnection, tmp_proxy_config.is_reconnection);
  ASSERT_EQ(proxy_config.allow_reconnect, tmp_proxy_config.allow_reconnect);

  ASSERT_EQ(proxy_config.recv_buffer_size, tmp_proxy_config.recv_buffer_size);
  ASSERT_EQ(proxy_config.send_queue_capacity, tmp_proxy_config.send_queue_capacity);
  ASSERT_EQ(proxy_config.send_queue_timeout, tmp_proxy_config.send_queue_timeout);
  ASSERT_EQ(proxy_config.threadmodel_instance_name, tmp_proxy_config.threadmodel_instance_name);
  ASSERT_EQ(proxy_config.selector_name, tmp_proxy_config.selector_name);
  ASSERT_EQ(proxy_config.namespace_, tmp_proxy_config.namespace_);
  ASSERT_EQ(proxy_config.load_balance_name, tmp_proxy_config.load_balance_name);
  ASSERT_EQ(proxy_config.load_balance_type, tmp_proxy_config.load_balance_type);
  ASSERT_EQ(proxy_config.disable_servicerouter, tmp_proxy_config.disable_servicerouter);
  ASSERT_EQ(proxy_config.callee_name, tmp_proxy_config.callee_name);
  ASSERT_EQ(proxy_config.callee_set_name, tmp_proxy_config.callee_set_name);
  ASSERT_EQ(proxy_config.stream_max_window_size, tmp_proxy_config.stream_max_window_size);

  ASSERT_EQ(proxy_config.redis_conf.enable, tmp_proxy_config.redis_conf.enable);
  ASSERT_EQ(proxy_config.redis_conf.user_name, tmp_proxy_config.redis_conf.user_name);
  ASSERT_EQ(proxy_config.redis_conf.password, tmp_proxy_config.redis_conf.password);

  auto tmp_retry_hedging_config = std::any_cast<RetryHedgingLimitConfig>(
      tmp_proxy_config.service_filter_configs[kRetryHedgingLimitFilter]);
  ASSERT_EQ(retry_hedging_config.max_tokens, tmp_retry_hedging_config.max_tokens);

  ASSERT_EQ(client_config.filters[0], tmp_client_config.filters[0]);
}

}  // namespace trpc::testing
