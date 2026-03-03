# ==============================================================================
# PC Examples (Simple Chat + Agent with Tools)
# ==============================================================================

add_executable(foresthub_chat chat.cpp)
add_executable(foresthub_agent agent.cpp)
add_executable(foresthub_output_debug
    output_debug.cpp
    ../embedded/output/src/workflow_logic.cpp
)

target_link_libraries(foresthub_chat PRIVATE foresthub_core)
target_link_libraries(foresthub_agent PRIVATE foresthub_core)
target_link_libraries(foresthub_output_debug PRIVATE foresthub_core)

# workflow_logic.cpp includes headers from the embedded output example
target_include_directories(foresthub_output_debug PRIVATE
    ${CMAKE_SOURCE_DIR}/examples/embedded/output/include
)
