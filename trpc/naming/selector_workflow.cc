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

#include "trpc/naming/selector_workflow.h"

#include <utility>

#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/naming/common/constants.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/string/string_util.h"
#include "trpc/util/time.h"

namespace trpc {

int SelectorWorkFlow::Init() {
  if (selector_) {
    return 0;
  }

  TRPC_LOG_ERROR("selector " << plugin_name_ << " no found");
  return -1;
}

void SelectorWorkFlow::RunFilter(FilterStatus& status, FilterPoint point, const ClientContextPtr& context) {
  switch (point) {
    case FilterPoint::CLIENT_PRE_RPC_INVOKE:
      if (!SelectTarget(context)) {
        status = FilterStatus::REJECT;
        return;
      }

      break;
    case FilterPoint::CLIENT_POST_RPC_INVOKE:
      ReportInvokeResult(context);
      break;
    default:
      TRPC_ASSERT(0 && "Unexpected filter point");
      break;
  }

  status = FilterStatus::CONTINUE;
}

bool SelectorWorkFlow::SelectTarget(const ClientContextPtr& context) {
  if (context->IsSetAddr()) {
    return true;
  }

  if (selector_ == nullptr) {
    TRPC_LOG_ERROR("Selector is null");
    Status status(TrpcRetCode::TRPC_CLIENT_ROUTER_ERR, 0, "Selector is null");
    context->SetStatus(std::move(status));
    return false;
  }

  SelectorInfo selector_info;
  FillSelectorInfo(context, selector_info);

  int ret = 0;
  TrpcEndpointInfo service_instance;
  if (context->IsBackupRequest()) {
    // Execute backup request routing logic
    std::vector<TrpcEndpointInfo> instances;
    ret = selector_->SelectBatch(&selector_info, &instances);

    if (ret == 0) {
      if (instances.size() == 1) {
        TRPC_LOG_WARN("there is only one node, can not send as backup-request");
        context->CancelBackupRequest();
        service_instance = std::move(instances[0]);
      } else {
        // Copy data to service_instance for later setting into context
        service_instance = instances[0];
        SetBackupRequestAddrs(context, std::move(instances));
      }
    }
  } else {
    ret = selector_->Select(&selector_info, &service_instance);
  }

  if (ret != 0) {
    auto error = util::FormatString("service {} selector {} failed: {}", selector_info.name, selector_->Name(), ret);
    TRPC_LOG_ERROR(error);
    Status status(TrpcRetCode::TRPC_CLIENT_ROUTER_ERR, 0, std::move(error));
    context->SetStatus(std::move(status));
    return false;
  }

  SetRequestAddr(context, std::move(service_instance));

  return true;
}

const std::string& SelectorWorkFlow::GetServiceName(const ClientContextPtr& context) {
  if (get_service_name_func_) {
    return get_service_name_func_(context);
  }

  return context->GetServiceTarget();
}

void SelectorWorkFlow::FillSelectorInfo(const ClientContextPtr& context, SelectorInfo& selector_info) {
  const auto* service_proxy_option = context->GetServiceProxyOption();
  TRPC_ASSERT(service_proxy_option != nullptr);

  selector_info.name = GetServiceName(context);
  selector_info.load_balance_name = service_proxy_option->load_balance_name;
  selector_info.load_balance_type = service_proxy_option->load_balance_type;
  selector_info.context = context;
  // Set extend select info at the service level.
  auto iter = service_proxy_option->service_filter_configs.find(plugin_name_);
  if (iter != service_proxy_option->service_filter_configs.end()) {
    selector_info.extend_select_info = &iter->second;
  }

  // For backup request
  if (context->IsBackupRequest()) {
    selector_info.policy = SelectorPolicy::MULTIPLE;
    // Only 2 is supported for the number of retries.
    selector_info.select_num = 2;
  } else {
    // Currently only optimizing for single-node route discovery
    selector_info.is_from_workflow = true;
  }
}

void SelectorWorkFlow::SetRequestAddr(const ClientContextPtr& context, TrpcEndpointInfo&& instance) {
  ExtendNodeAddr extend_addr;
  extend_addr.addr.ip = std::move(instance.host);
  extend_addr.addr.port = instance.port;
  extend_addr.addr.addr_type = instance.is_ipv6 ? NodeAddr::AddrType::kIpV6 : NodeAddr::AddrType::kIpV4;
  if (has_metadata_) {
    extend_addr.metadata[naming::kNodeSetName] = std::move(instance.meta[naming::kNodeSetName]);
    extend_addr.metadata[naming::kNodeContainerName] = std::move(instance.meta[naming::kNodeContainerName]);
  }

  context->SetRequestAddrByNaming(std::move(extend_addr));
}

void SelectorWorkFlow::SetBackupRequestAddrs(const ClientContextPtr& context,
                                             std::vector<TrpcEndpointInfo>&& instances) {
  std::vector<ExtendNodeAddr> extend_addrs;
  extend_addrs.reserve(instances.size());
  for (auto& ins : instances) {
    ExtendNodeAddr extend_addr;
    extend_addr.addr.ip = std::move(ins.host);
    extend_addr.addr.port = ins.port;
    extend_addr.addr.addr_type = ins.is_ipv6 ? NodeAddr::AddrType::kIpV6 : NodeAddr::AddrType::kIpV4;
    if (has_metadata_) {
      extend_addr.metadata[naming::kNodeSetName] = std::move(ins.meta[naming::kNodeSetName]);
      extend_addr.metadata[naming::kNodeContainerName] = std::move(ins.meta[naming::kNodeContainerName]);
    }
    extend_addrs.emplace_back(extend_addr);
  }

  context->SetBackupRequestAddrsByNaming(std::move(extend_addrs));
}

void SelectorWorkFlow::SetActualBackupRequestNode(const ClientContextPtr& context) {
  auto* retry_info = context->GetBackupRequestRetryInfo();
  if (retry_info && retry_info->succ_rsp_node_index > 0) {
    auto& node_addr = retry_info->backup_addrs[retry_info->succ_rsp_node_index];
    context->SetRequestAddrByNaming(std::move(node_addr));
  }
}

void SelectorWorkFlow::FillInvokeResult(const ClientContextPtr& context, InvokeResult& result) {
  result.name = GetServiceName(context);
  uint64_t send_timestamp_ms = context->GetSendTimestampUs() / 1000;
  result.cost_time = send_timestamp_ms ? trpc::time::GetMilliSeconds() - send_timestamp_ms : 0;
  result.framework_result = context->GetStatus().GetFrameworkRetCode();
  result.interface_result = context->GetStatus().GetFuncRetCode();
  result.context = context;
}

int SelectorWorkFlow::ReportInvokeResult(const ClientContextPtr& context) {
  // set the actual request's backend node into the context when using backup request
  SetActualBackupRequestNode(context);

  // If the context ip and port have been set manually by the user, there is no need to report
  if (context->IsSetAddr()) {
    return 0;
  }

  if (need_report_) {
    // Determine if a circuit breaker needs to be reported based on the framework return code
    if (ShouldReport(context->GetStatus().GetFrameworkRetCode())) {
      InvokeResult invoke_result;
      FillInvokeResult(context, invoke_result);
      return selector_->ReportInvokeResult(&invoke_result);
    }
  }

  return 0;
}

bool SelectorWorkFlow::ShouldReport(int framework_retcode) {
  // These framework error code scenarios do not need to be reported because
  // the actual call to the server has not been made yet, including:
  // TRPC_CLIENT_ENCODE_ERR, TRPC_CLIENT_ROUTER_ERR, TRPC_CLIENT_LIMITED_ERR, TRPC_CLIENT_OVERLOAD_ERR
  if (framework_retcode == TrpcRetCode::TRPC_CLIENT_ENCODE_ERR ||
      framework_retcode == TrpcRetCode::TRPC_CLIENT_ROUTER_ERR ||
      framework_retcode == TrpcRetCode::TRPC_CLIENT_LIMITED_ERR ||
      framework_retcode == TrpcRetCode::TRPC_CLIENT_OVERLOAD_ERR) {
    return false;
  }

  return true;
}

}  // namespace trpc
