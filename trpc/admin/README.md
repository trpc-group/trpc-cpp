tRPC-Cpp  Service Management Console 

1. Setting up the compilation environment
   Reference: [Compile & Install](../../docs/en/setup_env.md)

2. Compiling 
   Execute the following command in the "trpc-cpp" root directory:
   bazel build //trpc/admin:admin_service --incompatible_disable_deprecated_attr_params=false --incompatible_new_actions_api=false

3. Testing 
   Execute the following command in the "trpc-cpp" root directory:
   bazel build //trpc/admin:admin_service_test --incompatible_disable_deprecated_attr_params=false --incompatible_new_actions_api=false
   ./bazel-bin/trpc/admin/admin_service_test

4. Available Commands

4.1. Views framework version information 
```bash
curl http://ip:port/version
```
Returned results:
```bash
{
    "errorcode": 0,
    "message": "",
    "version": "v0.1.0"
}
```

4.2. Views the list of all management commands
```bash
curl http://ip:port/cmds
```
Returned result:
```bash
{
    "errorcode": 0,
    "message": "",
    "cads": [
        "loglevel",
        "config"
    ]
}
```

4.3. View and set the log level of the framework
Views the log level:
```bash
curl http://ip:port/cmds/loglevel
```
Returned results:
```bash
{
    "errorcode": 0,
    "message": "",
    "level": "INFO"
}
```

Sets the log level (the parameter value is the log level):
```bash
curl http://ip:port/cmds/loglevel -XPUT -d "value=ERROR"
```
Returned results:
```bash
{
    "errorcode": 0,
    "message": "",
    "level": "ERROR",
    "prelevel": "INFO",
}
```

4.4. View framework configuration file
```bash
curl http://ip:port/cmds/config
```
Returned results:
```bash
{
    "errorcode": 0,
    "message": "",
    "content": ,
}
```
