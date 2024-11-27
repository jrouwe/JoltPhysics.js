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
set(JOLT_TYPING ${OUTPUT_FOLDER}/types.d.ts)

# Option to turn on memory profiling
option(ENABLE_MEMORY_PROFILER "Enable emscriptens memory profiler to help find leaks" OFF)

# Option to turn on double precision
option(DOUBLE_PRECISION "Compile the library in double precision mode" OFF)

# Option to turn multi threading on
option(ENABLE_MULTI_THREADING "Compile the library in multi threading mode (warning: doesn't work in Safari nor with any JS callbacks currently)'" OFF)

# Option to turn on SIMD
option(ENABLE_SIMD "Enable SIMD instructions (warning: last major browser to support this was Safari 16.4 in March 2023)" OFF)

# Option to speed up compilation by only compiling the WASM compat version
option(BUILD_WASM_COMPAT_ONLY "Compile online the WASM compat version of the library (speeds up compilation)" OFF)

# The configurations we support
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Distribution")

# No floating point exception checking
set(FLOATING_POINT_EXCEPTIONS_ENABLED OFF)

# Turn off LTO
set(INTERPROCEDURAL_OPTIMIZATION OFF)

# Override debug symbols
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	set(GENERATE_DEBUG_SYMBOLS ON)
	set(JPH_DEBUG_SYMBOL_FORMAT "source-map" CACHE STRING "")
else()
	set(GENERATE_DEBUG_SYMBOLS OFF)
endif()

# Turn off custom allocator
set(DISABLE_CUSTOM_ALLOCATOR ON)

# Turn off ObjectStream
set(ENABLE_OBJECT_STREAM OFF)

# Use 32-bit object layers to support more bits in ObjectLayerPairFilterMask
set(OBJECT_LAYER_BITS 32)

# Enable multi threading
if (ENABLE_MULTI_THREADING)
	set(MULTI_THREADED_FLAG 
		-pthread 
		--post-js ${CMAKE_CURRENT_SOURCE_DIR}/multi-threaded.js
		-s SHARED_MEMORY)
	set(ENVIRONMENT_FLAG
		-s ENVIRONMENT='web,node,worker' 
		-s PTHREAD_POOL_SIZE=16)
	set(CMAKE_CXX_FLAGS "-pthread -s SHARED_MEMORY")
	set(OUTPUT_BASE_NAME "${OUTPUT_FOLDER}jolt-physics.multithread")
	set(REMOVE_THREAD_LOCAL "")
else()
	set(MULTI_THREADED_FLAG "")
	set(ENVIRONMENT_FLAG "-s ENVIRONMENT='web,node'")
	set(OUTPUT_BASE_NAME "${OUTPUT_FOLDER}jolt-physics")
	# Tried to keep this multi-platform since windows/mac/linux all treat -i different,
	# though this creates extra backup file in a non-tracked folder.
	# Removing 'thread_local' from the generated binding code fixes a small 'residual' allocation
	# in the proper_cleanup.html demo and it is not needed when in single threaded mode.
	# See: https://github.com/jrouwe/JoltPhysics.js/discussions/222#discussioncomment-11381106	
	set(REMOVE_THREAD_LOCAL sed -i.bak -e "s:thread_local::g" glue.cpp)
endif()

# Enable SIMD flags
if (ENABLE_SIMD)
	set(ENABLE_SIMD_FLAG 
		-msimd128 
		-msse4.2)
	set(USE_WASM_SIMD ON)
else()
	set(ENABLE_SIMD_FLAG "")
	set(USE_WASM_SIMD OFF)
endif()

# Include Jolt
include(FetchContent)
FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics"
        GIT_TAG "cb9cc9b575d66552128da3374192504df0c37077"
		SOURCE_SUBDIR "Build"
)
FetchContent_MakeAvailable(JoltPhysics)

# Default is 64*1024*1024 = 64MB
set(TOTAL_MEMORY 67108864 CACHE STRING "Total Memory")

# Enable for resizable heap, with some amount of slowness
set(ALLOW_MEMORY_GROWTH 0 CACHE STRING "Allow Memory Growth")

# Enable memory profiler
if (ENABLE_MEMORY_PROFILER)
	set(MEMORY_PROFILER_FLAG "--memoryprofiler")
	set(INCLUDE_DEMANGLE_ALL_FLAG "-s DEFAULT_LIBRARY_FUNCS_TO_INCLUDE='$demangleAll'")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MEMORY_PROFILER_FLAG}")
else()
	set(MEMORY_PROFILER_FLAG "")
	set(INCLUDE_DEMANGLE_ALL_FLAG "")
endif()

set(EMCC_ARGS
  -fno-rtti
  -fno-exceptions
  --extern-pre-js ${JOLT_FRONT_MATTER_FILE}
  --post-js glue.js
  --closure=1
  ${MULTI_THREADED_FLAG}
  --post-js ${CMAKE_CURRENT_SOURCE_DIR}/helpers.js
  -s ALLOW_MEMORY_GROWTH=${ALLOW_MEMORY_GROWTH}
  -s ALLOW_TABLE_GROWTH=1
  ${ENVIRONMENT_FLAG}
  -s EXPORT_ES6=1
  -s EXPORT_NAME="Jolt"
  -s MODULARIZE=1
  -s NO_EXIT_RUNTIME=1
  -s NO_FILESYSTEM=1
  -s TOTAL_STACK=1048576
  -s TOTAL_MEMORY=${TOTAL_MEMORY}
  ${MEMORY_PROFILER_FLAG}
  ${INCLUDE_DEMANGLE_ALL_FLAG}
  ${ENABLE_SIMD_FLAG})

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	set(EMCC_ARGS 
		${EMCC_ARGS} 
		-g${JPH_DEBUG_SYMBOL_FORMAT}
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
  -Wall
  -std=c++17
  -fno-rtti
  -fno-exceptions
  ${MULTI_THREADED_FLAG}
  -DJPH_OBJECT_LAYER_BITS=32
  -DJPH_DISABLE_CUSTOM_ALLOCATOR
  -include ${JOLT_HEADER_FILE}
  ${MEMORY_PROFILER_FLAG}
  ${ENABLE_SIMD_FLAG})

