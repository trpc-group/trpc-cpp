#
#
# Copyright (C) 2014 gRPC authors
# gRPC is licensed under the Apache 2.0 License.
# The source codes in this file based on
# https://github.com/grpc/grpc/blob/v1.39.1/bazel/protobuf.bzl.
# https://github.com/grpc/grpc/blob/v1.39.1/bazel/cc_grpc_library.bzl
# This source file may have been modified by THL A29 Limited, and licensed under the Apache 2.0 License.
#
#
"""This module is used to generate corresponding cc/h files and stub code for pb files."""
load("@bazel_skylib//lib:versions.bzl", "versions")

def _GetPath(ctx, path):
    if ctx.label.workspace_root:
        return ctx.label.workspace_root + "/" + path
    else:
        return path

def _IsNewExternal(ctx):
    # Bazel 0.4.4 and older have genfiles paths that look like:
    #   bazel-out/local-fastbuild/genfiles/external/repo/foo
    # After the exec root rearrangement, they look like:
    #   ../repo/bazel-out/local-fastbuild/genfiles/foo
    return ctx.label.workspace_root.startswith("../")

def _GenDir(ctx):
    if _IsNewExternal(ctx):
        # We are using the fact that Bazel 0.4.4+ provides repository-relative paths
        # for ctx.genfiles_dir.
        return ctx.genfiles_dir.path + (
            "/" + ctx.attr.includes[0] if ctx.attr.includes and ctx.attr.includes[0] else ""
        )

    # This means that we're either in the old version OR the new version in the local repo.
    # Either way, appending the source path to the genfiles dir works.
    return ctx.var["GENDIR"] + "/" + _SourceDir(ctx)

def _SourceDir(ctx):
    if not ctx.attr.includes:
        return ctx.label.workspace_root
    if not ctx.attr.includes[0]:
        return _GetPath(ctx, ctx.label.package)
    if not ctx.label.package:
        return _GetPath(ctx, ctx.attr.includes[0])
    return _GetPath(ctx, ctx.label.package + "/" + ctx.attr.includes[0])

def _ReplaceProtoExt(srcs, ext):
    # ret = [s.replace("proto", ext) for s in srcs]
    ret = [s[:-len(".proto")] + "." + ext for s in srcs]
    return ret

def _CcHdrs(srcs, use_plugin = False, gen_mock = False, ext = "", gen_pbcc = True):  #replace whit protocal name:trpc,nrpc...
    ret1 = []
    ret2 = []
    if gen_pbcc:
        ret1 = [s[:-len(".proto")] + ".pb.h" for s in srcs]
    if use_plugin:
        if gen_mock:
            tmp = ext
            tmp = tmp + ".pb.mock.h"
            ret2 += _ReplaceProtoExt(srcs, tmp)
        ext = ext + ".pb.h"
        ret2 += _ReplaceProtoExt(srcs, ext)
    return struct(
        raw = ret1,
        plugin = ret2,
    )

def _CcSrcs(srcs, use_plugin = False, ext = "", gen_pbcc = True):  #replace whit protocal name:trpc,nrpc...
    ret1 = []
    ret2 = []
    if gen_pbcc:
        ret1 = [s[:-len(".proto")] + ".pb.cc" for s in srcs]
    if use_plugin:
        ext = ext + ".pb.cc"
        ret2 += _ReplaceProtoExt(srcs, ext)
    return struct(
        raw = ret1,
        plugin = ret2,
    )

def _CcOuts(srcs, use_plugin = False, gen_mock = False, ext = "", gen_pbcc = True):  #replace param...
    hdrs = _CcHdrs(srcs, use_plugin, gen_mock, ext, gen_pbcc)
    srcs = _CcSrcs(srcs, use_plugin, ext, gen_pbcc)
    return hdrs.raw + hdrs.plugin + srcs.raw + srcs.plugin

TrpcProtoCollectorInfo = provider(
    "Proto file collector by aspect",
    fields = {
        "proto": "depset of pb files collected from proto_library",
        "proto_deps": "depset of pb files being directly depented by cc_proto_library",
        "import_flags": "depset of import flags(resolve the external deps not found issue)",
    },
)

