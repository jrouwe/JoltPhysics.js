#!/bin/bash

npm install

bash ./ci/install-emsdk.sh
source ./emsdk/emsdk_env.sh

npm run build
