cc_binary(
    name = "demo_server",
    srcs = ["demo_server.cc"],
    deps = [
        "//examples/helloworld:helloworld_proto",
        "@com_github_fmtlib_fmt//:fmtlib",
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/log:trpc_log",
    ],
)
