# ==============================================================================
# Core Library Definition
# ==============================================================================

# 1. Define Platform-Independent Sources
# These files contain the logic that is identical on all platforms.
set(FORESTHUB_SOURCES
    llm/client.cpp
    llm/agent/agent.cpp
    llm/agent/runner.cpp
    llm/serialization.cpp
    llm/provider/remote/foresthub/provider.cpp
    llm/provider/remote/gemini/provider.cpp
    llm/provider/remote/gemini/mapping.cpp
    llm/provider/remote/anthropic/provider.cpp
    llm/provider/remote/anthropic/mapping.cpp
    llm/provider/remote/openai/provider.cpp
    llm/provider/remote/openai/mapping.cpp
    llm/provider/remote/schema_utils.cpp
    llm/rag/serialization.cpp
    llm/rag/remote/retriever.cpp
    util/schema.cpp
)

# 2. Platform Selection
# Embedded builds use PlatformIO (see pio/build_test/platformio.ini).
# CMake builds target one of the platforms below.
set(FORESTHUB_PLATFORM "pc" CACHE STRING "Target platform (pc, pc_debug)")
set_property(CACHE FORESTHUB_PLATFORM PROPERTY STRINGS pc pc_debug)

if(FORESTHUB_PLATFORM STREQUAL "pc_debug")
    message(STATUS "Configuring foresthub_core for: PC / DEBUG")
    list(APPEND FORESTHUB_SOURCES
        # Reuse PC implementations for network, time, crypto, http
        hal/pc/network.cpp
        hal/pc/time.cpp
        hal/pc/crypto.cpp
        hal/pc/http_client.cpp
        # Debug-specific implementations (override console, gpio, platform)
        hal/pc/debug/platform.cpp
        hal/pc/debug/console.cpp
        hal/pc/debug/gpio.cpp
    )
elseif(FORESTHUB_PLATFORM STREQUAL "pc")
    message(STATUS "Configuring foresthub_core for: PC / LINUX")
    list(APPEND FORESTHUB_SOURCES
        hal/pc/platform.cpp
        hal/pc/network.cpp
        hal/pc/console.cpp
        hal/pc/time.cpp
        hal/pc/crypto.cpp
        hal/pc/gpio.cpp
        hal/pc/http_client.cpp
    )
else()
    message(FATAL_ERROR "Unknown FORESTHUB_PLATFORM: '${FORESTHUB_PLATFORM}'. Valid values: pc, pc_debug")
endif()

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

    # Install-time include path
    $<INSTALL_INTERFACE:include>
)

# 5. Platform Compile Definitions
# Exactly one FORESTHUB_PLATFORM_* macro is defined, matching FORESTHUB_PLATFORM.
# JSON_HAS_CPP_14 prevents nlohmann/json from auto-detecting C++17 and including
# headers like <string_view> or <optional> that may be missing on embedded toolchains.
string(TOUPPER "${FORESTHUB_PLATFORM}" _PLAT_UPPER)
target_compile_definitions(foresthub_core PUBLIC
    FORESTHUB_PLATFORM_${_PLAT_UPPER} JSON_HAS_CPP_14
    FORESTHUB_ENABLE_NETWORK FORESTHUB_ENABLE_CRYPTO FORESTHUB_ENABLE_GPIO
)

# 6. Link Dependencies
# Find the system's threading library (needed for std::this_thread::sleep_for)
find_package(Threads REQUIRED)

# Link CPR and Threads to the library.
# "PRIVATE": These are implementation details. The 'app' using the library
# does not need to know that CPR is used internally.
target_link_libraries(foresthub_core PRIVATE cpr::cpr Threads::Threads)

# ==============================================================================
# Install Targets (for find_package(foresthub) support)
# ==============================================================================
# Export name: installed as foresthub::platform
set_target_properties(foresthub_core PROPERTIES EXPORT_NAME platform)

install(TARGETS foresthub_core
    EXPORT foresthubTargets
    ARCHIVE DESTINATION lib
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION include
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/third_party/nlohmann/
    DESTINATION include/nlohmann
)

# Install debug platform header so generated workflow code can #include it
if(FORESTHUB_PLATFORM STREQUAL "pc_debug")
    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/hal/pc/debug/platform.hpp
        DESTINATION include/foresthub/hal/debug
    )
endif()

install(EXPORT foresthubTargets
    NAMESPACE foresthub::
    DESTINATION lib/cmake/foresthub
)
