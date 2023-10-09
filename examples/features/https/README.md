# HTTPS Unary calling (Request-Response).

HTTPS stands for Hypertext Transfer Protocol Secure. It is a secure version of HTTP. HTTPS uses encryption to protect
the data that is being transferred, making it more secure than HTTP.

In HTTPS request-response calling scenery, by configuring the SSL configuration items, HTTPS capability can be enabled.
To enable HTTPS capability, an SSL certificate needs to be installed on the server, and the configuration file of the
web server needs to be modified
to specify the use of SSL protocol for encrypted communication. Generally, SSL certificates need to be purchased from
trusted certificate authorities (CA) to ensure the authenticity and reliability of the certificate. Once the SSL
certificate is successfully installed and configured, the server can provide secure services through the HTTPS
protocol, ensuring the security of user data.

## Usage

Using HTTPS has two key points: 
- When compiling the code, enable the SSL module by using the option parameter `--define trpc_include_ssl=true`; 
- Add SSL configuration items to the configuration, such as certificates, private keys, encryption suites, etc.

We can use the following command to view the directory tree.

```shell
$ tree examples/features/https/
examples/features/https/
├── cert
│   ├── client_ca_cert.pem
│   ├── README.md
│   ├── server_cert.pem
│   ├── server_dhparam.pem
│   └── server_key.pem
├── client
│   ├── BUILD
│   ├── client.cc
│   └── trpc_cpp_fiber.yaml
├── CMakeLists.txt
├── README.md
├── run_cmake.sh
├── run.sh
└── server
    ├── BUILD
    ├── https_server.cc
    └── trpc_cpp_fiber.yaml
```

We can use the following script to quickly compile and run a program.

```shell
sh examples/features/https/run.sh
```

* Compilation

We can run the following command to compile the client and server programs.

```shell
bazel build  --define trpc_include_ssl=true //examples/features/https/server:https_server
bazel build  --define trpc_include_ssl=true //examples/features/https/client:client
```

Alternatively, you can use cmake.
```shell
# remove path-to-trpc-cpp/build to reduce the compile affects to other examples
$ rm -rf build
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DTRPC_BUILD_WITH_SSL=ON -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/https
$ mkdir -p examples/features/https/build && cd examples/features/https/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# remove path-to-trpc-cpp/build to reduce the compile affects to other examples
$ rm -rf build
```

* Run the server program

We can run the following command to start the server program.

*CMake build targets can be found at `build` of this directory, you can replace below server&client binary path when you use cmake to compile.*

```shell
bazel-bin/examples/features/https/server/https_server --config=examples/features/https/server/trpc_cpp_fiber.yaml 
```

* Run the client program

We can run the following command to start the client program.

```shell
bazel-bin/examples/features/https/client/client --client_config=examples/features/https/client/trpc_cpp_fiber.yaml
```

The content of the output from the client program is as follows:

``` text
name: GET(https)string, ok: 1
name: HEAD(https) response, ok: 1
name: HEAD https://github.com/, ok: 1
name: POST(https) string, ok: 1
name: POST(https) json, ok: 1
final result of http calling: 1
```