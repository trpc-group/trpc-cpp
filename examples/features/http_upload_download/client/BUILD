licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "download_client",
    srcs = ["download_client.cc"],
    linkopts = ["-ldl"],
   
    deps = [
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client/http:http_service_proxy",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@trpc_cpp//trpc/common:status",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/coroutine:fiber",
        "@trpc_cpp//trpc/util/log:logging",
        "//examples/features/http_upload_download/common:file_hasher", 
    ],
)

cc_binary(
    name = "upload_client",
    srcs = ["upload_client.cc"],
    linkopts = ["-ldl"],
    deps = [
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client/http:http_service_proxy",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@trpc_cpp//trpc/common:status",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/coroutine:fiber",
        "@trpc_cpp//trpc/util/log:logging",

        "//examples/features/http_upload_download/common:file_hasher",
    ],
)
