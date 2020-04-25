#!/bin/bash

# This script encrypts and then decrypts the 'Makefile' file as an example of workflow

set -exu

docker build -t file_enc_dec .

docker run -v "${PWD}:/opt/src" -it file_enc_dec -i /opt/src/Makefile
