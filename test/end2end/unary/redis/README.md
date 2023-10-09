# end2end testing
## unary
### redis
Dode directory organization：
```text
test/end2end/unary/redis
|-- conf  <- same code run with different config
|   |-- BUILD
|   |-- redis_client_fiber.yaml
|   |-- redis_client_merge.yaml
|   |-- redis_client_separate.yaml
|   `-- redis_server.yaml
|-- redis_client_test.cc  <- redis client end2end testing
|-- redis_server.cc       <- mock redis server,just mock,need deploy and run redis-server
```

The comment explains the redis-client-scene in file 'redis_client_test.cc'.

Deploy and run redis-server
```
wget https://download.redis.io/releases/redis-6.2.4.tar.gz
tar -zvxf redis-6.2.4.tar.gz
mv redis-6.2.4 /usr/local/redis
cd /usr/local/redis/
make
make PREFIX=/usr/local/redis install
./bin/redis-server &
```