def _proto_collector_aspect_impl(target, ctx):
    proto = []
    import_flags = []
    proto_deps = []
    if ProtoInfo in target:
        for src in ctx.rule.attr.srcs:
            for pbfile in src.files.to_list():
                proto.append(pbfile)
                proto_deps.append(pbfile)

                # If you are using external dependencies imported through "@" in your code, you need to set additional
                # include paths to ensure that the compiler can find the header files of these libraries.
                if len(target.label.workspace_root) > 0:
                    import_flags.append("-I" + target.label.workspace_root)

                    # "com_google_protobuf" generates .pb files in the bin directory. If you are using protobuf in
                    # your project, you need to include this path.
                    import_flags.append("-I" + ctx.bin_dir.path + "/" + target.label.workspace_root)

                    # In newer versions of protobuf, such as protobuf 3.14, the import path has changed.
                    # Therefore, you need to add the following prefix to keep up with the changes.
                    if target.label.workspace_root == Label("@com_google_protobuf//:protoc").workspace_root:
                        import_flags.append("-I" + target.label.workspace_root + "/src")

    for dep in ctx.rule.attr.deps:
        proto += [pbfile for pbfile in dep[TrpcProtoCollectorInfo].proto.to_list()]
        import_flags += [flag for flag in dep[TrpcProtoCollectorInfo].import_flags.to_list()]
        if CcInfo in target and ProtoInfo in dep:
            # "cc_proto_library" can only depend on one "proto_library", so the following assignment only occurs once.
            for proto_dep in dep[TrpcProtoCollectorInfo].proto_deps.to_list():
                proto_deps.append(proto_dep)

    return [TrpcProtoCollectorInfo(
        proto = depset(proto),
        proto_deps = depset(proto_deps),
        import_flags = depset(import_flags),
    )]

proto_collector_aspect = aspect(
    implementation = _proto_collector_aspect_impl,
    attr_aspects = ["deps"],
)

