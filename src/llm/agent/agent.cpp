// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "foresthub/llm/agent/agent.hpp"

#include <functional>
#include <utility>

#include "foresthub/llm/agent/runner.hpp"
#include "foresthub/llm/input.hpp"
#include "foresthub/llm/tools.hpp"
#include "foresthub/util/json.hpp"

namespace foresthub {
namespace agent {

using json = nlohmann::json;

Agent::Agent(std::string name) : name_(std::move(name)) {}

Agent& Agent::WithInstructions(std::string instructions) {
    instructions_ = std::move(instructions);
    return *this;
}

Agent& Agent::WithResponseFormat(llm::ResponseFormat format) {
    response_format_ = std::move(format);
    return *this;
}

Agent& Agent::WithOptions(llm::Options options) {
    options_ = std::move(options);
    return *this;
}

Agent& Agent::WithTools(std::vector<std::shared_ptr<llm::Tool>> tools) {
    tools_ = std::move(tools);
    return *this;
}

Agent& Agent::AddTool(std::shared_ptr<llm::Tool> tool) {
    tools_.push_back(std::move(tool));
    return *this;
}

std::shared_ptr<llm::ExternalTool> Agent::FindExternalTool(const std::string& name) const {
    for (const auto& tool : tools_) {
        if (tool->ToolName() == name && tool->IsExternal()) {
            return std::static_pointer_cast<llm::ExternalTool>(tool);
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

std::shared_ptr<llm::Tool> Agent::AsTool(std::string tool_name, std::string description,
                                          const std::shared_ptr<Runner>& runner) {
    // Snapshot agent state so the lambda does not capture raw `this`.
    auto snapshot = std::make_shared<Agent>(*this);

    std::function<json(InputPrompt)> handler = [snapshot, runner](const InputPrompt& args) -> json {
        auto input = std::make_shared<llm::InputString>(args.prompt);
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

    return llm::NewFunctionTool<InputPrompt, json>(std::move(tool_name), std::move(description), schema, handler);
}

}  // namespace agent
}  // namespace foresthub
