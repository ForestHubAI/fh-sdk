# ==============================================================================
# PC Examples (ForestHub + OpenAI — Chat + Agent with Tools)
# ==============================================================================

# --- ForestHub Provider ---
add_executable(foresthub_chat foresthub/chat.cpp)
add_executable(foresthub_agent foresthub/agent.cpp)

# --- OpenAI Provider ---
add_executable(foresthub_chat_openai openai/chat.cpp)
add_executable(foresthub_agent_openai openai/agent.cpp)

target_link_libraries(foresthub_chat PRIVATE foresthub_core)
target_link_libraries(foresthub_agent PRIVATE foresthub_core)
target_link_libraries(foresthub_chat_openai PRIVATE foresthub_core)
target_link_libraries(foresthub_agent_openai PRIVATE foresthub_core)