def _proto_gen_impl(ctx):
    """General implementation for generating protos"""
    srcs = ctx.files.srcs
    deps = []
    deps += ctx.files.srcs

    # When "srcs" and "deps" are empty, it means generating trpc stub code for "native_proto_deps" and
    # "native_cc_proto_deps".
    # It is compatible with the scenario where "srcs" does not exist but there are
    # multiple "trpc_proto_library" in "deps".
    pblibdep_gen_trpc_stub = (len(ctx.attr.srcs) == 0 and len(ctx.attr.deps) == 0)

    # Currently, only "trpc_proto_library" is supported.
    if ctx.executable.plugin == None or "trpc_cpp_plugin" != ctx.executable.plugin.basename:
        pblibdep_gen_trpc_stub = False

    source_dir = _SourceDir(ctx)
    gen_dir = _GenDir(ctx).rstrip("/")
    if source_dir:
        import_flags = ["-I" + source_dir, "-I" + gen_dir]
    else:
        import_flags = ["-I."]

    for dep in ctx.attr.deps:
        # It is compatible with the issue of "depset" type when both "bazel-4.2" and "protobuf-3.20" are
        # present at the same time.
        # Note that "protobuf-3.20" can only be compiled successfully with "bazel-4" or higher.
        if type(dep.proto.import_flags) == "depset":
            import_flags += dep.proto.import_flags.to_list()
        else:
            import_flags += dep.proto.import_flags

        if type(dep.proto.deps) == "depset":
            deps += dep.proto.deps.to_list()
        else:
            deps += dep.proto.deps

    for dep in ctx.attr.native_cc_proto_deps:
        deps += dep[TrpcProtoCollectorInfo].proto.to_list()
        import_flags += dep[TrpcProtoCollectorInfo].import_flags.to_list()

    # Let pb files be imported through the current folder.
    import_flags_dirs = ["-I" + dep.dirname for dep in deps]

    # Remove duplicates
    deps = depset(deps).to_list()
    import_flags = depset(import_flags).to_list()
    import_flags_dirs = depset(import_flags_dirs).to_list()

    if not ctx.attr.gen_cc and not ctx.executable.plugin:
        # buildifier: disable=rule-impl-return
        return struct(
            proto = struct(
                srcs = srcs,
                import_flags = import_flags,
                deps = deps,
            ),
        )
    actual_srcs = [src for src in srcs]
    if pblibdep_gen_trpc_stub:
        pb_dep_files = ctx.attr.native_cc_proto_deps[0][TrpcProtoCollectorInfo].proto_deps.to_list()
        if len(pb_dep_files) != 1:
            fail("Target of native_proto_deps must have only one .proto file. Or native_cc_proto_deps can only dep on proto_library which has only one .proto file")
        actual_srcs += pb_dep_files

    import_flags_real = []
    for src in actual_srcs:
        args = []

        in_gen_dir = src.root.path == gen_dir
        if in_gen_dir:
            for f in depset(import_flags).to_list():
                path = f.replace("-I", "")
                import_flags_real.append("-I$(realpath -s %s)" % path)
            for f in import_flags_dirs:
                path = f.replace("-I", "")
                import_flags_real.append("-I$(realpath -s %s)" % path)

        outs = []
        use_plugin = (len(ctx.attr.plugin_language) > 0 and ctx.attr.plugin)  #expand to trpc

        path_tpl = "$(realpath %s)" if in_gen_dir else "%s"
        if ctx.attr.gen_cc:
            if ctx.attr.gen_pbcc:
                args.append(("--cpp_out=" + path_tpl) % gen_dir)
            if ctx.attr.generate_new_mock_code:
                outs.extend(_CcOuts([src.basename], use_plugin, ctx.attr.generate_new_mock_code, ctx.attr.plugin_language, ctx.attr.gen_pbcc))
            else:
                outs.extend(_CcOuts([src.basename], use_plugin, ctx.attr.generate_mock_code, ctx.attr.plugin_language, ctx.attr.gen_pbcc))

        if pblibdep_gen_trpc_stub:
            # If you don't fill in the "sibling" field, Bazel will generate stub code in the same package
            filename = ctx.label.name[:-len("_genproto")]
            include_suffix = [".trpc.pb.h", ".trpc.pb.cc"]
            if ctx.attr.generate_new_mock_code or ctx.attr.generate_mock_code:
                include_suffix.append(".trpc.pb.mock.h")
            outs = [ctx.actions.declare_file(filename + suffix) for suffix in include_suffix]
        else:
            outs = [ctx.actions.declare_file(out, sibling = src) for out in outs]

        inputs = [src] + deps
        tools = []
        if ctx.executable.plugin:
            plugin = ctx.executable.plugin
            lang = ctx.attr.plugin_language
            if not lang and plugin.basename.startswith("protoc-gen-"):
                lang = plugin.basename[len("protoc-gen-"):]
            if not lang:
                fail("cannot infer the target language of plugin", "plugin_language")

            outdir = "." if in_gen_dir else gen_dir

            if ctx.attr.plugin_options:
                outdir = ",".join(ctx.attr.plugin_options) + ":" + outdir
            args.append(("--plugin=protoc-gen-%s=" + path_tpl) % (lang, plugin.path))

            # If both "generate_new_mock_code" and "generate_mock_code" are set to true, the newer version of
            # "MockMethod" from "gmock" will be used first.
            if ctx.attr.generate_new_mock_code:
                outdir = "generate_new_mock_code=true:" + outdir
            elif ctx.attr.generate_mock_code:
                outdir = "generate_mock_code=true:" + outdir

            if ctx.attr.enable_explicit_link_proto:
                if ctx.attr.generate_new_mock_code or ctx.attr.generate_mock_code:
                    outdir = "enable_explicit_link_proto=true," + outdir
                else:
                    outdir = "enable_explicit_link_proto=true:" + outdir

            if pblibdep_gen_trpc_stub:
                # To pass parameters to a plugin to generate stub code with appropriate paths
                filename = ctx.label.name[:-len("_genproto")]
                generate_trpc_stub_path = ctx.label.package + "/" + filename
                params = "generate_trpc_stub_path=" + generate_trpc_stub_path

                # When generating stub code using "proto_library" in "native_proto_library", and "strip_import_prefix"
                # is used in "proto_library", the include header file path needs to be adjusted to the project root
                # directory (Even though the semantics of "strip_import_prefix" is to remove subdirectories, the pb
                # files are still generated in the root directory. It is unclear whether this is a bug in the
                # implementation of Bazel or an error in the semantic description.). This option is only needed in
                # special scenarios.
                if ctx.attr.proto_include_prefix != "ProtoIncludePrefixNotValid":
                    params = params + ",proto_include_prefix=" + ctx.attr.proto_include_prefix

                # Avoid passing incorrect parameter when there is no mock code generation.
                if ctx.attr.generate_new_mock_code or ctx.attr.generate_mock_code:
                    outdir = params + "," + outdir
                else:
                    outdir = params + ":" + outdir

            args.append("--%s_out=%s" % (lang, outdir))
            tools = [plugin]

        if not in_gen_dir:
            ctx.actions.run(
                inputs = inputs,
                tools = tools,
                outputs = outs,
                progress_message = "Invoking Protoc " + ctx.label.name,
                arguments = args + import_flags + import_flags_dirs + [src.path],
                executable = ctx.executable.protoc,
                mnemonic = "ProtoCompile",
                use_default_shell_env = True,
            )
        else:
            for out in outs:
                orig_command = " ".join(
                    ["$(realpath %s)" % ctx.executable.protoc.path] + args +
                    import_flags_real + ["-I.", src.basename],
                )
                command = ";".join([
                    'CMD="%s"' % orig_command,
                    "cd %s" % src.dirname,
                    "${CMD}",
                    "cd -",
                ])
                generated_out = "/".join([gen_dir, out.basename])
                if generated_out != out.path:
                    command += ";mv %s %s" % (generated_out, out.path)
                ctx.actions.run_shell(
                    inputs = inputs + [ctx.executable.protoc],
                    outputs = [out],
                    command = command,
                    mnemonic = "ProtoCompile",
                    use_default_shell_env = True,
                )

    # buildifier: disable=rule-impl-return
    return struct(
        proto = struct(
            srcs = srcs,
            import_flags = import_flags,
            deps = deps,
        ),
    )

