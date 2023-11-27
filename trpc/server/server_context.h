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

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef TRPC_PROTO_USE_ARENA
#include "google/protobuf/arena.h"
#endif
#include "rapidjson/document.h"

#include "trpc/codec/protocol.h"
#include "trpc/codec/server_codec.h"
#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/common/status.h"
#include "trpc/compressor/compressor_type.h"
#include "trpc/filter/server_filter_controller.h"
#include "trpc/serialization/serialization_type.h"
#include "trpc/server/method_handler.h"
#include "trpc/stream/stream_provider.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/flatbuffers/message_fbs.h"

namespace trpc {

class Service;

/// @brief Context class for server-side request processing, use `MakeServerContext` to create it.
/// @note  It is not thread-safe.
///        It can not reused by multiple request.
/// @note  The requested statistical information will be increased or decreased
///        concurrently in the constructor and destructor of the class.
///        it should be noted that if the user saves the context,
///        concurrent current limiting cannot be used.
class ServerContext : public RefCounted<ServerContext> {
 public:
  /// requested network type
  enum class NetType : uint8_t {
    kTcp,
    kUdp,
    kOther,
  };

 public:
  ServerContext();

  ~ServerContext();

  //////////////////////////////////////////////////////////////////////////

  /// @brief Framework use or for testing. Set the time(us) when the current request is received from the network.
  void SetRecvTimestampUs(uint64_t value) { metrics_info_.recv_timestamp_us = value; }

  /// @brief Get the time(us) when the current request is received from the network.
  /// @note  Time accuracy is 100us level.
  uint64_t GetRecvTimestampUs() const { return metrics_info_.recv_timestamp_us; }

  /// @brief Framework use or for testing. Set the time(us) when the current request starts processing.
  ///        This time point is before user data (rpc data) decoding.
  void SetBeginTimestampUs(uint64_t value) { metrics_info_.begin_timestamp_us = value; }

  /// @brief Get the time(us) when the current request starts processing (before user data is decoded).
  /// @note  time accuracy is 100us level.
  uint64_t GetBeginTimestampUs() const { return metrics_info_.begin_timestamp_us; }

  /// @brief Framework use or for testing. Set the time(us) when the current request processing is completed.
  ///        This time point is after the rpc head + body data encoding is completed.
  void SetEndTimestampUs(uint64_t value) { metrics_info_.end_timestamp_us = value; }

  /// @brief Get the time(us) when the current request processing is completed.
  /// @note  Time accuracy is 100us level.
  uint64_t GetEndTimestampUs() const { return metrics_info_.end_timestamp_us; }

  /// @brief Framework use or for testing. Set the time(us) when the response data corresponding to the current request
  ///        is sent. This time point is after calling the transport's sending data interface,
  ///        and the data may not be sent to the network at this time.
  void SetSendTimestampUs(uint64_t value) { metrics_info_.send_timestamp_us = value; }

  /// @brief Get the time(us) when the response data corresponding to the current request is sent.
  /// @note  time accuracy is 100us level.
  uint64_t GetSendTimestampUs() const { return metrics_info_.send_timestamp_us; }

  /// @brief Get the time when a request is received, in milliseconds.
  /// @note For the purpose of measuring the elapsed time of the main framework process.
  inline uint64_t GetRecvTimestamp() const { return GetRecvTimestampUs() / 1000; }

  /// @brief Get the time when a request starts to be processed, in milliseconds.
  inline uint64_t GetBeginTimestamp() const { return GetBeginTimestampUs() / 1000; }

  //////////////////////////////////////////////////////////////////////////

  /// @brief Get the network type of the current request.
  NetType GetNetType() const { return net_info_.type; }

  /// @brief Framework use or for testing. Set the network type of the current request.
  void SetNetType(NetType type) { net_info_.type = type; }

  /// @brief Get the network connection id of the current request.
  /// @private
  uint64_t GetConnectionId() const { return net_info_.connection_id; }

  /// @brief Framework use or for testing. Get the network connection id of the current request.
  /// @private
  void SetConnectionId(uint64_t conn_id) { net_info_.connection_id = conn_id; }

  /// @brief Get the peer ip of the current request.
  std::string GetIp() const { return net_info_.ip; }

