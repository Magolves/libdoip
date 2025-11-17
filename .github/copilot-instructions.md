# GitHub Copilot Instructions for libdoip

## Repository Overview

**libdoip** is a C++17 library implementing **Diagnostics over IP (DoIP)** protocol according to ISO 13400-2:2019. This is a fork/refactor from GerritRiesch94/libdoip with improved design and modern C++ standards.

### High-Level Information
- **Language**: C++17 (required, no extensions)
- **Build System**: CMake 3.15+ with Ninja generator preferred
- **Testing Framework**: doctest 2.4.9 for unit tests
- **Logging**: spdlog (auto-fetched if not installed)
- **Target Platforms**: Linux (primary)
- **Repository Size**: ~50 source files, 20k+ lines of code
- **Architecture**: Multi-threaded DoIP server/client with state machines

### Core Components
- **DoIP Server**: TCP/UDP socket handling, vehicle announcement, routing activation
- **DoIP Client**: Diagnostic client implementation
- **State Machines**: Connection state management and protocol compliance
- **Message Handling**: DoIP protocol message parsing and generation
- **Threading**: Timer management, thread-safe queues, alive check timers

## Build Instructions

### Prerequisites
```bash
# Ubuntu/Debian - Install required packages
sudo apt-get update
sudo apt-get install -y cmake gcc-11 g++-11
# Optional: libspdlog-dev (otherwise auto-fetched)
# For tests: sudo apt install doctest-dev
```

### Standard Build Sequence
**CRITICAL**: Always use this exact sequence to avoid build failures:

```bash
# 1. Clean build (required after header changes)
rm -rf build
mkdir build
cd build

# 2. Configure with required options
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DWITH_UNIT_TEST=ON \
  -DWITH_EXAMPLES=ON \
  -DWARNINGS_AS_ERRORS=OFF

# 3. Build (parallel compilation)
cmake --build . --parallel 4

# 4. Run tests
ctest --output-on-failure --parallel 4
```

### Alternative Build Methods
```bash
# Debug build with sanitizers
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON -DWITH_UNIT_TEST=ON

# Static analysis build
cmake .. -DENABLE_STATIC_ANALYSIS=ON -DWARNINGS_AS_ERRORS=OFF

# Release install
cmake --install . --prefix /usr/local
```

### Build Timing
- **Full clean build**: ~45-60 seconds (4 cores)
- **Incremental build**: ~5-15 seconds
- **Test execution**: ~30-45 seconds (20k+ assertions)
- **Documentation generation**: ~10-15 seconds

## Project Layout & Architecture

### Directory Structure
```
libdoip/
├── inc/                    # Public header files
│   ├── DoIPServer.h        # Main server implementation
│   ├── DoIPConnection.h    # Connection management
│   ├── DoIPServerStateMachine.h  # Protocol state machine
│   ├── TimerManager.h      # Thread-safe timer management
│   ├── ThreadSafeQueue.h   # Producer-consumer queue template
│   └── DoIP*.h            # Protocol message types and identifiers
├── src/                    # Implementation files (.cpp)
├── test/                   # Unit tests (doctest)
│   ├── *_Test.cpp         # Individual component tests
│   └── CMakeLists.txt     # Test configuration
├── examples/               # Usage examples
├── cmake/                  # CMake modules and configuration
├── doc/                    # Documentation (Markdown)
├── .github/workflows/      # CI/CD pipelines
└── build/                  # Build artifacts (created)
```

### Key Configuration Files
- **CMakeLists.txt**: Main build configuration, options, dependencies
- **cmake/projectSettings.cmake**: Compiler flags, warnings, sanitizers
- **.clang-tidy**: Static analysis rules (extensive rule set)
- **.clang-format**: Code formatting (Google style base)
- **Doxyfile**: API documentation generation
- **.github/workflows/ci.yml**: CI pipeline (build/test/docs/coverage)

### Critical Dependencies
- **Timer System**: TimerManager singleton used throughout codebase
- **Thread Safety**: ThreadSafeQueue, std::mutex, std::condition_variable
- **Logging Macros**: DOIP_LOG_* macros (requires Logger.h include)

### CI/CD Validation Pipeline
The GitHub Actions CI runs:
1. **Build Matrix**: GCC 11, Clang 14 × Debug/Release × Ubuntu 22.04
2. **Static Analysis**: clang-tidy with extensive rule set
3. **Sanitizers**: AddressSanitizer + UBSan in Debug builds
4. **Coverage**: lcov coverage collection and Codecov upload
5. **Documentation**: Doxygen generation → GitHub Pages deployment

To replicate CI locally:
```bash
# Match CI configuration exactly
cmake -B build -G Ninja \
  -DCMAKE_C_COMPILER=gcc-11 \
  -DCMAKE_CXX_COMPILER=g++-11 \
  -DCMAKE_BUILD_TYPE=Release \
  -DWITH_UNIT_TEST=ON \
  -DWARNINGS_AS_ERRORS=ON

cmake --build build --parallel 4
cd build && ctest --output-on-failure --parallel 4
```

### File Categories by Importance

**Core Architecture (Modify Carefully)**:
- `inc/DoIPServer.h` - Main server class, socket handling
- `inc/DoIPConnection.h` - Connection management, state machine integration
- `inc/DoIPServerStateMachine.h` - Protocol state machine (ISO compliance)
- `inc/TimerManager.h` - Thread-safe timer system

**Protocol Implementation**:
- `inc/DoIPMessage.h` - Message parsing/generation (4000+ lines)
- `inc/DoIPIdentifiers.h` - Protocol constants and types
- `inc/DoIP*.h` - Individual message type implementations

**Testing Infrastructure**:
- `test/*_Test.cpp` - Unit tests (11 test suites, 20k+ assertions)
- `test/ThreadSafeQueue_Test.cpp` - Thread safety validation
- `test/TimerManager_Test.cpp` - Timer system validation

**Known Issues to Avoid**:
- DoIPClient.h line 63: `HACK` comment indicates temporary solution
- State machine naming inconsistency: some use `snake_case`, headers use `PascalCase`
- Missing LOG_* macro definitions in some source files

### Validation Steps for Changes
1. **Always** run clean build after header modifications
2. **Always** run full test suite: `ctest --output-on-failure`
3. **Verify** no circular include chains with static analysis
4. **Check** member initialization order matches declaration order
5. **Test** thread safety with sanitizers: `-DENABLE_SANITIZERS=ON`

### Trust These Instructions
These instructions are validated and comprehensive. Only search for additional information if:
- Specific error messages not covered in "Common Build Issues"
- New features not documented in existing headers
- CI pipeline failures that don't match documented build sequence
