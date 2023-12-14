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

#pragma once

#include <algorithm>
#include <any>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "trpc/client/service_proxy_option.h"
#include "trpc/codec/client_codec.h"
#include "trpc/codec/protocol.h"
#include "trpc/common/status.h"
#include "trpc/compressor/compressor_type.h"
#include "trpc/filter/filter_point.h"
#include "trpc/naming/common/common_inc_deprecated.h"
#include "trpc/naming/common/constants.h"
#include "trpc/transport/client/retry_info_def.h"
#include "trpc/transport/common/transport_message_common.h"
#include "trpc/util/object_pool/object_pool_ptr.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

/// @brief Context for client-side rpc invoke, every request has its own context,
/// use `MakeClientContext` to create it.
/// @note It is not thread-safe.
/// It can not reused by multiple request.
class ClientContext : public RefCounted<ClientContext> {
 public:
  ClientContext() = default;

  /// @brief Constructs client context based on client codec.
  /// @note It is fast and recommended to use due to request protocol message object is created automatically.
  explicit ClientContext(const ClientCodecPtr& client_codec);

  ~ClientContext();

  /// @brief Get the request protocol message object.
  /// @note It is only used internally by the framework or used by filter plugins.
  ProtocolPtr& GetRequest() { return req_msg_; }

  /// @brief Set the request protocol message object.
  /// @note It is only used internally by the framework or used by filter plugins.
  void SetRequest(ProtocolPtr&& request) { req_msg_ = std::move(request); }
  void SetRequest(const ProtocolPtr& request) { req_msg_ = request; }

  /// @brief Get the response protocol message object.
  /// @note It is only used internally by the framework or used by filter plugins.
  ProtocolPtr& GetResponse() { return rsp_msg_; }

  /// @brief Set the response protocol message object.
  /// @note It is only used internally by the framework or used by filter plugins.
  void SetResponse(ProtocolPtr&& response) { rsp_msg_ = std::move(response); }
  void SetResponse(const ProtocolPtr& response) { rsp_msg_ = response; }

  /// @brief Framework use, or for testing. Set user request data struct,
  ///        eg: rpc request protobuf struct.
  void SetRequestData(void* req) { req_data_ = req; }

  /// @brief Get set user request data struct
  void* GetRequestData() { return req_data_; }

  /// @brief Framework use, or for testing. Set user response data struct,
  ///        eg: rpc response protobuf struct
  void SetResponseData(void* rsp) { rsp_data_ = rsp; }

  /// @brief Get set user response data struct
  void* GetResponseData() { return rsp_data_; }

  /// @brief Set request attachment
  void SetRequestAttachment(NoncontiguousBuffer&& attachment) {
    TRPC_ASSERT(req_msg_);
    req_msg_->SetProtocolAttachment(std::move(attachment));
  }

  /// @brief Get response attachment
  const NoncontiguousBuffer& GetResponseAttachment() { return rsp_attachment_; }

  /// @brief Set response attachment
  /// @note framework use or for testing
  void SetResponseAttachment(NoncontiguousBuffer&& attachment) {
    rsp_attachment_ = std::move(attachment);
  }

  /// @brief Get the status of RPC invoking result.
  const Status& GetStatus() const { return status_; }

  /// @brief Set the status of RPC invoking result.
  void SetStatus(const Status& status) { status_ = status; }
  void SetStatus(Status&& status) { status_ = std::move(status); }

  /// @brief Get the protocol name of target service which is requested by user, e.g. "trpc" or "http".
  std::string GetCodecName() const { return codec_->Name(); }

  //////////////////////////////////////////////////////////////////////////////

  /// @brief Get the unique id of the request.
  uint32_t GetRequestId() const { return invoke_info_.seq_id; }

  /// @brief Set the unique id of the request.
  void SetRequestId(uint32_t seq_id) {
    invoke_info_.seq_id = seq_id;
    SetStateFlag(true, kIsSetRequestId);
  }

  /// @brief Indicate whether the request ID has been set.
  /// @note It is only used internally by the framework.
  /// @private
  bool IsSetRequestId() { return GetStateFlag(kIsSetRequestId); }

  /// @brief Framework use, or for testing. Gets or sets stream id.
  /// @private
  uint32_t GetStreamId() const { return invoke_info_.stream_id; }

  /// @brief Framework use, or for testing. Set id of stream.
  /// @private 
  void SetStreamId(uint32_t stream_id) { invoke_info_.stream_id = stream_id; }

  /// @brief Get the timeout value for requesting remote service.
  uint32_t GetTimeout() { return invoke_info_.timeout; }

  /// @brief Set the timeout value for requesting remote service.
  void SetTimeout(uint32_t value, bool ignore_proxy_timeout = false) {
    invoke_info_.timeout = value;

    SetStateFlag(false, kIsUseFullLinkTimeoutMask);
    SetStateFlag(ignore_proxy_timeout, kIsIgnoreProxyTimeoutMask);
  }

