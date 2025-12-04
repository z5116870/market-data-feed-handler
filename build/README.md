# Build Folder Usage Guide

This guide explains how to use the existing build/ directory to configure, compile, and run the project.

All commands assume you are inside:
 ```
cd build
```

1. Configure the Build

Run:
```
cmake ..
```

This generates Makefiles and configures the build system.

2. Build Everything

Run:
```
make -j
```

This produces the following executables:
```
bin/mdfh
bin/mdfh_unit_tests
bin/benchmark_multithreading
```

All executables are located in:
```
build/bin/
```

3. Running the Main Application

Run:
```
sudo ./bin/mdfh <MULTICAST_IP> <PORT>
```

4. Running Tests
Option A: Run the test binary directly
```
./bin/mdfh_unit_tests
```

To filter tests:
```
./bin/mdfh_unit_tests --gtest_filter=ParseTests.*
```

Option B: Run tests via CTest
```
ctest -V
```

Very verbose mode:
```
ctest -VV
```
5. Running Benchmarks

Run:
```
./bin/benchmark_multithreading
```

6. Reconfiguring or Rebuilding

If CMake files changed:
```
cmake ..
make -j
```

If only source files changed:
```
make -j
```

7. Cleaning the Build Folder

To remove all build artifacts:
```
rm -rf *
```
Then regenerate:
```
cmake ..
make -j
```

8. Directory Layout (Inside build/)

```
build/
├── bin/ (Executables)
├── lib/ (Libraries if generated)
├── CTestTestfile.cmake (CTest registry)
├── CMakeCache.txt
├── Makefile
└── CMakeFiles/ (Auto-generated CMake metadata)
```

### Summary

Run `sudo ./bin/mdfh <MULTICAST_IP> <PORT>`

Run `cmake ..` and `make -j` inside build/.

Binaries appear in `build/bin/.`

Run tests using `ctest -V` or `./bin/mdfh_unit_tests`

Run benchmarks using `./bin/benchmark_multithreading`.