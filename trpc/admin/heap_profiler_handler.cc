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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. incubator-brpc
// Copyright (C) 2019 The Apache Software Foundation
// incubator-brpc is licensed under the Apache 2.0 License.
//
//

#include "trpc/admin/heap_profiler_handler.h"

#include <map>
#include <memory>
#include <string>

#include "trpc/admin/web_css_jquery.h"
#include "trpc/util/http/body_params.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc::admin {

void HeapProfilerHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                        rapidjson::Document::AllocatorType& alloc) {
#ifndef TRPC_ENABLE_PROFILER
  result.AddMember("errorcode", -1, alloc);
  result.AddMember("message", "please recomplile with -DTRPC_ENABLE_PROFILER and link libtcmalloc_and_profiler.a",
                   alloc);
  return;
#else
  // first: find in query_param
  auto enable = req->GetQueryParameter("enable");
  if (enable.empty()) {
    http::BodyParam body_param(req->GetContent());
    // sencond: find in body
    enable = body_param.GetBodyParam("enable");
  }
  if (enable.empty() || (enable != "y" && enable != "n")) {
    result.AddMember("errorcode", -1, alloc);
    result.AddMember("message", "wrong parameter, please use ?enable=y/n", alloc);
    return;
  }

  if (enable == "y" && !IsHeapProfilerRunning()) {
    HeapProfilerStart("heap.prof");
  } else if (enable == "n" && IsHeapProfilerRunning()) {
    HeapProfilerDump("");
    HeapProfilerStop();
  }

  result.AddMember("errorcode", 0, alloc);
  result.AddMember("message", "OK", alloc);
#endif
}

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/0.9.6-rc02/src/brpc/builtin/hotspots_service.cpp.

const char* WebHeapProfilerHandler::SetUpdateEvents() {
  return "      var target_url = '/cmdsweb/profile/heap_draw?seconds=' + h_seconds;\n"
         "      target_url += '&display_type=' + h_dtype + '&view_name=' + h_vname;\n"
         "      window.history.pushState('', '', target_url);\n"
         "      data = data.substring(selEnd + '[addViewName]'.length);\n"
         "    }\n"
         "    var index = data.indexOf('digraph ');\n"
         "    if (index == -1) {\n"
         "      $(\"#profiling-result\").html('<pre>' + data + '</pre>');\n"
         "      if (data.indexOf('FlameGraph') != -1) { init(); }"
         "    } else {\n"
         "      $(\"#profiling-result\").html('Plotting ...');\n"
         "      var svg = Viz(data.substring(index), \"svg\");\n"
         "      $(\"#profiling-result\").html(svg);\n"
         "    }\n"
         "  }\n"
         "  function onErrorReceived(xhr, ajaxOptions, thrownError) {\n"
         "    $(\"#profiling-result\").html(xhr.responseText);\n"
         "  }\n"

         "function SendHeapProfilingReq() {\n"
         "   var dtype = document.getElementById('display_type').value;\n"
         "   var vname = document.getElementById('view_name').value;\n"
         "   var req_url = '/cmdsweb/profile/heap_draw?display_type=';\n"
         "   req_url = req_url + dtype + '&view_name=' + vname;\n"
         "    $.ajax({\n"
         "      url: req_url,\n"
         "      type: \"GET\",\n"
         "      dataType: \"html\",\n"
         "      success: onDataReceived,\n"
         "      error: onErrorReceived\n"
         "    });\n"
         "}\n"

         "function onSetDisplayType() {\n"
         "  SendHeapProfilingReq();\n"
         "}\n"

         "function onSetViewName() {\n"
         "  SendHeapProfilingReq();\n"
         "}\n"

         "$(function() {\n"
         "  var h_seconds = document.getElementById('hidden_seconds').value;\n"
         "  var h_vname = document.getElementById('hidden_view_name').value;\n"
         "  var h_dtype = document.getElementById('hidden_display_type').value;\n"
         "  var req_url = '/cmdsweb/profile/heap_draw?seconds=' + h_seconds;\n"
         "  req_url += '&display_type=' + h_dtype + '&view_name=' + h_vname;\n"
         "  $.ajax({\n"
         "   url: req_url,\n"
         "   type: \"GET\",\n"
         "   dataType: \"html\",\n"
         "   success: onDataReceived,\n"
         "   error: onErrorReceived\n"
         "  });\n"
         "});\n\n"
         "</script>\n\n";
}

