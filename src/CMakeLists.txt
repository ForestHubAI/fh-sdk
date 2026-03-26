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
    message(STATUS "Configuring foresthub for: PC / DEBUG")
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
    message(STATUS "Configuring foresthub for: PC / LINUX")
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
# This compiles all .cpp files into a single library file (libforesthub.a).
add_library(foresthub STATIC ${FORESTHUB_SOURCES})

# 4. Define Include Directories
# "PUBLIC": The library uses these paths AND anyone linking against this library.
target_include_directories(foresthub PUBLIC
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
target_compile_definitions(foresthub PUBLIC
    FORESTHUB_PLATFORM_${_PLAT_UPPER} JSON_HAS_CPP_14
    FORESTHUB_ENABLE_NETWORK FORESTHUB_ENABLE_CRYPTO FORESTHUB_ENABLE_GPIO
)

# 6. Link Dependencies
# Find the system's threading library (needed for std::this_thread::sleep_for)
find_package(Threads REQUIRED)

# Link CPR and Threads to the library.
# "PRIVATE": These are implementation details. The 'app' using the library
# does not need to know that CPR is used internally.
target_link_libraries(foresthub PRIVATE cpr::cpr Threads::Threads)

# ==============================================================================
# Fat Static Archive
# ==============================================================================
# Merges foresthub + cpr + curl + zlib into a single self-contained archive.
# Consumers link one file — no transitive dependency resolution needed.
#
# The merge command is platform-specific:
#   Linux  — ar MRI script
#   macOS  — libtool -static
#   Windows — lib.exe

# Collect archive paths via generator expressions.
set(_FAT_INPUTS
    $<TARGET_FILE:foresthub>
    $<TARGET_FILE:cpr>
    $<TARGET_FILE:libcurl_static>
    $<TARGET_FILE:zlibstatic>
)

set(_FAT_OUTPUT_DIR "${CMAKE_BINARY_DIR}/lib")
if(WIN32)
    set(_FAT_OUTPUT "${_FAT_OUTPUT_DIR}/foresthub_fat.lib")
else()
    set(_FAT_OUTPUT "${_FAT_OUTPUT_DIR}/libforesthub_fat.a")
endif()

if(WIN32)
    # Windows: use lib.exe to merge .lib files
    add_custom_command(
        OUTPUT "${_FAT_OUTPUT}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_FAT_OUTPUT_DIR}"
        COMMAND lib.exe /NOLOGO /OUT:"${_FAT_OUTPUT}" ${_FAT_INPUTS}
        DEPENDS foresthub cpr libcurl_static zlibstatic
        COMMENT "Merging static libraries into foresthub_fat.lib"
    )
elseif(APPLE)
    # macOS: use libtool -static (ar doesn't support MRI scripts)
    add_custom_command(
        OUTPUT "${_FAT_OUTPUT}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_FAT_OUTPUT_DIR}"
        COMMAND libtool -static -o "${_FAT_OUTPUT}" ${_FAT_INPUTS}
        DEPENDS foresthub cpr libcurl_static zlibstatic
        COMMENT "Merging static libraries into libforesthub_fat.a"
    )
else()
    # Linux: use ar MRI script
    set(_MRI_SCRIPT "${CMAKE_BINARY_DIR}/merge_fat.mri")
    file(GENERATE OUTPUT "${_MRI_SCRIPT}" CONTENT
"CREATE ${_FAT_OUTPUT}
ADDLIB $<TARGET_FILE:foresthub>
ADDLIB $<TARGET_FILE:cpr>
ADDLIB $<TARGET_FILE:libcurl_static>
ADDLIB $<TARGET_FILE:zlibstatic>
SAVE
END
")
    add_custom_command(
        OUTPUT "${_FAT_OUTPUT}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_FAT_OUTPUT_DIR}"
        COMMAND ${CMAKE_AR} -M < "${_MRI_SCRIPT}"
        DEPENDS foresthub cpr libcurl_static zlibstatic "${_MRI_SCRIPT}"
        COMMENT "Merging static libraries into libforesthub_fat.a"
    )
endif()

add_custom_target(foresthub_fat ALL DEPENDS "${_FAT_OUTPUT}")

# ==============================================================================
# Install
# ==============================================================================
# Installs the fat archive + headers. No cmake config files — the consumer's
# CMakeLists links directly against the archive and sets compile definitions.

install(FILES "${_FAT_OUTPUT}" DESTINATION lib)

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
