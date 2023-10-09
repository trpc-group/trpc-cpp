#
#
# Copyright (c) flatbuffers original author and authors
# flatbuffers is licensed under the Apache 2.0 License.
# The source codes in this file based on
# https://github.com/google/flatbuffers/blob/v23.5.26/build_defs.bzl.
# This source file may have been modified by THL A29 Limited, and licensed under the Apache 2.0 License.
#
#
load("@rules_cc//cc:defs.bzl", "cc_library")

DEFAULT_INCLUDE_PATHS = [
    "./",
    "$(GENDIR)",
    "$(BINDIR)",
]

DEFAULT_FLATC_ARGS = [
    "--gen-object-api",
    "--gen-compare",
    "--no-includes",
    "--gen-mutable",
    "--reflect-names",
    "--cpp-ptr-type flatbuffers::unique_ptr",
]

def _gen_out(
    src_files = [],
    flatc_args = [],
    out_files = []):
    """
    Generate Output flatbuffers stub code and trpc stub code
    
    Args:
      src_files: source files which has .fbs as suffix
      includes: deps files
      flatc_args: arguments that pass to flatc
      out_files: the generated *.trpc.fb.h, *.trpc.fb.cc, *_generated.h and *.trpc.fb.mock.h(optional)
    """
    for s in src_files:
        file_name = s.split(".")[0]
        gen_file = file_name + "_generated.h"
        out_files.append(gen_file)
        if "--trpc" in flatc_args:
            trpc_h = file_name + ".trpc.fb.h"
            trpc_s = file_name + ".trpc.fb.cc"
            out_files.append(trpc_h)
            out_files.append(trpc_s)
            if "--need_mock" in flatc_args:
                mock_fb_h = file_name + ".trpc.fb.mock.h"
                out_files.append(mock_fb_h)

def fbs_library_public(
        name,
        srcs,
        outs,
        language_flag,
        out_prefix = "",
        includes = [],
        include_paths = DEFAULT_INCLUDE_PATHS,
        flatc_args = DEFAULT_FLATC_ARGS,
        reflection_name = "",
        reflection_visibility = None,
        compatible_with = None,
        restricted_to = None,
        output_to_bindir = False,
        trpc_cpp_root_path = "@trpc_cpp"):
    """Generates code files for reading/writing the given flatbuffers in the requested language using the public compiler.

    Args:
      name: Rule name.
      srcs: Source .fbs files. Sent in order to the compiler.
      outs: Output files from flatc.
      language_flag: Target language flag. One of [-c, -j, -js].
      out_prefix: Prepend this path to the front of all generated files except on
          single source targets. Usually is a directory name.
      includes: Optional, list of filegroups of schemas that the srcs depend on.
      include_paths: Optional, list of paths the includes files can be found in.
      flatc_args: Optional, list of additional arguments to pass to flatc.
      reflection_name: Optional, if set this will generate the flatbuffer
        reflection binaries for the schemas.
      reflection_visibility: The visibility of the generated reflection Fileset.
      output_to_bindir: Passed to genrule for output to bin directory.
      compatible_with: Optional, The list of environments this rule can be
        built for, in addition to default-supported environments.
      restricted_to: Optional, The list of environments this rule can be built
        for, instead of default-supported environments.
      output_to_bindir: Passed to genrule for output to bin directory.


    This rule creates a filegroup(name) with all generated source files, and
    optionally a Fileset([reflection_name]) with all generated reflection
    binaries.
    """
    include_paths_cmd = ["-I %s" % (s) for s in include_paths]
    # '$(@D)' when given a single source target will give the appropriate
    # directory. Appending 'out_prefix' is only necessary when given a build
    # target with multiple sources.
    output_directory = (
        ("-o $(@D)/%s" % (out_prefix)) if len(srcs) > 1 else ("-o $(@D)")
    )
    genrule_cmd = " ".join([
        "SRCS=($(SRCS));",
        "for f in $${SRCS[@]:0:%s}; do" % len(srcs),
        "$(location %s)" % "%s//trpc/tools/flatbuffers_tool:fbs_cpp_plugin" % trpc_cpp_root_path,
        " ".join(include_paths_cmd),
        " ".join(flatc_args),
        language_flag,
        output_directory,
        "$$f;",
        "done",
    ])
    native.genrule(
        name = name,
        srcs = srcs + includes,
        outs = outs,
        output_to_bindir = output_to_bindir,
        tools = [
            "%s//trpc/tools/flatbuffers_tool:fbs_cpp_plugin" % trpc_cpp_root_path
        ],
        cmd = genrule_cmd,
        compatible_with = compatible_with,
        restricted_to = restricted_to,
        message = "Generating flatbuffer files for %s:" % (name),
    )
    if reflection_name:
        reflection_genrule_cmd = " ".join([
            "SRCS=($(SRCS));",
            "for f in $${SRCS[@]:0:%s}; do" % len(srcs),
            "$(location %s)" % "%s//trpc/tools/flatbuffers_tool:fbs_cpp_plugin" % trpc_cpp_root_path,
            "-b --schema",
            " ".join(flatc_args),
            " ".join(include_paths_cmd),
            language_flag,
            output_directory,
            "$$f;",
            "done",
        ])
        reflection_outs = [
            (out_prefix + "%s.bfbs") % (s.replace(".fbs", "").split("/")[-1])
            for s in srcs
        ]
        native.genrule(
            name = "%s_srcs" % reflection_name,
            srcs = srcs + includes,
            outs = reflection_outs,
            output_to_bindir = output_to_bindir,
            tools = [
                "%s//trpc/tools/flatbuffers_tool:fbs_cpp_plugin" % trpc_cpp_root_path,
            ],
            compatible_with = compatible_with,
            restricted_to = restricted_to,
            cmd = reflection_genrule_cmd,
            message = "Generating flatbuffer reflection binary for %s:" % (name),
        )
        native.filegroup(
            name = "%s_out" % reflection_name,
            srcs = reflection_outs,
            visibility = reflection_visibility,
            compatible_with = compatible_with,
            restricted_to = restricted_to,
        )

