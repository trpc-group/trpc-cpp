#include "lru_cache_service.h"

trpc::Status LruCacheServiceImpl::Put(trpc::ServerContextPtr,
                                      const trpc::examples::lru::PutRequest* request,
                                      trpc::examples::lru::PutResponse* response) {
  response->set_success(cache_.Put(request->key(), request->value()));
  return trpc::kSuccStatus;
}

trpc::Status LruCacheServiceImpl::Get(trpc::ServerContextPtr,
                                      const trpc::examples::lru::GetRequest* request,
                                      trpc::examples::lru::GetResponse* response) {
  std::string value;
  bool found = cache_.Get(request->key(), value);
  response->set_found(found);
  if (found) response->set_value(value);
  return trpc::kSuccStatus;
}
