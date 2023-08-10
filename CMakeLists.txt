cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

FIND_PACKAGE(Python3)
set(PYTHON ${Python3_EXECUTABLE} CACHE STRING "Python path")
set(EMSCRIPTEN_ROOT $ENV{EMSDK}/upstream/emscripten CACHE STRING "Emscripten path")
set(CMAKE_TOOLCHAIN_FILE ${EMSCRIPTEN_ROOT}/cmake/Modules/Platform/Emscripten.cmake)
set(WEBIDL_BINDER_SCRIPT ${EMSCRIPTEN_ROOT}/tools/webidl_binder.py)
set(JOLT_FRONT_MATTER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/front-matter.js)
set(JOLT_HEADER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/JoltJS.h)
set(JOLT_IDL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/JoltJS.idl)
set(OUTPUT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/dist/)

# The configurations we support
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Distribution")

# No floating point exception checking
set(FLOATING_POINT_EXCEPTIONS_ENABLED OFF)

# Turn off LTO
set(INTERPROCEDURAL_OPTIMIZATION OFF)

# Turn on symbols
set(GENERATE_DEBUG_SYMBOLS ON)

# Include Jolt
include(FetchContent)
FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics"
        GIT_TAG "a957cca6197e29b5961f97e389c1e2d3b84a7988"
		SOURCE_SUBDIR "Build"
)
FetchContent_MakeAvailable(JoltPhysics)

# Default is 64*1024*1024 = 64MB
set(TOTAL_MEMORY 67108864 CACHE STRING "Total Memory")

# Enable for resizable heap, with some amount of slowness
set(ALLOW_MEMORY_GROWTH 0 CACHE STRING "Allow Memory Growth")

set(EMCC_ARGS
  --extern-pre-js ${JOLT_FRONT_MATTER_FILE}
  --post-js glue.js
  --closure=1
  -s ALLOW_MEMORY_GROWTH=${ALLOW_MEMORY_GROWTH}
  -s ALLOW_TABLE_GROWTH=1
  -s ENVIRONMENT='web,node'
  -s EXPORT_ES6=1
  -s EXPORT_NAME="Jolt"
  -s MODULARIZE=1
  -s NO_EXIT_RUNTIME=1
  -s NO_FILESYSTEM=1
  -s TOTAL_STACK=1048576
  -s TOTAL_MEMORY=${TOTAL_MEMORY})

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	set(EMCC_ARGS 
		${EMCC_ARGS} 
		-g
		-s ASSERTIONS)
else()
	set(EMCC_ARGS 
		${EMCC_ARGS} 
		-O3)
endif()

set(EMCC_JS_ARGS ${EMCC_ARGS}
  -s AGGRESSIVE_VARIABLE_ELIMINATION=1
  -s ELIMINATE_DUPLICATE_FUNCTIONS=1
  -s LEGACY_VM_SUPPORT=1
  -s SINGLE_FILE=1
  -s WASM=0)

set(EMCC_WASM_ARGS ${EMCC_ARGS}
  -s BINARYEN_IGNORE_IMPLICIT_TRAPS=1
  -s WASM=1)

set(EMCC_WASM_COMPAT_ARGS ${EMCC_WASM_ARGS}
  -s SINGLE_FILE=1)

set(EMCC_GLUE_ARGS
  -c
  -I${JoltPhysics_SOURCE_DIR}/..
  -std=c++17
  -include ${JOLT_HEADER_FILE})

# Can't find a way to automatically inherit settings set by target_compile_definitions, so hardcoding defines here
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	set(EMCC_GLUE_ARGS
		${EMCC_GLUE_ARGS}
		-DJPH_DEBUG_RENDERER -DJPH_PROFILE_ENABLED -D_DEBUG)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	set(EMCC_GLUE_ARGS
		${EMCC_GLUE_ARGS}
		-DJPH_DEBUG_RENDERER -DJPH_PROFILE_ENABLED -DNDEBUG)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "Distribution")
	set(EMCC_GLUE_ARGS
		${EMCC_GLUE_ARGS}
		-DNDEBUG)
else()
	message("Unknown CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif()

#######################################
project("JoltPhysics.js")

#######################################
add_custom_command(
  OUTPUT glue.cpp glue.js
  BYPRODUCTS parser.out WebIDLGrammar.pkl
  COMMAND ${PYTHON} ${WEBIDL_BINDER_SCRIPT} ${JOLT_IDL_FILE} glue
  DEPENDS ${JOLT_IDL_FILE}
  COMMENT "Generating JoltPhysics.js bindings"
  VERBATIM)
add_custom_command(
  OUTPUT glue.o
  COMMAND emcc glue.cpp ${EMCC_GLUE_ARGS} -o glue.o
  DEPENDS glue.cpp ${JOLT_HEADER_FILE}
  COMMENT "Building JoltPhysics.js bindings"
  VERBATIM)
add_custom_target(jolt-bindings ALL DEPENDS glue.js glue.o)

#######################################
add_custom_command(
  OUTPUT ${OUTPUT_FOLDER}jolt-physics.js
  COMMAND emcc glue.o $<TARGET_FILE:Jolt> ${EMCC_JS_ARGS} -o ${OUTPUT_FOLDER}jolt-physics.js
  DEPENDS jolt-bindings ${JOLT_FRONT_MATTER_FILE} ${JOLT_TARGETS} ${JOLT_IDL_FILE} ${JOLT_HEADER_FILE}
  COMMENT "Building JoltPhysics.js javascript"
  VERBATIM)
add_custom_target(jolt-javascript ALL DEPENDS ${OUTPUT_FOLDER}jolt-physics.js)

#######################################
add_custom_command(
  OUTPUT ${OUTPUT_FOLDER}jolt-physics.wasm.js ${OUTPUT_FOLDER}jolt-physics.wasm.wasm
  COMMAND emcc glue.o $<TARGET_FILE:Jolt> ${EMCC_WASM_ARGS} -o ${OUTPUT_FOLDER}jolt-physics.wasm.js
  DEPENDS jolt-bindings ${JOLT_FRONT_MATTER_FILE} ${JOLT_TARGETS} ${JOLT_IDL_FILE} ${JOLT_HEADER_FILE}
  COMMENT "Building JoltPhysics.js webassembly"
  VERBATIM)
add_custom_target(jolt-wasm ALL DEPENDS ${OUTPUT_FOLDER}jolt-physics.wasm.js ${OUTPUT_FOLDER}jolt-physics.wasm.wasm)

#######################################
add_custom_command(
  OUTPUT ${OUTPUT_FOLDER}jolt-physics.wasm-compat.js
  COMMAND emcc glue.o $<TARGET_FILE:Jolt> ${EMCC_WASM_COMPAT_ARGS} -o ${OUTPUT_FOLDER}jolt-physics.wasm-compat.js
  DEPENDS jolt-bindings ${JOLT_FRONT_MATTER_FILE} ${JOLT_TARGETS} ${JOLT_IDL_FILE} ${JOLT_HEADER_FILE}
  COMMENT "Building JoltPhysics.js compat webassembly"
  VERBATIM)
add_custom_target(jolt-wasm-compat ALL DEPENDS ${OUTPUT_FOLDER}jolt-physics.wasm-compat.js)