#!/bin/bash

BUILD_COMMAND="clang++ -std=c++11 -I
/opt/homebrew/Cellar/boost/1.81.0_1/include -L
/opt/homebrew/Cellar/boost/1.81.0_1/lib -o client clientboost.cpp
-lboost_system -lboost_thread-mt"

$BUILD_COMMAND
echo "finished"