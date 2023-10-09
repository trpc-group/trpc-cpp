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

#include "trpc/admin/sysvars_handler.h"

#include <map>

#include "trpc/admin/sample.h"
#include "trpc/admin/web_css_jquery.h"

#define ONE_HOUR_SEC 3600
#define ONE_DAY_SEC 3600 * 24

namespace trpc::admin {

void WebSysVarsHandler::SetNonFlotVar(std::string* html, const std::string& var_name, const std::string& value) {
  html->append("<p class=\"nonplot-variable\">" + var_name);
  html->append(": <span id=\"value-" + var_name + "\">");
  html->append(value + "</span></p>\n");
  html->append("<br>\n");
}

void WebSysVarsHandler::SetFlotVar(std::string* html, const std::string& var_name, const std::string& value) {
  html->append("<p class=\"variable\"><font color='#0000FF'><u>" + var_name);
  html->append(": <span id=\"value-" + var_name + "\">");
  html->append(value + "</span></u></font></p>\n");
  html->append("<div class=\"detail\"><div id=\"" + var_name);
  html->append("\" class=\"flot-placeholder\"></div></div>\n");
  html->append("<br>\n");
}

void WebSysVarsHandler::PrintProcRusage(std::string* html, bool force) {
  std::string str = "-";
  double res = 0.0;
  rusage stat;
  int ret = GetProcRusage(&stat);
  if (force || ret) {
    SetFlotVar(html, "proc_real_time", str);
    SetFlotVar(html, "proc_sys_time", str);
    SetFlotVar(html, "proc_user_time", str);
  } else {
    ProcRusage proc_usage;
    proc_usage.real_time = stat.ru_stime.tv_sec + stat.ru_utime.tv_sec;
    if (proc_usage.real_time <= ONE_HOUR_SEC) {
      str = std::to_string(proc_usage.real_time) + "(secs)";
    } else if (proc_usage.real_time > ONE_HOUR_SEC && proc_usage.real_time <= ONE_DAY_SEC) {
      res = (static_cast<double>(proc_usage.real_time)) / (static_cast<double>(ONE_HOUR_SEC));
      str = std::to_string(res) + "(hours)";
    } else if (proc_usage.real_time > ONE_DAY_SEC) {
      res = (static_cast<double>(proc_usage.real_time)) / (static_cast<double>(ONE_DAY_SEC));
      str = std::to_string(res) + "(days)";
    }
    SetFlotVar(html, "proc_real_time", str);

    proc_usage.sys_time = stat.ru_stime.tv_sec;
    if (proc_usage.sys_time <= ONE_HOUR_SEC) {
      str = std::to_string(proc_usage.sys_time) + "(secs)";
    } else if (proc_usage.sys_time > ONE_HOUR_SEC && proc_usage.sys_time <= ONE_DAY_SEC) {
      res = (static_cast<double>(proc_usage.sys_time)) / (static_cast<double>(ONE_HOUR_SEC));
      str = std::to_string(res) + "(hours)";
    } else if (proc_usage.sys_time > ONE_DAY_SEC) {
      res = (static_cast<double>(proc_usage.sys_time)) / (static_cast<double>(ONE_DAY_SEC));
      str = std::to_string(res) + "(days)";
    }
    SetFlotVar(html, "proc_sys_time", str);

    proc_usage.user_time = stat.ru_utime.tv_sec;
    if (proc_usage.user_time <= ONE_HOUR_SEC) {
      str = std::to_string(proc_usage.user_time) + "(secs)";
    } else if (proc_usage.user_time > ONE_HOUR_SEC && proc_usage.user_time <= ONE_DAY_SEC) {
      res = (static_cast<double>(proc_usage.user_time)) / (static_cast<double>(ONE_HOUR_SEC));
      str = std::to_string(res) + "(hours)";
    } else if (proc_usage.user_time > ONE_DAY_SEC) {
      res = (static_cast<double>(proc_usage.user_time)) / (static_cast<double>(ONE_DAY_SEC));
      str = std::to_string(res) + "(days)";
    }
    SetFlotVar(html, "proc_user_time", str);
  }
}

void WebSysVarsHandler::PrintProcIoAndMem(std::string* html, bool force) {
  // proc io
  std::string str = "-";
  ProcIO pio;
  int ret = ReadProcIo(&pio);
  if (force || ret) {
    SetFlotVar(html, "proc_io_read_bytes_second", str);
    SetFlotVar(html, "proc_io_read_second", str);
    SetFlotVar(html, "proc_io_write_bytes_second", str);
    SetFlotVar(html, "proc_io_write_second", str);
  } else {
    str = std::to_string(pio.read_bytes);
    SetFlotVar(html, "proc_io_read_bytes_second", str);
    str = std::to_string(pio.syscr);
    SetFlotVar(html, "proc_io_read_second", str);
    str = std::to_string(pio.write_bytes);
    SetFlotVar(html, "proc_io_write_bytes_second", str);
    str = std::to_string(pio.syscw);
    SetFlotVar(html, "proc_io_write_second", str);
  }

  // proc memory
  str = "-";
  ProcMemory pmem;
  ret = ReadProcMemory(&pmem);
  if (force || ret) {
    SetFlotVar(html, "proc_mem_drs", str);
    SetFlotVar(html, "proc_mem_resident", str);
    SetFlotVar(html, "proc_mem_share", str);
    SetFlotVar(html, "proc_mem_trs", str);
    SetFlotVar(html, "proc_mem_size", str);
  } else {
    str = std::to_string(pmem.drs);
    SetFlotVar(html, "proc_mem_drs", str);
    str = std::to_string(pmem.resident);
    SetFlotVar(html, "proc_mem_resident", str);
    str = std::to_string(pmem.share);
    SetFlotVar(html, "proc_mem_share", str);
    str = std::to_string(pmem.trs);
    SetFlotVar(html, "proc_mem_trs", str);
    str = std::to_string(pmem.size);
    SetFlotVar(html, "proc_mem_size", str);
  }
}

void WebSysVarsHandler::PrintSysVarsData(std::string* html, bool force) {
  html->append("<div id=\"layer1\">\n");

  struct timeval curr_tm = GetProcUptime();
  std::string str = "";
  GetGccVersion(&str);
  SetNonFlotVar(html, "gcc_version", str);

  str = "";
  GetKernelVersion(&str);
  SetNonFlotVar(html, "kernel", str);

  str = "";
  GetSystemCoreNums(&str);
  SetNonFlotVar(html, "core_nums", str);

  str = "";
  GetUserName(&str);
  SetNonFlotVar(html, "user_name", str);

  str = "";
  GetCurrentDirectory(&str);
  SetNonFlotVar(html, "work_directory", str);

  str = "-";
  char buff[1024] = {0};
  int ret = ReadProcCommandLine(buff, sizeof(buff));
  if (force || ret) {
    SetNonFlotVar(html, "command_line", str);
  } else {
    std::string tbuf = buff;
    SetNonFlotVar(html, "command_line", tbuf);
  }

  str = "";
  time_t sec = curr_tm.tv_sec;
  double res = 0.0;
  if (sec <= ONE_HOUR_SEC) {
    str = std::to_string(sec) + "(secs)";
  } else if (sec > ONE_HOUR_SEC && sec <= ONE_DAY_SEC) {
    res = (static_cast<double>(sec)) / (static_cast<double>(ONE_HOUR_SEC));
    str = std::to_string(res) + "(hours)";
  } else if (sec > ONE_DAY_SEC) {
    res = (static_cast<double>(sec)) / (static_cast<double>(ONE_DAY_SEC));
    str = std::to_string(res) + "(days)";
  }
  SetNonFlotVar(html, "running_time", str);

  str = "-";
  LoadAverage load;
  ret = ReadLoadUsage(&load);
  if (force || ret) {
    SetFlotVar(html, "proc_loadavg_1m", str);
    SetFlotVar(html, "proc_loadavg_5m", str);
    SetFlotVar(html, "proc_loadavg_15m", str);
  } else {
    str = std::to_string(load.loadavg_1m);
    SetFlotVar(html, "proc_loadavg_1m", str);
    str = std::to_string(load.loadavg_5m);
    SetFlotVar(html, "proc_loadavg_5m", str);
    str = std::to_string(load.loadavg_15m);
    SetFlotVar(html, "proc_loadavg_15m", str);
  }

  // proc rusage
  WebSysVarsHandler::PrintProcRusage(html, force);

  // proc stat
  str = "-";
  ProcStat pstat;
  ret = ReadProcState(&pstat);
  if (force || ret) {
    SetNonFlotVar(html, "pgrp", str);
    SetNonFlotVar(html, "ppid", str);
    SetNonFlotVar(html, "pid", str);
    SetFlotVar(html, "proc_faults_major", str);
    SetFlotVar(html, "proc_faults_minor_second", str);
  } else {
    str = std::to_string(pstat.pgrp);
    SetNonFlotVar(html, "pgrp", str);
    str = std::to_string(pstat.ppid);
    SetNonFlotVar(html, "ppid", str);
    str = std::to_string(pstat.pid);
    SetNonFlotVar(html, "pid", str);
    str = std::to_string(pstat.majflt);
    SetFlotVar(html, "proc_faults_major", str);
    str = std::to_string(pstat.minflt);
    SetFlotVar(html, "proc_faults_minor_second", str);
  }

  str = "-";
  int fd_count = 0;
  ret = GetProcFdCount(&fd_count);
  if (force || ret) {
    SetNonFlotVar(html, "fd_count", str);
  } else {
    str = std::to_string(fd_count);
    SetNonFlotVar(html, "fd_count", str);
  }

  // print io and meme
  WebSysVarsHandler::PrintProcIoAndMem(html, force);

  html->append("</div>\n");
}

void WebSysVarsHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                      rapidjson::Document::AllocatorType& alloc) {
  std::map<std::string, CmdInfo> m_tabs;
  SetTabItems(&m_tabs);

  std::string html = HtmlHeader();
  html.append("<script language=\"javascript\" type=\"text/javascript\" ");
  html.append("src=\"/cmdsweb/js/flot_min\"></script>\n");
  html.append(TabsHead());
  html.append(PutSysvarsHeading());
  html.append("</head>\n");

  html.append("<body>\n");
  PrintTabsBody(&html, m_tabs);
  html.append("<br>\n<br>\n");
  html.append("<p><font color='#0000FF'><b>");
  html.append("Instruction: Click on the underlined field to display the graph");
  html.append("</b></p><br>\n");

  std::string sys_data = GetBaseProfileDir();
  sys_data += "sysvars.data";
  ReadSysvarsData(sys_data, &html);
  html.append("\n\n");
  html.append("</body>\n</html>\n");

  result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
}

void WebSeriesVarHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                        rapidjson::Document::AllocatorType& alloc) {
  std::string var_name = req->GetQueryParameter("var");
  GetVarSeriesData(var_name, result, alloc);
  return;
}

#undef ONE_HOUR_SEC
#undef ONE_DAY_SEC

}  // namespace trpc::admin
