// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_LLM_AGENT_HANDOFF_HPP
#define FORESTHUB_LLM_AGENT_HANDOFF_HPP

/// @file
/// Handoff tool that transfers control between agents.

#include <memory>
#include <string>
#include <utility>

#include "foresthub/llm/agent/agent.hpp"
#include "foresthub/llm/model.hpp"
#include "foresthub/llm/tools.hpp"
#include "foresthub/util/json.hpp"

namespace foresthub {
namespace agent {

using json = nlohmann::json;

/// Tool that transfers control from one agent to another.
class Handoff : public foresthub::llm::ExternalToolBase {
public:
    std::shared_ptr<Agent> target_agent;  ///< Agent to switch to when invoked.
    llm::ModelID model;                   ///< Optional model override (empty = use runner default).

    /// Construct a handoff tool that transfers control to the given target agent.
    /// @param name Tool name visible to the LLM.
    /// @param description Human-readable description sent to the LLM.
    /// @param agent Target agent to transfer control to.
    /// @param model Optional model override for the target agent.
    Handoff(std::string name, std::string description, std::shared_ptr<Agent> agent, foresthub::llm::ModelID model = "")
        : target_agent(std::move(agent)), model(std::move(model)) {
        this->name = std::move(name);
        this->description = std::move(description);

        parameters = json{{"type", "object"}, {"properties", json::object()}};
    }

    /// Identifies this tool as a handoff.
    llm::ToolType GetToolType() const override { return llm::ToolType::kHandoff; }
};

/// Create a new Handoff tool.
/// @param name Tool name visible to the LLM.
/// @param description Human-readable description sent to the LLM.
/// @param agent Target agent to transfer control to.
/// @param model Optional model override for the target agent.
/// @return Configured Handoff tool ready for use with Agent::AddTool().
inline std::shared_ptr<Handoff> NewHandoff(std::string name, std::string description, std::shared_ptr<Agent> agent,
                                           llm::ModelID model = "") {
    return std::make_shared<Handoff>(std::move(name), std::move(description), std::move(agent), std::move(model));
}

}  // namespace agent
}  // namespace foresthub

#endif  // FORESTHUB_LLM_AGENT_HANDOFF_HPP
