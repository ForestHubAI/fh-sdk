cmake_minimum_required(VERSION 3.14)

# Define the project name and supported language (C++)
project(fh-sdk VERSION 0.1.2 LANGUAGES CXX)

# ==============================================================================
# Global Compiler Settings
# ==============================================================================
# Default to Release build if no build type is specified.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Force C++14 standard.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Disable compiler-specific extensions (e.g., -std=gnu++14) to ensure standard compliance.
set(CMAKE_CXX_EXTENSIONS OFF)

# ==============================================================================
# Build Options
# ==============================================================================
# Allow enabling/disabling tests (Default: ON)
option(BUILD_TESTING "Build unit tests" ON)
option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)
if(ENABLE_COVERAGE)
    message(STATUS "Code coverage enabled")
    add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
    add_link_options(--coverage)
endif()
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(ENABLE_SANITIZERS)
    message(STATUS "Sanitizers enabled (ASan + UBSan)")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()
# CPR overwrites BUILD_TESTING=OFF via FORCE — save and restore around FetchContent.
set(_USER_BUILD_TESTING ${BUILD_TESTING})

# Note: Embedded builds use PlatformIO (see pio/build_test/platformio.ini)

# ==============================================================================
# Dependencies (FetchContent)
# ==============================================================================
# FetchContent allows us to download dependencies at build time.
# This makes the project portable (Cloud/GitHub Actions) without manual installation.
include(FetchContent)

# 1. GoogleTest
# Only download GTest if tests are enabled.
if(BUILD_TESTING)
    message(STATUS "Fetching GoogleTest...")
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG v1.17.0        # Pinning a stable commit hash
      GIT_SHALLOW TRUE # Download only the latest commit to save time/space
    )
    # Fix for Windows builds to prevent C Runtime conflicts (harmless on Mac/Linux)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# 2. CPR (C++ Requests)
# This is the HTTP client for the PC platform.
message(STATUS "Fetching CPR (libcurl wrapper)...")
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.9.9      # Last version supporting C++14
    GIT_SHALLOW TRUE # Download only the latest commit to save time/space
)

# Configure CPR options:
# - CPR_USE_SYSTEM_CURL: OFF -> Forces CPR to build its own version of curl.
#   This ensures to have a known-good version independent of the host system.
# - CPR_BUILD_TESTS: OFF -> Skips building CPR's internal tests to save compilation time.
# - CPR_CURL_USE_LIBPSL: OFF -> Disables 'libpsl' (Public Suffix List) support.
#   Enabling this would require the 'meson' build system to be installed on the host,
#   which caused build failures. No need of strict cookie domain validation here.
set(CPR_USE_SYSTEM_CURL OFF CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CPR_CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(cpr)
# Restore BUILD_TESTING after CPR (CPR sets it to OFF with FORCE)
set(BUILD_TESTING ${_USER_BUILD_TESTING} CACHE BOOL "Build unit tests" FORCE)

# ==============================================================================
# Suppress Warnings from Third-Party Dependencies
# ==============================================================================
# All warnings in our build come from dependencies (curl deprecated macOS APIs,
# zlib macro redefinitions, unused clang flags). Our own code compiles clean.
# Suppress with -w so the build output stays readable.
foreach(_dep_target libcurl_shared cpr zlib zlibstatic)
    if(TARGET ${_dep_target})
        target_compile_options(${_dep_target} PRIVATE -w)
    endif()
endforeach()

# ==============================================================================
# Subdirectories
# ==============================================================================

# 1. Build the Core Library
add_subdirectory(src)

# 2. Build the PC Examples (only when building as top-level project, not when used as a dependency)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_subdirectory(examples/pc)
endif()

# 3. Build Tests
if(BUILD_TESTING)
    enable_testing() # Required by CTest
    add_subdirectory(tests)
endif()

# ==============================================================================
# Install (for find_package(foresthub) support)
# ==============================================================================
include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/foresthubConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/foresthubConfig.cmake"
    INSTALL_DESTINATION lib/cmake/foresthub
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/foresthubConfig.cmake"
    DESTINATION lib/cmake/foresthub
)