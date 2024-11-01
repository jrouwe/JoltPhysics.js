#!/bin/sh
set -e

if [ -z $1 ] 
then
	BUILD_TYPE=Distribution
else
	BUILD_TYPE=$1
	shift
fi

rm -rf ./dist

mkdir dist

cmake -B Build/$BUILD_TYPE/ST -DCMAKE_BUILD_TYPE=$BUILD_TYPE "${@}"
cmake --build Build/$BUILD_TYPE/ST -j`nproc`

cmake -B Build/$BUILD_TYPE/MT -DENABLE_MULTI_THREADING=ON -DENABLE_SIMD=ON -DCMAKE_BUILD_TYPE=$BUILD_TYPE "${@}"
cmake --build Build/$BUILD_TYPE/MT -j`nproc`

cat > ./dist/jolt-physics.d.ts << EOF
import Jolt from "./types";

export default Jolt;
export * from "./types";

EOF

cp ./dist/jolt-physics.d.ts ./dist/jolt-physics.wasm.d.ts
cp ./dist/jolt-physics.d.ts ./dist/jolt-physics.wasm-compat.d.ts
cp ./dist/jolt-physics.d.ts ./dist/jolt-physics.multithread.d.ts
cp ./dist/jolt-physics.d.ts ./dist/jolt-physics.multithread.wasm.d.ts
cp ./dist/jolt-physics.d.ts ./dist/jolt-physics.multithread.wasm-compat.d.ts

cp ./dist/jolt-physics*.wasm-compat.js ./Examples/js/
