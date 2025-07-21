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

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#pragma once

#include "trpc/rpcz/span.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "trpc/rpcz/util/link_list.h"
#include "trpc/util/object_pool/object_pool.h"

namespace trpc::rpcz {

/// @brief Store admin request data.
/// @private
struct RpczAdminReqData {
  // Which span to query.
  int span_id = 0;
  // Size of query string para.
  int params_size;
};

/// @brief The type of span.
/// @private
enum class SpanType {
  // Used by server.
  kSpanTypeServer = 0,
  // Used by client.
  kSpanTypeClient,
  // Used by TRPC_RPCZ_PRINT.
  kSpanTypePrint,
  // Used by user-defined span.
  kSpanTypeUser,
};

/// @brief Phase type of a ViewerEvent.
/// @private
enum class PhaseType {
  // Begin phase.
  kPhaseTypeBegin = 0,
  // End phase.
  kPhaseTypeEnd,
};

/// @brief A complete user-defined span consists of a begin-type ViewerEvent and an end-type ViewerEvent.
/// @note Used by kSpanTypeUser span.
/// @private
struct ViewerEvent {
  // Begin or end.
  PhaseType type;
  // Begin timestamp or end timestamp, in microseconds.
  uint64_t timestamp_us;
  // Thread id in which event occured.
  int thread_id;
};

/// @brief One request per span.
class Span : public LinkNode<Span> {
 public:
  /// @brief Default constructor.
  Span() = default;

  /// @brief For span objects of type kSpanTypeUser, the user specifies the name when creating
  ///        either the root span object or a child span object.
  explicit Span(const std::string& viewer_name);

  /// @brief Default destructor.
  ~Span() = default;

  /// @brief Set trace id.
  /// @private
  void SetTraceId(uint32_t trace_id) { trace_id_ = trace_id; }

  /// @brief Set span id.
  /// @private
  void SetSpanId(uint32_t span_id) { span_id_ = span_id; }

  /// @brief Set timestamp to receive request.
  /// @param tm Unit of microsecond, from 1970-1-1:0:0:0.
  /// @private
  void SetReceivedRealUs(uint64_t tm) { received_real_us_ = tm + base_real_us_; }

  /// @brief Set call type.
  /// @param call_type See RpcCallType in protocol.h.
  /// @private
  void SetCallType(uint32_t call_type) { call_type_ = call_type; }

  /// @brief Set remote peer address.
  /// @param ip_port Ip:Port pair.
  /// @private
  void SetRemoteSide(const std::string& ip_port) { remote_endpoint_ = ip_port; }

  /// @brief Set codec name.
  /// @private
  void SetProtocolName(const std::string& codec_name) { codec_name_ = codec_name; }

  /// @brief Set request size.
  /// @private
  void SetRequestSize(uint32_t size) { request_size_ = size; }

  /// @brief Set type of span.
  /// @private
  void SetSpanType(const SpanType& span_type) { type_ = span_type; }

  /// @brief Set full method name.
  /// @private
  void SetFullMethodName(const std::string& full_method_name) { full_method_name_ = full_method_name; }

  /// @brief Set remote name.
  /// @private
  void SetRemoteName(const std::string& remote_name) { remote_name_ = remote_name; }

  /// @brief Triggered by TRPC_RPCZ_PRINT to append user log into current span.
  /// @private
  void AppendCustomLogs(const std::string& custom_log) { custom_logs_.append(custom_log); }

  /// @brief Append sub spans into current span.
  /// @private
  void AppendSubSpan(Span* sub_span) {
    std::lock_guard<std::mutex> lock(mutex_);
    sub_spans_.emplace_back(sub_span);
  }

  /// @brief Set timeout.
  /// @private
  void SetTimeout(uint32_t value) { timeout_ = value; }

  /// @brief Set base timestamp in microsecond, to adjust local timestamp to international.
  /// @private
  void SetBaseRealUs(uint64_t tm) { base_real_us_ = tm; }

  /// @brief Set timestamp into recv queue.
  /// @private
  void SetStartHandleRealUs(uint64_t tm) { start_handle_real_us_ = tm + base_real_us_; }

  /// @brief Set timestamp out of recv queue.
  /// @private
  void SetHandleRealUs(uint64_t tm) { handle_real_us_ = tm + base_real_us_; }

  /// @brief Set timestamp to invoke user callback.
  /// @private
  void SetStartCallbackRealUs(uint64_t tm) { start_callback_real_us_ = tm + base_real_us_; }

