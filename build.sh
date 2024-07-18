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

cmake -B Build/$BUILD_TYPE -DENABLE_MULTI_THREADING=ON -DCMAKE_BUILD_TYPE=$BUILD_TYPE "${@}"
cmake --build Build/$BUILD_TYPE -j`nproc` --target jolt-wasm-compat jolt-wasm

for file in ./dist/jolt-physics.*; do
	mv "${file}" "${file/physics/physics.multithread}"
done

cmake -B Build/$BUILD_TYPE -DENABLE_MULTI_THREADING=OFF -DCMAKE_BUILD_TYPE=$BUILD_TYPE "${@}"
cmake --build Build/$BUILD_TYPE -j`nproc`

cat > ./dist/jolt-physics.d.ts << EOF
import Jolt from "./types";

export default Jolt;
export * from "./types";

EOF

cp ./dist/jolt-physics.d.ts ./dist/jolt-physics.wasm.d.ts

cp ./dist/jolt-physics.d.ts ./dist/jolt-physics.wasm-compat.d.ts

cp ./dist/jolt-physics*.wasm-compat.js ./Examples/js/

