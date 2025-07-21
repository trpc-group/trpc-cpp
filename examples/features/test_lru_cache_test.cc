#include <gtest/gtest.h>
#include "server/lru_cache.h"

TEST(LruCacheTest, PutGetEvict) {
  LRUCache<std::string, std::string> cache(2);
  cache.Put("a", "1");
  cache.Put("b", "2");
  std::string v;
  EXPECT_TRUE(cache.Get("a", v));
  EXPECT_EQ(v, "1");
  cache.Put("c", "3");
  EXPECT_FALSE(cache.Get("b", v));
}
