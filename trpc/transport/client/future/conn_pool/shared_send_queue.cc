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

#include "trpc/transport/client/future/conn_pool/shared_send_queue.h"

namespace trpc::internal {

bool SharedSendQueue::Push(uint64_t index, CTransportReqMsg* msg, uint64_t expire_time_ms) {
  if (send_queue_[index].msg == nullptr) {
    DataType data;
    data.msg = msg;
    data.time_node = timing_wheel_.Add(expire_time_ms, send_queue_.begin() + index);
    send_queue_[index] = std::move(data);
    return true;
  }

  return false;
}

CTransportReqMsg* SharedSendQueue::Pop(uint64_t index) {
  auto& data = send_queue_[index];
  CTransportReqMsg* msg = data.msg;
  if (msg != nullptr) {
    timing_wheel_.Delete(data.time_node);
    data.msg = nullptr;
    data.time_node = nullptr;
    return msg;
  }

  return nullptr;
}

CTransportReqMsg* SharedSendQueue::Get(uint64_t index) {
  return send_queue_[index].msg;
}

CTransportReqMsg* SharedSendQueue::GetAndPop(const DataIterator& iter) {
  uint64_t index = GetIndex(iter);
  auto& data = send_queue_[index];
  CTransportReqMsg* msg = data.msg;
  if (msg != nullptr) {
    data.msg = nullptr;
    data.time_node = nullptr;
  }
  return msg;
}

void SharedSendQueue::DoTimeout(size_t now_ms, const Function<void(const DataIterator&)>& timeout_handle) {
  timing_wheel_.DoTimeout(now_ms, timeout_handle);
}

}  // namespace trpc::internal
