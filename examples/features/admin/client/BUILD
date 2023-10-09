licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "client",
    srcs = ["client.cc"],
    deps = [
        "//examples/features/admin/proxy:forward_proto",
        "@com_github_gflags_gflags//:gflags",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/common:runtime_manager",
    ],
)