  /// @brief Framework use or for testing. Set the peer ip of the current request.
  void SetIp(std::string&& ip) { net_info_.ip = std::move(ip); }

  /// @brief Get the peer port of the current request.
  uint16_t GetPort() const { return net_info_.port; }

  /// @brief Framework use or for testing. Set the peer port of the current request.
  void SetPort(int port) { net_info_.port = port; }

  /// @brief Get the socket fd of the current request.
  int GetFd() const { return net_info_.fd; }

  /// @brief Framework use or for testing. Set the socket fd of the current request.
  void SetFd(int fd) { net_info_.fd = fd; }

  /// @brief Framework use or for testing. Get reserved data.
  /// @private
  void* GetReserved() { return net_info_.reserved; }

  /// @brief Framework use or for testing. Set reserved data.
  /// @private
  void SetReserved(void* ptr) { net_info_.reserved = ptr; }

  //////////////////////////////////////////////////////////////////////////

  /// @brief Framework use or for testing. Set the service which the current request belongs to.
  void SetService(Service* service) { service_ = service; }

  /// @brief Get the service which the current request belongs to.
  Service* GetService() { return service_; }

  /// @brief Framework use or for testing. Set the codec which the current request use.
  void SetServerCodec(ServerCodec* codec) { codec_ = codec; }

  /// @brief Get the codec which the current request use.
  ServerCodec* GetServerCodec() { return codec_; }

  /// @brief Framework use or for testing. Set filter controller.
  void SetFilterController(ServerFilterController* filter_controller) {
    extend_info_.filter_controller = filter_controller;
  }

  /// @brief Get filter controller.
  ServerFilterController& GetFilterController() {
    TRPC_ASSERT(extend_info_.filter_controller);
    return *extend_info_.filter_controller;
  }

  /// @brief Check if the FilterController has been set.
  bool HasFilterController() { return extend_info_.filter_controller != nullptr; }

  /// @brief Framework use only. Set which filter has been executed for a certain pre-tracking point.
  /// @param point filter point
  /// @param index the index of the filter currently being executed
  /// @private
  void SetFilterExecIndex(FilterPoint point, int index) {
    // max filter number is limited to 127
    TRPC_ASSERT(index <= std::numeric_limits<int8_t>::max());
    extend_info_.filter_exec_index[(static_cast<int>(point) & kServerFilterMask) >> 1] = index;
  }

  /// @brief Framework use only. Get the index of the filter currently being executed.
  /// @param point filter point
  /// @return the index of the filter currently being executed
  /// @note It is only used internally by the framework
  /// @private
  int GetFilterExecIndex(FilterPoint point) {
    return extend_info_.filter_exec_index[(static_cast<int>(point) & kServerFilterMask) >> 1];
  }

  //////////////////////////////////////////////////////////////////////////

  /// @brief Framework use or for testing. Set the protocol request message for the current request.
  void SetRequestMsg(ProtocolPtr&& req) { req_msg_ = std::move(req); }

  /// @brief Get the protocol request message for the current request.
  ProtocolPtr& GetRequestMsg() { return req_msg_; }

  /// @brief Framework use or for testing. Set the protocol response message for the current request.
  void SetResponseMsg(ProtocolPtr&& rsp) { rsp_msg_ = std::move(rsp); }

  /// @brief Get the protocol response message for the current request.
  ProtocolPtr& GetResponseMsg() { return rsp_msg_; }

  /// @brief Framework use or for testing. Set user request data struct.
  ///        eg: rpc request protobuf struct after decode.
  void SetRequestData(void* req) { req_data_ = req; }

  /// @brief Get set user request data struct
  void* GetRequestData() { return req_data_; }

  /// @brief Framework use or for testing. Set user response data struct.
  ///        eg: rpc response protobuf struct after decode.
  void SetResponseData(void* rsp) { rsp_data_ = rsp; }

  /// @brief Get set user response data struct.
  void* GetResponseData() { return rsp_data_; }

#ifdef TRPC_PROTO_USE_ARENA
  /// @brief Framework use or for testing. Set request pb-arena.
  ///        if use pb arena, req_data_ is create by req_arena_.
  /// @private
  void SetReqArenaObj(google::protobuf::Arena* req_arena) { req_arena_ = req_arena; }

