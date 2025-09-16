# BUILD - put next to sse_server.cc

cc_binary(
    name = "http_sse_server",
    srcs = ["http_sse_server.cc"],
    copts = ["-std=c++17"],
    # 注意：下面的 deps 是常见的/示例性的 trpc target 标签，可能需要替换为你仓库中的真实 target 名称。
    deps = [
        "//trpc/common:trpc_app",                       
        "//trpc/server:http_service",                   
        "//trpc/util/http:function_handlers",           
        "//trpc/util/http:http_handler",                
        "//trpc/util/http:routes",                       
        "//trpc/util/log:logging",                       
        "//trpc/util/http/sse:http_sse",               
        "//trpc/stream/http:http_sse_stream",           
        # 下面的依赖项按需添加：codec、transport、runtime 等
        #"//trpc/codec:http:http_protocol",
       #"//trpc/codec:http_sse:http_sse_protocol",
       #"//trpc/codec/http_sse:http_sse_server_codec",
    ],
)
