#!/usr/bin/env bash

gcc -I./src/client src/client/client.c -o run_client

if [ $? -eq 0 ]; then
    "./run_client"
fi