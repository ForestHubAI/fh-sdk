// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_AGENT_RUNNER_HPP
#define FORESTHUB_AGENT_RUNNER_HPP

/// @file
/// Runner that executes agent loops with LLM calls, tool dispatch, and handoffs.

#include <memory>
#include <string>
#include <vector>

#include "foresthub/llm/agent/agent.hpp"
#include "foresthub/llm/input.hpp"
#include "foresthub/llm/provider.hpp"
#include "foresthub/util/json.hpp"
#include "foresthub/util/optional.hpp"

namespace foresthub {
namespace agent {

using json = nlohmann::json;

/// Final outcome of an agent execution workflow.
struct RunResult {
    json final_output;                  ///< Output produced by the agent.
    std::shared_ptr<Agent> last_agent;  ///< Agent active when execution finished.
    int turns = 0;                      ///< Number of LLM round-trips taken.
};

/// Result wrapper that holds either a RunResult or an error message.
struct RunResultOrError {
    foresthub::Optional<RunResult> result;  ///< Present on success.
    std::string error;                      ///< Non-empty on failure.

    /// Check if an error occurred.
    bool HasError() const { return !error.empty(); }

    /// Create a successful result.
    /// @param result Completed run result to wrap.
    /// @return RunResultOrError wrapping the given result.
    static RunResultOrError Success(RunResult result) { return {std::move(result), ""}; }

    /// Create a failure result.
    /// @param error_message Description of what went wrong.
    /// @return RunResultOrError with the error message.
    static RunResultOrError Failure(std::string error_message) {
        return {foresthub::Optional<RunResult>{}, std::move(error_message)};
    }
};

/// Manages the agent execution loop: LLM calls, tool dispatch, and handoffs.
class Runner {
public:
    /// Construct a runner with an LLM client and default model.
    /// @param client LLM client for chat requests.
    /// @param model Default model identifier.
    Runner(std::shared_ptr<foresthub::llm::LLMClient> client, foresthub::llm::ModelID model);

    /// Set the turn limit for agent execution.
    Runner& WithMaxTurns(int max_turns);

    /// Execute an agent workflow from the given starting point.
    /// @param starting_agent Agent that begins execution.
    /// @param input User prompt or conversation history.
    /// @return Success with RunResult, or failure with error message.
    RunResultOrError Run(const std::shared_ptr<Agent>& starting_agent, const std::shared_ptr<llm::Input>& input);

    /// @return LLM client for chat requests.
    const std::shared_ptr<foresthub::llm::LLMClient>& client() const { return client_; }
    /// @return Model used when agents don't specify one.
    const llm::ModelID& default_model() const { return default_model_; }
    /// @return Turn limit (empty = unlimited).
    const foresthub::Optional<int>& max_turns() const { return max_turns_; }

private:
    std::shared_ptr<foresthub::llm::LLMClient> client_;  ///< LLM client for chat requests.
    llm::ModelID default_model_;                         ///< Model used when agents don't specify one.
    foresthub::Optional<int> max_turns_;                  ///< Turn limit (empty = unlimited).

    /// Internal result from a single tool execution phase.
    struct ExecResult {
        std::shared_ptr<foresthub::llm::Tool> handoff_tool;  ///< Set if a handoff occurred.
        foresthub::llm::InputItems new_items;                ///< New history items from tool calls/results.
        bool is_handoff = false;                              ///< Whether a handoff was triggered.
        std::string error;                                    ///< Non-empty on internal error.
    };

    /// Execute a batch of tool call requests, returning new history items or handoff info.
    /// @param requests Tool calls received from the LLM response.
    /// @param agent Current agent, used to look up registered tools.
    /// @return ExecResult containing new history items, a possible handoff tool, and any error message.
    ExecResult ExecuteFunctionTools(const std::vector<foresthub::llm::ToolCallRequest>& requests,
                                    const std::shared_ptr<Agent>& agent);
};

}  // namespace agent
}  // namespace foresthub

#endif  // FORESTHUB_AGENT_RUNNER_HPP