// End of source codes that are from incubator-brpc.

void WebHeapProfilerHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                           rapidjson::Document::AllocatorType& alloc) {
  std::string html;
#ifndef TRPC_ENABLE_PROFILER
  std::string alert_info = "alert(please recomplile with -DTRPC_ENABLE_PROFILER ";
  alert_info.append("and link libtcmalloc_and_profiler.a)");
  html.append(alert_info);
#else
  std::map<std::string, CmdInfo> m_tabs;
  SetTabItems(&m_tabs);

  std::map<std::string, std::string> mfiles;
  if (GetFiles(ProfilingType::kHeap, &mfiles)) {
    TRPC_LOG_ERROR("do GetFiles fail");
    html.append("GetFiles fail");
    result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
    return;
  }

  uint32_t seconds = 0;
  std::string str_seconds = req->GetQueryParameter("seconds");
  if (str_seconds.empty()) {
    seconds = 5;
  } else {
    seconds = atoi(str_seconds.c_str());
  }
  std::string display_type = req->GetQueryParameter("display_type");
  if (display_type.empty()) {
    display_type = kDisplayDot;
  }
  if (CheckDisplayType(display_type)) {
    TRPC_LOG_ERROR("display_type: " << display_type << " is not supported - 1");
    html.append("display_type is not supported");
    result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
    return;
  }
  std::string view_name = req->GetQueryParameter("view_name");
  if (view_name.empty()) {
    view_name = "new_profile";
  }

  html = HtmlHeader();
  html.append("<script language=\"javascript\" type=\"text/javascript\" ");
  html.append("src=\"/cmdsweb/js/viz_min\"></script>\n");
  html.append(TabsHead());
  html.append(CommonOnRecvData());
  html.append(SetUpdateEvents());
  html.append("</head>\n");

  html.append("<body>\n");
  PrintTabsBody(&html, m_tabs);
  html.append("<br>\n<br>\n");

  PrintDisplayInfo(&html, display_type);
  PrintViewListInfo(&html, view_name, mfiles);

  // add hidden variable
  html.append("<div>\n");
  html.append("<input type='hidden' id='hidden_view_name' value='");
  html.append(view_name);
  html.append("'>\n");
  html.append("<input type='hidden' id='hidden_display_type' value='");
  html.append(display_type);
  html.append("'>\n");
  html.append("<input type='hidden' id='hidden_seconds' value='");
  html.append(std::to_string(seconds));
  html.append("'>\n");
  html.append("</div>\n");

  html.append("<div id=\"profiling-result\">");
  if (view_name == "new_profile") {
    html.append("Profiling heap for ");
    html.append(std::to_string(seconds));
    html.append(" seconds, please waiting ...");
  } else {
    html.append("Profiling heap for view: ");
    html.append(view_name);
  }
  html.append("</div>\n");
  html.append("<br>\n<br>\n");
  html.append("</body></html>");

#endif
  result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
}

void WebHeapProfilerDrawHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                               rapidjson::Document::AllocatorType& alloc) {
  std::string html;
#ifndef TRPC_ENABLE_PROFILER
  std::string alert_info = "alert(please recomplile with -DTRPC_ENABLE_PROFILER ";
  alert_info.append("and link libtcmalloc_and_profiler.a)");
  html.append(alert_info);
#else
  std::map<std::string, std::string> mfiles;
  if (GetFiles(ProfilingType::kHeap, &mfiles)) {
    TRPC_LOG_ERROR("do GetFiles fail");
    html.append("GetFiles fail");
    result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
    return;
  }

  uint32_t seconds = 0;
  std::string str_seconds = req->GetQueryParameter("seconds");
  if (str_seconds.empty()) {
    seconds = 10;
  } else {
    seconds = atoi(str_seconds.c_str());
  }
  // check display_type
  std::string display_type = req->GetQueryParameter("display_type");
  if (display_type.empty()) {
    display_type = kDisplayDot;
  }
  if (CheckDisplayType(display_type)) {
    TRPC_LOG_ERROR("display_type: " << display_type << " is not supported");
    html.append("display_type is not supported");
    result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
    return;
  }
  // check view_name
  bool is_new_profile = false;
  std::string view_name = req->GetQueryParameter("view_name");
  if (view_name.empty() || view_name == "new_profile") {
    is_new_profile = true;
  }

  bool use_cache_file = false;
  bool del_old_file = false;
  std::string output;
  std::string del_file_name;

  if (is_new_profile) {
    if (mfiles.size() >= GetMaxProfNum()) {
      del_old_file = true;
      del_file_name = mfiles.begin()->first;
    }

    char prof_name[256];
    if (MakeNewProfName(ProfilingType::kHeap, prof_name, sizeof(prof_name), &mfiles)) {
      TRPC_LOG_ERROR("do MakeNewProfName fail");
      html.append("MakeNewProfName fail");
      result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
      return;
    }
    TRPC_LOG_INFO("start heap Profile, prof_name: " << prof_name);
    HeapProfilerStart(prof_name);
    sleep(seconds);
    // HeapProfilerDump("");
    char* heap_prof = GetHeapProfile();
    HeapProfilerStop();
    TRPC_LOG_INFO("stop heap Profile, heap_prof content: " << heap_prof);

    std::string tmp_content = heap_prof;
    // write content to prof_name
    if (WriteCacheFile(prof_name, tmp_content)) {
      TRPC_LOG_ERROR("do WriteCacheFile fail");
      html.append("WriteCacheFile fail");
      result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
      return;
    }
    free(heap_prof);
    view_name = prof_name;
  } else {
    // check base file and cache exists
    std::string cache_path = view_name;
    if (MakeProfCacheName(display_type, &cache_path)) {
      TRPC_LOG_ERROR("do MakeProfCacheName fail");
      html.append("MakeProfCacheName fail");
      result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
      return;
    }
    if (GetProfileFileContent(cache_path, &output)) {
      TRPC_LOG_ERROR("not find cache_file: " << cache_path << ", need producing");
    } else {
      TRPC_LOG_INFO("!!!, will use cache file: " << cache_path);
      use_cache_file = true;
    }
  }

  std::string tool_content;
  PprofPerl(&tool_content);
  if (WritePprofPerlFile(tool_content)) {
    TRPC_LOG_ERROR("do WritePprofPerlFile fail");
    html.append("WritePprofPerlFile fail");
    result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
    return;
  }

  if (!use_cache_file) {
    std::string prog_name;
    if (GetCurrProgressName(&prog_name)) {
      TRPC_LOG_ERROR("do GetCurrProgressName fail");
      html.append("GetCurrProgressName fail");
      result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
      return;
    }
    std::string cmd_str = "perl ";
    cmd_str += GetPprofToolPath();
    DisplayTypeToCmd(display_type, &cmd_str);
    cmd_str.append(prog_name + " ");
    cmd_str.append(view_name + " ");
    cmd_str.append(" 2>&1 ");
    TRPC_LOG_INFO("draw profile cmd: " << cmd_str);

    // exec cmd
    uint64_t begin_time = trpc::time::GetMilliSeconds();
    if (ExecCommandByPopen(cmd_str.c_str(), &output)) {
      TRPC_LOG_ERROR("do ExecCommandByPopen fail");
      html.append("ExecCommand fail");
      result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
      return;
    }
    uint64_t end_time = trpc::time::GetMilliSeconds();
    TRPC_LOG_WARN("do heap profile drawing used: " << end_time - begin_time);

    begin_time = end_time;
    // check cache file exists
    std::string cache_path = view_name;
    if (MakeProfCacheName(display_type, &cache_path)) {
      TRPC_LOG_ERROR("do MakeProfCacheName fail");
      html.append("MakeProfCacheName fail");
      result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
      return;
    }
    // write output to cache file
    if (WriteCacheFile(cache_path, output)) {
      TRPC_LOG_ERROR("do WriteCacheFile fail");
      html.append("WriteCacheFile fail");
      result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
      return;
    }
    end_time = trpc::time::GetMilliSeconds();
    TRPC_LOG_WARN("heap profile write to file used: " << end_time - begin_time);
  }

  // reset view_name
  if (del_old_file && del_file_name.size()) {
    html.append(del_file_name + "[delViewName]");
  }
  if (is_new_profile) {
    html.append(view_name + "[addViewName]");
  }
  html.append(output);

#endif

  result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
}

}  // namespace trpc::admin
