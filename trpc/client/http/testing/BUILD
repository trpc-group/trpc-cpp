licenses(["notice"])

package(default_visibility = ["//visibility:public"])

exports_files(["http_fiber.yaml", "http_fiber_with_ssl.yaml"])

cc_library(
    name = "http_client_call_testing",
    srcs = ["http_client_call_testing.cc"],
    hdrs = ["http_client_call_testing.h"],
    deps = [
        "//trpc/client:make_client_context",
        "//trpc/client/http:http_service_proxy",
        "//trpc/proto/testing:cc_helloworld_proto",
        "@com_github_tencent_rapidjson//:rapidjson",
    ] + select({
           "//trpc:include_ssl": [
               "//trpc/transport/common:ssl_helper",
               "//trpc/transport/common/ssl",
               "//trpc/transport/common/ssl:core",
           ],
           "//trpc:trpc_include_ssl": [
               "//trpc/transport/common:ssl_helper",
               "//trpc/transport/common/ssl",
               "//trpc/transport/common/ssl:core",
           ],
           "//conditions:default": [],
       }),
)
