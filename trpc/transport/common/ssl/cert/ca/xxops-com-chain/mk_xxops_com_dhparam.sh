 #! /usr/bin/env bash

DHPARAM_NAME='xxopsop-com.dhparam'

openssl dhparam 2048 -check -out ${DHPARAM_NAME}