  /// @brief Set the full-link timeout value for requesting remote service.
  /// @note It is only used internally by the framework or used by filter plugins.
  /// @private
  void SetFullLinkTimeout(uint32_t value) {
    if (value < invoke_info_.timeout) {
      invoke_info_.timeout = value;
    }
    SetStateFlag(true, kIsUseFullLinkTimeoutMask);
  }

  /// @brief Reports whether use the full-link timeout as the client requesting timeout.
  bool IsUseFullLinkTimeout() const { return GetStateFlag(kIsUseFullLinkTimeoutMask); }

  /// @brief Set whether to ignore the timeout configuration of service proxy.
  void SetIsIgnoreProxyTimeout(bool ignore_proxy) { SetStateFlag(ignore_proxy, kIsIgnoreProxyTimeoutMask); }

  /// @brief Get whether to ignore the timeout configuration of service proxy.
  bool IsIgnoreProxyTimeout() const { return GetStateFlag(kIsIgnoreProxyTimeoutMask); }

  /// @brief Get the caller name, it may look like: trpc.${app}.${server}.${service}
  std::string GetCallerName() const { return req_msg_->GetCallerName(); }

  /// @brief Set the caller name, it may look like: trpc.${app}.${server}.${service}
  void SetCallerName(std::string value) { req_msg_->SetCallerName(std::move(value)); }

  /// @brief Get the callee name, it may look like: trpc.${app}.${server}.${service}
  std::string GetCalleeName() const { return req_msg_->GetCalleeName(); }

  /// @brief Get the callee name, it may look like: trpc.${app}.${server}.${service}
  void SetCalleeName(std::string value) { req_msg_->SetCalleeName(std::move(value)); }

  /// @brief Get the function name for requesting remote service.
  std::string GetFuncName() const { return req_msg_->GetFuncName(); }

  /// @brief Set the function name for requesting remote service.
  void SetFuncName(std::string value) { req_msg_->SetFuncName(std::move(value)); }

  /// @brief Use GetFuncName instead. Get the function name for requesting remote service using trpc protocol.
  /// @private
  [[deprecated("Use GetFuncName instead")]] std::string GetTrpcFuncName() const { return GetFuncName(); }
  /// @brief Use SetFuncName instead. Set the function name for requesting remote service using trpc protocol.
  /// @private
  [[deprecated("Use SetFuncName instead")]] void SetTrpcFuncName(std::string value) { SetFuncName(std::move(value)); }

  /// @brief Get the function name of the caller for metrics reporting.
  /// @return Returns function name of the caller.
  std::string GetCallerFuncName() const { return invoke_info_.caller_func_name; }

  /// @brief Set the function name of the caller for metrics reporting.
  /// @param value is the function name of the caller.
  void SetCallerFuncName(std::string value) { invoke_info_.caller_func_name = std::move(value); }

  /// @brief Set the call type of the request.
  /// @note It is only used internally by the framework.
  /// @private
  uint8_t GetCallType() const { return invoke_info_.call_type; }

  /// @brief Set the call type of request.
  /// @note It is only used internally by the framework.
  /// @private
  void SetCallType(uint8_t value) { invoke_info_.call_type = value; }

  /// @brief Get the type of request message.
  /// @note It is only used internally by the framework.
  /// @private
  uint32_t GetMessageType() const { return invoke_info_.message_type; }

  /// @brief Set the type of request message.
  /// @note It is only used internally by the framework.
  /// @private
  void SetMessageType(uint32_t message_type) { invoke_info_.message_type = message_type; }

  /// @brief Get the content encoding type of request message.
  /// Protobuf, FlatBuffers, JSON, string are available here, default type is protobuf.
  uint8_t GetReqEncodeType() { return invoke_info_.req_encode_type; }

  /// @brief Set the content encoding type for specific request message.
  /// ProtoBuf, FlatBuffers, JSON, string are available here, default type is protobuf.
  /// @note Although the framework will select the corresponding content-encoding-type according to the request
  /// message type, it can only guarantee a general match. Please set the content-encoding-type explicitly if
  /// the type of request message is not the default ProtoBuf.
  void SetReqEncodeType(uint8_t type) { invoke_info_.req_encode_type = type; }

  /// @private 
  [[deprecated("Use SetReqEncodeType instead")]] void SetEncodeType(uint8_t type) { SetReqEncodeType(type); }

  /// @brief Get the content encoding type of request message object.
  /// Protobuf message, FlatBuffers message, rapidjson document, string object, NoncontiguousBuffer object
  /// are available here.
  /// @note It is only used internally by the framework.
  /// @private
  uint8_t GetReqEncodeDataType() { return invoke_info_.req_encode_data_type; }

  /// @brief Set the content encoding type of request message object.
  /// Protobuf message, FlatBuffers message, rapidjson document, string object, NoncontiguousBuffer object
  /// are available here.
  /// @note It is only used internally by the framework.
  /// @private
  void SetReqEncodeDataType(uint8_t type) { invoke_info_.req_encode_data_type = type; }

  /// @brief Get the content encoding type of response message.
  /// Protobuf, FlatBuffers, JSON, string are available here, default type is protobuf.
  uint8_t GetRspEncodeType() { return invoke_info_.rsp_encode_type; }

