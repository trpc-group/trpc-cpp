#pragma once
#include "proto/lru_cache.pb.h"
#include "proto/lru_cache.trpc.pb.h"
#include "lru_cache.h"

class LruCacheServiceImpl : public trpc::examples::lru::LruCacheService {
 public:
  LruCacheServiceImpl() : cache_(1000) {}

  trpc::Status Put(trpc::ServerContextPtr context,
                   const trpc::examples::lru::PutRequest* request,
                   trpc::examples::lru::PutResponse* response) override;

  trpc::Status Get(trpc::ServerContextPtr context,
                   const trpc::examples::lru::GetRequest* request,
                   trpc::examples::lru::GetResponse* response) override;

 private:
  LRUCache<std::string, std::string, trpc::FiberMutex> cache_;
};