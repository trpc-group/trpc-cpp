## Features

This folder lists usage examples of various features, please ensure that any newly added examples adhere to the following standards:

* The subdirectory naming of features should reflect the features themselves, and be concise and expressive.
* The subdirectories of features need to follow a specific structure that includes a `README.md` file. The client and server implementations should each be in their own folder . If other shared components are needed, they can be provided in a new folder. The example directory structure is as follows:
```shell
$ tree somefeature/ # eg: somefeature named commpression
somefeature/
├── README.md
├── client/ # test client
│   ├── client.cc
|   └── trpc_cpp.yaml
│── proxy/  # optional, if exist, client(req/rsp) <---> proxy(req/rsp) ---> server
│   ├── xxx_server.cc
│   ├── xxx_server.h
|   └── trpc_cpp.yaml
|   └── ...
│── server/ # if `proxy` dir not exist, client(req/rsp) <---> server
│   ├── xxx_server.cc
│   ├── xxx_server.h
|   └── trpc_cpp.yaml
|   └── ...
└── common/ # optional
    └── xxx.h
    └── xxx.cc
    └── ...
```
* The README.md file in each subdirectory for a feature should also follow a specific format. The template is as follows:
````markdown
# Feature Name

Brief introduction of the feature.

## Usage

Steps to use the feature. Typically:

* Compile

run the command in `trpc-cpp-example` dir
```shell
$ bazel build //examples/features/somefeature/server:xxx_server
$ bazel build //examples/features/somefeature/client:client
```

* Run Server

run the command in `trpc-cpp-example` dir
```shell
$ ./bazel-bin/examples/features/somefeature/server/xxx_server --config=./examples/features/somefeature/server/trpc_cpp.yaml
```

* Run Client

run the command in `trpc-cpp-example` dir
```shell
$ ./bazel-bin/examples/features/somefeature/client/client --config=./examples/features/somefeature/client/trpc_cpp.yaml
```

Then explain the expected result.
````