  /// @brief Set the content encoding type for specific response message.
  /// Protobuf, FlatBuffers, JSON, string are available here, default type is protobuf.
  /// @note Although the framework will select the corresponding content-encoding-type according to the request
  /// message type, it can only guarantee a general match. Please set the content-encoding-type explicitly if
  /// the type of request message is not the default Protobuf.
  void SetRspEncodeType(uint8_t type) { invoke_info_.rsp_encode_type = type; }

  /// @brief Set the content encoding type of response message object.
  /// Protobuf message, FlatBuffers message, rapidjson document, string object, NoncontiguousBuffer object
  /// are available here.
  /// @note It is only used internally by the framework.
  /// @private
  uint8_t GetRspEncodeDataType() { return invoke_info_.rsp_encode_data_type; }

  /// @brief Set the content encoding type of response message object.
  /// Protobuf message, FlatBuffers message, rapidjson document, string object, NoncontiguousBuffer object
  /// are available here.
  /// @note It is only used internally by the framework.
  /// @private
  void SetRspEncodeDataType(uint8_t type) { invoke_info_.rsp_encode_data_type = type; }

  /// @brief Set the compression type for compressing request message.
  void SetReqCompressType(uint8_t compress_type) {
    uint16_t tmp = invoke_info_.req_compress_info;
    invoke_info_.req_compress_info = (static_cast<uint16_t>(compress_type) << 8) | (tmp & 0xFF);
  }

  /// @brief Get the compression type for compressing request message.
  uint8_t GetReqCompressType() const { return static_cast<uint8_t>((invoke_info_.req_compress_info >> 8)); }

  /// @brief Set the compression level for compressing request message.
  void SetReqCompressLevel(uint8_t compress_level) {
    uint16_t tmp = invoke_info_.req_compress_info;
    invoke_info_.req_compress_info = (tmp & 0xFF00) | static_cast<uint16_t>(compress_level);
  }

  /// @brief Get the compression level for compressing request message.
  uint8_t GetReqCompressLevel() const { return static_cast<uint8_t>((invoke_info_.req_compress_info & 0xFF)); }

  /// @brief Set the compression type for decompressing response message.
  void SetRspCompressType(uint8_t compress_type) {
    uint16_t tmp = invoke_info_.rsp_compress_info;
    invoke_info_.rsp_compress_info = (static_cast<uint16_t>(compress_type) << 8) | (tmp & 0xFF);
  }

  /// @brief Get the compression type for decompressing response message.
  uint8_t GetRspCompressType() const { return static_cast<uint8_t>((invoke_info_.rsp_compress_info >> 8)); }

  /// @brief Set the compression level for decompressing response message.
  void SetRspCompressLevel(uint8_t compress_level) {
    uint16_t tmp = invoke_info_.rsp_compress_info;
    invoke_info_.rsp_compress_info = (tmp & 0xFF00) | static_cast<uint16_t>(compress_level);
  }

  /// @brief Get the compression level for decompressing response message.
  uint8_t GetRspCompressLevel() const { return static_cast<uint8_t>((invoke_info_.rsp_compress_info & 0xFF)); }

  /// @brief Get the const key-value pairs of request.
  const google::protobuf::Map<std::string, std::string>& GetPbReqTransInfo() const {
    return *(req_msg_->GetMutableKVInfos());
  }

  /// @brief Get the mutable key-value pairs of request.
  google::protobuf::Map<std::string, std::string>* GetMutablePbReqTransInfo() { return req_msg_->GetMutableKVInfos(); }

  /// @brief Set key-value pairs of request in batches.
  template <typename InputIt>
  void SetReqTransInfo(InputIt first, InputIt last) {
    InputIt it = first;
    while (it != last) {
      AddReqTransInfo(std::move(it->first), std::move(it->second));
      ++it;
    }
  }

  /// @brief Set key-value pair of request.
  void AddReqTransInfo(std::string key, std::string value) { req_msg_->SetKVInfo(std::move(key), std::move(value)); }

  /// @brief Get the const key-value pairs of response.
  const google::protobuf::Map<std::string, std::string>& GetPbRspTransInfo() const {
    return *(rsp_msg_->GetMutableKVInfos());
  }

  /// @brief Get the mutable key-value pairs of response.
  google::protobuf::Map<std::string, std::string>* GetMutablePbRspTransInfo() { return rsp_msg_->GetMutableKVInfos(); }

  /// @brief Set the key-value pairs of response in batches.
  template <typename InputIt>
  void SetRspTransInfo(const InputIt& first, const InputIt& last) {
    InputIt it = first;
    while (it != last) {
      AddRspTransInfo(it->first, it->second);
      ++it;
    }
  }

  /// @brief Set the key-value pair of response.
  void AddRspTransInfo(std::string key, std::string value) { rsp_msg_->SetKVInfo(std::move(key), std::move(value)); }

  /// @brief Get the bytes size long of request which is encoded into binary bytes.
  uint32_t GetRequestLength() const {
    if (req_msg_.get()) {
      return req_msg_->GetMessageSize();
    }

    return 0;
  }

