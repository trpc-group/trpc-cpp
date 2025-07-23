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

#pragma once

#include <cstdint>

namespace trpc {

/// Config default value of global config.

/// Merge threadmodel type.
constexpr char kMergeThreadmodelType[] = "merge";

/// Separate threadmodel type.
constexpr char kSeparateThreadmodelType[] = "separate";

/// Fiber threadmodel type.
constexpr char kFiberThreadmodelType[] = "fiber";

/// The default thread model type.
/// It's compatible with old version configurations, including merging separate thread models.
constexpr char kDefaultThreadmodelType[] = "default";

/// Then default configuration plugins names.
constexpr char kDefaultConfigString[] = "default";

/// The default logging plugins names.
constexpr char kTrpcLogCacheStringDefault[] = "default";

/// The default protocol used by the service.
constexpr char kDefaultProtocol[] = "trpc";

/// The default network type of the called service.
constexpr char kDefaultNetwork[] = "tcp";

/// The default connection type, long or short.
constexpr char kDefaultConnType[] = "long";

/// The default maximum packet size limit.
constexpr uint32_t kDefaultMaxPacketSize = 10000000;

/// The default maximum packet size limit under the HTTP protocol.
constexpr uint32_t kDefaultHttpMaxPacketSize = 0;

/// The default maximum data length allowed to be received at one time.
constexpr uint32_t kDefaultRecvBufferSize = 64 * 1024;

/// The default maximum data length of the io-send queue has cached.
/// Note: use in fiber runtime. If set 0, it means there is no limit.
constexpr uint32_t kDefaultSendQueueCapacity = 0;

/// The default timeout(ms) of data in the io-send queue when sending network data.
/// Note: use in fiber runtime.
constexpr uint32_t kDefaultSendQueueTimeout = 3000;

 /// The default size of hashmap bucket for storing ip/port <--> Connector
constexpr uint32_t kEndpointHashBucketSize = 1024;

/// The default maximum number of connections that can be established to the backend nodes.
constexpr uint32_t kDefaultMaxConnNum = 64;

/// The default timeout(ms) for idle connections.
constexpr uint32_t kDefaultIdleTime = 50000;

/// The thread model used by the admin service.
/// Note: If no thread model is configured, then this thread model is used by default.
constexpr char kDefaultThreadmodelInstanceName[] = "trpc_separate_admin_instance";

/// The default service-level timeout in milliseconds.
constexpr uint32_t kDefaultTimeout = UINT32_MAX;

/// The default request timeout detection interval, in milliseconds.
/// Note: use in IO/Handle separation and merging mode.
constexpr uint32_t kDefaultRequestTimeoutCheckInterval = 10;

/// The default value whether to reconnect after the idle connection is disconnected when reach connection idle timeout.
constexpr bool kDefaultIsReconnection = false;

/// The default value whether to support reconnection in fixed connection mode, the default value is true.
constexpr bool kDefaultAllowReconnect = true;
  
/// The default selector plugin used by the service.
constexpr char kDefaultSelectorName[] = "";

/// The default value whether to enforce the set route.
constexpr bool kDefaultEnableSetForce = false;

/// The default value whether to disable service routing.
constexpr bool kDefaultDisableServiceRouter = false;

/// The default value whether to use connection reuse.
constexpr bool kDefaultIsConnComplex = true;

/// The default value whether pipeline is supported.
constexpr bool kDefaultSupportPipeline = false;

/// The default connect timeout, zero means no checking.
constexpr uint32_t kDefaultConnectTimeout = 0;

/// The default sliding window size(byte) for flow control in recv-side Under streaming.
constexpr uint32_t kDefaultStreamMaxWindowSize = 65535;

}  // namespace trpc
