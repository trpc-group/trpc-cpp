# 1.SYNOPSIS
This tool named trpc_cpp_plugin is a pb compiler plugin to generate trpc server and client cpp code out of Protobuf IDL.

# 2.BUILD
Enter the root directory of project trpc-cpp and then execute following command.
```
bazel build @com_google_protobuf
bazel build //trpc/tools/trpc_cpp_plugin:trpc_cpp_plugin
```

If succeed, you will get trpc_cpp_plugin in directory bazel-bin/trpc/tools/trpc_cpp_plugin and protoc in directory bazel-out/host/bin/external/com_google_protobuf.

# 3.USAGE
```
bazel-out/host/bin/external/com_google_protobuf/protoc --plugin=protoc-gen-trpc=[trpc_cpp_plugin path] -I[proto file directory] --trpc_out=[OUT_DIR] [proto file]
```

For example, execute following command, and then you will get two files helloworld.trpc.pb.h and helloworld.trpc.pb.cc in directory ./trpc/proto/testing.
```
bazel-out/host/bin/external/com_google_protobuf/protoc --plugin=protoc-gen-trpc=./bazel-bin/trpc/tools/trpc_cpp_plugin/trpc_cpp_plugin --trpc_out=. -I. ./trpc/proto/testing/helloworld.proto
```

If you want get mock code, you need to set flag(generate_mock_code=true) on trpc plugin for protoc, just like:
```
bazel-out/host/bin/external/com_google_protobuf/protoc --plugin=protoc-gen-trpc=[trpc_cpp_plugin path] --trpc_out=generate_mock_code=true:[OUT_DIR] -I[proto file directory] [proto file]
```