proto_gen = rule(
    attrs = {
        "srcs": attr.label_list(allow_files = True),
        "deps": attr.label_list(providers = ["proto"]),
        "native_cc_proto_deps": attr.label_list(providers = [CcInfo, OutputGroupInfo], aspects = [proto_collector_aspect]),
        "proto_include_prefix": attr.string(default = "ProtoIncludePrefixNotValid"),
        "includes": attr.string_list(),
        "protoc": attr.label(
            cfg = "host",
            executable = True,
            allow_single_file = True,
            mandatory = True,
        ),
        "plugin": attr.label(
            cfg = "host",
            allow_files = True,
            executable = True,
        ),
        "plugin_language": attr.string(),
        "plugin_options": attr.string_list(),
        "gen_cc": attr.bool(),
        "gen_pbcc": attr.bool(default = True),
        "outs": attr.output_list(),
        "generate_mock_code": attr.bool(default = False),
        "generate_new_mock_code": attr.bool(default = False),
        "enable_explicit_link_proto": attr.bool(default = False),
    },
    output_to_genfiles = True,
    implementation = _proto_gen_impl,
)

# buildifier: disable=no-effect
"""Generates codes from Protocol Buffers definitions.

This rule helps you to implement Skylark macros specific to the target
language. tc_cc_proto_library is based cc_proto_library and surport not only grpc, but 
any IDL with it's plugin. you should wrraper it with a micro to specify the plugin and libs
you relied on.
Args:
  srcs: Protocol Buffers definition files (.proto) to run the protocol compiler
    against.
  deps: a list of dependency labels; must be other proto libraries.
  includes: a list of include paths to .proto files.
  protoc: the label of the protocol compiler to generate the sources.
  plugin: the label of the protocol compiler plugin to be passed to the protocol
    compiler.
  plugin_language: the language of the generated sources 
  plugin_options: a list of options to be passed to the plugin
  gen_cc: generates C++ sources in addition to the ones from the plugin.
  outs: a list of labels of the expected outputs from the protocol compiler.
"""

