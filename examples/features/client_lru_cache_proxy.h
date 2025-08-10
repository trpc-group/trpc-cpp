#pragma once
#include "proto/lru_cache.pb.h"
#include "proto/lru_cache.trpc.pb.h"
#include "trpc/client/service_proxy.h"

class LruCacheProxy : public trpc::ServiceProxy {
 public:
  using PutRequest = trpc::examples::lru::PutRequest;
  using PutResponse = trpc::examples::lru::PutResponse;
  using GetRequest = trpc::examples::lru::GetRequest;
  using GetResponse = trpc::examples::lru::GetResponse;

  TRPC_PROXY_METHOD(trpc::examples::lru::LruCacheService, Put, PutRequest, PutResponse)
  TRPC_PROXY_METHOD(trpc::examples::lru::LruCacheService, Get, GetRequest, GetResponse)
};
