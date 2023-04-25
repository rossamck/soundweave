#!/bin/bash

BUILD_COMMAND="clang++ -std=c++17 -pthread -lboost_system -lboost_thread -lboost_serialization server.cpp -o server"
# BUILD_COMMAND="cd build && cmake .. && make"

$BUILD_COMMAND
echo "finished building server"