  /// @brief Framework use or for testing. Get request pb-arena.
  /// @private
  google::protobuf::Arena* GetReqArenaObj() { return req_arena_; }

  /// @brief Framework use or for testing. Set response pb-arena.
  ///        if use pb arena, rsp_data_ is create by rsp_arena_.
  /// @private
  void SetRspArenaObj(google::protobuf::Arena* rsp_arena) { rsp_arena_ = rsp_arena; }

  /// @brief Framework use or for testing. Get response pb-arena.
  /// @private
  google::protobuf::Arena* GetRspArenaObj() { return rsp_arena_; }
#endif

  /// @brief Framework use or for testing. Set rpc method_handler to destroy request/response data and arena object.
  /// @private
  void SetRpcMethodHandler(RpcMethodHandlerInterface* method_handler) { rpc_method_handler_ = method_handler; }

  /// @brief Framework use or for testing. Get rpc method_handler.
  /// @private
  RpcMethodHandlerInterface* GetRpcMethodHandler() { return rpc_method_handler_; }

  /// @brief Get request attachment data.
  const NoncontiguousBuffer& GetRequestAttachment() { return req_attachment_; }

  /// @brief Framework use or for testing. Set request attachment data.
  void SetRequestAttachment(NoncontiguousBuffer&& attachment) { req_attachment_ = std::move(attachment); }

  /// @brief Set response attachment data.
  void SetResponseAttachment(NoncontiguousBuffer&& attachment) {
    TRPC_ASSERT(rsp_msg_);
    rsp_msg_->SetProtocolAttachment(std::move(attachment));
  }

  /// @brief Get the last execution result of the current request.
  const Status& GetStatus() const { return status_; }
  Status& GetStatus() { return status_; }

  /// @brief Set the last execution result of the current request.
  void SetStatus(const Status& status) { status_ = status; }
  void SetStatus(Status&& status) { status_ = std::move(status); }

  //////////////////////////////////////////////////////////////////////////

  /// @brief Get Stream id.
  /// @private
  uint32_t GetStreamId() const { return invoke_info_.stream_id; }
  /// @brief Framework use or for testing. Set Stream id.
  /// @private
  uint32_t SetStreamId(uint32_t stream_id) { return invoke_info_.stream_id = stream_id; }

  /// @brief Get codec name.
  std::string GetCodecName() const { return codec_->Name(); }

  /// @brief Get the timeout of the current request.
  uint32_t GetTimeout() const { return invoke_info_.timeout; }

  /// @brief Framework use or for testing. Set the timeout of the current request.
  void SetTimeout(uint32_t timeout) {
    if (timeout > 0) {
      invoke_info_.timeout = timeout;
    }
  }

  /// @brief Whether the server timeout time is the full link timeout time.
  bool IsUseFullLinkTimeout() const { return GetStateFlag(kIsUseFulllinkTimeoutMask); }

  /// @brief Framework use or for testing. Set whether the server timeout is the full link timeout.
  void SetIsUseFullLinkTimeout(bool use_fulllink) { return SetStateFlag(use_fulllink, kIsUseFulllinkTimeoutMask); }

  /// @brief Framework use or for testing. Set the actual timeout time of the current request.
  ///        It take the minimum value between the link timeout time and the timeout time configured by the service.
  /// @private
  void SetRealTimeout();

  /// @brief Get the call type of the current request.
  uint8_t GetCallType() const { return invoke_info_.rpc_call_type; }

  /// @brief Framework use or for testing. Set the call type of the current request.
  void SetCallType(uint8_t call_type) { invoke_info_.rpc_call_type = call_type; }

  /// @brief Get the unique id of the current request.
  uint32_t GetRequestId() const { return invoke_info_.seq_id; }

  /// @brief Framework use or for testing. Set the unique id of the current request.
  void SetRequestId(uint32_t id) { invoke_info_.seq_id = id; }

  /// @brief Get the caller name of the current request.
  const std::string& GetCallerName() const { return req_msg_->GetCallerName(); }