def trpc_fbs_library(
        name,
        srcs,
        deps=[],
        srcs_filegroup_name = "",
        out_prefix = "",
        includes = [],
        include_paths = DEFAULT_INCLUDE_PATHS,
        generate_mock_code = False,
        flatc_args = DEFAULT_FLATC_ARGS,
        visibility = None,
        compatible_with = None,
        restricted_to = None,
        srcs_filegroup_visibility = None,
        gen_reflections = False,
        trpc_cpp_root_path = "@trpc_cpp"):
    '''A cc_library with the generated reader/writers for the given flatbuffer definitions.

    Args:
      name: Rule name.
      srcs: Source .fbs files. Sent in order to the compiler.
      srcs_filegroup_name: Name of the output filegroup that holds srcs. Pass this
          filegroup into the `includes` parameter of any other
          flatbuffer_cc_library that depends on this one's schemas.
      out_prefix: Prepend this path to the front of all generated files. Usually
          is a directory name.
      includes: Optional, list of filegroups of schemas that the srcs depend on.
          ** SEE REMARKS BELOW **
      include_paths: Optional, list of paths the includes files can be found in.
      flatc_args: Optional list of additional arguments to pass to flatc
          (e.g. --gen-mutable).
      visibility: The visibility of the generated cc_library. By default, use the
          default visibility of the project.
      srcs_filegroup_visibility: The visibility of the generated srcs filegroup.
          By default, use the value of the visibility parameter above.
      gen_reflections: Optional, if true this will generate the flatbuffer
        reflection binaries for the schemas.
      compatible_with: Optional, The list of environments this rule can be built
        for, in addition to default-supported environments.
      restricted_to: Optional, The list of environments this rule can be built
        for, instead of default-supported environments.

    This produces:
      filegroup([name]_srcs): all generated .h files.
      filegroup(srcs_filegroup_name if specified, or [name]_includes if not):
          Other flatbuffer_cc_library's can pass this in for their `includes`
          parameter, if they depend on the schemas in this library.
      Fileset([name]_reflection): (Optional) all generated reflection binaries.
      cc_library([name]): library with sources and flatbuffers deps.

    Remarks:
      ** Because the genrule used to call flatc does not have any trivial way of
        computing the output list of files transitively generated by includes and
        --gen-includes (the default) being defined for flatc, the --gen-includes
        flag will not work as expected. The way around this is to add a dependency
        to the flatbuffer_cc_library defined alongside the flatc included Fileset.
        For example you might define:

        flatbuffer_cc_library(
            name = "my_fbs",
            srcs = [ "schemas/foo.fbs" ],
            includes = [ "//third_party/bazz:bazz_fbs_includes" ],
        )

        In which foo.fbs includes a few files from the Fileset defined at
        //third_party/bazz:bazz_fbs_includes. When compiling the library that
        includes foo_generated.h, and therefore has my_fbs as a dependency, it
        will fail to find any of the bazz *_generated.h files unless you also
        add bazz's flatbuffer_cc_library to your own dependency list, e.g.:

        cc_library(
            name = "my_lib",
            deps = [
                ":my_fbs",
                "//third_party/bazz:bazz_fbs"
            ],
        )

        Happy dependent Flatbuffering!
    '''
    output_headers = []
    if generate_mock_code:
        flatc_args.append("--need_mock")
    _gen_out(srcs, flatc_args, output_headers)
    reflection_name = "%s_reflection" % name if gen_reflections else ""

    srcs_lib = "%s_srcs" % (name)
    fbs_library_public(
        name = srcs_lib,
        srcs = srcs,
        outs = output_headers,
        language_flag = "-c",
        out_prefix = out_prefix,
        includes = includes,
        include_paths = include_paths,
        flatc_args = flatc_args,
        compatible_with = compatible_with,
        restricted_to = restricted_to,
        reflection_name = reflection_name,
        reflection_visibility = visibility,
        trpc_cpp_root_path = trpc_cpp_root_path,
    )
    deps = deps + [
        "%s//trpc/util/flatbuffers:trpc_fbs" % trpc_cpp_root_path,
        "@com_github_google_flatbuffers//:runtime_cc",
    ]
    if "--trpc" in flatc_args:
        deps = deps + [
            "%s//trpc/client:rpc_service_proxy" % trpc_cpp_root_path,
            "%s//trpc/server:rpc_service_impl" % trpc_cpp_root_path,
            "%s//trpc/server:rpc_method_handler" % trpc_cpp_root_path,
        ]
    cc_library(
        name = name,
        hdrs = [
            ":" + srcs_lib,
        ],
        srcs = [
            ":" + srcs_lib,
        ],
        features = [
            "-parse_headers",
        ],
        deps = deps,
        includes = [],
        compatible_with = compatible_with,
        restricted_to = restricted_to,
        linkstatic = 1,
        visibility = visibility,
    )

    # A filegroup for the `srcs`. That is, all the schema files for this
    # Flatbuffer set.
    native.filegroup(
        name = srcs_filegroup_name if srcs_filegroup_name else "%s_includes" % (name),
        srcs = srcs,
        compatible_with = compatible_with,
        restricted_to = restricted_to,
        visibility = srcs_filegroup_visibility if srcs_filegroup_visibility != None else visibility,
    )
