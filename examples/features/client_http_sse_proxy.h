#pragma once
#include "lru_cache_proxy.h"

class HttpSseProxy : public LruCacheProxy {
 public:
  void RegisterSseHandler(); // 可选：注册 SSE 接口到 Web Server
};