  /// @brief Set the caller name of the current request.
  void SetCallerName(std::string caller) { req_msg_->SetCallerName(std::move(caller)); }

  /// @brief Get the callee name of the current request.
  const std::string& GetCalleeName() const { return req_msg_->GetCalleeName(); }

  /// @brief Set the callee name of the current request.
  void SetCalleeName(std::string callee) { req_msg_->SetCalleeName(std::move(callee)); }

  /// @brief Get the function name of the current request.
  const std::string& GetFuncName() const { return req_msg_->GetFuncName(); }

  /// @brief Framework use or for testing. Set the function name of the current request.
  void SetFuncName(std::string func) { req_msg_->SetFuncName(std::move(func)); }

  /// @brief Get the message type of the current request.
  uint32_t GetMessageType() const { return invoke_info_.message_type; }

  /// @brief Framework use or for testing. Set the message type of the current request.
  void SetMessageType(uint32_t msg_type) { invoke_info_.message_type = msg_type; }

  /// @brief Get the encoding type of request data.
  uint8_t GetReqEncodeType() const { return invoke_info_.req_encode_type; }

  /// @brief Deprecated: use `GetReqEncodeType` instead.
  [[deprecated("Use GetReqEncodeType instead")]] uint8_t GetEncodeType() const { return GetReqEncodeType(); }

  /// @brief Framework use or for testing. Set the encoding type of request data.
  void SetReqEncodeType(uint8_t type) { invoke_info_.req_encode_type = type; }

  /// @brief Framework use or for testing. Set the compression type of request data.
  void SetReqCompressType(uint8_t compress_type) { invoke_info_.req_compress_type = compress_type; }

  /// @brief Get the compression type for compressing request message.
  uint8_t GetReqCompressType() const { return invoke_info_.req_compress_type; }
  /// @brief Deprecated: use `GetReqCompressType` instead.
  [[deprecated("use GetReqCompressType instead")]] uint8_t GetCompressType() const { return GetReqCompressType(); }

  /// @brief Set the compression type for decompressing response message.
  void SetRspCompressType(uint8_t compress_type) { invoke_info_.rsp_compress_type = compress_type; }

  /// @brief Get the compression type for decompressing response message.
  uint8_t GetRspCompressType() const { return invoke_info_.rsp_compress_type; }

  /// @brief Set the compression level for decompressing response message.
  void SetRspCompressLevel(uint8_t compress_level) { invoke_info_.rsp_compress_level = compress_level; }

  /// @brief Get the compression level for decompressing response message.
  uint8_t GetRspCompressLevel() const { return invoke_info_.rsp_compress_level; }

  /// @brief Get the encoding type of response body.
  uint8_t GetRspEncodeType() const { return invoke_info_.rsp_encode_type; }

  /// @brief Set the encoding type of rsp_data_
  void SetRspEncodeType(uint8_t encode_type) { invoke_info_.rsp_encode_type = encode_type; }

  /// @brief Get the transparent transmission information of the request.
  const TransInfoMap& GetPbReqTransInfo() const { return *(req_msg_->GetMutableKVInfos()); }

  /// @brief Get the transparent transmission information of the request.
  TransInfoMap* GetMutablePbReqTransInfo() { return req_msg_->GetMutableKVInfos(); }

  /// @brief Add the transparent transmission information of the request.
  void AddReqTransInfo(const std::string& key, const std::string& value) { req_msg_->SetKVInfo(key, value); }

  /// @brief Get the transparent transmission information of the response.
  const TransInfoMap& GetPbRspTransInfo() const { return rsp_msg_->GetKVInfos(); }

  /// @brief Get the transparent transmission information of the response.
  TransInfoMap* GetMutablePbRspTransInfo() { return rsp_msg_->GetMutableKVInfos(); }

  /// @brief Add the transparent transmission information of the response.
  void AddRspTransInfo(std::string key, std::string value) { rsp_msg_->SetKVInfo(std::move(key), std::move(value)); }

  /// @brief Add the transparent transmission information of the response.
  template <typename InputIt>
  void SetRspTransInfo(InputIt first, InputIt last) {
    TRPC_ASSERT(rsp_msg_);
    InputIt it = first;
    while (it != last) {
      rsp_msg_->SetKVInfo(std::move(it->first), std::move(it->second));

      ++it;
    }
  }

