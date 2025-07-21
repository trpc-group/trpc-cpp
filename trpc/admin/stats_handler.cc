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

#include "trpc/admin/stats_handler.h"

#include <map>
#include <utility>

#include "json/json.h"

#include "trpc/admin/web_css_jquery.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/runtime/common/stats/frame_stats.h"
#include "trpc/tvar/common/tvar_group.h"
#include "trpc/util/log/logging.h"
#ifdef TRPC_BUILD_INCLUDE_RPCZ
#include "trpc/rpcz/rpcz.h"
#endif
#include "trpc/util/string_helper.h"

namespace trpc::admin {

namespace {
void PrintStatsData(std::string* html) {
  html->append("<table class=\"gridtable sortable\" border=\"1\">\n");
  html->append("<tr><th>Item</th><th>Value</th></tr>\n");

  html->append("<tr>\n<td>conn_count</td>\n<td>");
  html->append(std::to_string(FrameStats::GetInstance()->GetServerStats().GetConnCount()));
  html->append("</td>\n</tr>\n");

  html->append("<tr>\n<td>total_req_count</td>\n<td>");
  html->append(std::to_string(FrameStats::GetInstance()->GetServerStats().GetTotalReqCount()));
  html->append("</td>\n</tr>\n");

  html->append("<tr>\n<td>now_req_count</td>\n<td>");
  html->append(std::to_string(FrameStats::GetInstance()->GetServerStats().GetReqCount()));
  html->append("</td>\n</tr>\n");

  html->append("<tr>\n");
  html->append("<td>last_req_count</td>\n<td>");
  html->append(std::to_string(FrameStats::GetInstance()->GetServerStats().GetLastReqCount()));
  html->append("</td>\n</tr>\n");

  html->append("<tr>\n");
  html->append("<td>total_failed_req_count</td>\n<td>");
  html->append(std::to_string(FrameStats::GetInstance()->GetServerStats().GetTotalFailedReqCount()));
  html->append("</td>\n</tr>\n");

  html->append("<tr>\n");
  html->append("<td>now_failed_req_count</td>\n<td>");
  html->append(std::to_string(FrameStats::GetInstance()->GetServerStats().GetFailedReqCount()));
  html->append("</td>\n</tr>\n");

  html->append("<tr>\n");
  html->append("<td>last_failed_req_count</td>\n<td>");
  html->append(std::to_string(FrameStats::GetInstance()->GetServerStats().GetLastFailedReqCount()));
  html->append("</td>\n</tr>\n");

  html->append("<tr>\n");
  html->append("<td>total_avg_delay</td>\n<td>");
  html->append(std::to_string(FrameStats::GetInstance()->GetServerStats().GetAvgTotalDelay()));
  html->append("</td>\n</tr>\n");

  html->append("<tr>\n");
  html->append("<td>now_avg_delay</td>\n<td>");
  html->append(std::to_string(FrameStats::GetInstance()->GetServerStats().GetAvgDelay()));
  html->append("</td>\n</tr>\n");

  html->append("<tr>\n");
  html->append("<td>last_avg_delay</td>\n<td>");
  html->append(std::to_string(FrameStats::GetInstance()->GetServerStats().GetAvgLastDelay()));
  html->append("</td>\n</tr>\n");

  html->append("<tr>\n");
  html->append("<td>max_delay</td>\n<td>");
  html->append(std::to_string(FrameStats::GetInstance()->GetServerStats().GetMaxDelay()));
  html->append("</td>\n</tr>\n");

  html->append("<tr>\n");
  html->append("<td>last_max_delay</td>\n<td>");
  html->append(std::to_string(FrameStats::GetInstance()->GetServerStats().GetLastMaxDelay()));
  html->append("</td>\n</tr>\n");

  html->append("</table>\n");
}
}  // namespace

void StatsHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                 rapidjson::Document::AllocatorType& alloc) {
  if (!TrpcConfig::GetInstance()->GetServerConfig().enable_server_stats) {
    result.AddMember("errorcode", -1, alloc);
    result.AddMember("message", "server stats not running, please config enable_server_stats in server", alloc);
    return;
  }

  result.AddMember("errorcode", 0, alloc);
  result.AddMember("message", "", alloc);

  rapidjson::Value stats(rapidjson::kObjectType);
  stats.AddMember("conn_count", FrameStats::GetInstance()->GetServerStats().GetConnCount(), alloc);
  stats.AddMember("total_req_count", FrameStats::GetInstance()->GetServerStats().GetTotalReqCount(), alloc);
  stats.AddMember("req_concurrency", FrameStats::GetInstance()->GetServerStats().GetReqConcurrency(), alloc);
  stats.AddMember("now_req_count", FrameStats::GetInstance()->GetServerStats().GetReqCount(), alloc);
  stats.AddMember("last_req_count", FrameStats::GetInstance()->GetServerStats().GetLastReqCount(), alloc);
  stats.AddMember("total_failed_req_count", FrameStats::GetInstance()->GetServerStats().GetTotalFailedReqCount(),
                  alloc);
  stats.AddMember("now_failed_req_count", FrameStats::GetInstance()->GetServerStats().GetFailedReqCount(), alloc);
  stats.AddMember("last_failed_req_count", FrameStats::GetInstance()->GetServerStats().GetLastFailedReqCount(), alloc);
  stats.AddMember("total_avg_delay", FrameStats::GetInstance()->GetServerStats().GetAvgTotalDelay(), alloc);
  stats.AddMember("now_avg_delay", FrameStats::GetInstance()->GetServerStats().GetAvgDelay(), alloc);
  stats.AddMember("last_avg_delay", FrameStats::GetInstance()->GetServerStats().GetAvgLastDelay(), alloc);
  stats.AddMember("max_delay", FrameStats::GetInstance()->GetServerStats().GetMaxDelay(), alloc);
  stats.AddMember("last_max_delay", FrameStats::GetInstance()->GetServerStats().GetLastMaxDelay(), alloc);

  result.AddMember("stats", stats, alloc);
}



void WebStatsHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                    rapidjson::Document::AllocatorType& alloc) {
  if (!TrpcConfig::GetInstance()->GetServerConfig().enable_server_stats) {
    result.AddMember("errorcode", -1, alloc);
    result.AddMember("message", "server stats not running, please config enable_server_stats in server", alloc);
    return;
  }

  std::map<std::string, CmdInfo> m_tabs;
  SetTabItems(&m_tabs);

  std::string html = HtmlHeader();
  html.append(TabsHead());
  html.append(GridTableStyle());
  html.append("</head>\n");

  html.append("<body>\n");
  PrintTabsBody(&html, m_tabs);
  html.append("<br>\n<br>\n");

  PrintStatsData(&html);
  html.append("\n\n");
  html.append("</body>\n</html>\n");

  result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
}

namespace {
void NormalizePath(std::string* path) {
  if (path->back() == '/') {
    path->pop_back();
  }
  if (path->empty()) {
    *path = "/";
  }
}

std::string GetFormattedString(const Json::Value& value) {
  Json::StreamWriterBuilder json_builder;
  json_builder["indentation"] = "  ";
  json_builder["commentStyle"] = "None";
  return Json::writeString(json_builder, value);
}
}  // namespace

VarHandler::VarHandler(std::string prefix) : uri_prefix_(std::move(prefix)) {
  description_ = "[GET /cmds/var] get stats";
}

trpc::Status VarHandler::Handle(const std::string& path, trpc::ServerContextPtr context, http::HttpRequestPtr req,
                                http::HttpResponse* reply) {
  auto route_url = req->GetRouteUrl();
  if (route_url.find("//") != std::string::npos) {
    Json::Value jsv;
    jsv["message"] = "illegal url:" + route_url;
    reply->SetContent(GetFormattedString(jsv));
    reply->Done("json");
    return kDefaultStatus;
  }

  std::string abs_path;
  if (StartsWith(route_url, uri_prefix_)) {
    abs_path = route_url.substr(uri_prefix_.size());
    NormalizePath(&abs_path);
  }

  TRPC_ASSERT(abs_path[0] == '/');

  auto instance = req->GetQueryParameter("history");
  bool get_series = !instance.empty() && (instance == "true");
  auto opt_jsv = tvar::TrpcVarGroup::TryGet(abs_path, get_series);
  if (!opt_jsv) {
    Json::Value jsv;
    jsv["message"] = "have no exposed var";
    reply->SetContent(GetFormattedString(jsv));
  } else {
    reply->SetContent(GetFormattedString(*opt_jsv));
  }
  reply->Done("json");
  return kDefaultStatus;
}

#ifdef TRPC_BUILD_INCLUDE_RPCZ
RpczHandler::RpczHandler(std::string&& prefix) : url_prefix_(std::move(prefix)) {
  description_ = "[GET /cmds/rpcz] get stats";
}

trpc::Status RpczHandler::Handle(const std::string& path, trpc::ServerContextPtr context, http::HttpRequestPtr req,
                                 http::HttpResponse* reply) {
  std::string route_url = req->GetUrl();
  if (route_url.find("//") != std::string::npos || (!StartsWith(route_url, url_prefix_))) {
    Json::Value jsv;
    jsv["message"] = "illegal url:" + route_url;
    reply->SetContent(GetFormattedString(jsv));
    reply->Done("json");
    return Status(-1, "illegal url");
  }

  std::string result = trpc::rpcz::TryGet(req);
  if (result.empty()) {
    result = "have no rpcz span";
    reply->SetContent(result);
  } else {
    reply->SetContent(result);
  }
  reply->Done("json");
  return kDefaultStatus;
}
#endif
}  // namespace trpc::admin
