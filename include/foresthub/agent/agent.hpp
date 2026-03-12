// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_AGENT_AGENT_HPP
#define FORESTHUB_AGENT_AGENT_HPP

/// @file
/// Agent with instructions, tools, and optional response format.

#include <memory>
#include <string>
#include <vector>

#include "foresthub/core/options.hpp"
#include "foresthub/core/tools.hpp"
#include "foresthub/core/types.hpp"
#include "foresthub/util/optional.hpp"

namespace foresthub {
namespace agent {

/// Forward declaration to avoid circular dependency with Runner.
class Runner;

/// LLM agent with instructions, tools, and optional response format.
class Agent {
public:
    /// Construct an agent with the given display name.
    explicit Agent(std::string name);

    /// Set the system prompt for this agent.
    Agent& WithInstructions(std::string instructions);

    /// Set the structured output format.
    Agent& WithResponseFormat(core::ResponseFormat format);

    /// Set generation parameters (temperature, max_tokens, etc.).
    Agent& WithOptions(core::Options options);

    /// Replace all tools with the given list.
    Agent& WithTools(std::vector<std::shared_ptr<core::Tool>> tools);

    /// Append a tool to the agent's tool list.
    Agent& AddTool(std::shared_ptr<core::Tool> tool);

    /// Search for an ExternalTool by name within the agent's tools.
    /// @param name Tool name to search for (exact match).
    /// @return Matching tool, or nullptr if not found.
    std::shared_ptr<core::ExternalTool> FindExternalTool(const std::string& name) const;

    /// Convert this agent into a tool that other agents can invoke.
    /// When called, the tool runs this agent via the given runner.
    /// @param tool_name Tool name visible to the LLM.
    /// @param description Human-readable description sent to the LLM.
    /// @param runner Runner instance used to execute this agent.
    /// @return A Tool wrapping this agent as a callable handoff.
    std::shared_ptr<core::Tool> AsTool(std::string tool_name, std::string description,
                                       const std::shared_ptr<Runner>& runner);

    /// @return Display name for identification.
    const std::string& name() const { return name_; }
    /// @return System prompt sent to the model.
    const std::string& instructions() const { return instructions_; }
    /// @return Optional structured output format.
    const foresthub::Optional<core::ResponseFormat>& response_format() const { return response_format_; }
    /// @return Generation parameters for this agent.
    const core::Options& options() const { return options_; }
    /// @return Tools available to this agent.
    const std::vector<std::shared_ptr<core::Tool>>& tools() const { return tools_; }

private:
    std::string name_;                                           ///< Display name for identification.
    std::string instructions_;                                   ///< System prompt sent to the model.
    foresthub::Optional<core::ResponseFormat> response_format_;  ///< Optional structured output format.
    core::Options options_;                                      ///< Generation parameters.
    std::vector<std::shared_ptr<core::Tool>> tools_;             ///< Tools available to this agent.
};

}  // namespace agent
}  // namespace foresthub

#endif  // FORESTHUB_AGENT_AGENT_HPP
