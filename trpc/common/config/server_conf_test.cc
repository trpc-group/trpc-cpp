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

#include "trpc/common/config/server_conf.h"

#include <gtest/gtest.h>

#include "trpc/common/config/server_conf_parser.h"

namespace trpc::testing {

TEST(ServerConfigTest, server_config_test) {
  ServerConfig server_config;

  server_config.app = "test";
  server_config.server = "helloworld";
  server_config.bin_path = "/usr/local/trpc/bin";
  server_config.conf_path = "/usr/local/trpc/conf";
  server_config.data_path = "/usr/local/trpc/data";
  server_config.admin_ip = "0.0.0.0";
  server_config.admin_port = "8888";
  server_config.admin_idle_time = 50000;
  server_config.registry_name = "meshpolaris";
  server_config.enable_server_stats = false;
  server_config.server_stats_interval = 60000;
  server_config.filters = {"tpstelemetry"};
  server_config.stop_max_wait_time = 1000;

  ServiceConfig service_config;
  service_config.service_name = "trpc.test.helloworld.Greeter";
  service_config.socket_type = "net";
  service_config.network = "tcp";
  service_config.ip = "0.0.0.0";
  service_config.is_ipv6 = false;
  service_config.port = 10001;
  service_config.protocol = "long";
  service_config.max_conn_num = 10000;
  service_config.queue_timeout = 10000;
  service_config.idle_time = 60000;
  service_config.timeout = 5000;
  service_config.disable_request_timeout = false;
  service_config.share_transport = true;
  service_config.max_packet_size = 10000000;
  service_config.recv_buffer_size = 20000000;
  service_config.send_queue_capacity = 20000000;
  service_config.send_queue_timeout = 5000;
  service_config.threadmodel_instance_name = "instance1";
  service_config.accept_thread_num = 2;
  service_config.stream_read_timeout = 3000;
  service_config.stream_max_window_size = 65535;

  service_config.Display();

  server_config.services_config.push_back(service_config);

  YAML::Node server_config_node = YAML::convert<trpc::ServerConfig>::encode(server_config);

  ServerConfig tmp;

  ASSERT_EQ(YAML::convert<trpc::ServerConfig>::decode(server_config_node, tmp), true);

  tmp.Display();

  ASSERT_EQ(server_config.app, tmp.app);
  ASSERT_EQ(server_config.server, tmp.server);
  ASSERT_EQ(server_config.bin_path, tmp.bin_path);
  ASSERT_EQ(server_config.conf_path, tmp.conf_path);
  ASSERT_EQ(server_config.data_path, tmp.data_path);
  ASSERT_EQ(server_config.admin_ip, tmp.admin_ip);
  ASSERT_EQ(server_config.admin_port, tmp.admin_port);
  ASSERT_EQ(server_config.admin_idle_time, tmp.admin_idle_time);
  ASSERT_EQ(server_config.registry_name, tmp.registry_name);
  ASSERT_EQ(server_config.enable_server_stats, tmp.enable_server_stats);
  ASSERT_EQ(server_config.server_stats_interval, tmp.server_stats_interval);
  ASSERT_EQ(server_config.filters[0], tmp.filters[0]);
  ASSERT_EQ(server_config.stop_max_wait_time, tmp.stop_max_wait_time);

  ASSERT_EQ(server_config.services_config.front().service_name, tmp.services_config.front().service_name);
  ASSERT_EQ(server_config.services_config.front().network, tmp.services_config.front().network);
  ASSERT_EQ(server_config.services_config.front().ip, tmp.services_config.front().ip);
  ASSERT_EQ(server_config.services_config.front().is_ipv6, tmp.services_config.front().is_ipv6);
  ASSERT_EQ(server_config.services_config.front().port, tmp.services_config.front().port);
  ASSERT_EQ(server_config.services_config.front().protocol, tmp.services_config.front().protocol);
  ASSERT_EQ(server_config.services_config.front().max_conn_num, tmp.services_config.front().max_conn_num);
  ASSERT_EQ(server_config.services_config.front().queue_timeout, tmp.services_config.front().queue_timeout);
  ASSERT_EQ(server_config.services_config.front().idle_time, tmp.services_config.front().idle_time);
  ASSERT_EQ(server_config.services_config.front().timeout, tmp.services_config.front().timeout);
  ASSERT_EQ(server_config.services_config.front().disable_request_timeout,
            tmp.services_config.front().disable_request_timeout);
  ASSERT_EQ(server_config.services_config.front().max_packet_size, tmp.services_config.front().max_packet_size);
  ASSERT_EQ(server_config.services_config.front().recv_buffer_size, tmp.services_config.front().recv_buffer_size);
  ASSERT_EQ(server_config.services_config.front().send_queue_capacity, tmp.services_config.front().send_queue_capacity);
  ASSERT_EQ(server_config.services_config.front().send_queue_timeout, tmp.services_config.front().send_queue_timeout);
  ASSERT_EQ(server_config.services_config.front().stream_read_timeout, tmp.services_config.front().stream_read_timeout);
  ASSERT_EQ(server_config.services_config.front().share_transport, tmp.services_config.front().share_transport);
  ASSERT_EQ(server_config.services_config.front().stream_max_window_size,
            tmp.services_config.front().stream_max_window_size);

#if defined(SO_REUSEPORT) && !defined(TRPC_DISABLE_REUSEPORT)
  ASSERT_EQ(YAML::convert<trpc::ServerConfig>::decode(server_config_node, tmp), true);
  ASSERT_EQ(tmp.services_config.front().accept_thread_num, server_config.services_config.front().accept_thread_num);
#else
  ASSERT_EQ(YAML::convert<trpc::ServerConfig>::decode(server_config_node, tmp), true);
  ASSERT_EQ(tmp.services_config.front().accept_thread_num, 1);
#endif
}

TEST(ServerConfigTest, test_ip) {
  ServerConfig server_config;

  ServiceConfig service_config;
  service_config.service_name = "trpc.test.helloworld.Greeter";
  service_config.nic = "lo";

  server_config.services_config.push_back(service_config);

  YAML::Node server_config_node = YAML::convert<trpc::ServerConfig>::encode(server_config);

  // remove default ip, test get ip by nic
  server_config_node["service"][0].remove("ip");

  ServerConfig tmp;

  ASSERT_EQ(YAML::convert<trpc::ServerConfig>::decode(server_config_node, tmp), true);

  tmp.Display();

  ASSERT_EQ(tmp.services_config.front().ip, "127.0.0.1");
  ASSERT_EQ(tmp.services_config.front().nic, "lo");
}

}  // namespace trpc::testing
