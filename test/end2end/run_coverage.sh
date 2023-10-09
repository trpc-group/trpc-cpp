#! /bin/bash

echo """
""" >> .bazelrc
>g_unary_coverage_path
>g_stream_coverage_path

function GetByRegex() {
  regex=$1
  content=$2
  if [[ ${content} =~ ${regex} ]]; then
    echo ${BASH_REMATCH[1]}
  else
    echo "Error: regex not found"
    echo "regex: ${regex}"
    echo "content: ${content}"
    exit 1
  fi
}

function RunCoverage() {
  module=$1
  submodule=$2
  coverage_path=$3
  echo "module: ${module}, submodule: ${submodule}, coverage_path: ${coverage_path}"
  if [ -z "${coverage_path}" ]; then
    echo "Error: coverage_path must be specified."
    exit 1
  fi
  coverage_dir=coverage
  test_path=//test/end2end

  if [ "${module}" = "all" ] && [ "${submodule}" = "all" ]; then
    echo "End2EndTest all"
    test_path=${test_path}/...
  fi

  if [ "${module}" != "all" ] && [ "${submodule}" = "all" ]; then
    echo "End2EndTest ${module} module"
    test_path=${test_path}/${module}/...
  fi

  if [ "${module}" != "all" ] && [ "${submodule}" != "all" ]; then
    echo "End2EndTest ${module}/${submodule} submodule"
    test_path=${test_path}/${module}/${submodule}/...

    echo -n "$coverage_path," >> g_${module}_coverage_path
  fi
  coverage_dir=${coverage_dir}_${module}_${submodule}
  metrics_name=${module}_${submodule}

  bazel coverage --copt="-O0" ${test_path} --coverage_report_generator="@bazel_tools//tools/test/CoverageOutputGenerator/java/com/google/devtools/coverageoutputgenerator:Main" --combined_report=lcov --nocache_test_results --javabase=@bazel_tools//tools/jdk:remote_jdk --instrumentation_filter="${coverage_path}"

  touch coverage_output
  >coverage_output

  genhtml ./bazel-out/_coverage/_coverage_report.dat --output-directory ${coverage_dir} > coverage_output 2>&1
  zip -qr ${coverage_dir}.zip ${coverage_dir}
  echo "gen ${coverage_dir}.zip"

  line_regex="  lines......: ([0-9.]+)%"
  functions_regex="  functions..: ([0-9.]+)%"
  coverage_summary=$(cat coverage_output)
  lines_coverage=$(GetByRegex "${line_regex}" "${coverage_summary}")
  functions_coverage=$(GetByRegex "${functions_regex}" "${coverage_summary}")
  coverage_info=$(tail -n 2 coverage_output | tr -d '\n. ' | tr ':' '=')
  echo "lines_coverage: ${lines_coverage}%, functions_coverage: ${functions_coverage}%"
  echo "::set-variable name=${metrics_name}_lc::${lines_coverage}"
  echo "::set-variable name=${metrics_name}_fc::${functions_coverage}"
  echo "::set-variable name=${metrics_name}_info::${coverage_info}"
}

module="unary"
submodule="redis"
coverage_path="trpc/client/redis[/:],trpc/codec/redis[/:]"
RunCoverage ${module} ${submodule} ${coverage_path}

module="unary"
submodule="tvar"
coverage_path="trpc/tvar[/:]"
RunCoverage ${module} ${submodule} ${coverage_path}

module="unary"
submodule="future"
coverage_path="trpc/future[/:],trpc/transport/client/future[/:]"
RunCoverage ${module} ${submodule} ${coverage_path}

module="unary"
submodule="fiber"
coverage_path="trpc/coroutine[/:]"
RunCoverage ${module} ${submodule} ${coverage_path}