  /// @brief Get the size of the current request.
  uint32_t GetReqMsgSize() const { return req_msg_->GetMessageSize(); }

  /// @brief Get the size of the response corresponding to the current request.
  uint32_t GetRspMsgSize() const { return rsp_msg_->GetMessageSize(); }

  uint32_t GetRequestLength() const { return GetReqMsgSize(); }

  /// @brief Framework use or for testing. Check whether the request has timed out after processing.
  /// @private
  bool CheckHandleTimeout();

  //////////////////////////////////////////////////////////////////////////

  /// @brief Whether the current request is a dye request message.
  bool IsDyeingMessage() const;

  /// @brief Get dye key.
  std::string GetDyeingKey();

  /// @brief Get the dye key by name.
  std::string GetDyeingKey(const std::string& name);

  /// @brief Set the dye key of the request.
  void SetDyeingKey(const std::string& value);
  void SetDyeingKey(const std::string& name, const std::string& value);

  /// @brief Set the data structure associated with a specific filter.
  /// @param id The id is shared between the filter id and the plugin id.
  /// tRPC-CPP ensures that the plugin id starts growing from 65535, and each filter has its own unique id.
  /// It is recommended to start growing from 0 and assign no more than 65535.
  /// @param filter_data is the data structure for filter function.
  template <typename T>
  void SetFilterData(uint32_t id, T&& filter_data) {
    std::any data = std::forward<T>(filter_data);
    extend_info_.filter_data.emplace(std::make_pair(id, std::move(data)));
  }

  /// @brief Get the data structure associated with a specific filter.
  /// @param id The id that shared between the filter id and the plugin id.
  /// tRPC-CPP ensures that the plugin id starts growing from 65535, and each filter has its own unique id.
  /// It is recommended to start growing from 0 and assign no more than 65535.
  /// @return The data structure for the filter.
  template <typename T>
  T* GetFilterData(uint32_t id) {
    auto it = extend_info_.filter_data.find(id);
    if (it != extend_info_.filter_data.end()) {
      return std::any_cast<T>(&(it->second));
    }
    return nullptr;
  }

  /// @brief Get the filter data for all filter Settings.
  std::unordered_map<uint32_t, std::any>& GetAllFilterData() { return extend_info_.filter_data; }

  //////////////////////////////////////////////////////////////////////////

  /// @brief Set the flag of whether the framework actively returns packet.
  /// @note  default is true, when async return packet, it need to set false.
  void SetResponse(bool is_response) { SetStateFlag(is_response, kIsResponseMask); }

  /// @brief Get the flag of whether the framework actively returns packets.
  bool IsResponse() const { return GetStateFlag(kIsResponseMask); }

  /// @brief When the codec decode fails, set whether to allow the framework to actively return package.
  ///        By default no packet is return.
  void SetResponseWhenDecodeFail(bool need_response) { SetStateFlag(need_response, kNeedResponseWhenDecodeFailMask); }

  /// @brief Get whether to allow the framework to actively return package.
  ///        By default no package is return.
  bool NeedResponseWhenDecodeFail() const { return GetStateFlag(kNeedResponseWhenDecodeFailMask); }

  /// @brief Whether a request is using the HTTP or TRPC protocol, you can inspect the request headers.
  /// @note Used internally by the framework for the trpc_http protocol scenario. Business logic should not use this.
  bool IsHttpRequest() { return GetStateFlag(kTrpcHttpProtocolMask); }

  /// @brief Used to identify a request as using the HTTP protocol.
  /// @note Used internally by the framework for the trpc_http protocol scenario. Business logic should not use this.
  void SetIsHttpRequest() { return SetStateFlag(true, kTrpcHttpProtocolMask); }

  /// @brief Set the data associated with the current request.
  /// @param data user-defined data struct.
  void SetUserData(const std::any& data) { extend_info_.user_data = data; }
  void SetUserData(std::any&& data) { extend_info_.user_data = std::move(data); }

  /// @brief Get the data associated with the current request.
  const std::any& GetUserData() const { return extend_info_.user_data; }
  std::any& GetUserData() { return extend_info_.user_data; }