# Can't find a way to automatically inherit settings set by target_compile_definitions, so hardcoding defines here
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	set(JOLT_IDL_FILE ${JOLT_IDL_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/JoltJS-DebugRenderer.idl )
	set(EMCC_GLUE_ARGS
		${EMCC_GLUE_ARGS}
		-DJPH_DEBUG_RENDERER -DJPH_PROFILE_ENABLED -D_DEBUG)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	set(JOLT_IDL_FILE ${JOLT_IDL_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/JoltJS-DebugRenderer.idl )
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

# Add double precision define
if (DOUBLE_PRECISION)
	set(EMCC_GLUE_ARGS ${EMCC_GLUE_ARGS} -DJPH_DOUBLE_PRECISION)
endif()

project("JoltPhysics.js")

add_custom_command(
	OUTPUT ${JOLT_TYPING}
	COMMAND npx webidl-dts-gen -e -d -i jolt.idl -o ${JOLT_TYPING} -n Jolt
	DEPENDS jolt.idl
	COMMENT "Generating JoltPhysics.js bindings"
	VERBATIM)

add_custom_command(
	OUTPUT glue.cpp glue.js jolt.idl
	BYPRODUCTS parser.out WebIDLGrammar.pkl
	COMMAND cat ${JOLT_IDL_FILE} > jolt.idl
	COMMAND ${PYTHON} ${WEBIDL_BINDER_SCRIPT} jolt.idl glue
	COMMAND ${REMOVE_THREAD_LOCAL}
	DEPENDS ${JOLT_IDL_FILE}
	COMMENT "Generating JoltPhysics.js bindings"
	VERBATIM)

add_custom_command(
	OUTPUT glue.o
	COMMAND emcc glue.cpp ${EMCC_GLUE_ARGS} -o glue.o
	DEPENDS glue.cpp ${JOLT_HEADER_FILE}
	COMMENT "Building JoltPhysics.js bindings"
	VERBATIM)
add_custom_target(jolt-bindings ALL DEPENDS glue.js glue.o ${JOLT_TYPING})

if (NOT BUILD_WASM_COMPAT_ONLY)
	if (NOT ENABLE_MULTI_THREADING # Not compatible with multi threading due to error: 'void wasm::I64ToI32Lowering::visitStore(Store *): Assertion `!curr->isAtomic && "atomic store not implemented"' failed.'
		AND NOT ENABLE_SIMD) # Not compatible with SIMD due to error: 'unhandled unaligned load'
		add_custom_command(
			OUTPUT ${OUTPUT_BASE_NAME}.js
			COMMAND emcc glue.o $<TARGET_FILE:Jolt> ${EMCC_JS_ARGS} -o ${OUTPUT_BASE_NAME}.js
			DEPENDS jolt-bindings ${JOLT_FRONT_MATTER_FILE} ${JOLT_TARGETS} ${JOLT_IDL_FILE} ${JOLT_HEADER_FILE}
			COMMENT "Building JoltPhysics.js javascript"
			VERBATIM)
		add_custom_target(jolt-javascript ALL DEPENDS ${OUTPUT_BASE_NAME}.js)
	endif()

	add_custom_command(
		OUTPUT ${OUTPUT_BASE_NAME}.wasm.js ${OUTPUT_BASE_NAME}.wasm.wasm
		COMMAND emcc glue.o $<TARGET_FILE:Jolt> ${EMCC_WASM_ARGS} -o ${OUTPUT_BASE_NAME}.wasm.js
		DEPENDS jolt-bindings ${JOLT_FRONT_MATTER_FILE} ${JOLT_TARGETS} ${JOLT_IDL_FILE} ${JOLT_HEADER_FILE}
		COMMENT "Building JoltPhysics.js webassembly"
		VERBATIM)
	add_custom_target(jolt-wasm ALL DEPENDS ${OUTPUT_BASE_NAME}.wasm.js ${OUTPUT_BASE_NAME}.wasm.wasm)
endif()

add_custom_command(
	OUTPUT ${OUTPUT_BASE_NAME}.wasm-compat.js
	COMMAND emcc glue.o $<TARGET_FILE:Jolt> ${EMCC_WASM_COMPAT_ARGS} -o ${OUTPUT_BASE_NAME}.wasm-compat.js
	DEPENDS jolt-bindings ${JOLT_FRONT_MATTER_FILE} ${JOLT_TARGETS} ${JOLT_IDL_FILE} ${JOLT_HEADER_FILE}
	COMMENT "Building JoltPhysics.js compat webassembly"
	VERBATIM)
add_custom_target(jolt-wasm-compat ALL DEPENDS ${OUTPUT_BASE_NAME}.wasm-compat.js)