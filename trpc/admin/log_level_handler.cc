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

#include "trpc/admin/log_level_handler.h"

#include <map>
#include <utility>

#include "trpc/admin/web_css_jquery.h"
#include "trpc/util/http/body_params.h"
#include "trpc/util/log/logging.h"

namespace trpc::admin {

namespace {

const char* LogLevelToStr(Log::Level level) {
  switch (level) {
    case Log::Level::trace:
      return "TRACE";
    case Log::Level::debug:
      return "DEBUG";
    case Log::Level::info:
      return "INFO";
    case Log::Level::warn:
      return "WARNING";
    case Log::Level::error:
      return "ERROR";
    case Log::Level::critical:
      return "CRITICAL";
    default:
      return "UNKNOWN";
  }
}

std::pair<Log::Level, bool> StrToLogLevel(const std::string& level) {
  if (level == "TRACE") {
    return std::make_pair(Log::Level::trace, true);
  } else if (level == "DEBUG") {
    return std::make_pair(Log::Level::debug, true);
  } else if (level == "INFO") {
    return std::make_pair(Log::Level::info, true);
  } else if (level == "WARNING") {
    return std::make_pair(Log::Level::warn, true);
  } else if (level == "ERROR") {
    return std::make_pair(Log::Level::error, true);
  } else if (level == "CRITICAL") {
    return std::make_pair(Log::Level::critical, true);
  } else {
    return std::make_pair(Log::Level::info, false);
  }
}
}  // namespace

void LogLevelHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                    rapidjson::Document::AllocatorType& alloc) {
  auto log = LogFactory::GetInstance()->Get();
  if (!log) {
    result.AddMember("errorcode", -1, alloc);
    result.AddMember("message", "get log plugin failed, no log plugin?", alloc);
    return;
  }

  // first: find in query_param
  auto instance = req->GetQueryParameter("logger");
  // second: find in body
  http::BodyParam body_param(req->GetContent());
  if (instance.empty()) {
    instance = body_param.GetBodyParam("logger");
  }
  if (instance.empty()) {
    instance = trpc::log::kTrpcLogCacheStringDefault;
  }

  if (update_flag_) {
    auto level_str = body_param.GetBodyParam("value");
    auto ret = StrToLogLevel(level_str);
    if (!ret.second) {
      result.AddMember("errorcode", -3, alloc);
      result.AddMember("message", "wrong level, please use TRACE,DEBUG,INFO,WARNING,ERROR,CRITICAL", alloc);
      return;
    }
    ret = log->SetLevel(instance.data(), ret.first);
    if (!ret.second) {
      result.AddMember("errorcode", -4, alloc);
      result.AddMember("message", "set level failed, does logger exist?", alloc);
      return;
    }
  }

  auto ret = log->GetLevel(instance.data());
  if (!ret.second) {
    result.AddMember("errorcode", -5, alloc);
    result.AddMember("message", "get level failed, does logger exist?", alloc);
    return;
  }

  result.AddMember("errorcode", 0, alloc);
  result.AddMember("message", "", alloc);
  auto level_str = LogLevelToStr(ret.first);
  result.AddMember(rapidjson::StringRef("level"), rapidjson::StringRef(level_str), alloc);
}

const char* WebLogLevelHandler::SetUpdateEvents() {
  return "<script type=\"text/javascript\">\n"
         "function onSetLogLevel() {\n"
         "   var level = document.getElementById('set_level').value;\n"
         "   $.ajax({\n"
         "      url: \"/cmds/loglevel\",\n"
         "      type: \"PUT\",\n"
         "      data: \"value=\" + level,\n"
         "      dataType: \"json\",\n"
         "      success: function (res) {\n"
         "          alert(JSON.stringify(res));"
         "      },\n"
         "      error: function (msg) {\n"
         "          alert(JSON.stringify(msg));"
         "      },\n"
         "});\n"
         "}\n"
         "</script>";
}

void WebLogLevelHandler::GetLogLevelOption(std::string* html) {
  html->append("<div>\n<pre style=\"display:inline\">Modify: </pre>\n");
  html->append("<select id=\"set_level\" onchange=\"onSetLogLevel()\">\n");
  html->append(
      "<option value=\"TRACE\" selected="
      ">TRACE</option>\n");
  html->append("<option value=\"DEBUG\">DEBUG</option>\n");
  html->append("<option value=\"INFO\">INFO</option>\n");
  html->append("<option value=\"WARNING\">WARNING</option>\n");
  html->append("<option value=\"ERROR\">ERROR</option>\n");
  html->append("<option value=\"CRITICAL\">CRITICAL</option>\n");
  html->append("</select>\n</div>\n");
}

void WebLogLevelHandler::PrintLogConfig(std::string* html, LogPtr log, const std::string& instance) {
  html->append("<table class=\"gridtable sortable\" border=\"1\">\n");
  html->append("<tr><th>LogName</th><th>LogLevel</th><th>Operation</th></tr>\n");

  html->append("<tr>\n<td>");
  html->append(instance);
  html->append("</td>\n<td>");
  auto ret = log->GetLevel(instance.data());
  if (ret.second) {
    html->append(LogLevelToStr(ret.first));
    html->append("</td>\n<td>");
    GetLogLevelOption(html);
    html->append("</td>\n</tr>\n");
  } else {
    html->append("-");
    html->append("</td>\n<td>-</td>\n</tr>\n");
  }

  html->append("</table>\n");
}

void WebLogLevelHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                       rapidjson::Document::AllocatorType& alloc) {
  std::map<std::string, CmdInfo> m_tabs;
  SetTabItems(&m_tabs);

  std::string html = HtmlHeader();
  html.append(TabsHead());
  html.append(GridTableStyle());
  html.append(SetUpdateEvents());
  html.append("</head>\n");

  html.append("<body>\n");
  PrintTabsBody(&html, m_tabs);
  html.append("<br>\n<br>\n");

  auto log = LogFactory::GetInstance()->Get();
  if (log) {
    auto instance = req->GetQueryParameter("logger");
    if (instance.empty()) {
      instance = trpc::log::kTrpcLogCacheStringDefault;
    }
    PrintLogConfig(&html, log, instance);
  }

  html.append("\n\n");
  html.append("</body>\n</html>\n");

  result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
}

}  // namespace trpc::admin
