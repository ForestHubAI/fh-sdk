# ==============================================================================
# Unit Tests
# ==============================================================================

include(GoogleTest)

# --- Core Tests ---------------------------------------------------------------

add_executable(run_core_tests
    llm/input_test.cpp
    llm/model_test.cpp
    llm/options_test.cpp
    llm/tools_test.cpp
    llm/types_test.cpp
    llm/json_test.cpp
    llm/client_test.cpp
)

target_link_libraries(run_core_tests
    PRIVATE
    foresthub
    GTest::gtest_main
)

target_include_directories(run_core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

gtest_discover_tests(run_core_tests)

# --- Agent Tests --------------------------------------------------------------

add_executable(run_agent_tests
    llm/agent/agent_test.cpp
    llm/agent/runner_test.cpp
)

target_link_libraries(run_agent_tests
    PRIVATE
    foresthub
    GTest::gtest_main
)

target_include_directories(run_agent_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

gtest_discover_tests(run_agent_tests)

# --- Util Tests ---------------------------------------------------------------

add_executable(run_util_tests
    util/optional_test.cpp
    util/schema_test.cpp
    util/strprintf_test.cpp
    util/ticker_test.cpp
)

target_link_libraries(run_util_tests
    PRIVATE
    foresthub
    GTest::gtest_main
)

gtest_discover_tests(run_util_tests)

# --- Provider Tests -----------------------------------------------------------

add_executable(run_provider_tests
    llm/provider/anthropic_test.cpp
    llm/provider/foresthub_test.cpp
    llm/provider/gemini_test.cpp
    llm/provider/openai_test.cpp
    llm/provider/schema_utils_test.cpp
)

target_link_libraries(run_provider_tests
    PRIVATE
    foresthub
    GTest::gtest_main
)

target_include_directories(run_provider_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

gtest_discover_tests(run_provider_tests)

# --- Platform Tests -----------------------------------------------------------

add_executable(run_platform_tests
    hal/platform_test.cpp
    hal/console_test.cpp
    hal/gpio_test.cpp
)

target_link_libraries(run_platform_tests
    PRIVATE
    foresthub
    GTest::gtest_main
)

gtest_discover_tests(run_platform_tests)

# --- Debug Platform Tests -----------------------------------------------------
# Only built when targeting the debug platform, since the library must be compiled
# with FORESTHUB_PLATFORM_PC_DEBUG (platform macros are mutually exclusive).

if(FORESTHUB_PLATFORM STREQUAL "pc_debug")
    add_executable(run_debug_platform_tests
        hal/debug_console_test.cpp
        hal/debug_gpio_test.cpp
    )

    target_link_libraries(run_debug_platform_tests
        PRIVATE
        foresthub
        GTest::gtest_main
    )

    gtest_discover_tests(run_debug_platform_tests)
endif()

# --- Integration Tests --------------------------------------------------------

add_executable(run_integration_tests
    llm/integration/runner_provider_test.cpp
    llm/integration/client_routing_test.cpp
)

target_link_libraries(run_integration_tests
    PRIVATE
    foresthub
    GTest::gtest_main
)

target_include_directories(run_integration_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

gtest_discover_tests(run_integration_tests)

# --- RAG Tests ----------------------------------------------------------------

add_executable(run_rag_tests
    llm/rag/retriever_test.cpp
)

target_link_libraries(run_rag_tests
    PRIVATE
    foresthub
    GTest::gtest_main
)

target_include_directories(run_rag_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

gtest_discover_tests(run_rag_tests)

# --- Contract Tests -----------------------------------------------------------

add_executable(run_contract_tests
    llm/contract/foresthub_api_test.cpp
)

target_link_libraries(run_contract_tests
    PRIVATE
    foresthub
    GTest::gtest_main
)

target_include_directories(run_contract_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

gtest_discover_tests(run_contract_tests)
