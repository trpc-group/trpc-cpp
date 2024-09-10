#pragma once

#include "trpc/common/config/mysql_connect_pool_conf.h"
#include "yaml-cpp/yaml.h"

// 特化 YAML 转换逻辑以支持 trpc::MysqlConnectPoolConf
namespace YAML {

template <>
struct convert<trpc::MysqlConnectPoolConf> {
  // 将 trpc::MysqlConnectPoolConf 对象编码为 YAML 节点
  static YAML::Node encode(const trpc::MysqlConnectPoolConf& pool_conf) {
    YAML::Node node;
    node["minSize"] = pool_conf.minSize;
    node["maxSize"] = pool_conf.maxSize;
    node["maxIdleTime"] = pool_conf.maxIdleTime;
    node["timeout"] = pool_conf.timeout;
    return node;
  }

  // 从 YAML 节点解码为 trpc::MysqlConnectPoolConf 对象
  static bool decode(const YAML::Node& node, trpc::MysqlConnectPoolConf& pool_conf) {
    if (node["minSize"]) {
      pool_conf.minSize = node["minSize"].as<uint32_t>();
    }
    if (node["maxSize"]) {
      pool_conf.maxSize = node["maxSize"].as<uint32_t>();
    }
    if (node["maxIdleTime"]) {
      pool_conf.maxIdleTime = node["maxIdleTime"].as<uint32_t>();
    }
    if (node["timeout"]) {
      pool_conf.timeout = node["timeout"].as<uint32_t>();
    }
    return true;
  }
};

}  // namespace YAML