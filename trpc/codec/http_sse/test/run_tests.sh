#!/bin/bash

# HTTP SSE Codec Test Runner
# This script helps run tests for the HTTP SSE codec

set -e

echo "=== HTTP SSE Codec Test Runner ==="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "BUILD" ]; then
    print_error "BUILD file not found. Please run this script from the trpc-cpp root directory."
    exit 1
fi

print_status "Building HTTP SSE codec tests..."

# Build the tests
if bazel build //trpc/codec/http_sse/test:http_sse_codec_test //trpc/codec/http_sse/test:http_sse_proto_checker_test; then
    print_status "Build successful!"
else
    print_error "Build failed!"
    exit 1
fi

echo
print_status "Running HTTP SSE codec tests..."

# Run the main codec tests
if bazel test //trpc/codec/http_sse/test:http_sse_codec_test --test_output=all; then
    print_status "HTTP SSE codec tests passed!"
else
    print_error "HTTP SSE codec tests failed!"
    exit 1
fi

echo
print_status "Running HTTP SSE protocol checker tests..."

# Run the protocol checker tests
if bazel test //trpc/codec/http_sse/test:http_sse_proto_checker_test --test_output=all; then
    print_status "HTTP SSE protocol checker tests passed!"
else
    print_error "HTTP SSE protocol checker tests failed!"
    exit 1
fi

echo
print_status "All tests completed successfully!"
print_status "Test coverage includes:"
echo "  ✓ HTTP SSE Request Protocol"
echo "  ✓ HTTP SSE Response Protocol"
echo "  ✓ HTTP SSE Client Codec"
echo "  ✓ HTTP SSE Server Codec"
echo "  ✓ Protocol Checker Functions"
echo "  ✓ Edge Cases and Error Handling"
echo "  ✓ Integration Tests"

echo
print_status "To run individual tests, use:"
echo "  bazel test //trpc/codec/http_sse/test:http_sse_codec_test --test_filter=TestName"
echo "  bazel test //trpc/codec/http_sse/test:http_sse_proto_checker_test --test_filter=TestName"