  /// @brief Sets the reader and writer of Streaming RPC. It's used by 'StreamMethodHandler'.
  /// @note Used internally by the framework. Business logic should not use this.
  void SetStreamReaderWriterProvider(stream::StreamReaderWriterProviderPtr&& stream_provider) {
    invoke_info_.stream_provider_ = std::move(stream_provider);
  }

  /// @brief Gets the reader and writer of Streaming RPC. It's used by 'StreamMethodHandler'.
  /// @note Used internally by the framework. Business logic should not use this.
  stream::StreamReaderWriterProviderPtr GetStreamReaderWriterProvider() {
    return std::move(invoke_info_.stream_provider_);
  }

  //////////////////////////////////////////////////////////////////////////

  /// @brief Implementation of asynchronous packet return on the server side.
  /// @param status the result of request execution
  /// @param biz_rsp business response data, it will be seriliazed or compressed by framework
  /// @note  before calling this method, you should call `SetResponse(false)` in the rpc interface implemented
  template <typename T>
  void SendUnaryResponse(const Status& status, T& biz_rsp) {
    if constexpr (std::is_convertible_v<T*, google::protobuf::Message*>) {
      SendUnaryResponse(status, static_cast<google::protobuf::Message*>(&biz_rsp));
    } else if constexpr (std::is_convertible_v<T*, rapidjson::Document*>) {
      SendUnaryResponse(status, static_cast<rapidjson::Document*>(&biz_rsp));
    } else if constexpr (std::is_convertible_v<T*, flatbuffers::trpc::MessageFbs*>) {
      SendUnaryResponse(status, static_cast<flatbuffers::trpc::MessageFbs*>(&biz_rsp));
    } else if constexpr (std::is_convertible_v<T*, NoncontiguousBuffer*>) {
      SendUnaryResponse(status, static_cast<NoncontiguousBuffer*>(&biz_rsp));
    } else if constexpr (std::is_convertible_v<T*, std::string*>) {
      SendUnaryResponse(status, static_cast<std::string*>(&biz_rsp));
    } else {
      TRPC_LOG_ERROR("T type is not support");
      TRPC_ASSERT(false);
    }
  }

  /// @brief Implementation of asynchronous packet return on the server side.
  /// @param status the result of request execution
  /// @param bin_rsp business response data(raw data, protocol body),
  ///                it has been seriliazed or compressed.
  /// @note  before calling this method, you should call `SetResponse(false)` in the rpc interface implemented,
  ///        it is generally used in the scenario of transit data transparent transmission.
  void SendTransparentResponse(const Status& status, NoncontiguousBuffer&& bin_rsp);

  /// @brief Implementation of asynchronous packet return on the server side
  /// @param status the result of request execution
  /// @note  before calling this method, you should call `SetResponse(false)` in the rpc interface implemented,
  ///        it is generally used in scenarios where request processing fails
  ///        or invoked by `SendUnaryResponse`.
  void SendUnaryResponse(const Status& status);

  /// @brief Implementation of asynchronous packet return on the server side.
  /// @param buffer raw data, it(protocol header + body) has been seriliazed or compressed
  /// @return the sending result
  /// @note  before calling this method, you should call `SetResponse(false)` in the rpc interface implemented
  ///        it is generally used in custom protocol data transmission scenarios
  Status SendResponse(NoncontiguousBuffer&& buffer);

  /// @brief Set the remote log field information in the context extension.
  /// @note  Based on the filterIndex Settings, the business can specify the field information
  ///        of the remote logging plugin for remote reporting.
  void SetLogKVPair(uint32_t filter_id, std::pair<std::string, std::string> kv);

  /// @brief Get the remote log field information in the context extension.
  /// @note  Based on the filterIndex Settings, the business can get information by specify the field
  ///        of the remote logging plugin for remote reporting.
  std::string GetLogFieldValue(uint32_t filter_id, const std::string& key);

  /// @brief Check if a connection is still valid in a fiber context.
  bool IsConnected();

  /// @brief Framwork use. To actively close a connection on the server-side.
  /// @private
  void CloseConnection();

