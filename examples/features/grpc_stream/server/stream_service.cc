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

#include "examples/features/grpc_stream/server/stream_service.h"

#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

#include "examples/features/grpc_stream/common/helper.h"

namespace {
float ConvertToRadians(float num) { return num * 3.1415926 / 180; }

float GetDistance(const ::routeguide::Point& start, const ::routeguide::Point& end) {
  const float kCoordFactor = 10000000.0;
  float lat_1 = start.latitude() / kCoordFactor;
  float lat_2 = end.latitude() / kCoordFactor;
  float lon_1 = start.longitude() / kCoordFactor;
  float lon_2 = end.longitude() / kCoordFactor;
  float lat_rad_1 = ConvertToRadians(lat_1);
  float lat_rad_2 = ConvertToRadians(lat_2);
  float delta_lat_rad = ConvertToRadians(lat_2 - lat_1);
  float delta_lon_rad = ConvertToRadians(lon_2 - lon_1);

  float a = pow(sin(delta_lat_rad / 2), 2) + cos(lat_rad_1) * cos(lat_rad_2) * pow(sin(delta_lon_rad / 2), 2);
  float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  int R = 6371000;  // metres

  return R * c;
}

std::string GetFeatureName(const ::routeguide::Point& point, const std::vector<::routeguide::Feature>& feature_list) {
  for (const ::routeguide::Feature& f : feature_list) {
    if (f.location().latitude() == point.latitude() && f.location().longitude() == point.longitude()) {
      return f.name();
    }
  }
  return "";
}
}  // namespace

namespace test::helloworld {

RouteGuideImpl::RouteGuideImpl(const std::string db_path) : db_path_(db_path) {
  routeguide::ParseDb(routeguide::GetDbFileContent(db_path_), &feature_list_);
}

::trpc::Status RouteGuideImpl::GetFeature(::trpc::ServerContextPtr context, const ::routeguide::Point* point,
                                          ::routeguide::Feature* feature) {
  TRPC_FMT_INFO("Query feature at point, latitude: {}, longitude: {}", point->latitude(), point->longitude());
  feature->set_name(GetFeatureName(*point, feature_list_));
  feature->mutable_location()->CopyFrom(*point);
  TRPC_FMT_INFO("Got feature, name: {}, latitude: {}, longitude: {}", feature->name(), feature->location().latitude(),
                feature->location().longitude());
  return ::trpc::kSuccStatus;
}

::trpc::Status RouteGuideImpl::ListFeatures(const ::trpc::ServerContextPtr& context,
                                            const ::routeguide::Rectangle& rectangle,
                                            ::trpc::stream::StreamWriter<::routeguide::Feature>* writer) {
  ::trpc::Status status{};
  auto lo = rectangle.lo();
  auto hi = rectangle.hi();
  int32_t left = (std::min)(lo.longitude(), hi.longitude());
  int32_t right = (std::max)(lo.longitude(), hi.longitude());
  int32_t top = (std::max)(lo.latitude(), hi.latitude());
  int32_t bottom = (std::min)(lo.latitude(), hi.latitude());
  for (const ::routeguide::Feature& f : feature_list_) {
    if (f.location().longitude() >= left && f.location().longitude() <= right && f.location().latitude() >= bottom &&
        f.location().latitude() <= top) {
      status = writer->Write(f);
      if (status.OK()) {
        TRPC_FMT_INFO("ListFeatures write feature, name= {}, location.latitude: {}, location.longitude: {}", f.name(),
                      f.location().latitude(), f.location().longitude());
        continue;
      }
      TRPC_FMT_ERROR("ListFeatures stream got error: {}", status.ToString());
      break;
    }
  }
  return status;
}

::trpc::Status RouteGuideImpl::RecordRoute(const ::trpc::ServerContextPtr& context,
                                           const ::trpc::stream::StreamReader<::routeguide::Point>& reader,
                                           ::routeguide::RouteSummary* summary) {
  ::trpc::Status status{};
  ::routeguide::Point point;
  int point_count = 0;
  int feature_count = 0;
  float distance = 0.0;
  ::routeguide::Point previous;
  uint64_t now_ms = ::trpc::time::GetMilliSeconds();

  for (;;) {
    point_count++;
    status = reader.Read(&point, 3000);
    // Reads a Point successfully.
    if (status.OK()) {
      if (!GetFeatureName(point, feature_list_).empty()) {
        feature_count++;
      }
      if (point_count != 1) {
        distance += GetDistance(previous, point);
      }
      previous = point;
      TRPC_FMT_INFO("RecordRoute read point, latitude: {}, longitude: {}", point.latitude(), point.longitude());
      continue;
    }

    // Reads EOF from the client.
    if (status.StreamEof()) {
      TRPC_FMT_INFO("RecordRoute Stream Eof recv");
      // EOF is ok status.
      status = ::trpc::Status{0, 0, "OK"};
      break;
    }

    // Error.
    TRPC_FMT_ERROR("RecordRoute stream got error: {}", status.ToString());
    break;
  }

  // Reads all points form client, then response to client.
  if (status.OK()) {
    uint64_t end_time_ms = ::trpc::time::GetMilliSeconds();
    summary->set_point_count(point_count);
    summary->set_feature_count(feature_count);
    summary->set_distance(static_cast<int32_t>(distance));
    summary->set_elapsed_time((end_time_ms - now_ms) / 1000);
    TRPC_FMT_INFO("Return summary, point_count: {}, feature_count: {}", summary->point_count(),
                  summary->feature_count());
  }

  return status;
}

::trpc::Status RouteGuideImpl::RouteChat(const ::trpc::ServerContextPtr& context,
                                         const ::trpc::stream::StreamReader<::routeguide::RouteNote>& reader,
                                         ::trpc::stream::StreamWriter<::routeguide::RouteNote>* writer) {
  ::trpc::Status status{};
  ::routeguide::RouteNote note;

  for (;;) {
    status = reader.Read(&note, 3000);
    if (status.OK()) {
      TRPC_FMT_INFO("Success read note, location.latitude: {}, location.longitude: {}, message: {}",
                    note.location().latitude(), note.location().longitude(), note.message());
      std::unique_lock<std::mutex> lock(mu_);
      for (const ::routeguide::RouteNote& n : received_notes_) {
        if (n.location().latitude() == note.location().latitude() &&
            n.location().longitude() == note.location().longitude()) {
          // Responses the same RouteNode to client if received one which had been received.
          status = writer->Write(n);
          if (!status.OK()) {
            TRPC_FMT_ERROR("RouteChat write got error: {}", status.ToString());
            return status;
          }
          TRPC_FMT_INFO("RouteChat write note, message: {}, latitude: {}, longitude: {}", n.message(),
                        n.location().latitude(), n.location().longitude());
        }
      }
      received_notes_.push_back(note);
      continue;
    }

    // Reads the EOF from client.
    if (status.StreamEof()) {
      TRPC_LOG_INFO("RouteChat recv EOF");
      // EOF is ok status.
      status = ::trpc::Status{0, 0, "OK"};
      break;
    }

    TRPC_FMT_ERROR("RouteChat read got error: {}", status.ToString());
    break;
  }

  return status;
}

}  // namespace test::helloworld
