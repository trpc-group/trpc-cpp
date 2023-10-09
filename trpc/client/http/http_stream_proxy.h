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

#include "trpc/client/service_proxy.h"
#include "trpc/stream/http/async/stream_reader_writer.h"

namespace trpc::stream {

/// @brief HTTP streaming proxy.
class HttpStreamProxy : public ServiceProxy {
 public:
  /// @brief Gets an asynchronous streaming reader-writer.
  Future<HttpClientAsyncStreamReaderWriterPtr> GetAsyncStreamReaderWriter(const ClientContextPtr& ctx);

 protected:
  /// @brief Sets a flag indicates to use streaming proxy.
  /// @private For internal use purpose only.
  TransInfo ProxyOptionToTransInfo() override {
    TransInfo trans_info = ServiceProxy::ProxyOptionToTransInfo();
    trans_info.is_stream_proxy = true;
    return trans_info;
  }
};

}  // namespace trpc::stream
