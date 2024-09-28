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

#include "trpc/codec/codec_manager.h"

// codec trpc
#include "trpc/codec/trpc/trpc_client_codec.h"
#include "trpc/codec/trpc/trpc_server_codec.h"

// codec redis
#include "trpc/codec/redis/redis_client_codec.h"

// codec http
#include "trpc/codec/http/http_client_codec.h"
#include "trpc/codec/http/http_server_codec.h"

// codec trpc_http
// #include "trpc/codec/trpc_http/trpc_http_server_codec.h"

// codec grpc
#include "trpc/codec/grpc/grpc_client_codec.h"
#include "trpc/codec/grpc/grpc_server_codec.h"

// codec mysql
#include "trpc/codec/mysql/mysql_client_codec.h"

namespace trpc::codec {

bool Init() {
  bool ret{false};

  // http
  ret = InitCodecPlugins<HttpServerCodec>();
  TRPC_ASSERT(ret);
  ret = InitCodecPlugins<HttpClientCodec>();
  TRPC_ASSERT(ret);

  // trpc over http
  ret = InitCodecPlugins<TrpcOverHttpServerCodec>();
  TRPC_ASSERT(ret);
  ret = InitCodecPlugins<TrpcOverHttpClientCodec>();
  TRPC_ASSERT(ret);

  // trpc
  ret = InitCodecPlugins<TrpcServerCodec>();
  TRPC_ASSERT(ret);
  ret = InitCodecPlugins<TrpcClientCodec>();
  TRPC_ASSERT(ret);

  // trpc_http
  // ret = InitCodecPlugins<TrpcHttpServerCodec>();
  // TRPC_ASSERT(ret);

  // grpc
  ret = InitCodecPlugins<GrpcClientCodec>();
  TRPC_ASSERT(ret);
  ret = InitCodecPlugins<GrpcServerCodec>();
  TRPC_ASSERT(ret);

  // redis
  ret = InitCodecPlugins<RedisClientCodec>();
  TRPC_ASSERT(ret);

  ret = InitCodecPlugins<MySQLClientCodec>();
  TRPC_ASSERT(ret);
  return ret;
}

void Destroy() {
  ServerCodecFactory::GetInstance()->Clear();
  ClientCodecFactory::GetInstance()->Clear();
}

}  // namespace trpc::codec
