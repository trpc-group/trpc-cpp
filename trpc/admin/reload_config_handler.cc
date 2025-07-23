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

#include "trpc/admin/reload_config_handler.h"

#include <cstddef>
#include <map>

#include "trpc/admin/web_css_jquery.h"
#include "trpc/common/config/config_helper.h"
#include "trpc/common/config/trpc_config.h"

namespace trpc::admin {

void ReloadConfigHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                        rapidjson::Document::AllocatorType& alloc) {
  bool ret = ConfigHelper::GetInstance()->Reload();
  if (!ret) {
    result.AddMember("errorcode", -1, alloc);
    result.AddMember("message", "failed to reload config", alloc);
  } else {
    result.AddMember("errorcode", 0, alloc);
    result.AddMember("message", "reload config ok", alloc);
  }
}

namespace {
void DisplayGlobalConf(std::string* html, const trpc::GlobalConfig& gconf) {
  html->append("\n<br><table class=\"gridtable sortable\" border=\"1\">\n");
  html->append("<tr><th border='1' colspan='2'>global config</th></tr>\n");

  html->append("<tr>\n<td>local_ip</td>\n<td>");
  html->append(gconf.local_ip);
  html->append("</td>\n</tr>\n");

  html->append("<tr>\n<td>coroutine</td>\n<td>");
  std::string coroutine_conf;
  if (gconf.threadmodel_config.use_fiber_flag && !gconf.threadmodel_config.fiber_model.empty()) {
    const auto& fiber = gconf.threadmodel_config.fiber_model.front();
    coroutine_conf.append("fiber enable: true\n<br>");
    coroutine_conf.append("instance_name: ");
    coroutine_conf.append(fiber.instance_name + "\n<br>");
    coroutine_conf.append("concurrency_hint: ");
    coroutine_conf.append(std::to_string(fiber.concurrency_hint) + "\n<br>");
    coroutine_conf.append("scheduling_group_size: ");
    coroutine_conf.append(std::to_string(fiber.scheduling_group_size) + "\n<br>");
    coroutine_conf.append("stack_size: ");
    coroutine_conf.append(std::to_string(fiber.fiber_stack_size) + "\n<br>");
    coroutine_conf.append("fiber_run_queue_size: ");
    coroutine_conf.append(std::to_string(fiber.fiber_run_queue_size) + "\n<br>");
    if (fiber.numa_aware) {
      coroutine_conf.append("numa_aware: true\n");
    } else {
      coroutine_conf.append("numa_aware: false\n");
    }
  } else {
    coroutine_conf.append("fiber enable: false\n");
  }
  html->append(coroutine_conf);
  html->append("</td>\n</tr>\n");

  if (!gconf.threadmodel_config.default_model.empty()) {
    for (std::size_t i = 0; i < gconf.threadmodel_config.default_model.size(); i++) {
      html->append("<tr>\n");
      html->append("<td>thread_mode-" + std::to_string(i) + "</td>\n<br><td>");
      const auto& thread = gconf.threadmodel_config.default_model[i];
      std::string conf{""};
      conf.append("instance_name: ");
      conf.append(thread.instance_name + "\n<br>");
      conf.append("io_handle_type: ");
      conf.append(thread.io_handle_type + "\n<br>");
      conf.append("io_thread_num: ");
      conf.append(std::to_string(thread.io_thread_num )+ "\n<br>");
      conf.append("handle_thread_num: ");
      conf.append(std::to_string(thread.handle_thread_num) + "\n<br>");
      conf.append("io_thread_task_queue_size: ");
      conf.append(std::to_string(thread.io_thread_task_queue_size) + "\n<br>");
      conf.append("handle_thread_task_queue_size: ");
      conf.append(std::to_string(thread.handle_thread_task_queue_size) + "\n<br>");
      html->append(conf);
      html->append("</td>\n</tr>\n");
    }
  }

  html->append("</table>\n");
}

void DisplayServerConf(std::string* html, const trpc::ServerConfig& gserver) {
  html->append("<table class=\"gridtable sortable\" border=\"1\">\n");
  html->append("<tr><th border='1' colspan='2'>server config</th></tr>\n");

  html->append("<tr>\n<td>app</td>\n<td>");
  html->append(gserver.app);
  html->append("</td>\n</tr>\n");

  html->append("<tr>\n<td>server</td>\n<td>");
  html->append(gserver.server);
  html->append("</td>\n</tr>\n");

  for (std::size_t i = 0; i < gserver.services_config.size(); i++) {
    html->append("<tr>\n");
    html->append("<td>service-" + std::to_string(i) + "</td>\n<br><td>");
    std::string tmp_trans = "name: " + gserver.services_config[i].service_name + "\n<br>";
    tmp_trans += "socket_type: ";
    tmp_trans += gserver.services_config[i].socket_type;
    tmp_trans += "\n<br>network: ";
    tmp_trans += gserver.services_config[i].network;
    tmp_trans += "\n<br>ip: ";
    tmp_trans += gserver.services_config[i].ip;
    tmp_trans += "\n<br>port: ";
    tmp_trans += std::to_string(gserver.services_config[i].port);
    tmp_trans += "\n<br>protocol: ";
    tmp_trans += gserver.services_config[i].protocol;
    tmp_trans += "\n<br>max_conn_num: ";
    tmp_trans += std::to_string(gserver.services_config[i].max_conn_num);
    tmp_trans += "\n<br>queue_timeout: ";
    tmp_trans += std::to_string(gserver.services_config[i].queue_timeout);
    tmp_trans += "\n<br>timeout: ";
    tmp_trans += std::to_string(gserver.services_config[i].timeout);
    tmp_trans += "\n<br>disable_request_timeout: ";
    tmp_trans += std::to_string(gserver.services_config[i].disable_request_timeout);
    tmp_trans += "\n<br>threadmodel_instance_name: ";
    tmp_trans += gserver.services_config[i].threadmodel_instance_name;
    html->append(tmp_trans);
    html->append("</td>\n</tr>\n");
  }
  html->append("</table>\n");
}

void DisplayClientConf(std::string* html, const trpc::ClientConfig& gclient) {
  html->append("\n<br><br><br><table class=\"gridtable sortable\" border=\"1\">\n");
  html->append("<tr><th border='1' colspan='2'>client config</th></tr>\n");

  for (std::size_t i = 0; i < gclient.service_proxy_config.size(); i++) {
    html->append("<tr>\n");
    html->append("<td>proxy-" + std::to_string(i) + "</td>\n<td>");
    std::string tmp_trans = "name: " + gclient.service_proxy_config[i].name + "\n<br>";
    tmp_trans += "protocol: ";
    tmp_trans += gclient.service_proxy_config[i].protocol;
    tmp_trans += "\n<br>network: ";
    tmp_trans += gclient.service_proxy_config[i].network;
    tmp_trans += "\n<br>conn_type: ";
    tmp_trans += gclient.service_proxy_config[i].conn_type;
    tmp_trans += "\n<br>timeout: ";
    tmp_trans += std::to_string(gclient.service_proxy_config[i].timeout);
    tmp_trans += "\n<br>selector_name: ";
    tmp_trans += gclient.service_proxy_config[i].selector_name;
    tmp_trans += "\n<br>target: ";
    tmp_trans += gclient.service_proxy_config[i].target;
    tmp_trans += "\n<br>threadmodel_instance_name: ";
    tmp_trans += gclient.service_proxy_config[i].threadmodel_instance_name;
    html->append(tmp_trans);
    html->append("</td>\n</tr>\n");
  }

  html->append("</table>\n");
}
}  // namespace

void WebReloadConfigHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                           rapidjson::Document::AllocatorType& alloc) {
  std::map<std::string, CmdInfo> m_tabs;
  SetTabItems(&m_tabs);

  std::string html = HtmlHeader();
  html.append(TabsHead());
  html.append(GridTableStyle());
  html.append("</head>\n");

  html.append("<body>\n");
  PrintTabsBody(&html, m_tabs);
  html.append("<br>\n<br>\n");

  // get global config
  const trpc::GlobalConfig& gconf = trpc::TrpcConfig::GetInstance()->GetGlobalConfig();
  DisplayGlobalConf(&html, gconf);

  // get server config
  const trpc::ServerConfig& gserver = trpc::TrpcConfig::GetInstance()->GetServerConfig();
  DisplayServerConf(&html, gserver);

  // get client config
  const trpc::ClientConfig& gclient = trpc::TrpcConfig::GetInstance()->GetClientConfig();
  DisplayClientConf(&html, gclient);

  html.append("\n\n");
  html.append("</body>\n</html>\n");

  result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(html, alloc).Move(), alloc);
}

}  // namespace trpc::admin
