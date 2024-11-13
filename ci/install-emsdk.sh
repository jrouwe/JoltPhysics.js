#!/bin/bash

EMSDK_VERSION=3.1.71

git clone https://github.com/emscripten-core/emsdk.git

cd emsdk

./emsdk install $EMSDK_VERSION
./emsdk activate $EMSDK_VERSION
