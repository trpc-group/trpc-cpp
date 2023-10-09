// Copyright (c) 2023, Tencent Inc.
// All rights reserved.

#pragma once

#include <cstdint>

namespace trpc::testing {

// maximum connection limit of server side
constexpr int kMaxConnNum = 8;
// maximum packet size limit of server side
constexpr int kMaxPacketSize = 128;
// idle timeout in milliseconds
constexpr uint32_t kIdleTimeoutMs = 2000;

// used to test different UDP response packet size by server
constexpr char kUdpPacketOverLimit[] = "OverLimit";
constexpr char kUdpPacketUnderLimit[] = "UnderLimit";

// used to test different response way by server
constexpr char kSyncResponse[] = "SyncResponse";
constexpr char kAsyncResponse[] = "AsyncResponse";
constexpr char kNoResponse[] = "NoResponse";

// used to test different call type received by server
constexpr char kOnewayRequest[] = "Oneway";
constexpr char kUnaryRequest[] = "Unary";

// path of unix domain socket
constexpr char kUdsPath[] = "trpc_unix.test.socket";

}  // namespace trpc::testing