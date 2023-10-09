#! /usr/bin/env bash

ROOT_KEY="root-ca.key"
ROOT_CERT="root-ca.crt"

KEY_NAME='xxops-com.key'
CERT_NAME='xxops-com.crt'
CSR_NAME='xxops-com.csr'

## generate rsa key
openssl genrsa -out ${KEY_NAME} 2048

## create sefl-signed certificate
openssl req -new -key ${KEY_NAME} -sha256  -subj "/C=CN/S=Guangdong Province/L=Shenzhen/O=xxops Org, Inc./CN=*.xxops.com" -out ${CSR_NAME}

openssl x509 -req -in ${CSR_NAME} -CA ${ROOT_CERT} -CAkey ${ROOT_KEY} -CAcreateserial -out ${CERT_NAME} -days 1095 -sha256

## view csr
openssl req -in ${CSR_NAME} -noout -text

## view certificate
openssl x509 -in ${CERT_NAME} -noout -text 
