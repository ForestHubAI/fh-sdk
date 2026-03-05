# ==============================================================================
# PC Examples (Anthropic + ForestHub + Gemini + OpenAI — Chat + Agent + WebSearch + StructuredOutput)
# ==============================================================================

# --- Anthropic Provider ---
add_executable(foresthub_chat_anthropic anthropic/chat.cpp)
add_executable(foresthub_agent_anthropic anthropic/agent.cpp)
add_executable(foresthub_websearch_anthropic anthropic/websearch.cpp)
add_executable(foresthub_structured_output_anthropic anthropic/structured_output.cpp)

# --- ForestHub Provider ---
add_executable(foresthub_chat foresthub/chat.cpp)
add_executable(foresthub_agent foresthub/agent.cpp)
add_executable(foresthub_websearch foresthub/websearch.cpp)
add_executable(foresthub_structured_output foresthub/structured_output.cpp)

# --- Gemini Provider ---
add_executable(foresthub_chat_gemini gemini/chat.cpp)
add_executable(foresthub_agent_gemini gemini/agent.cpp)
add_executable(foresthub_websearch_gemini gemini/websearch.cpp)
add_executable(foresthub_structured_output_gemini gemini/structured_output.cpp)

# --- OpenAI Provider ---
add_executable(foresthub_chat_openai openai/chat.cpp)
add_executable(foresthub_agent_openai openai/agent.cpp)
add_executable(foresthub_websearch_openai openai/websearch.cpp)
add_executable(foresthub_structured_output_openai openai/structured_output.cpp)

target_link_libraries(foresthub_chat_anthropic PRIVATE foresthub_core)
target_link_libraries(foresthub_agent_anthropic PRIVATE foresthub_core)
target_link_libraries(foresthub_websearch_anthropic PRIVATE foresthub_core)
target_link_libraries(foresthub_structured_output_anthropic PRIVATE foresthub_core)
target_link_libraries(foresthub_chat PRIVATE foresthub_core)
target_link_libraries(foresthub_agent PRIVATE foresthub_core)
target_link_libraries(foresthub_websearch PRIVATE foresthub_core)
target_link_libraries(foresthub_structured_output PRIVATE foresthub_core)
target_link_libraries(foresthub_chat_gemini PRIVATE foresthub_core)
target_link_libraries(foresthub_agent_gemini PRIVATE foresthub_core)
target_link_libraries(foresthub_websearch_gemini PRIVATE foresthub_core)
target_link_libraries(foresthub_structured_output_gemini PRIVATE foresthub_core)
target_link_libraries(foresthub_chat_openai PRIVATE foresthub_core)
target_link_libraries(foresthub_agent_openai PRIVATE foresthub_core)
target_link_libraries(foresthub_websearch_openai PRIVATE foresthub_core)
target_link_libraries(foresthub_structured_output_openai PRIVATE foresthub_core)
