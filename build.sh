#!/bin/sh

if [ -z $1 ] 
then
	BUILD_TYPE=Distribution
else
	BUILD_TYPE=$1
	shift
fi

cmake -B Build -DCMAKE_BUILD_TYPE=$BUILD_TYPE
cmake --build Build -j`nproc`
