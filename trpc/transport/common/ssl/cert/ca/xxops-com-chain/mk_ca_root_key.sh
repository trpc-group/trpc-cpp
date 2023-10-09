#! /usr/bin/env bash

KEY_NAME='root-ca.key'
CERT_NAME='root-ca.crt'

## generate rsa key
openssl genrsa -out ${KEY_NAME} 2048

## create self-signed the root certificate
openssl req -x509 -new -nodes -key ${KEY_NAME} -sha256 -days 1095 -subj "/C=CN/ST=CA/O=xxops digit cert org" -out ${CERT_NAME}


