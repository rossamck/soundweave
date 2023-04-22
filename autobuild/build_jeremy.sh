#!/bin/bash

BUILD_COMMAND="g++ -std=c++11 -o client clientboost.cpp -lboost_system -lpthread -lboost_thread -lboost_serialization"

$BUILD_COMMAND
echo "finished"