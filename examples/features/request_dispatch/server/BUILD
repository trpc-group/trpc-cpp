cc_binary(
    name = "demo_server",
    srcs = ["demo_server.cc"],
    deps = [
        "//examples/helloworld:helloworld_proto",
        "@com_github_fmtlib_fmt//:fmtlib",
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/codec/trpc:trpc_protocol",
        "@trpc_cpp//trpc/log:trpc_log",
        "@trpc_cpp//trpc/runtime/threadmodel/common:worker_thread",
        "@trpc_cpp//trpc/util/algorithm:random",
        "@trpc_cpp//trpc/util/buffer:zero_copy_stream",
    ],
)
