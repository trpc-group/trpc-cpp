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

#include "trpc/transport/client/future/future_transport.h"

#include <memory>
#include <tuple>
#include <utility>

#include "trpc/future/future_utility.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/separate_runtime.h"
#include "trpc/runtime/threadmodel/common/worker_thread.h"
#include "trpc/transport/client/fixed_connector_id.h"
#include "trpc/transport/client/future/common/utils.h"
#include "trpc/transport/client/future/future_connector_group.h"
#include "trpc/transport/client/retry_info_def.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

namespace trpc {

int FutureTransport::Init(Options&& options) {
  options_ = options;

  if (options_.thread_model->Type() == std::string(kMerge)) {
    merge_thread_model_ = true;
  }

  std::vector<Reactor*> reactors;
  if (merge_thread_model_) {
    reactors = merge::GetReactors(options_.thread_model);
  } else {
    reactors = separate::GetReactors(options_.thread_model);
  }

  for (auto* reactor : reactors) {
    FutureTransportAdapter::Options transport_adapter_option;
    transport_adapter_option.reactor = reactor;
    transport_adapter_option.trans_info = &(options_.trans_info);

    adapters_.emplace_back(std::make_unique<FutureTransportAdapter>(std::move(transport_adapter_option)));
  }

  return 0;
}

void FutureTransport::Stop() {
  for (auto& adapter : adapters_) {
    adapter->Stop();
  }
}

void FutureTransport::Destroy() {
  for (auto& adapter : adapters_) {
    adapter->Destroy();
  }
}

int FutureTransport::SendRecv(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) {
  // Allocated in heap to make sure longer lifecycle.
  auto* new_msg = trpc::object_pool::New<CTransportReqMsg>();
  new_msg->context = req_msg->context;
  new_msg->extend_info = trpc::object_pool::MakeLwShared<trpc::ClientExtendInfo>();
  new_msg->extend_info->is_blocking_invoke = true;
  new_msg->send_data = std::move(req_msg->send_data);

  auto fut = AsyncSendRecv(new_msg).Then([new_msg](Future<CTransportRspMsg>&& fut_rsp) {
    // Release heap resource above.
    trpc::object_pool::Delete(new_msg);
    return std::move(fut_rsp);
  });

  fut = future::BlockingGet(std::move(fut));
  if (fut.IsReady()) {
    rsp_msg->msg = std::move(fut.GetValue0().msg);
    return TrpcRetCode::TRPC_INVOKE_SUCCESS;
  }

  auto exception = fut.GetException();
  TRPC_FMT_ERROR(exception.what());
  return exception.GetExceptionCode();
}

Future<CTransportRspMsg> FutureTransport::AsyncSendRecv(CTransportReqMsg* req_msg) {
  if (TRPC_UNLIKELY(!req_msg->extend_info)) {
    req_msg->extend_info = trpc::object_pool::MakeLwShared<trpc::ClientExtendInfo>();
  }

  if (req_msg->context->IsBackupRequest()) {
    return AsyncSendRecvForBackupRequest(req_msg);
  }

  if (!req_msg->context->GetFixedConnectorId()) {
    uint16_t id = SelectTransportAdapter(req_msg);
    return AsyncSendRecvImpl(req_msg, id);
  }

  uint16_t adapter_id = GetAdapterIdByConnectorId(req_msg->context->GetFixedConnectorId());
  uint16_t id = SelectTransportAdapter(req_msg, adapter_id);
  return AsyncSendRecvImpl(req_msg, id);
}

int FutureTransport::SendOnly(CTransportReqMsg* req_msg) {
  uint16_t id = 0;
  if (req_msg->context->GetFixedConnectorId() == 0) {
    id = SelectIOThread(req_msg);
  } else {
    id = GetAdapterIdByConnectorId(req_msg->context->GetFixedConnectorId());
  }

  bool ret = SendRequest(req_msg, id, TrpcCallType::TRPC_ONEWAY_CALL);
  if (!ret) trpc::object_pool::Delete(req_msg);

  return ret ? 0 : -1;
}

std::optional<uint64_t> FutureTransport::GetOrCreateConnector(const PreallocationOption& preallocate_option) {
  uint16_t index = SelectTransportAdapter();
  auto promise = Promise<uint64_t>();
  auto fut = promise.GetFuture();
  MsgTask* task = CreateAllocateTask(preallocate_option.node_addr, index, std::move(promise));
  if (IsSameIOThread(index)) {
    task->handler();
    object_pool::Delete<MsgTask>(task);
  } else {
    bool result = SubmitIoTask(task);
    if (!result) {
      return std::nullopt;
    }
  }

  auto res_fut = future::BlockingTryGet(std::move(fut), kCreateRealseConnectorTimeout);
  if (res_fut.has_value()) {
    if (res_fut->IsFailed()) {
      TRPC_LOG_ERROR("GetOrCreateConnector fail addr=" << preallocate_option.node_addr.ip << ":"
                                                       << preallocate_option.node_addr.port
                                                       << ", error_msg:" << res_fut->GetException().what());
      return std::nullopt;
    }

    uint64_t connector_id = res_fut->GetValue0();
    reinterpret_cast<FixedConnectorId*>(connector_id)->adapter_id = index;
    return connector_id;
  } else {
    TRPC_LOG_ERROR("GetOrCreateConnector Timeout addr=" << preallocate_option.node_addr.ip << ":"
                                                        << preallocate_option.node_addr.port);
    return std::nullopt;
  }
}

bool FutureTransport::ReleaseFixedConnector(uint64_t fixed_connector_id) {
  std::uint16_t adapter_id = GetAdapterIdByConnectorId(fixed_connector_id);
  Promise<bool> promise;
  auto fut = promise.GetFuture();
  MsgTask* task = CreateReleaseTask(fixed_connector_id, adapter_id, std::move(promise));
  if (IsSameIOThread(adapter_id)) {
    task->handler();
    object_pool::Delete<MsgTask>(task);
  } else {
    bool ret = SubmitIoTask(task);
    if (!ret) {
      return false;
    }
  }

  auto res_fut = future::BlockingTryGet(std::move(fut), kCreateRealseConnectorTimeout);
  if (res_fut.has_value()) {
    if (res_fut->IsFailed()) {
      TRPC_LOG_ERROR("ReleaseConnector fail fixed_connector_id=" << fixed_connector_id
                                                                 << ",error_msg:" << res_fut->GetException().what());
      return false;
    }

    return res_fut->GetValue0();
  } else {
    TRPC_LOG_ERROR("ReleaseConnector Timeout fixed_connector_id=" << fixed_connector_id);
    return false;
  }
}

void FutureTransport::Disconnect(const std::string& target_ip) {
  for (auto& adapter : adapters_) {
    adapter->Disconnect(target_ip);
  }
}

stream::StreamReaderWriterProviderPtr FutureTransport::CreateStream(const NodeAddr& addr,
                                                                    stream::StreamOptions&& stream_options) {
  auto current_thread = WorkerThread::GetCurrentWorkerThread();
  bool allowed = current_thread && (current_thread->Role() == kIoAndHandle);
  if (!allowed) {
    TRPC_FMT_ERROR("only merge threads can create stream");
    return nullptr;
  }

  TRPC_CHECK_EQ(options_.thread_model->Type(), kMerge);

  auto index = GetLogicId(current_thread);
  TRPC_CHECK_LT(index, adapters_.size());
  auto& adapter = adapters_[index];

  FutureConnectorGroup* group = nullptr;
  if (stream_options.connection_id) {
    auto ptr = reinterpret_cast<FixedConnectorId*>(stream_options.connection_id);
    group = reinterpret_cast<FutureConnectorGroup*>(ptr->connector_group);
  } else {
    group = adapter->GetConnectorGroup(addr);
    if (!group) {
      TRPC_FMT_ERROR("can't get connector group of {}:{}", addr.ip, addr.port);
      return nullptr;
    }
  }

  return group->CreateStream(std::move(stream_options));
}

uint16_t FutureTransport::SelectIOThread() {
  return io_index_++ % adapters_.size();
}

uint16_t FutureTransport::SelectIOThread(CTransportReqMsg* msg) {
  uint64_t dispatch_key = msg->context->GetRequestDispatchKey();
  if (dispatch_key == std::numeric_limits<std::size_t>::max() - 1) {
    return SelectIOThread();
  }

  return dispatch_key % adapters_.size();
}

uint16_t FutureTransport::GetLogicId(const WorkerThread* work_thread) {
  uint16_t id = work_thread->Id() & 0xFFFF;
  return id;
}

uint16_t FutureTransport::SelectTransportAdapter() {
  auto* current_thread = WorkerThread::GetCurrentWorkerThread();
  if (!current_thread) {
    return SelectIOThread();
  }

  uint16_t index = 0;
  uint16_t dst_thread_model = options_.thread_model->GroupId();
  if (current_thread->Role() != kHandle && dst_thread_model == current_thread->GroupId()) {
    index = GetLogicId(current_thread);
  } else {
    index = SelectIOThread();
  }

  return index;
}

uint16_t FutureTransport::SelectTransportAdapter(CTransportReqMsg* msg) {
  auto* current_thread = WorkerThread::GetCurrentWorkerThread();
  if (!current_thread) {
    msg->extend_info->dispatch_info.src_thread_id = -1;
    return SelectIOThread(msg);
  }

  uint16_t index = 0;
  uint16_t dst_thread_model = options_.thread_model->GroupId();
  if (current_thread->Role() != kHandle && dst_thread_model == current_thread->GroupId()) {
    index = GetLogicId(current_thread);
  } else {
    index = SelectIOThread(msg);
  }
  msg->extend_info->dispatch_info.src_thread_model_id = current_thread->GroupId();
  msg->extend_info->dispatch_info.src_thread_id = GetLogicId(current_thread);

  return index;
}

uint16_t FutureTransport::SelectTransportAdapter(CTransportReqMsg* msg, uint16_t index) {
  auto* current_thread = WorkerThread::GetCurrentWorkerThread();
  if (!current_thread) {
    msg->extend_info->dispatch_info.src_thread_id = -1;
    return index;
  }

  msg->extend_info->dispatch_info.src_thread_model_id = current_thread->GroupId();
  msg->extend_info->dispatch_info.src_thread_id = GetLogicId(current_thread);
  return index;
}

bool FutureTransport::IsSameIOThread(uint16_t index) {
  auto* current_thread = WorkerThread::GetCurrentWorkerThread();
  if (current_thread && current_thread->Role() != kHandle) {
    if ((index == GetLogicId(current_thread)) && (options_.thread_model->GroupId() == current_thread->GroupId())) {
      return true;
    }
  }

  return false;
}

bool FutureTransport::SendRequest(CTransportReqMsg* msg, uint16_t id, TrpcCallType call_type) {
  if (options_.trans_info.run_client_filters_function) {
    options_.trans_info.run_client_filters_function(FilterPoint::CLIENT_PRE_SCHED_SEND_MSG, msg);
  }

  MsgTask* task;
  if (!msg->context->GetFixedConnectorId()) {
    task = CreateTransportRequestTask(msg, id, call_type);
  } else {
    task = CreateFixedConnectorRequestTask(msg, id, call_type);
  }

  return SubmitIoTask(task);
}

std::vector<Future<CTransportRspMsg>> FutureTransport::SendBackupRequest(ClientContextPtr& msg_context,
                                                                         Future<CTransportRspMsg>&& res_fut_first,
                                                                         uint16_t id, bool is_blocking_invoke,
                                                                         NoncontiguousBuffer&& send_data) {
  auto* retry_info = msg_context->GetBackupRequestRetryInfo();
  retry_info->resend_count++;
  int backup_addr_size = retry_info->backup_addrs.size();
  std::vector<Future<CTransportRspMsg>> vecs;
  if (res_fut_first.IsFailed()) {
    vecs.reserve(backup_addr_size - 1);
  } else {
    vecs.reserve(backup_addr_size);
    vecs.emplace_back(std::move(res_fut_first));
  }

  for (int i = 1; i < backup_addr_size; i++) {
    auto* new_msg = trpc::object_pool::New<CTransportReqMsg>();
    new_msg->context = msg_context;
    // Set some fields of CTransportReqMsg.
    new_msg->context->SetAddr(retry_info->backup_addrs[i].addr.ip, retry_info->backup_addrs[i].addr.port);
    // Minus delay.
    new_msg->context->SetTimeout(msg_context->GetTimeout() - msg_context->GetBackupRequestRetryInfo()->delay);
    new_msg->extend_info = trpc::object_pool::MakeLwShared<trpc::ClientExtendInfo>();
    new_msg->extend_info->backup_promise = nullptr;
    new_msg->extend_info->is_blocking_invoke = is_blocking_invoke;

    // Last request just move.
    if (i != backup_addr_size - 1) {
      new_msg->send_data = send_data;
    } else {
      new_msg->send_data = std::move(send_data);
    }

    TRPC_FMT_DEBUG("resend request to peer:{}:{}", new_msg->context->GetIp(), new_msg->context->GetPort());

    uint16_t new_id = SelectTransportAdapter(new_msg, id);
    auto new_fut = AsyncSendRecvImpl(new_msg, new_id).Then([new_msg](Future<CTransportRspMsg>&& fut) {
      trpc::object_pool::Delete(new_msg);
      return std::move(fut);
    });

    vecs.emplace_back(std::move(new_fut));
  }

  return vecs;
}

Future<CTransportRspMsg> FutureTransport::AsyncSendRecvImpl(CTransportReqMsg* msg, uint16_t id) {
  auto fut = msg->extend_info->promise.GetFuture();
  bool ret = SendRequest(msg, id, TrpcCallType::TRPC_UNARY_CALL);
  if (ret) return fut;

  return MakeExceptionFuture<CTransportRspMsg>(
    CommonException("maybe io task queue is overload", TrpcRetCode::TRPC_CLIENT_OVERLOAD_ERR));
}

Future<CTransportRspMsg> FutureTransport::AsyncSendRecvForBackupRequest(CTransportReqMsg* msg) {
  Promise<CTransportRspMsg> final_promise;
  auto fut = final_promise.GetFuture();
  auto fut_first = msg->extend_info->promise.GetFuture();
  auto res_fut_first = fut_first.Then([msg](Future<CTransportRspMsg>&& fut) mutable {
    trpc::object_pool::Delete(msg);
    return std::move(fut);
  });

  auto* backup_promise = new Promise<bool>();
  auto backup_fut = backup_promise->GetFuture();
  msg->extend_info->backup_promise = backup_promise;

  uint16_t id = SelectTransportAdapter(msg);
  bool is_blocking_invoke = msg->extend_info->is_blocking_invoke;
  backup_fut.Then([this, id, msg_context = msg->context, send_data = msg->send_data, is_blocking_invoke,
                   final_promise = std::move(final_promise),
                   res_fut_first = std::move(res_fut_first)](Future<bool>&& fut) mutable {
    // Success.
    if (fut.IsReady()) {
      if (res_fut_first.IsReady()) {
        final_promise.SetValue(res_fut_first.GetValue0());
      } else {
        final_promise.SetException(res_fut_first.GetException());
      }

      return MakeReadyFuture<>();
    }

    bool first_failed = res_fut_first.IsFailed();
    auto vecs = SendBackupRequest(msg_context, std::move(res_fut_first), id, is_blocking_invoke, std::move(send_data));
    return WhenAnyWithoutException(vecs.begin(), vecs.end())
        .Then([msg_context, final_promise = std::move(final_promise),
               first_failed](Future<size_t, std::tuple<CTransportRspMsg>>&& fut) mutable {
          // Set value or exception.
          if (fut.IsReady()) {
            auto* retry_info = msg_context->GetBackupRequestRetryInfo();
            auto result = fut.GetValue();

            if (first_failed) {
              retry_info->succ_rsp_node_index = 1;
            } else if (retry_info->succ_rsp_node_index == 0) {
              retry_info->succ_rsp_node_index = std::get<0>(result);
            }

            final_promise.SetValue(std::move(std::get<1>(result)));
          } else {
            final_promise.SetException(fut.GetException());
          }

          return MakeReadyFuture<>();
        });
  });

  bool ret = SendRequest(msg, id, TrpcCallType::TRPC_UNARY_CALL);
  if (ret) {
    return fut;
  }

  // Task queue full.
  if (backup_promise) {
    Exception ex(CommonException("maybe io task queue is overload", TrpcRetCode::TRPC_CLIENT_OVERLOAD_ERR));
    msg->extend_info->promise.SetException(ex);

    static_cast<Promise<bool>*>(backup_promise)->SetValue(false);
    delete static_cast<Promise<bool>*>(backup_promise);
    backup_promise = nullptr;
  }

  return fut;
}

MsgTask* FutureTransport::CreateTransportRequestTask(CTransportReqMsg* msg,
                                                     uint16_t id, TrpcCallType call_type) {
  MsgTask* task = object_pool::New<MsgTask>();
  task->task_type = runtime::kRequestMsg;
  task->handler = [this, msg, id, call_type]() mutable {
    if (options_.trans_info.run_client_filters_function) {
      options_.trans_info.run_client_filters_function(FilterPoint::CLIENT_POST_SCHED_SEND_MSG, msg);
    }

    switch (call_type) {
      case TrpcCallType::TRPC_UNARY_CALL:
        {
          FutureConnectorGroup* connector_group = adapters_[id]->GetConnectorGroup(msg->context->GetNodeAddr());
          if (connector_group) {
            connector_group->SendReqMsg(msg);
          } else {
            std::string error_msg = "connector group resource allocate failure";
            future::DispatchException(msg, TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR, std::move(error_msg),
                                options_.trans_info.rsp_dispatch_function);
          }
        }
        break;
      case TrpcCallType::TRPC_ONEWAY_CALL:
        {
          FutureConnectorGroup* connector_group = adapters_[id]->GetConnectorGroup(msg->context->GetNodeAddr());
          if (connector_group) {
            connector_group->SendOnly(msg);
          } else {
            TRPC_FMT_ERROR("GetConnectorGroup failed, target: {}", msg->context->GetIp());
          }
          trpc::object_pool::Delete(msg);
        }
        break;
      default:
        TRPC_ASSERT(false && "no support yet");
        break;
    }
  };

  task->dst_thread_key = id;
  task->group_id = options_.thread_model->GroupId();
  return task;
}

MsgTask* FutureTransport::CreateFixedConnectorRequestTask(CTransportReqMsg* msg,
                                                          uint16_t id, TrpcCallType call_type) {
  MsgTask* task = object_pool::New<MsgTask>();
  task->task_type = runtime::kRequestMsg;
  task->handler = [this, msg, call_type]() mutable {
    TRPC_FMT_TRACE("call send func");
    if (options_.trans_info.run_client_filters_function) {
      options_.trans_info.run_client_filters_function(FilterPoint::CLIENT_POST_SCHED_SEND_MSG, msg);
    }

    FutureConnectorGroup* connector_group = nullptr;
    auto* ptr = reinterpret_cast<FixedConnectorId*>(msg->context->GetFixedConnectorId());
    connector_group = static_cast<FutureConnectorGroup*>(ptr->connector_group);

    switch (call_type) {
      case TrpcCallType::TRPC_UNARY_CALL:
        connector_group->SendReqMsg(msg);
        break;
      case TrpcCallType::TRPC_ONEWAY_CALL:
        connector_group->SendOnly(msg);
        trpc::object_pool::Delete(msg);
        break;
      default:
        TRPC_ASSERT(false && "no support yet");
        break;
    }
  };

  task->dst_thread_key = id;
  task->group_id = options_.thread_model->GroupId();
  return task;
}

MsgTask* FutureTransport::CreateAllocateTask(const NodeAddr& node_addr, uint16_t index, Promise<uint64_t>&& promise) {
  MsgTask* task = object_pool::New<MsgTask>();
  task->task_type = runtime::kRequestMsg;
  auto& trans_adapter = adapters_[index];
  task->handler = [&trans_adapter, promise = std::move(promise), node_addr]() mutable {
    bool ret = trans_adapter->GetOrCreateConnector(node_addr, promise);
    if (!ret) {
      std::string err = "create or get connector failed";
      promise.SetException(CommonException(err.c_str(), TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR));
    }
  };

  task->dst_thread_key = index;
  task->group_id = options_.thread_model->GroupId();
  return task;
}

MsgTask* FutureTransport::CreateReleaseTask(uint64_t fixed_connector_id, uint16_t index, Promise<bool>&& promise) {
  MsgTask* task = object_pool::New<MsgTask>();
  task->task_type = runtime::kRequestMsg;
  task->handler = [promise = std::move(promise), fixed_connector_id]() mutable {
    FixedConnectorId* ptr = reinterpret_cast<FixedConnectorId*>(fixed_connector_id);
    auto* connector_group = static_cast<FutureConnectorGroup*>(ptr->connector_group);
    bool result = connector_group->ReleaseConnector(fixed_connector_id);
    promise.SetValue(result);
  };

  task->dst_thread_key = index;
  task->group_id = options_.thread_model->GroupId();
  return task;
}

uint16_t FutureTransport::GetAdapterIdByConnectorId(uint64_t fixed_connetor_id) {
  return reinterpret_cast<FixedConnectorId*>(fixed_connetor_id)->adapter_id;
}

bool FutureTransport::SubmitIoTask(MsgTask* task) {
  if (merge_thread_model_) {
    return trpc::merge::SubmitIoTask(options_.thread_model, task);
  }

  return trpc::separate::SubmitIoTask(options_.thread_model, task);
}

}  // namespace trpc
