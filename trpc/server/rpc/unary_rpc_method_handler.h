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

#include <string>
#include <type_traits>

#include "google/protobuf/message.h"
#include "rapidjson/document.h"

#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/compressor/trpc_compressor.h"
#include "trpc/serialization/serialization_factory.h"
#include "trpc/server/method_handler.h"
#include "trpc/server/server_context.h"
#include "trpc/util/buffer/buffer.h"
#include "trpc/util/buffer/zero_copy_stream.h"
#include "trpc/util/flatbuffers/message_fbs.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/pb2json.h"

namespace trpc {

/// @brief Common implementation class for unary rpc method handler
template <class RequestType, class ResponseType>
class UnaryRpcMethodHandler : public RpcMethodHandlerInterface {
 public:
  void DestroyReqObj(ServerContext* context) override {
#ifdef TRPC_PROTO_USE_ARENA
    if constexpr (IsEnablePbArena()) {
      if (context->GetReqArenaObj() != nullptr) {
        delete context->GetReqArenaObj();
        context->SetReqArenaObj(nullptr);
        context->SetRequestData(nullptr);
      }
      return;
    }
#endif
    if (context->GetRequestData() != nullptr) {
      delete static_cast<RequestType*>(context->GetRequestData());
      context->SetRequestData(nullptr);
    }
  }

  void DestroyRspObj(ServerContext* context) override {
#ifdef TRPC_PROTO_USE_ARENA
    if constexpr (IsEnablePbArena()) {
      if (context->GetRspArenaObj() != nullptr) {
        delete context->GetRspArenaObj();
        context->SetRspArenaObj(nullptr);
        context->SetResponseData(nullptr);
      }
      return;
    }
#endif
    if (context->GetResponseData() != nullptr) {
      delete static_cast<ResponseType*>(context->GetResponseData());
      context->SetResponseData(nullptr);
    }
  }

 protected:
  bool PreExecute(const ServerContextPtr& context, NoncontiguousBuffer&& req_body) {
    TRPC_ASSERT(context->GetServerCodec() != nullptr);

    CreateReqAndRspObj(context);

    auto decompress_type = context->GetReqCompressType();
    auto ret = compressor::DecompressIfNeeded(decompress_type, req_body);
    if (TRPC_UNLIKELY(!ret)) {
      std::string err_msg = "request body decompress failed, decompress type:";
      err_msg += std::to_string(decompress_type);
      SetErrorStatus(context, codec::ServerRetCode::DECODE_ERROR, std::move(err_msg));
      return false;
    }

    uint8_t encode_type = context->GetReqEncodeType();
    auto serialization = serialization::SerializationFactory::GetInstance()->Get(encode_type);
    if (serialization == nullptr) {
      std::string error_msg = "request body deserialize failed, not support encode_type:";
      error_msg += std::to_string(encode_type);
      SetErrorStatus(context, codec::ServerRetCode::DECODE_ERROR, std::move(error_msg));
      return false;
    }

    ret = Deserialize(serialization, &req_body, context->GetRequestData());
    if (TRPC_UNLIKELY(!ret)) {
      std::string error_msg = "request body deserialize failed, encode_type:";
      error_msg += std::to_string(encode_type);
      SetErrorStatus(context, codec::ServerRetCode::DECODE_ERROR, std::move(error_msg));
      return false;
    }

    req_body.Clear();

    auto& filter_controller = context->GetFilterController();
    auto filter_status = filter_controller.RunMessageServerFilters(FilterPoint::SERVER_PRE_RPC_INVOKE, context);

    return filter_status != FilterStatus::REJECT;
  }

  void PostExecute(const ServerContextPtr& context, NoncontiguousBuffer& rsp_body) {
    TRPC_CHECK(context->IsResponse());

    auto& filter_controller = context->GetFilterController();
    filter_controller.RunMessageServerFilters(FilterPoint::SERVER_POST_RPC_INVOKE, context);

    if (TRPC_UNLIKELY(!context->GetStatus().OK())) {
      return;
    }

    uint8_t encode_type = context->GetRspEncodeType();
    auto serialization = serialization::SerializationFactory::GetInstance()->Get(encode_type);
    bool ret = Serialize(serialization, context->GetResponseData(), rsp_body);
    if (TRPC_UNLIKELY(!ret)) {
      std::string error_msg = "response body serialize failed, encode_type:";
      error_msg += std::to_string(encode_type);
      SetErrorStatus(context, codec::ServerRetCode::ENCODE_ERROR, std::move(error_msg));
      return;
    }

    ret = compressor::CompressIfNeeded(context->GetRspCompressType(), rsp_body, context->GetRspCompressLevel());
    if (TRPC_UNLIKELY(!ret)) {
      std::string error_msg = "response body serialize compress, compress_type:";
      error_msg += std::to_string(context->GetRspCompressType());
      SetErrorStatus(context, codec::ServerRetCode::ENCODE_ERROR, "response body compress failed");
      return;
    }

    DestroyReqAndRspObj(context);
  }

#ifdef TRPC_PROTO_USE_ARENA
  static constexpr bool IsEnablePbArena() {
    return std::is_convertible_v<RequestType*, google::protobuf::MessageLite*> &&
           google::protobuf::Arena::is_arena_constructable<RequestType>::value &&
           std::is_convertible_v<ResponseType*, google::protobuf::MessageLite*> &&
           google::protobuf::Arena::is_arena_constructable<ResponseType>::value;
  }
#endif