  /// @brief Get the bytes size long of response which is encoded into binary bytes.
  uint32_t GetResponseLength() const {
    if (rsp_msg_.get()) {
      return rsp_msg_->GetMessageSize();
    }

    return 0;
  }
  //////////////////////////////////////////////////////////////////////////

  /// @brief Get the IP address of remote service instance.
  std::string GetIp() const { return endpoint_info_.addr.ip; }

  /// @brief Get the port of remote service instance.
  uint16_t GetPort() const { return endpoint_info_.addr.port; }

  /// @brief Reports whether IP address of remote service is IPv6.
  bool GetIsIpv6() const { return endpoint_info_.addr.addr_type == NodeAddr::AddrType::kIpV6; }

  /// @brief Get target endpoint's metadata.
  const std::unordered_map<std::string, std::string>& GetTargetMetadata() const { return endpoint_info_.metadata; }

  /// @brief Get target endponit's metadata by key.
  /// @return Return "" if no found, otherwise return the related value.
  std::string GetTargetMetadata(const std::string& key) const;

  /// @brief Indicates whether address of remote service instance is set by user.
  bool IsSetAddr() { return GetStateFlag(kIsSetAddrMask); }

  /// @brief Set the IP address and port of remote service instance.
  /// @note It is provided for user to set IP:Port for remote service instance.
  /// The request will be send directly to the address of remote service instance set by user after this function
  /// is called by user. The naming selector will be ignored.
  void SetAddr(const std::string& ip, uint16_t port) {
    endpoint_info_.addr.ip = ip;
    endpoint_info_.addr.port = port;
    endpoint_info_.addr.addr_type =
        ip.find(':') != std::string::npos ? NodeAddr::AddrType::kIpV6 : NodeAddr::AddrType::kIpV4;
    SetStateFlag(true, kIsSetAddrMask);
  }

  /// @brief Get address of the backend node instance.
  const NodeAddr& GetNodeAddr() const { return endpoint_info_.addr; }

  /// @private
  [[deprecated("Use SetBackupRequestAddrs instead")]] void SetMultiAddr(const std::vector<NodeAddr>& addrs) {
    SetBackupRequestAddrs(addrs);
  }

  /// @private
  [[deprecated("Use SetBackupRequestDelay instead")]] void SetRetryInfo(uint32_t delay, int times = 2) {
    SetBackupRequestDelay(delay);
  }

  /// @brief Set the IP address and port of remote service instance for backup requests.
  void SetBackupRequestAddrs(const std::vector<NodeAddr>& addrs) {
    TRPC_ASSERT(addrs.size() == 2 && "addrs size should be 2");
    TRPC_ASSERT(extend_info_.backup_req_msg_retry_info && "You should call 'SetBackupRequestDelay' first");
    for (auto& addr : addrs) {
      ExtendNodeAddr extend_addr;
      extend_addr.addr = addr;
      extend_info_.backup_req_msg_retry_info->backup_addrs.emplace_back(std::move(extend_addr));
    }
    SetStateFlag(true, kIsSetAddrMask);
  }

  /// @brief Set the interval between main request and backup request.
  /// @param delay is retry interval for backup requesting.
  void SetBackupRequestDelay(uint32_t delay) {
    if (!extend_info_.backup_req_msg_retry_info) {
      extend_info_.backup_req_msg_retry_info = object_pool::MakeLwShared<BackupRequestRetryInfo>();
    }
    extend_info_.backup_req_msg_retry_info->delay = delay;
    SetStateFlag(true, kIsBackupRequestMask);
  }

  /// @brief Indicates whether backup request is set by user.
  bool IsBackupRequest() const { return GetStateFlag(kIsBackupRequestMask); }

  /// @brief Get retry info of backup request.
  /// @note It's used internally by the framework.
  /// @private
  BackupRequestRetryInfo* GetBackupRequestRetryInfo() { return extend_info_.backup_req_msg_retry_info.Get(); }

  /// @brief Cancel a backup request, it will treat the request as a regular request.
  /// @note It's used internally by the framework.
  /// @private
  void CancelBackupRequest() {
    extend_info_.backup_req_msg_retry_info = nullptr;
    SetStateFlag(false, kIsBackupRequestMask);
  }

  /// @brief Set the addrs of remote service instance in backuprequest.
  /// @note It is only called by naming selector.
  void SetBackupRequestAddrsByNaming(std::vector<ExtendNodeAddr>&& addrs) {
    TRPC_ASSERT(addrs.size() == 2 && "addrs size should be 2");
    TRPC_ASSERT(extend_info_.backup_req_msg_retry_info);
    extend_info_.backup_req_msg_retry_info->backup_addrs = std::move(addrs);
  }

  /// @brief Set the addr of remote service instance.
  /// @note It is only called by naming selector.
  void SetRequestAddrByNaming(ExtendNodeAddr&& addr) {
    endpoint_info_.addr = std::move(addr.addr);
    endpoint_info_.metadata = std::move(addr.metadata);
  }

  /// @brief Get options of related service proxy.
  const ServiceProxyOption* GetServiceProxyOption() { return extend_info_.service_proxy_option; }

