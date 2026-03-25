// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "mapping.hpp"

#include "foresthub/llm/input.hpp"
#include "foresthub/llm/tools.hpp"
#include "foresthub/util/schema.hpp"
#include "llm/provider/remote/schema_utils.hpp"

namespace foresthub {
namespace llm {
namespace anthropic {

// Normalizes and applies Anthropic strict-mode requirements to a JSON Schema.
static json PrepareSchemaForAnthropic(const json& schema) {
    return SetNoAdditionalProperties(EnsureAllRequired(util::NormalizeSchema(schema)));
}

// Converts the polymorphic Input to Anthropic messages array.
static json ToAnthropicMessages(const llm::ChatRequest& req) {
    json messages = json::array();

    if (!req.input) {
        return messages;
    }

    // InputString → single user message
    if (req.input->GetInputType() == llm::InputType::kString) {
        auto str = std::static_pointer_cast<llm::InputString>(req.input);
        messages.push_back(json{{"role", "user"}, {"content", str->text}});
        return messages;
    }

    // InputItems → dispatch on item type.
    // Anthropic requires strictly alternating user/assistant roles. Consecutive items with the
    // same role (e.g. multiple tool_use blocks, or tool_result + text) are merged into a single
    // message with a multi-element content array.
    auto items = std::static_pointer_cast<llm::InputItems>(req.input);

    // Appends a content block to the messages array, merging with the previous message if same role.
    auto append_block = [&messages](const std::string& role, json block) {
        if (!messages.empty() && messages.back().value("role", "") == role && messages.back()["content"].is_array()) {
            messages.back()["content"].push_back(std::move(block));
        } else {
            messages.push_back(json{{"role", role}, {"content", json::array({std::move(block)})}});
        }
    };

    for (const auto& item : items->items) {
        if (!item) continue;

        switch (item->GetItemType()) {
            case llm::InputItemType::kToolCall: {
                auto tcr = std::static_pointer_cast<llm::ToolCallRequest>(item);
                // Parse arguments back to JSON object (Anthropic expects object, not string)
                json input_obj = json::parse(tcr->arguments, nullptr, false);
                if (input_obj.is_discarded()) {
                    input_obj = json::object();
                }
                json block;
                block["type"] = "tool_use";
                block["id"] = tcr->call_id;
                block["name"] = tcr->name;
                block["input"] = std::move(input_obj);
                append_block("assistant", std::move(block));
                break;
            }
            case llm::InputItemType::kToolResult: {
                auto tr = std::static_pointer_cast<llm::ToolResult>(item);
                // output must be a string for the tool_result content
                std::string output_str = tr->output.is_string() ? tr->output.get<std::string>() : tr->output.dump();
                json block;
                block["type"] = "tool_result";
                block["tool_use_id"] = tr->call_id;
                block["content"] = std::move(output_str);
                append_block("user", std::move(block));
                break;
            }
            default: {
                // kString — user text. InputItemType only defines kString, kToolCall, kToolResult,
                // so all default items are user messages.
                std::string text = item->ToString();
                append_block("user", json{{"type", "text"}, {"text", std::move(text)}});
                break;
            }
        }
    }

    return messages;
}

// Maps llm::Options to Anthropic request parameters.
static void IncludeOptions(json& j, const llm::Options& opts) {
    if (opts.temperature) {
        j["temperature"] = *opts.temperature;
    }
    if (opts.top_p) {
        j["top_p"] = *opts.top_p;
    }
    if (opts.top_k) {
        j["top_k"] = *opts.top_k;
    }
    // seed, frequency_penalty, presence_penalty are not supported by Anthropic — silently ignored.
}

json ToAnthropicRequest(const llm::ChatRequest& req, int default_max_tokens) {
    json j;
    j["model"] = req.model;

    // max_tokens is required by Anthropic (unlike OpenAI/Gemini where it's optional)
    j["max_tokens"] = req.options.max_tokens.HasValue() ? *req.options.max_tokens : default_max_tokens;

    // System prompt is a top-level field (not in messages array)
    if (!req.system_prompt.empty()) {
        j["system"] = req.system_prompt;
    }

    // Messages
    j["messages"] = ToAnthropicMessages(req);

    // Tools — ExternalTools only.
    // WebSearch tools are intentionally not included — Anthropic's server-side web search
    // requires encrypted_content passthrough in multi-turn conversations, which the SDK's
    // InputItems system does not currently support. See 19.2-CONTEXT.md for full rationale.
    if (!req.tools.empty()) {
        json tools = json::array();
        for (const auto& tool : req.tools) {
            if (!tool) continue;

            if (tool->IsExternal()) {
                auto ext = std::static_pointer_cast<llm::ExternalTool>(tool);
                json t;
                t["name"] = ext->ToolName();
                t["description"] = ext->ToolDescription();
                t["input_schema"] = PrepareSchemaForAnthropic(ext->ToolParameters());
                tools.push_back(std::move(t));
            }
            // WebSearch and other non-external tools are silently skipped.
        }
        if (!tools.empty()) {
            j["tools"] = std::move(tools);
        }
    }

    // Structured output → output_config.format.
    // Anthropic only supports type + schema; name and description are not part of the API.
    if (req.response_format.HasValue()) {
        json fmt;
        fmt["type"] = "json_schema";
        fmt["schema"] = PrepareSchemaForAnthropic(req.response_format->schema);
        j["output_config"] = json{{"format", std::move(fmt)}};
    }

    // Options (temperature, top_p, top_k)
    IncludeOptions(j, req.options);

    return j;
}

std::shared_ptr<llm::ChatResponse> FromAnthropicResponse(const json& j) {
    // Defensive guard: reject refusals before content extraction.
    // Anthropic may return "refusal" for policy violations (not yet confirmed in stable API).
    std::string stop_reason = j.value("stop_reason", "");
    if (stop_reason == "refusal") {
        return nullptr;
    }

    auto resp = std::make_shared<llm::ChatResponse>();

    resp->response_id = j.value("id", "");

    // Usage: input_tokens + output_tokens
    if (j.contains("usage") && j["usage"].is_object()) {
        int input_tokens = j["usage"].value("input_tokens", 0);
        int output_tokens = j["usage"].value("output_tokens", 0);
        resp->tokens_used = input_tokens + output_tokens;
    }

    // Content blocks: text and tool_use
    if (!j.contains("content") || !j["content"].is_array()) {
        return nullptr;
    }

    const json& content = j["content"];
    if (content.empty()) {
        return nullptr;
    }

    for (const auto& block : content) {
        std::string type = block.value("type", "");

        if (type == "text") {
            std::string text = block.value("text", "");
            if (!text.empty()) {
                if (!resp->text.empty()) {
                    resp->text += text;
                } else {
                    resp->text = std::move(text);
                }
            }
        } else if (type == "tool_use") {
            llm::ToolCallRequest tcr;
            tcr.call_id = block.value("id", "");
            tcr.name = block.value("name", "");
            // Anthropic returns input as JSON object — dump to string for SDK compatibility
            if (block.contains("input") && block["input"].is_object()) {
                tcr.arguments = block["input"].dump();
            }
            resp->tool_call_requests.push_back(std::move(tcr));
        }
        // server_tool_use, web_search_tool_result, thinking — skip silently
    }

    return resp;
}

}  // namespace anthropic
}  // namespace llm
}  // namespace foresthub
