# Mayhem build of p2pool: mirrors tests/CMakeLists.txt (P2POOL_UNIT_TESTS core source list), but takes
# the submodule dependencies from /opt/toolchains/deps (see mayhem/fetch-deps.sh) instead of the
# (unfetched) submodule checkouts. MODE=fuzz builds the harnesses, MODE=tests builds upstream's gtest suite.
cmake_minimum_required(VERSION 3.10)
project(p2pool_mayhem C CXX)
set(CMAKE_CXX_STANDARD 17)
set(MODE "fuzz" CACHE STRING "fuzz or tests")
set(DEPS /opt/toolchains/deps CACHE PATH "dependency cache")
set(TOP ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(BSSL ${DEPS}/boringssl/src)

add_definitions(-DWITH_RANDOMX -DWITH_MERGE_MINING_DONATION -DP2POOL_UNIT_TESTS -DP2POOL_SIDECHAIN_EXTRA_1=1
  -DDEV_DEBUG -DWITH_INDEXED_HASHES -DZMQ_STATIC -DP2POOL_LOG_DISABLE
  -DHAVE_BUILTIN_CLZLL "-DRAPIDJSON_PARSE_DEFAULT_FLAGS=kParseTrailingCommasFlag")

set(CORE
  ${TOP}/external/src/cryptonote/crypto-ops-data.c ${TOP}/external/src/cryptonote/crypto-ops.c
  ${TOP}/external/src/cryptonote/fcmp_pp_crypto.cpp ${TOP}/external/src/hardforks/hardforks.cpp
  ${TOP}/external/src/siphash/siphash.c
  ${TOP}/src/block_cache.cpp ${TOP}/src/block_template.cpp ${TOP}/src/crypto.cpp ${TOP}/src/keccak.cpp
  ${TOP}/src/log.cpp ${TOP}/src/memory_leak_debug.cpp ${TOP}/src/mempool.cpp ${TOP}/src/merkle.cpp
  ${TOP}/src/p2p_server.cpp ${TOP}/src/p2pool.cpp ${TOP}/src/p2pool_api.cpp ${TOP}/src/params.cpp
  ${TOP}/src/pool_block.cpp ${TOP}/src/pow_hash.cpp ${TOP}/src/side_chain.cpp ${TOP}/src/stratum_server.cpp
  ${TOP}/src/tcp_server.cpp ${TOP}/src/thread_pool.cpp ${TOP}/src/util.cpp ${TOP}/src/wallet.cpp
  ${TOP}/src/i2p.cpp ${TOP}/src/indexed_hash.cpp ${TOP}/src/keccak_bmi.cpp)
set(BSSL_SRC ${BSSL}/crypto/curve25519/curve25519.cc ${BSSL}/crypto/sha/sha256.cc ${BSSL}/crypto/sha/sha512.cc
  ${TOP}/external/src/crypto/bssl_stub.cpp)
set_source_files_properties(${BSSL_SRC} PROPERTIES COMPILE_FLAGS "-w -DOPENSSL_NO_ASM")
# ref10 field arithmetic left-shifts negative values by design; upstream suppresses exactly this
# (tests/src/ubsan_sup.txt: shift-base:crypto-ops.c). Relax only that check, only on that file.
set_source_files_properties(${TOP}/external/src/cryptonote/crypto-ops.c PROPERTIES COMPILE_FLAGS "-fno-sanitize=shift-base")
set_source_files_properties(${TOP}/src/keccak_bmi.cpp PROPERTIES COMPILE_FLAGS "-mbmi")

include_directories(${TOP}/src ${TOP}/external/src ${TOP}/external/src/crypto ${TOP}/external/src/cryptonote
  ${TOP}/external/src/protobuf-c ${DEPS}/p2pool-include ${DEPS}/randomx/src/src ${DEPS}/rapidjson/src/include
  ${DEPS}/robin-hood-hashing/src/src/include ${BSSL} ${BSSL}/include)

# The test suite aborts on any LOGERR (upstream's test define); in the fuzz build an error log is the
# normal rejection of bad input, so only the tests keep it.
if (MODE STREQUAL "tests")
  add_definitions(-DP2POOL_ABORT_ON_LOG_ERROR)
endif()

add_library(p2pool_core STATIC ${CORE} ${BSSL_SRC})
set(LIBS p2pool_core ${DEPS}/randomx/lib/librandomx.a uv zmq pthread)

if (MODE STREQUAL "tests")
  set(T ${TOP}/tests/src)
  add_executable(p2pool_tests ${T}/block_template_tests.cpp ${T}/crypto_tests.cpp ${T}/difficulty_type_tests.cpp
    ${T}/hash_tests.cpp ${T}/keccak_tests.cpp ${T}/merkle_tests.cpp ${T}/log_tests.cpp ${T}/main.cpp
    ${T}/pool_block_tests.cpp ${T}/sha256_tests.cpp ${T}/util_tests.cpp ${T}/wallet_tests.cpp ${T}/i2p_tests.cpp
    ${T}/thread_pool_tests.cpp)
  target_include_directories(p2pool_tests PRIVATE ${T} ${DEPS}/googletest/include)
  target_link_libraries(p2pool_tests ${LIBS} ${DEPS}/googletest/lib/libgtest.a)
else()
  set(HARNESSES pool_block sidechain wallet params)
  add_library(fuzz_common STATIC harnesses/fuzz_common.cpp)
  foreach(h ${HARNESSES})
    add_executable(fuzz_${h} harnesses/fuzz_${h}.cpp)
    target_link_libraries(fuzz_${h} fuzz_common ${LIBS} ${FUZZ_ENGINE})
    add_executable(fuzz_${h}-standalone harnesses/fuzz_${h}.cpp)
    target_link_libraries(fuzz_${h}-standalone fuzz_common ${LIBS} ${STANDALONE_OBJ})
  endforeach()
endif()
