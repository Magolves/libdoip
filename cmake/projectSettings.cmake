
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
        -Wmissing-declarations  # Warn if a global function is defined without a declaration
        -Wmissing-include-dirs  # Warn if a user-supplied include directory does not exist
        -Wold-style-cast        # Warn about old-style casts
        -Woverloaded-virtual    # Warn about overloaded virtual function names
        -Wredundant-decls       # Warn about redundant declarations
        -Wshadow                # Warn about variable shadowing
        -Wsign-conversion       # Warn about sign conversions
        -Wsign-promo            # Warn about promotions from signed to unsigned
        -Wstrict-overflow=2     # Warn about optimizations based on strict overflow rules (level 2 = reasonable balance)
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

    # GCC-specific warnings (only for GCC, not Clang)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(
            -Wduplicated-cond       # Warn about duplicated conditions in if-else-if chains
            -Wduplicated-branches   # Warn about duplicated branches in if-else statements
            -Wnull-dereference      # Warn about potential null pointer dereferences
            -Wuseless-cast          # Warn about useless casts
            -Wlogical-op            # Warn about suspicious uses of logical operators
            -Wnoexcept              # Warn when a noexcept expression evaluates to false
            -Wstrict-null-sentinel  # Warn about uncasted NULL used as sentinel
        )

        # Disable false positive warnings in GCC 13+
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)
            add_compile_options(
                -Wno-dangling-reference  # False positive with fmt library's stream operator detection
            )
        endif()
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

    # DoIPAddress Sanitizer for Debug builds (optional)
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

# Function to configure clang-tidy for test targets
function(configure_clang_tidy_for_tests target_name)
    if(CLANG_TIDY_EXE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set_target_properties(${target_name} PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-checks=${CLANG_TIDY_CHECKS_TEST};-header-filter=.*"
        )
    endif()
endfunction()


# Static analysis tools
if(ENABLE_STATIC_ANALYSIS)
    # Find and enable clang-tidy (only when using Clang to avoid GCC warning conflicts)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # Base clang-tidy checks for main code (with magic-numbers and nodiscard enabled)
        set(CLANG_TIDY_CHECKS_MAIN
            "*,-llvmlibc-*,-fuchsia-*,-google-readability-todo,-readability-else-after-return,-llvm-header-guard,-llvm-namespace-comment,-modernize-use-trailing-return-type,-altera-struct-pack-align,-google-explicit-constructor,-hicpp-explicit-conversions,-cppcoreguidelines-pro-bounds-pointer-arithmetic,-hicpp-signed-bitwise,-readability-identifier-length,-bugprone-reserved-identifier,-cert-dcl37-c,-cert-dcl51-cpp,-google-runtime-int,-misc-include-cleaner,-misc-non-private-member-variables-in-classes,-google-build-using-namespace,-readability-convert-member-functions-to-static,-llvm-include-order,-misc-const-correctness"
        )

        # Additional checks to disable for test files
        set(CLANG_TIDY_CHECKS_TEST
            "${CLANG_TIDY_CHECKS_MAIN},-cppcoreguidelines-avoid-magic-numbers,-readability-magic-numbers,-modernize-use-nodiscard"
        )

        # Default clang-tidy configuration for main code
        set(CMAKE_CXX_CLANG_TIDY
            ${CLANG_TIDY_EXE};
            -checks=${CLANG_TIDY_CHECKS_MAIN};
            -header-filter=.*;
            --quiet
        )
        message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE} (enabled for Clang builds)")
    elseif(CLANG_TIDY_EXE)
        message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE} (disabled with GCC due to warning conflicts)")
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
            --suppress=missingIncludeSystem;
            --suppress=unusedFunction;
            -i${CMAKE_BINARY_DIR}/_deps;
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