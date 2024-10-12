
#! /bin/bash

# building.
mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/mysql/build && cd examples/features/mysql/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

# run mysql client.
examples/features/mysql/build/fiber_client --client_config=examples/features/mysql/client/fiber/fiber_client_config.yaml
examples/features/mysql/build/future_client --client_config=examples/features/mysql/client/future/future_client_config.yaml