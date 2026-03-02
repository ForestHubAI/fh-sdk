#ifndef FORESTHUB_AGENT_HANDOFF_HPP
#define FORESTHUB_AGENT_HANDOFF_HPP

#include <memory>
#include <string>
#include <utility>

#include "foresthub/agent/agent.hpp"
#include "foresthub/core/model.hpp"
#include "foresthub/core/tools.hpp"
#include "foresthub/util/json.hpp"

namespace foresthub {
namespace agent {

using json = nlohmann::json;

/// Tool that transfers control from one agent to another.
class Handoff : public foresthub::core::ExternalToolBase {
public:
    std::shared_ptr<Agent> target_agent;  ///< Agent to switch to when invoked.
    core::ModelID model;                  ///< Optional model override (empty = use runner default).

    /// Construct a handoff tool that transfers control to the given target agent.
    /// @param name Tool name visible to the LLM.
    /// @param description Human-readable description sent to the LLM.
    /// @param agent Target agent to transfer control to.
    /// @param model Optional model override for the target agent.
    Handoff(std::string name, std::string description, std::shared_ptr<Agent> agent,
            foresthub::core::ModelID model = "")
        : target_agent(std::move(agent)), model(std::move(model)) {
        this->name = std::move(name);
        this->description = std::move(description);

        parameters = json{{"type", "object"}, {"properties", json::object()}};
    }

    /// Identifies this tool as a handoff.
    core::ToolType GetToolType() const override { return core::ToolType::kHandoff; }
};

/// Create a new Handoff tool.
/// @param name Tool name visible to the LLM.
/// @param description Human-readable description sent to the LLM.
/// @param agent Target agent to transfer control to.
/// @param model Optional model override for the target agent.
/// @return Configured Handoff tool ready for use with Agent::AddTool().
inline std::shared_ptr<Handoff> NewHandoff(std::string name, std::string description, std::shared_ptr<Agent> agent,
                                           core::ModelID model = "") {
    return std::make_shared<Handoff>(std::move(name), std::move(description), std::move(agent), std::move(model));
}

}  // namespace agent
}  // namespace foresthub

#endif  // FORESTHUB_AGENT_HANDOFF_HPP
