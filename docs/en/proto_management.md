[中文](../zh/proto_management.md)


# Overview

tRPC uses protocol buffers as the default interface definition language (IDL). When calling between services, the proto interface file is used as the external user interface of the service, and there should be a relatively unified management method.

Conventional practice: the proto file of each service is managed by itself. When the services need to communicate, copy a copy of the proto file, and then use the protoc tool to generate stub code for use. This approach results in multiple copies of the proto file for each service , the copies of different proto files are not updated in time, resulting in inconsistent upstream and downstream protocols, resulting in intercommunication problems.

Therefore, the proto file dependency management method recommended by tRPC-Cpp here is: for the service provider, put the proto files that need to be externally placed in a unified remote repositories (for example: use github repositories or internal private repositories); for service consumer, The consumer only needs to automatically pull the dependent proto files based on the bazel build tool and generate stub code.

# How to automatically pull remote proto files based on bazel

Step 1: Pull the proto file of the dependent service

Before pulling, you need to ensure that the proto file of the dependent service is already in a remote repositories. It is recommended to put the external proto file of the service in the same remote repositories, although you can also obtain the dependency by directly pulling the code repositories of the service proto file, but many unnecessary code files other than proto will be pulled here.

Then, in the WORKSPACE file of this project, add the target rules for pulling the remote repositories, as follows:

Step 2: Add the proto target to the user's target dependencies

Here we mainly use the trpc_proto_library rules that is encapsulated by the framework to achieve dependency management between proto targets.

For example: the proto file of this service imports the proto files of other services, you can refer to the following method:

For example: a code file of this service needs to call other services, you can refer to the following method:
