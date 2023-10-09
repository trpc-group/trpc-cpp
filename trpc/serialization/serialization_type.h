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

#include <cstdint>

#include "trpc/codec/trpc/trpc.pb.h"

namespace trpc::serialization {

/// @brief The type of serialization and deserialization
/// @note  The value in the range of 0-127 is used internally by the framework
///        The value in the range of 128-255 can used by the business
using SerializationType = uint8_t;

/// @brief Protobuf type
const SerializationType kPbType = TrpcContentEncodeType::TRPC_PROTO_ENCODE;
/// @brief Tars/Jce type
const SerializationType kJceType = TrpcContentEncodeType::TRPC_JCE_ENCODE;
/// @brief Json type
const SerializationType kJsonType = TrpcContentEncodeType::TRPC_JSON_ENCODE;
/// @brief flatbuffers type
const SerializationType kFlatBuffersType = TrpcContentEncodeType::TRPC_FLATBUFFER_ENCODE;
/// @brief Text or Binary data type
const SerializationType kNoopType = TrpcContentEncodeType::TRPC_NOOP_ENCODE;
/// @brief Thrift type
const SerializationType kThriftType = TrpcContentEncodeType::TRPC_THRIFT_ENCODE;

/// @brief Max value of serialization type
const SerializationType kMaxType = 255;

/// @brief The data-struct type of serialization and deserialization
/// @note  The value in the range of 0-127 is used internally by the framework
///        The value in the range of 128-255 can used by the business
using DataType = uint8_t;

/// @brief Data struct: google::protobuf::Message
const DataType kPbMessage = 0;
/// @brief Data struct: tars
const DataType kJce = 1;
/// @brief Data struct: rapidjson
const DataType kRapidJson = 2;
/// @brief Data struct: flatbuffers
const DataType kFlatBuffers = 3;
/// @brief Data struct: std::string
const DataType kStringNoop = 4;
/// @brief Data struct: ContiguousBuffer
const DataType kNonContiguousBufferNoop = 5;

/// @brief Data struct: redis Request/Reply
const DataType kRedisType = 6;
/// @brief Data struct: http protocol
const DataType kHttpType = 7;

/// @brief Data sturct: thrift
const DataType kThrift = 8;

/// @brief Max value of data struct
const DataType kMaxDataType = 255;

}  // namespace trpc::serialization