  /// @brief Set timestamp to finish invoke user callback.
  /// @private
  void SetCallbackRealUs(uint64_t tm) { callback_real_us_ = tm + base_real_us_; }

  /// @brief Set timestamp to start encode.
  /// @private
  void SetStartEncodeRealUs(uint64_t tm) { start_encode_real_us_ = tm + base_real_us_; }

  /// @brief Set timestamp into send queue.
  /// @private
  void SetStartSendRealUs(uint64_t tm) { start_send_real_us_ = tm + base_real_us_; }

  /// @brief Set timestamp out of send queue.
  /// @private
  void SetSendRealUs(uint64_t tm) { send_real_us_ = tm + base_real_us_; }

  /// @brief Set timestamp finishing writev, data into kernel.
  /// @private
  void SetSendDoneRealUs(uint64_t tm) { send_done_real_us_ = tm + base_real_us_; }

  /// @brief Set size of response.
  /// @private
  void SetResponseSize(uint32_t size) { response_size_ = size; }

  /// @brief Set return code of framework.
  /// @private
  void SetErrorCode(int error_code) { error_code_ = error_code; }

  /// @brief Set timestamp start to invoke rpc.
  /// @private
  void SetStartRpcInvokeRealUs(uint64_t tm) { start_rpc_invoke_real_us_ = tm + base_real_us_; }

  /// @brief Set timestamp start to invoke transport.
  /// @private
  void SetStartTransInvokeRealUs(uint64_t tm) { start_trans_invoke_real_us_ = tm + base_real_us_; }

  /// @brief Set timestamp finish transport invoke.
  /// @private
  void SetTransInvokeDoneRealUs(uint64_t tm) { trans_invoke_done_real_us_ = tm + base_real_us_; }

  /// @brief Set timestamp finish invoke rpc.
  /// @private
  void SetRpcInvokeDoneRealUs(uint64_t tm) { rpc_invoke_done_real_us_ = tm + base_real_us_; }

  /// @brief Set timestamp of first log.
  /// @private
  void SetFirstLogRealUs(uint64_t first_log_real_us) { first_log_real_us_ = first_log_real_us; }

  /// @brief Set timestamp of last log.
  /// @private
  void SetLastLogRealUs(uint64_t last_log_real_us) { last_log_real_us_ = last_log_real_us; }

  /// @brief Get trace id.
  /// @private
  uint32_t TraceId() const { return trace_id_; }

  /// @brief Get span id.
  /// @private
  uint32_t SpanId() const { return span_id_; }

  /// @brief Get timestamp to receive request.
  /// @private
  uint64_t ReceivedRealUs() const { return received_real_us_; }

  /// @brief Get call type.
  /// @private
  uint32_t CallType() const { return call_type_; }

  /// @brief Get remote peer address.
  /// @private
  const std::string& RemoteSide() const { return remote_endpoint_; }

  /// @brief Get codec name.
  /// @private
  const std::string& ProtocolName() const { return codec_name_; }

  /// @brief Get request size.
  /// @private
  uint32_t RequestSize() const { return request_size_; }

  /// @brief Get base timestamp in microsecond.
  /// @private
  uint32_t BaseRealUs() const { return base_real_us_; }

  /// @brief Get type of span.
  /// @private
  const SpanType& Type() const { return type_; }

  /// @brief Get full method name.
  /// @private
  const std::string& FullMethodName() const { return full_method_name_; }

  /// @brief Get remote name.
  /// @private
  const std::string& RemoteName() const { return remote_name_; }

  /// @brief Get timestamp into recv queue.
  /// @private
  uint64_t StartHandleRealUs() const { return start_handle_real_us_; }

  /// @brief Get timestamp out of recv queue.
  /// @private
  uint64_t HandleRealUs() const { return handle_real_us_; }

  /// @brief Get timestamp to invoke user callback.
  /// @private
  uint64_t StartCallbackRealUs() const { return start_callback_real_us_; }

  /// @brief Get timestamp to finish invoke user callback.
  /// @private
  uint64_t CallbackRealUs() const { return callback_real_us_; }

  /// @brief Get timestamp to start encode.
  /// @private
  uint64_t StartEncodeRealUs() const { return start_encode_real_us_; }

  /// @brief Get timestamp into send queue.
  /// @private
  uint64_t StartSendRealUs() const { return start_send_real_us_; }

