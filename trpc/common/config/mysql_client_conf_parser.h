#pragma once

#include "trpc/common/config/mysql_client_conf.h"
#include "trpc/common/config/mysql_connect_pool_conf_parser.h"
#include "yaml-cpp/yaml.h"

namespace YAML {

// 特化 YAML 转换逻辑以支持 trpc::MysqlClientConf
template <>
struct convert<trpc::MysqlClientConf> {
  // 将 trpc::MysqlClientConf 对象编码为 YAML 节点
  static YAML::Node encode(const trpc::MysqlClientConf& mysql_conf) {
    YAML::Node node;
    node["user_name"] = mysql_conf.user_name;
    node["password"] = mysql_conf.password;
    node["dbname"] = mysql_conf.dbname;
    node["ip"] = mysql_conf.ip;
    node["port"] = mysql_conf.port;
    node["enable"] = mysql_conf.enable;

    // 编码 connectpool 部分
    node["connectpool"] = mysql_conf.connectpool;

    return node;
  }

  // 从 YAML 节点解码为 trpc::MysqlClientConf 对象
  static bool decode(const YAML::Node& node, trpc::MysqlClientConf& mysql_conf) {
    if (node["user_name"]) {
      mysql_conf.user_name = node["user_name"].as<std::string>();
    }
    if (node["password"]) {
      mysql_conf.password = node["password"].as<std::string>();
    }
    if (node["dbname"]) {
      mysql_conf.dbname = node["dbname"].as<std::string>();
    }
    if (node["ip"]) {
      mysql_conf.ip = node["ip"].as<std::string>();
    }
    if (node["port"]) {
      mysql_conf.port = node["port"].as<std::string>();
    }
    if (node["enable"]) {
      mysql_conf.enable = node["enable"].as<bool>();
    }

    if (node["connectpool"]) {
      mysql_conf.connectpool = node["connectpool"].as<trpc::MysqlConnectPoolConf>();
    }

    return true;
  }
};

}  // namespace YAML
