cmake_minimum_required(VERSION 3.15)
project(libdoip
    VERSION 1.0.0
    DESCRIPTION "DoIP (Diagnostic over Internet Protocol) Library"
    LANGUAGES CXX
)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(WITH_UNIT_TEST "Build unit tests" OFF)
option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers in Debug builds" OFF)
option(ENABLE_STATIC_ANALYSIS "Enable static analysis tools" OFF)
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Strict C++ compiler options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Core warning flags
    set(STRICT_WARNING_FLAGS
        -Wall                    # Enable most warning messages
        -Wextra                  # Enable extra warning messages
        -Wpedantic              # Strict ISO C++ compliance
        -Wcast-align            # Warn about pointer casts which increase alignment
        -Wcast-qual             # Warn about casts which discard qualifiers
        -Wctor-dtor-privacy     # Warn about inaccessible ctors/dtors
        -Wdisabled-optimization # Warn when optimizations are disabled
        -Wformat=2              # Extra format string checking
        -Winit-self             # Warn about uninitialized variables which are initialized with themselves
        -Wlogical-op            # Warn about suspicious uses of logical operators
        -Wmissing-declarations  # Warn if a global function is defined without a declaration
        -Wmissing-include-dirs  # Warn if a user-supplied include directory does not exist
        -Wnoexcept              # Warn when a noexcept expression evaluates to false
        -Wold-style-cast        # Warn about old-style casts
        -Woverloaded-virtual    # Warn about overloaded virtual function names
        -Wredundant-decls       # Warn about redundant declarations
        -Wshadow                # Warn about variable shadowing
        -Wsign-conversion       # Warn about sign conversions
        -Wsign-promo            # Warn about promotions from signed to unsigned
        -Wstrict-null-sentinel  # Warn about uncasted NULL used as sentinel
        -Wstrict-overflow=5     # Warn about optimizations based on strict overflow rules
        -Wswitch-default        # Warn about switch statements without default
        -Wundef                 # Warn about undefined macros
        -Wunreachable-code      # Warn about unreachable code
        -Wunused                # Warn about unused variables, functions, etc.
    )

    # Apply warning flags
    add_compile_options(${STRICT_WARNING_FLAGS})

    # Treat warnings as errors if enabled
    if(WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()

    # GCC-specific warnings
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(
            -Wduplicated-cond       # Warn about duplicated conditions in if-else-if chains
            -Wduplicated-branches   # Warn about duplicated branches in if-else statements
            -Wnull-dereference      # Warn about potential null pointer dereferences
            -Wuseless-cast          # Warn about useless casts
            -Wlogical-op            # Warn about suspicious uses of logical operators
        )
    endif()

    # Clang-specific warnings
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(
            -Wdocumentation         # Warn about documentation issues
            -Wloop-analysis         # Warn about potentially infinite loops
            -Wthread-safety         # Enable thread safety analysis
        )
    endif()

    # Build-specific optimizations
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(
            -g                      # Debug information
            -O0                     # No optimization
            -fno-omit-frame-pointer # Keep frame pointers for debugging
            -fstack-protector-strong # Stack protection
        )
        add_compile_definitions(DEBUG=1)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(
            -O3                     # Maximum optimization
            -DNDEBUG               # Disable assertions
            -fstack-protector-strong # Stack protection
        )
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_compile_options(
            -O2                     # Moderate optimization
            -g                      # Debug information
            -fstack-protector-strong # Stack protection
        )
    endif()

    # Address Sanitizer for Debug builds (optional)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND ENABLE_SANITIZERS)
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # MSVC strict settings
    set(MSVC_WARNING_FLAGS
        /W4                     # Warning level 4
        /permissive-            # Disable non-conforming code
        /w14242                 # 'identifier': conversion from 'type1' to 'type2', possible loss of data
        /w14254                 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits'
        /w14263                 # 'function': member function does not override any base class virtual member function
        /w14265                 # 'classname': class has virtual functions, but destructor is not virtual
        /w14287                 # 'operator': unsigned/negative constant mismatch
        /we4289                 # loop control variable declared in the for-loop is used outside the for-loop scope
        /w14296                 # 'operator': expression is always 'boolean_value'
        /w14311                 # 'variable': pointer truncation from 'type1' to 'type2'
        /w14545                 # expression before comma evaluates to a function which is missing an argument list
        /w14546                 # function call before comma missing argument list
        /w14547                 # 'operator': operator before comma has no effect
        /w14549                 # 'operator': operator before comma has no effect
        /w14555                 # expression has no effect
        /w14619                 # pragma warning: there is no warning number 'number'
        /w14640                 # Enable warning on thread un-safe static member initialization
        /w14826                 # Conversion from 'type1' to 'type2' is sign-extended
        /w14905                 # wide string literal cast to 'LPSTR'
        /w14906                 # string literal cast to 'LPWSTR'
        /w14928                 # illegal copy-initialization
    )

    # Apply MSVC warning flags
    add_compile_options(${MSVC_WARNING_FLAGS})

    # Treat warnings as errors if enabled
    if(WARNINGS_AS_ERRORS)
        add_compile_options(/WX)
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/Od /Zi)
        add_compile_definitions(DEBUG=1)
    else()
        add_compile_options(/O2)
        add_compile_definitions(NDEBUG=1)
    endif()
