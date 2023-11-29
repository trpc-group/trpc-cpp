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

#include "trpc/client/service_proxy_option_setter.h"

#include "trpc/common/config/default_value.h"

namespace trpc {
namespace detail {

// If the string field isn't empty, it means that the user has set it and it needs to be assigned.
void SetOutputByValidInput(const std::vector<std::string>& input, std::vector<std::string>& output) {
  if (input.empty()) {
    return;
  }
  output = input;
}

void SetOutputByValidInput(const std::any& input, std::any& output) {
  if (!input.has_value()) {
    return;
  }
  output = input;
}

void SetOutputByValidInput(const ProxyCallback& input, ProxyCallback& output) { output = input; }

void SetOutputByValidInput(const RedisClientConf& input, RedisClientConf& output) {
  auto enable = GetValidInput<bool>(input.enable, false);
  SetOutputByValidInput<bool>(enable, output.enable);

  auto password = GetValidInput<std::string>(input.password, "");
  SetOutputByValidInput<std::string>(password, output.password);

  auto user_name = GetValidInput<std::string>(input.user_name, "");
  SetOutputByValidInput<std::string>(user_name, output.user_name);

  auto db = GetValidInput<std::uint32_t>(input.db, 0);
  SetOutputByValidInput<std::uint32_t>(db, output.db);
}

void SetOutputByValidInput(const ClientSslConfig& input, ClientSslConfig& output) {
  auto enable = GetValidInput<bool>(input.enable, false);
  SetOutputByValidInput<bool>(enable, output.enable);

  auto sni_name = GetValidInput<std::string>(input.sni_name, "");
  SetOutputByValidInput<std::string>(sni_name, output.sni_name);

  auto ca_cert_path = GetValidInput<std::string>(input.ca_cert_path, "");
  SetOutputByValidInput<std::string>(ca_cert_path, output.ca_cert_path);

  auto ciphers = GetValidInput<std::string>(input.ciphers, "");
  SetOutputByValidInput<std::string>(ciphers, output.ciphers);

  auto dh_param_path = GetValidInput<std::string>(input.dh_param_path, "");
  SetOutputByValidInput<std::string>(dh_param_path, output.dh_param_path);

  SetOutputByValidInput(input.protocols, output.protocols);

  auto insecure = GetValidInput<bool>(input.insecure, false);
  SetOutputByValidInput<bool>(insecure, output.insecure);
}

// Set a std::map, use the values in input to overwrite the corresponding values in output.
void SetOutputByValidInput(const std::map<std::string, std::any>& input, std::map<std::string, std::any>& output) {
  for (auto& item : input) {
    output[item.first] = item.second;
  }
}

void SetDefaultOption(const std::shared_ptr<ServiceProxyOption>& option) {
  ServiceProxyOption option_tmp;
  *option = option_tmp;

  option->codec_name = kDefaultProtocol;
  option->network = kDefaultNetwork;
  option->conn_type = kDefaultConnType;
  option->timeout = kDefaultTimeout;
  option->enable_set_force = kDefaultEnableSetForce;
  option->disable_servicerouter = kDefaultDisableServiceRouter;
  option->is_conn_complex = kDefaultIsConnComplex;
  option->max_packet_size = kDefaultMaxPacketSize;
  option->recv_buffer_size = kDefaultRecvBufferSize;
  option->send_queue_capacity = kDefaultSendQueueCapacity;
  option->send_queue_timeout = kDefaultSendQueueTimeout;
  option->max_conn_num = kDefaultMaxConnNum;
  option->idle_time = kDefaultIdleTime;
  option->request_timeout_check_interval = kDefaultRequestTimeoutCheckInterval;
  option->is_reconnection = kDefaultIsReconnection;
  option->connect_timeout = kDefaultConnectTimeout;
  option->allow_reconnect = kDefaultAllowReconnect;
  option->threadmodel_type_name = kDefaultThreadmodelType;
  option->threadmodel_instance_name = "";
  option->support_pipeline = kDefaultSupportPipeline;
}

void SetSpecifiedOption(const ServiceProxyOption* option_ptr, const std::shared_ptr<ServiceProxyOption>& option) {
  auto name = GetValidInput<std::string>(option_ptr->name, "");
  SetOutputByValidInput<std::string>(name, option->name);

  auto target = GetValidInput<std::string>(option_ptr->target, "");
  SetOutputByValidInput<std::string>(target, option->target);

  auto codec_name = GetValidInput<std::string>(option_ptr->codec_name, kDefaultProtocol);
  SetOutputByValidInput<std::string>(codec_name, option->codec_name);
  if (option_ptr->codec_name == "http") {
    option->max_packet_size = kDefaultHttpMaxPacketSize;
  }

  auto network = GetValidInput<std::string>(option_ptr->network, kDefaultNetwork);
  SetOutputByValidInput<std::string>(network, option->network);

  auto conn_type = GetValidInput<std::string>(option_ptr->conn_type, kDefaultConnType);
  SetOutputByValidInput<std::string>(conn_type, option->conn_type);

  auto timeout = GetValidInput<uint32_t>(option_ptr->timeout, kDefaultTimeout);
  SetOutputByValidInput<uint32_t>(timeout, option->timeout);

  auto selector_name = GetValidInput<std::string>(option_ptr->selector_name, kDefaultSelectorName);
  SetOutputByValidInput<std::string>(selector_name, option->selector_name);

  auto caller_name = GetValidInput<std::string>(option_ptr->caller_name, "");
  SetOutputByValidInput<std::string>(caller_name, option->caller_name);

  auto callee_name = GetValidInput<std::string>(option_ptr->callee_name, "");
  SetOutputByValidInput<std::string>(callee_name, option->callee_name);

  auto name_space = GetValidInput<std::string>(option_ptr->name_space, "");
  SetOutputByValidInput<std::string>(name_space, option->name_space);

  auto callee_set_name = GetValidInput<std::string>(option_ptr->callee_set_name, "");
  SetOutputByValidInput<std::string>(callee_set_name, option->callee_set_name);

  auto load_balance_name = GetValidInput<std::string>(option_ptr->load_balance_name, "");
  SetOutputByValidInput<std::string>(load_balance_name, option->load_balance_name);

  auto load_balance_type = GetValidInput<LoadBalanceType>(option_ptr->load_balance_type, LoadBalanceType::DEFAULT);
  SetOutputByValidInput<LoadBalanceType>(load_balance_type, option->load_balance_type);

  auto enable_set_force = GetValidInput<bool>(option_ptr->enable_set_force, kDefaultEnableSetForce);
  SetOutputByValidInput<bool>(enable_set_force, option->enable_set_force);

  auto disable_servicerouter = GetValidInput<bool>(option_ptr->disable_servicerouter, kDefaultDisableServiceRouter);
  SetOutputByValidInput<bool>(disable_servicerouter, option->disable_servicerouter);

  auto is_conn_complex = GetValidInput<bool>(option_ptr->is_conn_complex, kDefaultIsConnComplex);
  SetOutputByValidInput<bool>(is_conn_complex, option->is_conn_complex);

  auto max_packet_size = GetValidInput<uint32_t>(option_ptr->max_packet_size, kDefaultMaxPacketSize);
  SetOutputByValidInput<uint32_t>(max_packet_size, option->max_packet_size);

  auto recv_buffer_size = GetValidInput<uint32_t>(option_ptr->recv_buffer_size, kDefaultRecvBufferSize);
  SetOutputByValidInput<uint32_t>(recv_buffer_size, option->recv_buffer_size);

  auto send_queue_capacity = GetValidInput<uint32_t>(option_ptr->send_queue_capacity, kDefaultSendQueueCapacity);
  SetOutputByValidInput<uint32_t>(send_queue_capacity, option->send_queue_capacity);

  auto send_queue_timeout = GetValidInput<uint32_t>(option_ptr->send_queue_timeout, kDefaultSendQueueTimeout);
  SetOutputByValidInput<uint32_t>(send_queue_timeout, option->send_queue_timeout);

  auto max_conn_num = GetValidInput<uint32_t>(option_ptr->max_conn_num, kDefaultMaxConnNum);
  SetOutputByValidInput<uint32_t>(max_conn_num, option->max_conn_num);

  auto idle_time = GetValidInput<uint32_t>(option_ptr->idle_time, kDefaultIdleTime);
  SetOutputByValidInput<uint32_t>(idle_time, option->idle_time);

  auto req_timeout_check_interval =
      GetValidInput<uint32_t>(option_ptr->request_timeout_check_interval, kDefaultRequestTimeoutCheckInterval);
  SetOutputByValidInput<uint32_t>(req_timeout_check_interval, option->request_timeout_check_interval);

  auto is_reconnection = GetValidInput<bool>(option_ptr->is_reconnection, kDefaultIsReconnection);
  SetOutputByValidInput<bool>(is_reconnection, option->is_reconnection);

  auto connect_timeout = GetValidInput<uint32_t>(option_ptr->connect_timeout, kDefaultConnectTimeout);
  SetOutputByValidInput<uint32_t>(connect_timeout, option->connect_timeout);

  auto allow_reconnect = GetValidInput<bool>(option_ptr->allow_reconnect, kDefaultAllowReconnect);
  SetOutputByValidInput<bool>(allow_reconnect, option->allow_reconnect);

  auto threadmodel_type_name = GetValidInput<std::string>(option_ptr->threadmodel_type_name, kDefaultThreadmodelType);
  SetOutputByValidInput<std::string>(threadmodel_type_name, option->threadmodel_type_name);

  auto threadmodel_instance_name = GetValidInput<std::string>(option_ptr->threadmodel_instance_name, "");
  SetOutputByValidInput<std::string>(threadmodel_instance_name, option->threadmodel_instance_name);

  SetOutputByValidInput(option_ptr->service_filters, option->service_filters);
  SetOutputByValidInput(option_ptr->proxy_callback, option->proxy_callback);
  SetOutputByValidInput(option_ptr->redis_conf, option->redis_conf);
  SetOutputByValidInput(option_ptr->ssl_config, option->ssl_config);

  auto support_pipeline = GetValidInput<bool>(option_ptr->support_pipeline, kDefaultSupportPipeline);
  SetOutputByValidInput<bool>(support_pipeline, option->support_pipeline);

  SetOutputByValidInput(option_ptr->service_filter_configs, option->service_filter_configs);

  auto stream_max_window_size = GetValidInput<uint32_t>(option_ptr->stream_max_window_size, 65535);
  SetOutputByValidInput<uint32_t>(stream_max_window_size, option->stream_max_window_size);

  auto fiber_pipeline_connector_queue_size =
      GetValidInput<uint32_t>(option_ptr->fiber_pipeline_connector_queue_size, 16 * 1024);
  SetOutputByValidInput<uint32_t>(fiber_pipeline_connector_queue_size, option->fiber_pipeline_connector_queue_size);

  auto fiber_connpool_shards = GetValidInput<uint32_t>(option_ptr->fiber_connpool_shards, 4);
  SetOutputByValidInput<uint32_t>(fiber_connpool_shards, option->fiber_connpool_shards);
}

}  // namespace detail
}  // namespace trpc
