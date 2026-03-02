#include "foresthub/agent/runner.hpp"

#include "foresthub/agent/handoff.hpp"
#include "foresthub/core/tools.hpp"

namespace foresthub {
namespace agent {

Runner::Runner(std::shared_ptr<foresthub::core::LLMClient> client, core::ModelID model)
    : client(std::move(client)), default_model(std::move(model)) {}

Runner& Runner::WithMaxTurns(int max_turns) {
    this->max_turns = max_turns;
    return *this;
}

RunResultOrError Runner::Run(const std::shared_ptr<Agent>& starting_agent, const std::shared_ptr<core::Input>& input) {
    if (!starting_agent) {
        return RunResultOrError::Failure("starting_agent cannot be null");
    }

    std::shared_ptr<Agent> current_agent = starting_agent;
    core::ModelID current_model = default_model;
    std::shared_ptr<core::InputItems> input_items_ptr = core::AsInputItems(input);

    for (int turn = 0; !max_turns.HasValue() || turn < *max_turns; ++turn) {
        core::ChatRequest req(current_model, input_items_ptr);
        req.system_prompt = current_agent->instructions;
        req.response_format = current_agent->response_format;
        req.tools = current_agent->tools;

        std::shared_ptr<core::ChatResponse> res_ptr = client->Chat(req);
        if (!res_ptr) {
            return RunResultOrError::Failure("Network error or empty response from provider");
        }
        core::ChatResponse& res = *res_ptr;

        // 1. Check for Final Output.
        if (!res.text.empty()) {
            if (req.response_format.HasValue() || res.tool_call_requests.empty()) {
                return RunResultOrError::Success(RunResult{res.text, current_agent, turn + 1});
            }
        }

        // 2. No text and no tool calls — model produced silence.
        if (res.tool_call_requests.empty()) {
            return RunResultOrError::Failure("Agent " + current_agent->name +
                                             " produced neither final output nor tool calls.");
        }

        // 3. Execute Tools.
        ExecResult exec_res = ExecuteFunctionTools(res.tool_call_requests, current_agent);

        if (!exec_res.error.empty()) {
            return RunResultOrError::Failure(exec_res.error);
        }

        // 4. Handoff Logic.
        if (exec_res.is_handoff) {
            auto handoff = std::static_pointer_cast<Handoff>(exec_res.handoff_tool);
            if (handoff && handoff->target_agent) {
                current_agent = handoff->target_agent;

                current_model = default_model;
                if (!handoff->model.empty()) {
                    current_model = handoff->model;
                }
                continue;  // Skip appending history.
            }
        }

        // 5. Update History.
        input_items_ptr->Insert(input_items_ptr->End(), exec_res.new_items.Begin(), exec_res.new_items.End());
    }

    return RunResultOrError::Failure("Max turns exceeded");
}

// Iterates tool call requests in order. Returns early on the first handoff.
// For function tools, executes via tool_call() and accumulates ToolCallRequest +
// ToolResult pairs in result.new_items. Returns error on unknown tool or unsupported type.
Runner::ExecResult Runner::ExecuteFunctionTools(const std::vector<core::ToolCallRequest>& requests,
                                                const std::shared_ptr<Agent>& agent) {
    ExecResult result;

    for (const auto& req : requests) {
        std::shared_ptr<core::ExternalTool> tool = agent->FindExternalTool(req.name);

        if (!tool) {
            result.error = "Tool " + req.name + " not found for agent " + agent->name;
            return result;
        }

        // Case A: Handoff — switch to another agent.
        if (tool->GetToolType() == core::ToolType::kHandoff) {
            auto handoff = std::static_pointer_cast<Handoff>(tool);
            result.handoff_tool = handoff;
            result.is_handoff = true;
            return result;
        }

        // Case B: Normal FunctionTool.
        if (tool->GetToolType() == core::ToolType::kFunction) {
            auto ft = std::static_pointer_cast<core::FunctionTool>(tool);
            json output = ft->tool_call(req.arguments);

            auto req_ptr = std::make_shared<core::ToolCallRequest>(req);
            result.new_items.PushBack(req_ptr);

            auto ft_result_ptr = std::make_shared<core::ToolResult>();
            ft_result_ptr->call_id = req.call_id;
            ft_result_ptr->name = ft->ToolName();
            ft_result_ptr->output = output;
            result.new_items.PushBack(ft_result_ptr);
        } else {
            result.error = "Unsupported tool execution type for tool: " + req.name;
            return result;
        }
    }
    return result;
}

}  // namespace agent
}  // namespace foresthub
