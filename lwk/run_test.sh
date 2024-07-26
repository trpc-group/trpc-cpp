#!/bin/bash


echo "Running Bazel test..."
bazel test //trpc/naming/common/util/loadbalance/weightpolling:test_weight_polling_load_balance


if [ $? -eq 0 ]; then
    echo "Bazel test completed successfully."
else
    echo "Bazel test failed."
    exit 1
fi


echo "Running test executable..."
../bazel-bin/trpc/naming/common/util/loadbalance/weightpolling/test_weight_polling_load_balance


if [ $? -eq 0 ]; then
    echo "Test executable completed successfully."
else
    echo "Test executable failed."
    exit 1
fi