  /// @brief Connection throttling is a technique used to control the flow of data over a connection by
  ///        pausing the reading of data from the connection
  /// @note  in high-load scenarios, it can be beneficial to temporarily pause reading data from certain connections in
  ///        order to prevent a cascading failure, also known as a "thundering herd" or "avalanche" effect. this can be
  ///        achieved by implementing a connection throttling mechanism that allows the business to control the rate at
  ///        which data is processed and sent to clients based on their own strategies, such as QPS, bandwidth, etc.
  /// @note  the business can implement their own strategies to determine when to pause reading data from certain
  ///        connections based on their specific needs and requirements. These strategies can include factors such as
  ///        QPS, traffic, bandwidth, and other metrics that are relevant to the business.
  /// @note  it is possible to record the QPS of each connection under serviceA and implement connection
  ///        throttling based on this information. When the total QPS of the service reaches its processing limit,
  ///        the business can choose to throttle connections with high QPS or lower priority connections in order to
  ///        prevent the service from becoming overwhelmed and crashing.
  ///        notice：
  ///          1.if the interval between opening and closing connections is too long, it can lead to clients not
  ///            receiving response packets in a timely manner and potentially timing out
  ///          2.the interface is an asynchronous interface and the request is submitted asynchronously to a reactor
  ///            for execution, there may be a delay before the request takes effect
  ///          3.it is important for the business to manage connection IDs and record the current throttle settings to
  ///            avoid duplicate submissions and prevent the reactor from becoming overwhelmed with duplicate tasks
  ///          4.the interface does not currently support fiber mode
  /// @param set true: enable throttling and pause reading data; false: disable throttling and resume reading data.
  void ThrottleConnection(bool set);

  /// @brief Get callback function after send response msg done.
  std::function<void()>& GetSendMsgCallback() { return extend_info_.send_msg_callback; }

  /// @brief Set callback function after send response msg done.
  void SetSendMsgCallback(std::function<void()>&& callback) { extend_info_.send_msg_callback = std::move(callback); }

 private:
  // Implementation of asynchronous packet return on the server side
  void SendUnaryResponse(const Status& status, google::protobuf::Message* pb);

  // Implementation of asynchronous packet return on the server side
  void SendUnaryResponse(const Status& status, flatbuffers::trpc::MessageFbs* fbs);

  // Implementation of asynchronous packet return on the server side
  void SendUnaryResponse(const Status& status, rapidjson::Document* json);

  // Implementation of asynchronous packet return on the server side
  void SendUnaryResponse(const Status& status, NoncontiguousBuffer* buffer);

  // Implementation of asynchronous packet return on the server side
  void SendUnaryResponse(const Status& status, std::string* str);

  // Implementation of asynchronous packet return on the server side
  void SendUnaryResponse(const Status& status, void* rsp_data, serialization::DataType type);

  void SetStateFlag(bool flag, uint8_t mask) {
    if (flag) {
      invoke_info_.state_flag |= mask;
    } else {
      invoke_info_.state_flag &= ~mask;
    }
  }

  bool GetStateFlag(uint8_t mask) const { return invoke_info_.state_flag & mask; }

  void HandleEncodeErrorResponse(std::string&& err_msg);

 private:
  static constexpr uint8_t kIsResponseMask = 0b00000001;
  static constexpr uint8_t kNeedResponseWhenDecodeFailMask = 0b00000010;
  static constexpr uint8_t kIsUseFulllinkTimeoutMask = 0b00000100;
  static constexpr uint8_t kTrpcHttpProtocolMask = 0b00001000;

  struct alignas(8) NetInfo {
    uint64_t connection_id{0};

    std::string ip;

    int fd{-1};

    int16_t port{0};

    NetType type;

    void* reserved{nullptr};
  };

  struct alignas(8) InvokeInfo {
    // request unique id
    uint32_t seq_id{0};

    // stream id
    uint32_t stream_id{0};

    // request timeout(ms)
    uint32_t timeout{UINT32_MAX};

