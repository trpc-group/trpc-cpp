#!/bin/bash
bazel build //examples/features/lru_cache/server:lru_cache_service
bazel-bin/examples/features/lru_cache/server/lru_cache_service
