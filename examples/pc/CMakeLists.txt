# ==============================================================================
# PC Examples (Simple Chat + Agent with Tools)
# ==============================================================================

add_executable(foresthub_chat chat.cpp)
add_executable(foresthub_agent agent.cpp)

target_link_libraries(foresthub_chat PRIVATE foresthub_core)
target_link_libraries(foresthub_agent PRIVATE foresthub_core)