# buildifier: disable=function-docstring-args
def tc_cc_proto_library(
        name,
        srcs = [],
        deps = [],
        native_proto_deps = [],
        native_cc_proto_deps = [],
        # 1.The reason for adding this option is that when "strip_import_prefix" is used in "proto_library",
        #   the generated pb stub code will be placed in the project root directory, which causes the problem of
        #   not being able to find the "trpc.pb.h" header file.
        # 2.In order to prevent conflicts with business code, the default value is explicitly
        #   set to "ProtoIncludePrefixNotValid".
        # 3.For the sake of clarity, if it is an empty string, it means that the generated "pb" files will be
        # placed in the root directory. If it is another directory, such as "test/proto", please change it to that.
        # If it is in another directory, such as "test/proto" directory, please change it to "test/proto".
        # 4.Generally, if "strip_import_prefix" is not used, there is no need to set this value.
        #   Please delete this option.
        proto_include_prefix = "ProtoIncludePrefixNotValid",
        cc_libs = [],
        include = None,
        protoc = "@com_google_protobuf//:protoc",
        use_plugin = False,
        plugin_language = "",
        generate_mock_code = False,
        generate_new_mock_code = False,
        plugin = "",
        default_runtime = "@com_google_protobuf//:protobuf",
        gen_pbcc = True,
        enable_explicit_link_proto = False,
        **kargs):
    """Bazel rule to create a C++ protobuf library from proto source files

    NOTE: the rule is only an internal workaround to generate protos. The
    interface may change and the rule may be removed when bazel has introduced
    the native rule.

    Args:
      name: the name of the cc_proto_library.
      srcs: the .proto files of the cc_proto_library.
      deps: a list of dependency labels; must be cc_proto_library.
      cc_libs: a list of other cc_library targets depended by the generated
          cc_library.
      include: a string indicating the include path of the .proto files.
      protoc: the label of the protocol compiler to generate the sources.
      use_plugin: a flag to indicate whether to call the C++ plugin
      plugin_language: indicate which RPC protocol to use,like trpc, grpc, nrpc...
          when processing the proto files.
      default_runtime: the implicitly default runtime which will be depended on by
          the generated cc_library target.
      **kargs: other keyword arguments that are passed to cc_library.

    """

    includes = []
    if include != None:
        includes = [include]

    # To ensure the correctness of generating stub code for pb files, it is necessary to perform pre-parameter checks
    stub_gen_hdrs = []
    stub_gen_srcs = []

    # When "srcs" and "deps" are empty, it indicates that trpc stub code will be generated for
    # "native_proto_deps" and "native_cc_proto_deps".
    # If the current "srcs" does not exist, but there are multiple "trpc_proto_library" in "deps", the "srcs" and "deps"
    # parameters of these libraries can be merged and used as the "srcs" and "deps" parameters of the current library.
    # This ensures that the generated code includes all necessary proto files and dependent libraries.
    pblibdep_gen_trpc_stub = (len(srcs) == 0 and len(deps) == 0)

    # The current support is only for "trpc_proto_library".
    if plugin == None or plugin.find("trpc_cpp_plugin") < 0:
        pblibdep_gen_trpc_stub = False
    if pblibdep_gen_trpc_stub:
        if len(native_proto_deps + native_cc_proto_deps) != 1:
            fail("Keep only one dep in native_proto_deps either native_cc_proto_deps when you want to generate stub code")

        # The generated TRPC stub code file names are uniformly named after the target (because it is not possible to
        # determine which PB file is being used at the macro definition stage).
        stub_gen_hdrs = [name + ".trpc.pb.h"]
        stub_gen_srcs = [name + ".trpc.pb.cc"]
        if generate_mock_code or generate_new_mock_code:
            stub_gen_hdrs.append(name + ".trpc.pb.mock.h")

    # Convert the dependencies of "proto_library" and "cc_proto_library" to the dependencies of "cc_proto_library".
    acutal_native_proto_deps = []
    pblib_target_suffix = "_genpblibproto"
    for dep in native_proto_deps:
        if dep.startswith(":"):
            dep_name = dep[1:] + pblib_target_suffix
        elif dep.startswith("@") or dep.startswith("/"):
            dep_name = Label(dep).name + pblib_target_suffix
        else:
            dep_name = dep
        acutal_native_proto_deps.append(dep_name)
        if dep_name not in native.existing_rules():
            native.cc_proto_library(
                name = dep_name,
                deps = [dep],
            )

    gen_srcs = _CcSrcs(srcs, use_plugin, plugin_language, gen_pbcc)

    # Set the default value for "gen_hdrs".
    gen_hdrs = _CcHdrs(srcs, use_plugin, False, plugin_language, gen_pbcc)

    # If "generate_new_mock_code" and "generate_mock_code" are both true, then the newer version of
    # "MockMethod" from GMock should be used first.
    if generate_new_mock_code:
        gen_hdrs = _CcHdrs(srcs, use_plugin, generate_new_mock_code, plugin_language, gen_pbcc)
    elif generate_mock_code:
        gen_hdrs = _CcHdrs(srcs, use_plugin, generate_mock_code, plugin_language, gen_pbcc)

    outs = gen_srcs.raw + gen_srcs.plugin + gen_hdrs.raw + gen_hdrs.plugin

    if pblibdep_gen_trpc_stub:
        outs = stub_gen_hdrs + stub_gen_srcs

    proto_gen(
        name = name + "_genproto",
        srcs = srcs,
        deps = [s + "_genproto" for s in deps],
        native_cc_proto_deps = acutal_native_proto_deps + native_cc_proto_deps,
        proto_include_prefix = proto_include_prefix,
        includes = includes,
        protoc = protoc,
        plugin = plugin,
        plugin_language = plugin_language,
        generate_mock_code = generate_mock_code,
        generate_new_mock_code = generate_new_mock_code,
        gen_cc = 1,
        outs = outs,
        visibility = ["//visibility:public"],
        gen_pbcc = gen_pbcc,
        enable_explicit_link_proto = enable_explicit_link_proto,
    )

    if not enable_explicit_link_proto:
        native.cc_library(
            name = name + "_raw",
            srcs = gen_srcs.raw,
            hdrs = gen_hdrs.raw,
            deps = deps + [default_runtime] + acutal_native_proto_deps + native_cc_proto_deps,
            includes = includes,
            **kargs
        )
    else:
        # In the scenario where the proto file does not exist locally and the remote proto file is pulled directly
        # through the native proto rules (proto_library/cc_proto_library), the generated stub code needs to load
        # the "pb.cc" file.
        remote_pb_srcs = []
        if len(srcs) == 0 and (len(acutal_native_proto_deps) + len(native_cc_proto_deps)) == 1:
            if len(acutal_native_proto_deps) == 1:
                remote_pb_srcs.append(acutal_native_proto_deps[0])
            elif len(native_cc_proto_deps) == 1:
                remote_pb_srcs.append(native_cc_proto_deps[0])
        native.cc_library(
            name = name + "_raw",
            # Even if the interfaces/data types defined in "pb.h" are not used, "pb.cc" still needs to be
            # loaded (as the options of the proto need to access the content of the proto file inside "pb.cc").
            alwayslink = True,
            srcs = gen_srcs.raw + remote_pb_srcs,
            hdrs = gen_hdrs.raw,
            deps = deps + [default_runtime] + acutal_native_proto_deps + native_cc_proto_deps,
            includes = includes,
            **kargs
        )

    native.cc_library(
        name = name,
        srcs = gen_srcs.plugin if not pblibdep_gen_trpc_stub else stub_gen_srcs,
        hdrs = gen_hdrs.plugin if not pblibdep_gen_trpc_stub else stub_gen_hdrs,
        deps = cc_libs + [name + "_raw"],
        **kargs
    )

