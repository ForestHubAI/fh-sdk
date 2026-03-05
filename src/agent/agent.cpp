#include "foresthub/agent/agent.hpp"

#include <functional>
#include <utility>

#include "foresthub/agent/runner.hpp"
#include "foresthub/core/input.hpp"
#include "foresthub/core/tools.hpp"
#include "foresthub/util/json.hpp"

namespace foresthub {
namespace agent {

using json = nlohmann::json;

Agent::Agent(std::string name) : name_(std::move(name)) {}

Agent& Agent::WithInstructions(std::string instructions) {
    instructions_ = std::move(instructions);
    return *this;
}

Agent& Agent::WithResponseFormat(core::ResponseFormat format) {
    response_format_ = std::move(format);
    return *this;
}

Agent& Agent::WithTools(std::vector<std::shared_ptr<core::Tool>> tools) {
    tools_ = std::move(tools);
    return *this;
}

Agent& Agent::AddTool(std::shared_ptr<core::Tool> tool) {
    tools_.push_back(std::move(tool));
    return *this;
}

std::shared_ptr<core::ExternalTool> Agent::FindExternalTool(const std::string& name) const {
    for (const auto& tool : tools_) {
        if (tool->ToolName() == name && tool->IsExternal()) {
            return std::static_pointer_cast<core::ExternalTool>(tool);
        }
    }
    return nullptr;
}

// Argument struct for the AsTool() handler: parsed from the LLM's JSON tool call.
struct InputPrompt {
    std::string prompt;
};

void from_json(const json& j, InputPrompt& args) {
    args.prompt = j.value("prompt", "");
}

std::shared_ptr<core::Tool> Agent::AsTool(std::string tool_name, std::string description,
                                          const std::shared_ptr<Runner>& runner) {
    // Snapshot agent state so the lambda does not capture raw `this`.
    auto snapshot = std::make_shared<Agent>(*this);

    std::function<json(InputPrompt)> handler = [snapshot, runner](const InputPrompt& args) -> json {
        auto input = std::make_shared<core::InputString>(args.prompt);
        RunResultOrError result_or_error = runner->Run(snapshot, input);
        if (result_or_error.HasError()) {
            return json(std::string("Error running agent tool: " + result_or_error.error));
        }

        return result_or_error.result->final_output;
    };

    json schema = {
        {"type", "object"},
        {"properties", {{"prompt", {{"type", "string"}, {"description", "The input prompt for the agent"}}}}},
        {"required", {"prompt"}},
        {"additionalProperties", false}};

    return core::NewFunctionTool<InputPrompt, json>(std::move(tool_name), std::move(description), schema, handler);
}

}  // namespace agent
}  // namespace foresthub
