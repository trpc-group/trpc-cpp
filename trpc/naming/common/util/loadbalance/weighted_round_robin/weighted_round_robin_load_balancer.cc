#include "smooth_weighted_polling_load_balance.h"

namespace trpc {

int SmoothWeightedPollingLoadBalance::Update(const LoadBalanceInfo* info) {
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
  std::shared_lock<std::shared_mutex> lock(mutex_);

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
    result.endpoint_info = info.endpoints[selected_index];
    return 0;
  }

  return -1;
}

bool SmoothWeightedPollingLoadBalance::IsLoadBalanceInfoDiff(const LoadBalanceInfo* info) {
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
