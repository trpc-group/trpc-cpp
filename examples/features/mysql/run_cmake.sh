
#! /bin/bash


MYSQL_USER="root"
MYSQL_PASSWORD="abc123"
MYSQL_HOST="127.0.0.1"
MYSQL_PORT="3306"
SQL_FILE="examples/features/mysql/client/create_table.sql"

# building.
mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/mysql/build && cd examples/features/mysql/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

# run mysql client.
mysql -u"$MYSQL_USER" -p"$MYSQL_PASSWORD" -h"$MYSQL_HOST" < "$SQL_FILE"
examples/features/mysql/build/fiber_client --client_config=examples/features/mysql/client/fiber/fiber_client_config.yaml
mysql -u"$MYSQL_USER" -p"$MYSQL_PASSWORD" -h"$MYSQL_HOST" < "$SQL_FILE"
examples/features/mysql/build/future_client --client_config=examples/features/mysql/client/future/future_client_config.yaml