  /// @brief Set options of service proxy.
  /// @note It's used internally by the framework.
  /// @private
  void SetServiceProxyOption(ServiceProxyOption* option) { extend_info_.service_proxy_option = option; }

  /// @brief Set the name of remote service.
  /// @param target name of the remote service
  /// @note If the user sets the service target in the context, the name resolution will prioritize this target value.
  ///       Call SetCalleeName at the same time to make the callee name accurate.
  void SetServiceTarget(const std::string& target) { endpoint_info_.target_service = target; }
  void SetServiceTarget(std::string&& target) { endpoint_info_.target_service = std::move(target); }
  /// @brief Get the name of remote service.
  /// @note It's called when performing name resolution internally within the framework.
  const std::string& GetServiceTarget() {
    if (!endpoint_info_.target_service.empty()) {
      return endpoint_info_.target_service;
    }

    if (extend_info_.service_proxy_option != nullptr) {
      return extend_info_.service_proxy_option->target;
    }

    return endpoint_info_.target_service;
  }

  /// The following naming-related interfaces have been deprecated (and are currently being handled by the framework for
  /// compatibility). Please use the SetFilterData interface to set the specific information required by the plugin.
  //////////////////////////////////////////////////////////////////////////
  /// @brief Get the namespace of the service on the polaris naming service.
  /// @private
  [[deprecated]] const std::string& GetNamespace() {
    InitNamingExtendInfo();
    return naming_extend_select_info_->name_space;
  }

  /// @brief Set the namespace of the service on the polaris naming service.
  /// @deprecated It's deprecated, use selector plugin api interface instead.
  /// @note It is only used internally by the framework.
  /// @private
  [[deprecated]] void SetNamespace(const std::string& value) {
    InitNamingExtendInfo();
    naming_extend_select_info_->name_space = value;
  }

  /// @brief Get the set name of the callee service on the polaris naming service.
  /// @private
  [[deprecated]] const std::string& GetCalleeSetName() {
    InitNamingExtendInfo();
    return naming_extend_select_info_->callee_set_name;
  }

  /// @brief Set the set name of the callee service on the polaris naming service.
  /// @private
  [[deprecated]] void SetCalleeSetName(const std::string& value) {
    InitNamingExtendInfo();
    naming_extend_select_info_->callee_set_name = value;
  }

  /// @brief Set label of the canary routing on the polaris naming service.
  /// @private
  [[deprecated]] void SetCanary(const std::string& value) {
    InitNamingExtendInfo();
    naming_extend_select_info_->canary_label = value;
    AddReqTransInfo("trpc-canary", value);
  }

  /// @brief Get whether to enable set force on the polaris naming service.
  /// @private
  [[deprecated]] bool GetEnableSetForce() {
    InitNamingExtendInfo();
    return naming_extend_select_info_->enable_set_force;
  }

  /// @brief Set whether to enable set force on the polaris naming service.
  /// @private
  [[deprecated]] void SetEnableSetForce(bool value) {
    InitNamingExtendInfo();
    naming_extend_select_info_->enable_set_force = value;
  }

  /// @brief Get whether to select unhealthy routing nodes on the polaris naming service.
  /// @private
  [[deprecated]] bool GetIncludeUnHealthyEndpoints() {
    InitNamingExtendInfo();
    return naming_extend_select_info_->include_unhealthy;
  }

  /// @brief Set whether to select unhealthy routing nodes on the polaris naming service.
  /// @private
  [[deprecated]] void SetIncludeUnHealthyEndpoints(bool flag) {
    InitNamingExtendInfo();
    naming_extend_select_info_->include_unhealthy = flag;
  }

  /// @brief Get the metadata used to filter service instances on the polaris naming service.
  /// @private
  [[deprecated]] const std::map<std::string, std::string>* GetFilterMetadataOfNaming(NamingMetadataType type) {
    InitNamingExtendInfo();
    auto& meta = naming_extend_select_info_->metadata[static_cast<int>(type)];
    return meta.size() > 0 ? &meta : nullptr;
  }

  /// @brief Set the metadata used to filter service instances on the polaris naming service.
  /// @private
  [[deprecated]] void SetFilterMetadataOfNaming(const std::map<std::string, std::string>& meta,
                                                NamingMetadataType type) {
    InitNamingExtendInfo();
    naming_extend_select_info_->metadata[static_cast<int>(type)] = meta;
  }

  /// @brief Get the ReplicateIndex of the remote service in the current request on the polaris naming service.
  /// @private
  [[deprecated]] uint32_t GetReplicateIndex() {
    InitNamingExtendInfo();
    return naming_extend_select_info_->replicate_index;
  }

  /// @brief Set the ReplicateIndex of the remote service in the current request on the polaris naming service.
  /// @note If using the ring hash, the value should be a uint32_t integer.
  /// @private
  [[deprecated]] void SetReplicateIndex(uint32_t value) {
    InitNamingExtendInfo();
    naming_extend_select_info_->replicate_index = value;
  }

  //////////////////////////////////////////////////////////////////////////

  /// @brief Framework use, or for testing. Set the time point (us) when the current request starts invoking RPC call.
  void SetBeginTimestampUs(uint64_t value) { metrics_info_.begin_timestamp_us = value; }