    // use bits to represent some status flags in the request processing process,
    // from low to high in order:
    // 1. after the rpc interface processing is completed,
    //    whether the framework actively returns the packet(default enable)
    //    use kIsResponseMask
    // 2. whether to reply a response after the request decode fails(default disable)
    //    eg: When the trpc protocol parsing request fails,
    //        there is no need to reply, because the unique id of the request is not known
    //    http protocol enabled
    //    use kNeedResponseWhenDecodeFailMask
    // 3. Whether the value of timeout is the full link timeout time(default disable)
    //    if protocol has timeout field, enable
    //    use kIsUseFulllinkTimeoutMask
    // 4. Identifies the actual protocol is `trpc` or `http` when codec is `trpc_http` (0 by default, means `trpc`).
    // 5. 6. 7. ....other more flags to be set
    uint8_t state_flag = 0b00000001;

    // rpc call type
    uint8_t rpc_call_type{0};

    // request message type
    uint32_t message_type{0};

    // request encode type, default(0) is pb
    uint8_t req_encode_type{0};

    // request data encode type, default(0) is pb data-struct
    uint8_t req_encode_data_type{0};

    // response encode type, default(0) is pb
    uint8_t rsp_encode_type{0};

    // response data encode type, default(0) is pb data-struct
    uint8_t rsp_encode_data_type{0};

    // request data compress type
    uint8_t req_compress_type{compressor::kNone};

    // response data compress type
    uint8_t rsp_compress_type{compressor::kNone};

    // response data compress level
    uint8_t rsp_compress_level{compressor::kDefault};

    // Reader or writer of streaming RPC.
    stream::StreamReaderWriterProviderPtr stream_provider_{nullptr};
  };

  struct alignas(8) MetricsInfo {
    // the time(us) when the current request is received from the network
    uint64_t recv_timestamp_us = 0;

    // the time(us) when the current request starts processing
    uint64_t begin_timestamp_us = 0;

    // the time(us) when the current request processing is completed
    uint64_t end_timestamp_us = 0;

    // the time(us) when the response data corresponding to the current request is sent
    uint64_t send_timestamp_us = 0;
  };

  struct alignas(8) ExtendInfo {
    // filter controller, not owned.
    ServerFilterController* filter_controller{nullptr};

    // the data structure associated with a specific filter
    // key is filter id
    // value is filter data structure
    std::unordered_map<uint32_t, std::any> filter_data;

    // This is used to store the index of the filter that has been executed for a certain pre-tracking point.
    // The initial value is -1, which indicates that no filter has been executed.
    int8_t filter_exec_index[kFilterTypeNum / 2] = {-1, -1, -1, -1, -1};

    // user-defined data
    // saving some data structures associated with the current request
    std::any user_data;

    std::function<void()> send_msg_callback{nullptr};
  };

 private:
  // not owned
  Service* service_{nullptr};

  // not owned
  ServerCodec* codec_{nullptr};

  // request
  ProtocolPtr req_msg_{nullptr};

  // response
  ProtocolPtr rsp_msg_{nullptr};

  // rpc method handler, use to destroy req_data_/rsp_data_
  RpcMethodHandlerInterface* rpc_method_handler_{nullptr};

  // user request data, eg: rpc request protobuf struct
  void* req_data_{nullptr};

  // user response data, eg: rpc request protobuf struct
  void* rsp_data_{nullptr};

#ifdef TRPC_PROTO_USE_ARENA
  google::protobuf::Arena* req_arena_{nullptr};

  google::protobuf::Arena* rsp_arena_{nullptr};
#endif

  // request attachment data
  NoncontiguousBuffer req_attachment_;

  Status status_;

  NetInfo net_info_;

  InvokeInfo invoke_info_;

  MetricsInfo metrics_info_;

  ExtendInfo extend_info_;
};

using ServerContextPtr = RefPtr<ServerContext>;

template <typename T>
using is_server_context = std::is_same<T, ServerContext>;

/// @brief Set the context to a thread-private variable. The private variable itself does not hold the context. The set
/// operation must be used when the ctx is valid within its lifecycle.
void SetLocalServerContext(const ServerContextPtr& context);

/// @brief Retrieve the context from a thread-private variable. The private variable itself does not hold the context.
/// The get operation must be used when the ctx is valid within its lifecycle.
ServerContextPtr GetLocalServerContext();

}  // namespace trpc
