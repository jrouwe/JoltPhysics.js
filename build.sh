#!/bin/sh

if [ -z $1 ] 
then
	BUILD_TYPE=Distribution
else
	BUILD_TYPE=$1
	shift
fi

cmake -B Build/$BUILD_TYPE -DCMAKE_BUILD_TYPE=$BUILD_TYPE
cmake --build Build/$BUILD_TYPE -j`nproc`

npx webidl-dts-gen -e -d -i ./JoltJS.idl -o ./Build/types.d.ts -n Jolt

cat > ./Build/jolt-physics.d.ts << EOF
import Jolt from "./types";

export default Jolt;
export * from "./types";

EOF

cp ./Build/jolt-physics.d.ts ./Build/jolt-physics.wasm.d.ts
cp ./Build/jolt-physics.d.ts ./Build/jolt-physics.wasm-compat.d.ts

cp ./Build/jolt-physics.wasm-compat.js ./Examples/js/jolt-physics.wasm-compat.js