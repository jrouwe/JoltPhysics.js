#!/bin/bash

EMSDK_VERSION=6.0.2

git clone https://github.com/emscripten-core/emsdk.git

cd emsdk

./emsdk install $EMSDK_VERSION
./emsdk activate $EMSDK_VERSION
