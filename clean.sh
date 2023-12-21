#! /bin/bash

bazel clean --expunge

# Delete xx.log or xx.bin on root directory of trpc-cpp project.
find ./ -maxdepth 1 -name "*.log" -exec rm {} \;
find ./ -maxdepth 1 -name "*.bin" -exec rm {} \;
