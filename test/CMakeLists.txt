cmake_minimum_required(VERSION 3.16)
project(mdfh_tests)

set(CMAKE_CXX_STANDARD 20)
set(SRC_DIR ../src)

include_directories(${SRC_DIR})

# ========== UNIT TESTS =====================
add_executable(test_parser unit/test_parser.cpp
    ${SRC_DIR}/parse.cpp
    ${SRC_DIR}/sequencer.cpp)

add_executable(test_spscq unit/test_spscq.cpp)

add_executable(test_sequencer unit/test_sequencer.cpp
    ${SRC_DIR}/sequencer.cpp)

add_executable(test_bufferpool unit/test_bufferpool.cpp
    ${SRC_DIR}/bufferpool.cpp)

# ========== INTEGRATION TESTS =============
add_executable(test_pipeline integration/test_pipeline.cpp
    ${SRC_DIR}/parse.cpp
    ${SRC_DIR}/sequencer.cpp
    ${SRC_DIR}/bufferpool.cpp)

add_executable(test_ring_emulator integration/test_ring_emulator.cpp
    ${SRC_DIR}/network.cpp
    ${SRC_DIR}/parse.cpp
    ${SRC_DIR}/sequencer.cpp
    ${SRC_DIR}/bufferpool.cpp)

add_executable(test_end_to_end integration/test_end_to_end.cpp
    ${SRC_DIR}/network.cpp
    ${SRC_DIR}/parse.cpp
    ${SRC_DIR}/sequencer.cpp
    ${SRC_DIR}/bufferpool.cpp)

# ========== STRESS TESTS ==================
add_executable(stress_burst stress/stress_burst.cpp
    ${SRC_DIR}/parse.cpp
    ${SRC_DIR}/sequencer.cpp
    ${SRC_DIR}/bufferpool.cpp)

add_executable(stress_random stress/stress_random.cpp
    ${SRC_DIR}/parse.cpp
    ${SRC_DIR}/sequencer.cpp)

add_executable(stress_concurrency stress/stress_concurrency.cpp
    ${SRC_DIR}/parse.cpp
    ${SRC_DIR}/sequencer.cpp
    ${SRC_DIR}/bufferpool.cpp)

add_executable(fuzz_parser stress/fuzz_parser.cpp
    ${SRC_DIR}/parse.cpp)