def check_protobuf_required_bazel_version():
    """For WORKSPACE files, to check the installed version of bazel.

    This ensures bazel supports our approach to proto_library() depending on a
    copied filegroup. (Fixed in bazel 0.5.4)
    """
    versions.check(minimum_bazel_version = "0.5.4")

def trpc_proto_library(
        name,
        srcs = [],
        deps = [],
        native_proto_deps = [],
        native_cc_proto_deps = [],
        proto_include_prefix = "ProtoIncludePrefixNotValid",
        include = None,
        use_trpc_plugin = False,
        generate_mock_code = False,
        generate_new_mock_code = False,
        rootpath = "",
        gen_pbcc = True,
        plugin = None,
        # Only enabling this option will generate the stub code for obtaining options, because
        # obtaining options requires that "pb.cc" can be linked. In complex scenarios, it may not be possible to link
        # successfully, so it needs to be investigated specifically, and it is disabled by default.
        enable_explicit_link_proto = False,
        **kargs):
    """Bazel rule to create a C++ protobuf library from proto source files by "trpc_plugin", the most common way

    Args:
      name: the name of the "cc_proto_library".
      deps: a list of dependency labels; must be other proto libraries.
      srcs: the .proto files of the "cc_proto_library".
      native_proto_deps: a list of dependency labels; must be "cc_proto_library".
      native_cc_proto_deps: a list of other cc_library targets depended by the generated cc_library.
      proto_include_prefix: The prefix path for a ".pb" file refers to the directory path that contains the ".proto"
                            file used to generate the ".pb" file.
      include: a string indicating the include path of the .proto files.
      use_trpc_plugin: a flag to indicate whether to call the C++ plugin
      generate_mock_code: whether to generate mock stub code for testing depends on the specific code generation
                          tool and options
      generate_new_mock_code: a new tool like "generate_mock_code"
      rootpath: remote root dependency path
      gen_pbcc: whether to generate "*.pb.cc" file by pb
      plugin: name of plugin tool to generate stub
      enable_explicit_link_proto: only enabling this option will generate the stub code for obtaining
                                  options, because obtaining options requires that "pb.cc" can be linked.
                                  In complex scenarios, it may not be possible to linksuccessfully, so it needs to be
                                  investigated specifically, and it is disabled by default.
      **kargs: other keyword arguments that are passed to cc_library.
    """
    trpc_libs = []
    plugin_language = ""
    if (use_trpc_plugin):
        trpc_libs = [
            "%s//trpc/client:rpc_service_proxy" % rootpath,
            "%s//trpc/server:rpc_async_method_handler" % rootpath,
            "%s//trpc/server:rpc_method_handler" % rootpath,
            "%s//trpc/server:rpc_service_impl" % rootpath,
            "%s//trpc/server:stream_rpc_async_method_handler" % rootpath,
            "%s//trpc/server:stream_rpc_method_handler" % rootpath,
            "%s//trpc/stream:stream" % rootpath,
        ]
        plugin_language = "trpc"
        if plugin == None:
            plugin = "%s//trpc/tools/trpc_cpp_plugin:trpc_cpp_plugin" % rootpath
    tc_cc_proto_library(
        name = name,
        srcs = srcs,
        deps = deps,
        native_proto_deps = native_proto_deps,
        native_cc_proto_deps = native_cc_proto_deps,
        proto_include_prefix = proto_include_prefix,
        cc_libs = trpc_libs,
        include = include,
        protoc = "@com_google_protobuf//:protoc",
        use_plugin = use_trpc_plugin,
        plugin_language = plugin_language,
        plugin = plugin,
        generate_mock_code = generate_mock_code,
        generate_new_mock_code = generate_new_mock_code,
        default_runtime = "@com_google_protobuf//:protobuf",
        gen_pbcc = gen_pbcc,
        enable_explicit_link_proto = enable_explicit_link_proto,
        **kargs
    )
