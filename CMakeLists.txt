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

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/Od)
    else()
        add_compile_options(/O2)
    endif()
endif()

# Find required packages
find_package(Threads REQUIRED)

# Enable testing
enable_testing()

# Add subdirectories for libraries
add_subdirectory(libdoipcommon)
add_subdirectory(libdoipserver)
add_subdirectory(libdoipclient)

# Add examples subdirectory
add_subdirectory(examples)

# Create main test runner executable
add_executable(TestRunner TestRunner.cpp)
target_link_libraries(TestRunner
    PRIVATE
    doipcommon
    doipserver
    doipclient
    Threads::Threads
)

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

