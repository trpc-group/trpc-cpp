#! /bin/bash

# building.
bazel build //examples/features/mysql/client/...


MYSQL_USER="root"
MYSQL_PASSWORD="abc123"
MYSQL_HOST="127.0.0.1"
MYSQL_PORT="3306"
SQL_FILE="examples/features/mysql/client/create_table.sql"

# run mysql client.
mysql -u"$MYSQL_USER" -p"$MYSQL_PASSWORD" -h"$MYSQL_HOST" < "$SQL_FILE"
./bazel-bin/examples/features/mysql/client/fiber/fiber_client --client_config=examples/features/mysql/client/fiber/fiber_client_config.yaml

mysql -u"$MYSQL_USER" -p"$MYSQL_PASSWORD" -h"$MYSQL_HOST" < "$SQL_FILE"
./bazel-bin/examples/features/mysql/client/future/future_client --client_config=examples/features/mysql/client/future/future_client_config.yaml