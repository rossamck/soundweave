#!/bin/bash

BUILD_COMMAND1="cd build"
BUILD_COMMAND2="cmake .."
BUILD_COMMAND3="make"
# BUILD_COMMAND="g++ -std=c++11 -o client clientboost.cpp -lboost_system -lpthread -lboost_thread -lboost_serialization"


$BUILD_COMMAND1
wait
$BUILD_COMMAND2
wait
$BUILD_COMMAND3
echo "finished building jeremy"