# I LOVE CMAKE :((
#----------------
#SETUP
#----------------
cmake_minimum_required(VERSION 3.16) #tell CMake the minimum version required
project(market-data-feed-handler LANGUAGES CXX) #set project name, CXX = C++ only
enable_testing()

set(CMAKE_CXX_STANDARD 20) #C++20 compiler
set(CMAKE_CXX_STANDARD_REQUIRED ON) #Use only C++20, if not supported then FAIL ALL BUILDS
set(CMAKE_CXX_EXTENSIONS OFF) # equivalent to -std=c++20, DOES NOT USE -std=gnu++20 etc.

# set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # let build executables be output to the bin/ directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # let libary files be output to lib/
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CMakeFiles) # other files output to CMakeFiles

# We can also set a certain directory as a public header directory, so we dont need to use
# absolute paths as includes
include_directories(include)

# -----------------
# MDFH EXECUTABLE
# -----------------
# BUILD FILES
file(GLOB SRC_FILES "src/*.cpp") # Match all .cpp files in src/ and store it as SRC_FILES
add_executable(mdfh ${SRC_FILES}) # use that variable to create the executable, by using the build flags above into buid/ folder
add_library(mdfh_lib ${SRC_FILES}) # add the include files as a public library (for use with the test and benchmarks)
target_include_directories(mdfh_lib PUBLIC include)

# -----------------
# BENCHMARKING
# -----------------
# BENCHMARK MULTITHREADING
file(GLOB BENCHMARK_MMAP "test/benchmarking/custom/benchmark_multithreading.cpp" "src/parse.cpp")
add_executable(benchmark_multithreading ${BENCHMARK_MMAP})

# -----------------
# TESTING
# -----------------
include(FetchContent) # Download external librarier during configuration (gtest)

# Declare the content to fetch
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest) # make the downloaded content available as a linkable target in test code

# TEST BUILD FILES
file(GLOB UNIT_TEST_FILES "test/unit/*.cpp")
add_executable(mdfh_unit_tests ${UNIT_TEST_FILES})
target_link_libraries(mdfh_unit_tests PRIVATE gtest gtest_main mdfh_lib) # Link thegtest, gtest_main and mdfh libraries to the test binary
include(GoogleTest)
gtest_discover_tests(mdfh_unit_tests)

