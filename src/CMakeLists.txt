# ==============================================================================
# Core Library Definition
# ==============================================================================

# 1. Define Platform-Independent Sources
# These files contain the logic that is identical on all platforms.
set(FORESTHUB_SOURCES
    client.cpp
    agent/agent.cpp
    agent/runner.cpp
    core/serialization.cpp
    provider/remote/forest_hub.cpp
)

# 2. Add PC Platform Sources
# Embedded builds use PlatformIO (see pio/build_test/platformio.ini)
message(STATUS "Configuring foresthub_core for: PC / LINUX")
list(APPEND FORESTHUB_SOURCES
    platform/pc/platform.cpp
    platform/pc/network.cpp
    platform/pc/console.cpp
    platform/pc/time.cpp
    platform/pc/crypto.cpp
    platform/pc/gpio.cpp
    platform/pc/http_client.cpp
)

# 3. Create the Static Library
# This compiles all .cpp files into a single library file (libforesthub_core.a).
add_library(foresthub_core STATIC ${FORESTHUB_SOURCES})

# 4. Define Include Directories
# "PUBLIC": The library uses these paths AND anyone linking against this library.
target_include_directories(foresthub_core PUBLIC
    # The public API headers (root/include)
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>

    # External libraries (nlohmann json)
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party>

    # Allow internal source files to include headers from 'src/' directly.
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# 5. Platform Compile Definitions
# Define FORESTHUB_PLATFORM_PC for Translation Unit Guards in platform-specific .cpp files.
# JSON_HAS_CPP_14 prevents nlohmann/json from auto-detecting C++17 and including
# headers like <string_view> or <optional> that may be missing on embedded toolchains.
target_compile_definitions(foresthub_core PUBLIC
    FORESTHUB_PLATFORM_PC JSON_HAS_CPP_14
    FORESTHUB_ENABLE_NETWORK FORESTHUB_ENABLE_CRYPTO FORESTHUB_ENABLE_GPIO
)

# 6. Link Dependencies
# Find the system's threading library (needed for std::this_thread::sleep_for)
find_package(Threads REQUIRED)

# Link CPR and Threads to the library.
# "PRIVATE": These are implementation details. The 'app' using the library
# does not need to know that CPR is used internally.
target_link_libraries(foresthub_core PRIVATE cpr::cpr Threads::Threads)
