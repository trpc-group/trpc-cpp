#! /bin/bash

# building.
bazel build //examples/features/mysql/client/...

# run mysql client.
./bazel-bin/examples/features/mysql/client/fiber/fiber_client --client_config=examples/features/mysql/client/fiber/fiber_client_config.yaml
./bazel-bin/examples/features/mysql/client/future/future_client --client_config=examples/features/mysql/client/future/future_client_config.yaml