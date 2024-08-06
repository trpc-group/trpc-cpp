#include "trpc/naming/common/util/loadbalance/weighted_round_robin/weighted_round_robin_load_balancer.h"

namespace trpc {

int SmoothWeightedPollingLoadBalance::Update(const LoadBalanceInfo* info) {
  if (nullptr == info || nullptr == info->info || nullptr == info->endpoints) {
    TRPC_LOG_ERROR("Endpoint info of name is empty");
    return -1;
  }
  std::unique_lock<std::shared_mutex> lock(mutex_);

  if (!IsLoadBalanceInfoDiff(info)) {
    return 0;
  }

  InnerEndpointInfos new_info;
  new_info.total_weight = 0;

  for (const auto& endpoint_info : *(info->endpoints)) {
    new_info.endpoints.push_back(endpoint_info);
    new_info.weights.push_back(endpoint_info.weight);
    new_info.current_weights.push_back(0);
    new_info.total_weight += endpoint_info.weight;
  }

  callee_router_infos_[info->info->name] = std::move(new_info);

  return 0;
}

int SmoothWeightedPollingLoadBalance::Next(LoadBalanceResult& result) {
  if (nullptr == result.info) {
    return -1;
  }

  std::shared_lock<std::shared_mutex> lock(mutex_);
  auto iter = callee_router_infos_.find((result.info)->name);
  if (iter == callee_router_infos_.end()) {
    TRPC_LOG_ERROR("Router info of name " << (result.info)->name << " no found");
    return -1;
  }

  std::vector<TrpcEndpointInfo>& endpoints = iter->second.endpoints;
  size_t endpoints_num = endpoints.size();
  if (endpoints_num < 1) {
    TRPC_LOG_ERROR("Router info of name is empty");
    return -1;
  }
  if (callee_router_infos_.empty()) {
    return -1;
  }

  auto& info = callee_router_infos_.begin()->second;

  int max_current_weight = -1;
  int selected_index = -1;

  for (size_t i = 0; i < info.endpoints.size(); ++i) {
    info.current_weights[i] += info.weights[i];
    if (info.current_weights[i] > max_current_weight) {
      max_current_weight = info.current_weights[i];
      selected_index = i;
    }
  }

  if (selected_index != -1) {
    info.current_weights[selected_index] -= info.total_weight;
    result.result = info.endpoints[selected_index];
    return 0;
  }

  return -1;
}

bool SmoothWeightedPollingLoadBalance::IsLoadBalanceInfoDiff(const LoadBalanceInfo* info) {
  if (nullptr == info || nullptr == info->info || nullptr == info->endpoints) {
    return false;
  }
  const auto& existing_info = callee_router_infos_.find(info->info->name);
  if (existing_info == callee_router_infos_.end()) {
    return true;
  }

  const auto& existing_endpoints = existing_info->second.endpoints;

  if (existing_endpoints.size() != info->endpoints->size()) {
    return true;
  }

  for (size_t i = 0; i < existing_endpoints.size(); ++i) {
    if (existing_endpoints[i].weight != (*info->endpoints)[i].weight ||
        existing_endpoints[i].host != (*info->endpoints)[i].host ||
        existing_endpoints[i].port != (*info->endpoints)[i].port) {
      return true;
    }
  }

  return false;
}

}  // namespace trpc