  /// @brief Get the time point (us) when the current request starts invoking RPC call.
  /// The time accuracy is 100us level.
  uint64_t GetBeginTimestampUs() const { return metrics_info_.begin_timestamp_us; }

  /// @brief Framework use, or for testing. Set the time point (us) when the current request starts sending to network.
  void SetSendTimestampUs(uint64_t value) { metrics_info_.send_timestamp_us = value; }

  /// @brief Get the time point (us) when the current request starts sending to network.
  /// The time accuracy is 100us level.
  uint64_t GetSendTimestampUs() const { return metrics_info_.send_timestamp_us; }

  /// @brief Framework use, or for testing. Set the time point (us) when the response corresponding to the request is
  /// received from network.
  void SetRecvTimestampUs(uint64_t value) { metrics_info_.recv_timestamp_us = value; }

  /// @brief Get the time point (us) when the response corresponding to the request is received from network.
  /// The time accuracy is 100us level.
  uint64_t GetRecvTimestampUs() const { return metrics_info_.recv_timestamp_us; }

  /// @brief Framework use, or for testing. Set the time point (us) when the processing
  /// of request and response is complete.
  void SetEndTimestampUs(uint64_t value) { metrics_info_.end_timestamp_us = value; }

  /// @brief Get the time point (us) when the processing of request and response is complete.
  /// The time accuracy is 100us level.
  uint64_t GetEndTimestampUs() const { return metrics_info_.end_timestamp_us; }

  /// @brief Get the set name of the backend node instance.
  /// @private
  [[deprecated("Use GetTargetMetadata method instead")]] std::string GetInstanceSetName() const {
    return GetTargetMetadata(naming::kNodeSetName);
  }

  /// @brief Get the container name of the backend node instance.
  /// @private
  [[deprecated("Use GetTargetMetadata method instead")]] std::string GetContainerName() const {
    return GetTargetMetadata(naming::kNodeContainerName);
  }

  //////////////////////////////////////////////////////////////////////////

  /// @brief Get the input key for hashing to select the instances of remote service.
  /// @note It is only called by naming selector plugin.
  const std::string& GetHashKey() const { return extend_info_.hash_key; }

  /// @brief Set the input key for hashing to select the instances of remote service.
  /// @note The value of hash key is a integer (uint64_t) if it is used as input of hash modulo algorithm.
  void SetHashKey(std::string value) { extend_info_.hash_key = std::move(value); }

  /// @brief Reports whether it is a dyeing request message.
  bool IsDyeingMessage() const;

  /// @brief Get the dyeing key of the current request.
  std::string GetDyeingKey() const;

  /// @brief Get the value corresponding to the dyeing key.
  std::string GetDyeingKey(const std::string& key) const;

  /// @brief Set the dyeing key of the current request.
  void SetDyeingKey(const std::string& key);

  /// @brief Set the dyeing key-value pair of the current request.
  void SetDyeingKey(const std::string& key, const std::string& value);

  /// @brief Set the data structure associated with a specific filter.
  /// @param id The id that shared between the filter id and the plugin id.
  /// tRPC-CPP ensures that the plugin id starts growing from 65535, and each filter has its own unique id.
  /// It is recommended to start growing from 0 and assign no more than 65535
  /// @param filter_data The data structure for the filter.
  template <typename T>
  void SetFilterData(uint32_t id, T&& filter_data) {
    std::any data = std::forward<T>(filter_data);
    extend_info_.filter_data.emplace(std::make_pair(id, std::move(data)));
  }

  /// @brief Get the data structure associated with a specific filter.
  /// @param id The id that shared between the filter id and the plugin id.
  /// tRPC-CPP ensures that the plugin id starts growing from 65535, and each filter has its own unique id.
  /// It is recommended to start growing from 0 and assign no more than 65535
  /// @return The data structure for the filter.
  template <typename T>
  T* GetFilterData(uint32_t id) {
    auto it = extend_info_.filter_data.find(id);
    if (it != extend_info_.filter_data.end()) {
      return std::any_cast<T>(&(it->second));
    }
    return nullptr;
  }

  /// @brief To set which filter has been executed for a certain pre-tracking point.
  /// @param point filter point
  /// @param index the index of the filter currently being executed
  /// @note It is only used internally by the framework.
  /// @private
  void SetFilterExecIndex(FilterPoint point, int index) {
    // max filter number is limited to 127
    TRPC_ASSERT(index <= std::numeric_limits<int8_t>::max());
    extend_info_.filter_exec_index[(static_cast<int>(point)) >> 1] = index;
  }

  /// @brief Get the index of the filter currently being executed.
  /// @param point filter
  /// @note It is only used internally by the framework.
  /// @private
  int GetFilterExecIndex(FilterPoint point) { return extend_info_.filter_exec_index[(static_cast<int>(point)) >> 1]; }

  /// @brief Get the filter data for all filter Settings
  std::unordered_map<uint32_t, std::any>& GetAllFilterData() { return extend_info_.filter_data; }