  void CreateReqAndRspObj(const ServerContextPtr& context) {
    CreateReqObj(context);
    CreateRspObj(context);

    context->SetRpcMethodHandler(this);
  }

  void CreateReqObj(const ServerContextPtr& context) {
    // if protobuf version < 3.14.0，need to enable pb area in proto file: option cc_enable_arenas = true;
    // https://developers.google.com/protocol-buffers/docs/reference/arenas
#ifdef TRPC_PROTO_USE_ARENA
    if constexpr (IsEnablePbArena()) {
      TRPC_FMT_TRACE("RpcAsyncMethodHandler is enable pb arena");
      context->SetReqArenaObj(GeneratePbArenaPtr());
      context->SetRequestData(google::protobuf::Arena::CreateMessage<RequestType>(context->GetReqArenaObj()));
      return;
    }
#endif
    auto req = new RequestType();
    context->SetRequestData(req);
  }

  void CreateRspObj(const ServerContextPtr& context) {
#ifdef TRPC_PROTO_USE_ARENA
    if constexpr (IsEnablePbArena()) {
      TRPC_FMT_TRACE("RpcAsyncMethodHandler is enable pb arena");
      context->SetRspArenaObj(GeneratePbArenaPtr());
      context->SetResponseData(google::protobuf::Arena::CreateMessage<ResponseType>(context->GetRspArenaObj()));
      return;
    }
#endif
    auto rsp = new ResponseType();
    context->SetResponseData(rsp);
  }

  void DestroyReqAndRspObj(const ServerContextPtr& context) {
    DestroyReqObj(context.Get());
    DestroyRspObj(context.Get());

    context->SetRpcMethodHandler(nullptr);
  }

  void SetErrorStatus(const ServerContextPtr& context, codec::ServerRetCode frame_ret, std::string err_msg = "") {
    context->GetStatus().SetFrameworkRetCode(context->GetServerCodec()->GetProtocolRetCode(frame_ret));
    context->GetStatus().SetErrorMessage(std::move(err_msg));
  }

 private:
  bool Serialize(serialization::Serialization* serialization, void* rsp, NoncontiguousBuffer& buff) {
    serialization::DataType type;
    if constexpr (std::is_convertible_v<RequestType*, google::protobuf::MessageLite*>) {
      type = serialization::kPbMessage;
    } else if constexpr (std::is_convertible_v<RequestType*, rapidjson::Document*>) {
      type = serialization::kRapidJson;
    } else if constexpr (std::is_convertible_v<RequestType*, flatbuffers::trpc::MessageFbs*>) {
      type = serialization::kFlatBuffers;
    } else if constexpr (std::is_convertible_v<RequestType*, NoncontiguousBuffer*>) {
      type = serialization::kNonContiguousBufferNoop;
    } else if constexpr (std::is_convertible_v<RequestType*, std::string*>) {
      type = serialization::kStringNoop;
    } else {
      return false;
    }

    return serialization->Serialize(type, rsp, &buff);
  }

  bool Deserialize(serialization::Serialization* serialization, NoncontiguousBuffer* req_data, void* req) {
    serialization::DataType type;
    if constexpr (std::is_convertible_v<RequestType*, google::protobuf::MessageLite*>) {
      type = serialization::kPbMessage;
    } else if constexpr (std::is_convertible_v<RequestType*, rapidjson::Document*>) {
      type = serialization::kRapidJson;
    } else if constexpr (std::is_convertible_v<RequestType*, flatbuffers::trpc::MessageFbs*>) {
      type = serialization::kFlatBuffers;
    } else if constexpr (std::is_convertible_v<RequestType*, NoncontiguousBuffer*>) {
      type = serialization::kNonContiguousBufferNoop;
    } else if constexpr (std::is_convertible_v<RequestType*, std::string*>) {
      type = serialization::kStringNoop;
    } else {
      return false;
    }

    return serialization->Deserialize(req_data, type, req);
  }
};

}  // namespace trpc
