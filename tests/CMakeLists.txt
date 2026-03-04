# ==============================================================================
# Unit Tests
# ==============================================================================

include(GoogleTest)

# --- Core Tests ---------------------------------------------------------------

add_executable(run_core_tests
    core/input_test.cpp
    core/model_test.cpp
    core/options_test.cpp
    core/tools_test.cpp
    core/types_test.cpp
    core/json_test.cpp
    core/client_test.cpp
)

target_link_libraries(run_core_tests
    PRIVATE
    foresthub_core
    GTest::gtest_main
)

target_include_directories(run_core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

gtest_discover_tests(run_core_tests)

# --- Agent Tests --------------------------------------------------------------

add_executable(run_agent_tests
    agent/agent_test.cpp
    agent/runner_test.cpp
)

target_link_libraries(run_agent_tests
    PRIVATE
    foresthub_core
    GTest::gtest_main
)

target_include_directories(run_agent_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

gtest_discover_tests(run_agent_tests)

# --- Util Tests ---------------------------------------------------------------

add_executable(run_util_tests
    util/optional_test.cpp
    util/ticker_test.cpp
)

target_link_libraries(run_util_tests
    PRIVATE
    foresthub_core
    GTest::gtest_main
)

gtest_discover_tests(run_util_tests)

# --- Provider Tests -----------------------------------------------------------

add_executable(run_provider_tests
    provider/foresthub_test.cpp
    provider/gemini_test.cpp
    provider/openai_test.cpp
)

target_link_libraries(run_provider_tests
    PRIVATE
    foresthub_core
    GTest::gtest_main
)

target_include_directories(run_provider_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

gtest_discover_tests(run_provider_tests)

# --- Platform Tests -----------------------------------------------------------

add_executable(run_platform_tests
    platform/platform_test.cpp
    platform/gpio_test.cpp
)

target_link_libraries(run_platform_tests
    PRIVATE
    foresthub_core
    GTest::gtest_main
)

gtest_discover_tests(run_platform_tests)

# --- Integration Tests --------------------------------------------------------

add_executable(run_integration_tests
    integration/runner_provider_test.cpp
    integration/client_routing_test.cpp
)

target_link_libraries(run_integration_tests
    PRIVATE
    foresthub_core
    GTest::gtest_main
)

target_include_directories(run_integration_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

gtest_discover_tests(run_integration_tests)

# --- Contract Tests -----------------------------------------------------------

add_executable(run_contract_tests
    contract/foresthub_api_test.cpp
)

target_link_libraries(run_contract_tests
    PRIVATE
    foresthub_core
    GTest::gtest_main
)

target_include_directories(run_contract_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

gtest_discover_tests(run_contract_tests)