  /// @brief Set the number of requests under the pipeline.
  /// @note Currently, this is only necessary when using the pipeline feature for the Redis protocol.
  void SetPipelineCount(uint32_t value) { extend_info_.request_merge_count = value; }

  /// @brief Get the number of requests under the pipeline.
  uint32_t GetPipelineCount() { return extend_info_.request_merge_count; }

  /// @brief Set the current request as transparent forwarding request.
  void SetTransparent(bool value) { SetStateFlag(value, kIsTransparentMask); }

  /// @brief Indicates whether it is a transparent forwarding request.
  bool IsTransparent() const { return GetStateFlag(kIsTransparentMask); }

  /// @brief Binds the request with the fixed connector, so the request will be send over expected connection.
  /// @param[in] fixed_connector_id is the ID of connector.
  void SetFixedConnectorId(uint64_t fixed_connector_id) {
    // Checks: the value 'fixed_connector_id' must be greater than zero.
    if (fixed_connector_id > 0) {
      extend_info_.fixed_connector_id = fixed_connector_id;
      SetStateFlag(true, kIsFixedConnectorMask);
    }
  }

  /// @brief Get the id of fixed connector
  uint64_t GetFixedConnectorId() { return extend_info_.fixed_connector_id; }

  /// @brief Indicates whether the request will be send over fixed connector.
  bool IsUseFixedConnector() const { return GetStateFlag(kIsFixedConnectorMask); }

  /// @brief Get the descriptor of the request method.
  /// @note If the descriptor returned by the function "GetprotobufMethodDescriptor()" is a nullptr, the descriptor
  /// of method will be set when stub code is executed which was generated by code generator tools (xx.trpc.pb.h).
  /// It can be used to obtain the options for RPC invoking.
  const google::protobuf::MethodDescriptor* GetProtobufMethodDescriptor() const { return invoke_info_.method_desc; }

  /// @brief Set the descriptor of the request method.
  /// @note If the descriptor returned by the function "GetProtobufMethodDescriptor()" is a nullptr, the descriptor
  /// of method will be set when stub code is executed which was generated by code generator tools (xx.trpc.pb.h).
  /// It can be used to obtain the options for RPC invoking.
  void SetProtobufMethodDescriptor(const google::protobuf::MethodDescriptor* method_desc) {
    invoke_info_.method_desc = method_desc;
  }

  /// @brief Set the key of Dispatching request, which is used for
  /// which io thread (separate/merge mode) or which fiber scheduling group (fiber) to handling.
  void SetRequestDispatchKey(uint64_t key) { extend_info_.req_dispatch_key = key; }
  /// @brief Get the key of Dispatching request.
  /// @private
  uint64_t GetRequestDispatchKey() const { return extend_info_.req_dispatch_key; }

  /// @brief Set the extra context options which will be used in transport.
  /// @private
  void SetCurrentContextExt(uint32_t context_ext) { extend_info_.context_ext = context_ext; }
  /// @brief Get the extra context options which will be used in transport.
  /// @private
  uint32_t GetCurrentContextExt() const { return extend_info_.context_ext; }

  /// @brief Sets custom headers when using the HTTP protocol.
  void SetHttpHeader(std::string key, std::string value) { req_msg_->SetKVInfo(std::move(key), std::move(value)); }

  /// @brief Gets custom header when using the HTTP protocol.
  const std::string& GetHttpHeader(const std::string& key) { return (*req_msg_->GetMutableKVInfos())[key]; }

  /// @brief Get all custom headers set when using the HTTP protocol.
  const auto& GetHttpHeaders() { return req_msg_->GetKVInfos(); }

 private:
  void InitNamingExtendInfo() {
    if (naming_extend_select_info_ == nullptr) {
      naming_extend_select_info_ = object_pool::New<NamingExtendSelectInfo>();
    }
  }

  // Set the flags of context state.
  void SetStateFlag(bool flag, uint8_t mask) {
    if (flag) {
      invoke_info_.state_flag_ |= mask;
    } else {
      invoke_info_.state_flag_ &= ~mask;
    }
  }

  // Get the flags of context state.
  bool GetStateFlag(uint8_t mask) const { return invoke_info_.state_flag_ & mask; }

 private:
  static constexpr uint8_t kIsUseFullLinkTimeoutMask = 0b00000001;
  static constexpr uint8_t kIsSetAddrMask = 0b00000010;
  static constexpr uint8_t kIsTransparentMask = 0b00000100;
  static constexpr uint8_t kIsFixedConnectorMask = 0b00001000;
  static constexpr uint8_t kIsBackupRequestMask = 0b00010000;
  static constexpr uint8_t kIsIgnoreProxyTimeoutMask = 0b00100000;
  static constexpr uint8_t kIsSetRequestId = 0b01000000;

  struct alignas(8) InvokeInfo {
    // Unique ID of request.
    uint32_t seq_id = 0;

    // Stream id.
    uint32_t stream_id = 0;

    // Timeout (ms) for requesting.
    uint32_t timeout = UINT32_MAX;

    // Type of RPC call.
    uint8_t call_type = TRPC_UNARY_CALL;