  /// @brief Get timestamp out of send queue.
  /// @private
  uint64_t SendRealUs() const { return send_real_us_; }

  /// @brief Get timestamp finishing writev.
  /// @private
  uint64_t SendDoneRealUs() const { return send_done_real_us_; }

  /// @brief Get timestamp start to invoke rpc.
  /// @private
  uint64_t StartRpcInvokeRealUs() const { return start_rpc_invoke_real_us_; }

  /// @brief Get timestamp start to invoke transport.
  /// @private
  uint64_t StartTransInvokeRealUs() const { return start_trans_invoke_real_us_; }

  /// @brief Get timestamp finish transport invoke.
  /// @private
  uint64_t TransInvokeDoneRealUs() const { return trans_invoke_done_real_us_; }

  /// @brief Get timestamp finish invoke rpc.
  /// @private
  uint64_t RpcInvokeDoneRealUs() const { return rpc_invoke_done_real_us_; }

  /// @brief Get timestamp of first log.
  /// @private
  uint64_t FirstLogRealUs() const { return first_log_real_us_; }

  /// @brief Get timestamp of last log.
  /// @private
  uint64_t LastLogRealUs() const { return last_log_real_us_; }

  /// @brief For span objects of type kSpanTypeUser, set the display name.
  /// @private
  void SetViewerName(const std::string& viewer_name) { viewer_name_ = viewer_name; }

  /// @brief For span objects of type kSpanTypeUser, get the display name.
  /// @private
  const std::string& ViewerName() const { return viewer_name_; }

  /// @brief For span objects of type kSpanTypeUser, set begin phase ViewerEvent.
  /// @private
  void SetBeginViewerEvent(const ViewerEvent& begin_viewer_event) { begin_viewer_event_ = begin_viewer_event; }

  /// @brief For span objects of type kSpanTypeUser, get begin phase ViewerEvent.
  /// @private
  ViewerEvent BeginViewerEvent() const { return begin_viewer_event_; }

  /// @brief For span objects of type kSpanTypeUser, set end phase ViewerEvent.
  /// @private
  void SetEndViewerEvent(const ViewerEvent& end_viewer_event) { end_viewer_event_ = end_viewer_event; }

  /// @brief For span objects of type kSpanTypeUser, get end phase ViewerEvent.
  /// @private
  ViewerEvent EndViewerEvent() const { return end_viewer_event_; }

  /// @brief For span objects of type kSpanTypeUser, add business key-value information to the current
  ///        Span object without a timestamp attribute.
  /// @note The interface provided for user usage.
  void AddAttribute(std::string key, std::string value) {
    viewer_attributes_[std::move(key)] = std::move(value);
  }

  /// @brief For span objects of type kSpanTypeUser, retrieve all key-value attributes.
  /// @private
  const std::unordered_map<std::string, std::string>& Attributes() const {
    return viewer_attributes_;
  }

  /// @brief For span objects of type kSpanTypeUser, end the span object.
  /// @note The interface provided for user usage. Users must call this interface on the child span object.
  ///       Users can simply call the SubmitUserRpczSpan interface on the root span object.
  void End();


  /// @brief For span objects of type kSpanTypeUser, add a child span object to the current Span object.
  ///        And return the newly generated child span object to the user.
  /// @return Return nullptr if creation fails, otherwise return a pointer to the child span object.
  /// @note The interface provided for user usage. The framework is responsible for releasing the child span object.
  Span* CreateSubSpan(const std::string& viewer_name);

  /// @brief Get sub spans.
  /// @private
  const std::vector<Span*>& SubSpans() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sub_spans_;
  }

  /// @brief Get error code.
  /// @private
  int ErrorCode() const { return error_code_; }

  /// @brief Get size of response.
  /// @private
  uint32_t ResponseSize() const { return response_size_; }

  /// @brief Get custom logs appended by TRPC_RPCZ_PRINT.
  /// @private
  const std::string& CustomLogs() const { return custom_logs_; }

  /// @brief Get timeout in microsecond.
  /// @private
  uint32_t Timeout() const { return timeout_; }

  /// @brief To find span.
  /// @private
  bool operator==(const Span& span);

  /// @brief Display span info.
  /// @private
  void Display();

  /// @brief Transform server span into string.
  std::string ServerSpanToString();

  /// @brief Transform client span into string.
  std::string ClientSpanToString();

  /// @brief For span objects of type kSpanTypeUser, format the information for output.
  std::string UserSpanToString();