endif()

# Static analysis tools
if(ENABLE_STATIC_ANALYSIS)
    # Find and enable clang-tidy
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY
            ${CLANG_TIDY_EXE};
            -checks=*,-llvmlibc-*,-fuchsia-*,-google-readability-todo,-readability-else-after-return;
            -header-filter=.*
        )
        message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    else()
        message(WARNING "clang-tidy not found!")
    endif()

    # Find and enable cppcheck
    find_program(CPPCHECK_EXE NAMES "cppcheck")
    if(CPPCHECK_EXE)
        set(CMAKE_CXX_CPPCHECK
            ${CPPCHECK_EXE};
            --enable=all;
            --std=c++17;
            --verbose;
            --quiet;
            --suppress=missingIncludeSystem
        )
        message(STATUS "cppcheck found: ${CPPCHECK_EXE}")
    else()
        message(WARNING "cppcheck not found!")
    endif()
endif()

# Additional strict settings for all compilers
add_compile_definitions(
    $<$<CONFIG:Debug>:_GLIBCXX_DEBUG>           # Enable libstdc++ debug mode in Debug builds
    $<$<CONFIG:Debug>:_GLIBCXX_DEBUG_PEDANTIC>  # Enable extra libstdc++ debug checks
)

# Position Independent Code for better security
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find required packages
find_package(Threads REQUIRED)

if (WITH_UNIT_TEST)
    # Enable testing
    enable_testing()

    # Find or download doctest
    find_package(doctest QUIET)
    if(NOT doctest_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            doctest
            GIT_REPOSITORY https://github.com/doctest/doctest.git
            GIT_TAG        v2.4.11
        )
        FetchContent_MakeAvailable(doctest)
    endif()

    # Include doctest CMake module for test discovery
    include(cmake/doctest.cmake)
endif()

# Add subdirectories for libraries
add_subdirectory(libdoipcommon)
add_subdirectory(libdoipserver)
add_subdirectory(libdoipclient)

# Add examples subdirectory
add_subdirectory(examples)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Install configuration
include(GNUInstallDirs)

# Install libraries
install(TARGETS doipcommon doipserver doipclient
    EXPORT libdoipTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers
install(DIRECTORY libdoipcommon/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libdoip)
install(DIRECTORY libdoipserver/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libdoip)
install(DIRECTORY libdoipclient/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libdoip)

# Create and install package config files
install(EXPORT libdoipTargets
    FILE libdoipTargets.cmake
    NAMESPACE libdoip::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libdoip
)

# Generate package config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/libdoipConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/libdoipConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/libdoipConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libdoip
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/libdoipConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/libdoipConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libdoip
)

