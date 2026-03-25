// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "mapping.hpp"

#include "foresthub/llm/input.hpp"
#include "foresthub/llm/tools.hpp"
#include "foresthub/util/schema.hpp"
#include "provider/remote/schema_utils.hpp"

namespace foresthub {
namespace provider {
namespace remote {

// Normalizes and applies OpenAI strict-mode requirements to a JSON Schema.
static json PrepareSchemaForOpenAI(const json& schema) {
    return SetNoAdditionalProperties(EnsureAllRequired(util::NormalizeSchema(schema)));
}

// Converts the polymorphic Input to an OpenAI Responses API input value.
static json ToOpenAIInput(const llm::ChatRequest& req) {
    if (!req.input) {
        return json::array();
    }

    // InputString → plain string value
    if (req.input->GetInputType() == llm::InputType::kString) {
        auto str = std::static_pointer_cast<llm::InputString>(req.input);
        return str->text;
    }

    // InputItems → typed array
    auto items = std::static_pointer_cast<llm::InputItems>(req.input);
    json messages = json::array();

    for (const auto& item : items->items) {
        if (!item) continue;

        switch (item->GetItemType()) {
            case llm::InputItemType::kToolCall: {
                auto tcr = std::static_pointer_cast<llm::ToolCallRequest>(item);
                json fc;
                fc["type"] = "function_call";
                fc["call_id"] = tcr->call_id;
                fc["name"] = tcr->name;
                fc["arguments"] = tcr->arguments;
                messages.push_back(std::move(fc));
                break;
            }
            case llm::InputItemType::kToolResult: {
                auto tr = std::static_pointer_cast<llm::ToolResult>(item);
                json fco;
                fco["type"] = "function_call_output";
                fco["call_id"] = tr->call_id;
                // output must be a string for the Responses API
                fco["output"] = tr->output.is_string() ? tr->output.get<std::string>() : tr->output.dump();
                messages.push_back(std::move(fco));
                break;
            }
            default: {
                // InputString and any other item types → user message
                json msg;
                msg["role"] = "user";
                msg["content"] = item->ToString();
                messages.push_back(std::move(msg));
                break;
            }
        }
    }

    return messages;
}

// Maps llm::Options to OpenAI Responses API parameters.
static void IncludeOptions(json& j, const llm::Options& opts) {
    if (opts.max_tokens) {
        j["max_output_tokens"] = *opts.max_tokens;
    }
    if (opts.temperature) {
        j["temperature"] = *opts.temperature;
    }
    if (opts.top_p) {
        j["top_p"] = *opts.top_p;
    }
}

// Extracts tool calls from the Responses API output array.
static void ExtractToolCalls(const json& output, std::vector<std::shared_ptr<llm::InternalToolCall>>& tools_called,
                             std::vector<llm::ToolCallRequest>& tool_call_requests) {
    for (const auto& item : output) {
        std::string type = item.value("type", "");

        if (type == "function_call") {
            llm::ToolCallRequest tcr;
            tcr.call_id = item.value("call_id", "");
            tcr.name = item.value("name", "");
            tcr.arguments = item.value("arguments", "");
            tool_call_requests.push_back(std::move(tcr));
        } else if (type == "web_search_call") {
            auto ws = std::make_shared<llm::WebSearchToolCall>();
            // OpenAI returns queries as an array; join into a single string.
            if (item.contains("queries") && item["queries"].is_array() && !item["queries"].empty() &&
                item["queries"][0].is_string()) {
                ws->query = item["queries"][0].get<std::string>();
            }
            tools_called.push_back(std::move(ws));
        }
    }
}

json ToOpenAIRequest(const llm::ChatRequest& req) {
    json j;
    j["model"] = req.model;
    j["input"] = ToOpenAIInput(req);

    // System prompt → instructions
    if (!req.system_prompt.empty()) {
        j["instructions"] = req.system_prompt;
    }

    // Previous response ID for multi-turn context
    if (!req.previous_response_id.empty()) {
        j["previous_response_id"] = req.previous_response_id;
    }

    // Tools
    if (!req.tools.empty()) {
        json tools = json::array();
        for (const auto& tool : req.tools) {
            if (!tool) continue;

            if (tool->IsExternal()) {
                // Responses API uses flat tool definition (no "function" wrapper).
                auto ext = std::static_pointer_cast<llm::ExternalTool>(tool);
                json t;
                t["type"] = "function";
                t["name"] = ext->ToolName();
                t["description"] = ext->ToolDescription();
                t["parameters"] = PrepareSchemaForOpenAI(ext->ToolParameters());
                tools.push_back(std::move(t));
            } else if (tool->GetToolType() == llm::ToolType::kWebSearch) {
                // Use "low" search context to minimize per-search cost.
                tools.push_back(json{{"type", "web_search"}, {"search_context_size", "low"}});
            }
        }
        if (!tools.empty()) {
            j["tools"] = std::move(tools);
        }
    }

    // Structured output → text.format
    if (req.response_format.HasValue()) {
        json fmt;
        fmt["type"] = "json_schema";
        fmt["name"] = req.response_format->name;
        fmt["schema"] = PrepareSchemaForOpenAI(req.response_format->schema);
        fmt["strict"] = true;
        if (!req.response_format->description.empty()) {
            fmt["description"] = req.response_format->description;
        }
        j["text"] = json{{"format", std::move(fmt)}};
    }

    // Options
    IncludeOptions(j, req.options);

    return j;
}

std::shared_ptr<llm::ChatResponse> FromOpenAIResponse(const json& j) {
    auto resp = std::make_shared<llm::ChatResponse>();

    resp->response_id = j.value("id", "");

    // Usage
    if (j.contains("usage") && j["usage"].is_object()) {
        resp->tokens_used = j["usage"].value("total_tokens", 0);
    }

    // Extract text from output[type=message].content[type=output_text].text.
    // Other content types (e.g., refusal, annotations) are ignored.
    if (j.contains("output") && j["output"].is_array()) {
        for (const auto& item : j["output"]) {
            if (item.value("type", "") == "message") {
                if (item.contains("content") && item["content"].is_array()) {
                    for (const auto& content : item["content"]) {
                        if (content.value("type", "") == "output_text") {
                            std::string text = content.value("text", "");
                            if (!text.empty()) {
                                if (!resp->text.empty()) {
                                    resp->text += text;
                                } else {
                                    resp->text = std::move(text);
                                }
                            }
                        }
                    }
                }
            }
        }

        // Extract tool calls
        ExtractToolCalls(j["output"], resp->tools_called, resp->tool_call_requests);
    }

    return resp;
}

}  // namespace remote
}  // namespace provider
}  // namespace foresthub