    // Bit flags for context state, the bit flag from low to high is as following:
    // 1: kIsUseFullLinkTimeoutMask, indicates whether full-link timeout is used by user.
    // 2: kIsSetAddrMask, indicates whether IP:Port is set by user.
    // 3: kIsTransparentMask, indicates whether it is a transparent forwarding request.
    // 4: kIsFixedConnectorMask, indicates whether request will be sending over fixed connector.
    // 5: kIsBackupRequestMask, indicates whether backup-request is used by user.
    // 6: kIsIgnoreProxyTimeoutMask, indicates whether to ignore timeout option of proxy.
    // 7: kIsSetRequestId, used to indicate whether the request ID has been set.
    // 8: reserved.
    uint8_t state_flag_ = 0b00000000;

    // Type of message.
    uint32_t message_type{0};

    // Type of content-encoding for request message, protobuf is the default (value: 0).
    uint8_t req_encode_type{0};

    // Type of content-encoding for request message object, protobuf message is the default (value: 0).
    // The framework will select the corresponding content-encoding-type according to the request
    // message type. The value set by user will be ignored.
    uint8_t req_encode_data_type{0};

    // Type of content-encoding for response message, protobuf is the default (value: 0).
    uint8_t rsp_encode_type{0};

    // Type of content-encoding for request message object, protobuf message is the default.
    // The framework will select the corresponding content-encoding-type according to the response
    // message type. the settings set by user will be ignored.
    uint8_t rsp_encode_data_type{0};

    // Compression settings of request: compression type (high 8 bits) + compression level (low 8 bits).
    // NONE:DEFAULT is the default.
    uint16_t req_compress_info{static_cast<uint16_t>(compressor::kNone << 8) | compressor::kDefault};

    // Compression settings of response: compression type (high 8 bits) + compression level (low 8 bits).
    // NONE:DEFAULT is the default.
    uint16_t rsp_compress_info{static_cast<uint16_t>(compressor::kNone << 8) | compressor::kDefault};

    // The function name of the caller who issues an RPC call.
    std::string caller_func_name;

    // Method descriptor used internally within protobuf.
    const google::protobuf::MethodDescriptor* method_desc{nullptr};
  };

  struct alignas(8) MetricsInfo {
    // Time point (us) when the request starts invoking RPC call.
    uint64_t begin_timestamp_us = 0;

    // Time point to (us) when the request starts sending to network.
    uint64_t send_timestamp_us = 0;

    // Time point (us) when the response is received from transport.
    uint64_t recv_timestamp_us = 0;

    // Time point when the processing of request and response is complete.
    uint64_t end_timestamp_us = 0;
  };

  struct alignas(8) ExtendInfo {
    // option of service proxy
    ServiceProxyOption* service_proxy_option = nullptr;

    // Input key for hashing to select the instances of remote service. It is set by user.
    std::string hash_key;

    // Merge request count in pipeline mode.
    uint32_t request_merge_count = 1;

    // Extra context options which will be used in transport.
    uint32_t context_ext = 0;

    /// Which io thread or fiber scheduling group is used to handle the request
    uint64_t req_dispatch_key = std::numeric_limits<std::size_t>::max() - 1;

    // Fixed connection ID which is used to fix data transmission and reception to a specific connection.
    // Is set to 0 by default, indicating no fixed connection is specified
    uint64_t fixed_connector_id = 0;

    // Parameters used by backup request.
    object_pool::LwSharedPtr<BackupRequestRetryInfo> backup_req_msg_retry_info;

    // Key-value pairs used by filter plugins.
    // Key: ID of filter, value: parameters used by filter.
    std::unordered_map<uint32_t, std::any> filter_data;

    // This is used to store the index of the filter that has been executed for a certain pre-tracking point.
    // The initial value is -1, which indicates that no filter has been executed.
    int8_t filter_exec_index[kFilterTypeNum / 2] = {-1, -1, -1, -1, -1};

    // Anything defined by user, it is associated with the current request.
    std::any user_data;
  };

  struct alignas(8) EndpointInfo {
    // IP:Port info of remote service instance.
    NodeAddr addr;

    // Metadata info of endpoint
    std::unordered_map<std::string, std::string> metadata;

    // Name of remote service
    std::string target_service;
  };

 private:
  ClientCodecPtr codec_{nullptr};

  ProtocolPtr req_msg_{nullptr};

  ProtocolPtr rsp_msg_{nullptr};

  // user request data, eg: rpc request protobuf struct
  void* req_data_{nullptr};

  // user response data, eg: rpc request protobuf struct
  void* rsp_data_{nullptr};

  // response attachment data
  NoncontiguousBuffer rsp_attachment_;

  Status status_;

  InvokeInfo invoke_info_;

  EndpointInfo endpoint_info_;

  MetricsInfo metrics_info_;

  ExtendInfo extend_info_;

  // Configuration of the naming plugin, used to be compatible with old versions of naming-related interfaces.
  // Use SetFilterData method to set the information required by the specific plugin.
  NamingExtendSelectInfo* naming_extend_select_info_{nullptr};
};

using ClientContextPtr = RefPtr<ClientContext>;

template <typename T>
using is_client_context = std::is_same<T, ClientContext>;

}  // namespace trpc