  /// @brief Convert the phase type to a string representation for display.
  /// @private
  std::string CastPhaseTypeToStr(const PhaseType& phase_type) const;

 private:
  /// @brief Transform call type into string.
  std::string CastCallTypeToStr(uint32_t call_type) const;

  /// @brief Transform span type into string.
  std::string CastSpanTypeToStr(const SpanType& span_type) const;

  /// @brief Transform sub span of client rpcz in route scenario into string.
  void FillRouteClientSpanInfo(const Span* sub_span, uint64_t last_log_us, std::ostream& span_info);

 private:
  uint32_t trace_id_;

  uint32_t span_id_;

  std::string remote_endpoint_;

  uint32_t call_type_;

  std::string codec_name_;

  SpanType type_;

  int error_code_;

  uint32_t request_size_;

  uint32_t response_size_;

  // Base timestamp in microsecond.
  uint64_t base_real_us_{0};

  // Timestamp recv request from kernel.
  uint64_t received_real_us_{0};

  // Timestamp before into recv queue.
  uint64_t start_handle_real_us_{0};

  // Timestamp out of recv queue.
  uint64_t handle_real_us_{0};

  // Timestamp before invoke user callback.
  uint64_t start_callback_real_us_{0};

  // Timestamp after invoke user callback.
  uint64_t callback_real_us_{0};

  // Timestamp before encode.
  uint64_t start_encode_real_us_{0};

  // Timestamp before into send queue.
  uint64_t start_send_real_us_{0};

  // Timestamp before send.
  uint64_t send_real_us_{0};

  // Timestamp to finish send.
  uint64_t send_done_real_us_{0};

  // Caller name.
  std::string remote_name_;

  // Format: remote_name.function_name.
  std::string full_method_name_;

  // Timestamp of span generation.
  uint64_t first_log_real_us_{0};

  // Store previous timestamp.
  uint64_t last_log_real_us_{0};

  // Store log generated by TRPC_RCPZ_PRINT.
  std::string custom_logs_;

  // Timestamp in filter point CLIENT_PRE_RPC_INVOKE.
  uint64_t start_rpc_invoke_real_us_{0};

  // Timestamp in filter point CLIENT_PRE_SEND_MSG.
  uint64_t start_trans_invoke_real_us_{0};

  // Timestamp in filter point CLIENT_POST_RECV_MSG.
  uint64_t trans_invoke_done_real_us_{0};

  // Timestamp in filter point CLIENT_POST_RPC_INVOKE.
  uint64_t rpc_invoke_done_real_us_{0};

  // Request timeout.
  uint32_t timeout_{0};

  // To store 1. client rpcz in route scenario, 2. span generated by TRPC_RCPZ_PRINT.
  // 3. If the span object is of type kSpanTypeUser, store the sub span objects.
  std::vector<Span*> sub_spans_;

  // To protect sub spans.
  mutable std::mutex mutex_;

  // For span objects of type kSpanTypeUser, used for external display.
  std::string viewer_name_;

  // For span objects of type kSpanTypeUser, the event at the begin phase.
  ViewerEvent begin_viewer_event_;

  // For span objects of type kSpanTypeUser, the event at the end phase.
  ViewerEvent end_viewer_event_;

  // For span objects of type kSpanTypeUser, the collection of attributes.
  std::unordered_map<std::string, std::string> viewer_attributes_;
};

/// @brief Compare two sapn by id.
/// @private
struct SpanIdSort {
  bool operator()(Span* s1, Span* s2) const { return s1->SpanId() < s2->SpanId(); }
};

/// @brief To sort span list before further handling.
/// @private
class SpanPreprocessor {
 public:
  /// @brief Get global unique SpanPreprocessor.
  static SpanPreprocessor* GetInstance() {
    static SpanPreprocessor instance;
    return &instance;
  }

  /// @brief Sort a list of span by span id.
  void process(std::vector<Span*>& list) {
    std::sort(list.begin(), list.end(), SpanIdSort());
  }
};

}  // namespace trpc::rpcz

namespace trpc::object_pool {

/// @brief Specialization for how to assign memory for span.
/// @private
template <>
struct ObjectPoolTraits<trpc::rpcz::Span> {
#if defined(TRPC_DISABLED_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kDisabled;
#elif defined(TRPC_SHARED_NOTHING_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
#else
  static constexpr auto kType = ObjectPoolType::kGlobal;
#endif
};

}  // namespace trpc::object_pool
#endif
