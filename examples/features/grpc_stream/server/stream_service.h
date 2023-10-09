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

#include <mutex>
#include <string>
#include <vector>

#include "examples/features/grpc_stream/server/stream.trpc.pb.h"

namespace test::helloworld {

class RouteGuideImpl : public ::routeguide::RouteGuide {
 public:
  RouteGuideImpl(const std::string db_path);

  ::trpc::Status GetFeature(::trpc::ServerContextPtr context, const ::routeguide::Point* point,
                            ::routeguide::Feature* feature) override;

  ::trpc::Status ListFeatures(const ::trpc::ServerContextPtr& context, const ::routeguide::Rectangle& rectangle,
                              ::trpc::stream::StreamWriter<::routeguide::Feature>* writer) override;

  ::trpc::Status RecordRoute(const ::trpc::ServerContextPtr& context,
                             const ::trpc::stream::StreamReader<::routeguide::Point>& reader,
                             ::routeguide::RouteSummary* summary) override;

  ::trpc::Status RouteChat(const ::trpc::ServerContextPtr& context,
                           const ::trpc::stream::StreamReader<::routeguide::RouteNote>& reader,
                           ::trpc::stream::StreamWriter<::routeguide::RouteNote>* writer) override;

 private:
  std::string db_path_;
  std::vector<::routeguide::Feature> feature_list_;
  // In the gRPC streaming example, this lock is used to operate on received_notes_ under concurrent operations.
  std::mutex mu_;
  std::vector<::routeguide::RouteNote> received_notes_;
};

}  // namespace test::helloworld
