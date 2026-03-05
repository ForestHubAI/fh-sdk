#include "mapping.hpp"

#include "foresthub/core/input.hpp"
#include "foresthub/core/tools.hpp"
#include "foresthub/util/schema.hpp"
#include "provider/remote/schema_utils.hpp"

namespace foresthub {
namespace provider {
namespace remote {

// Normalizes and applies Gemini schema requirements (required all, no additionalProperties).
static json PrepareSchemaForGemini(const json& schema) {
    return StripAdditionalProperties(EnsureAllRequired(util::NormalizeSchema(schema)));
}

// Converts the polymorphic Input to Gemini contents array.
static json ToGeminiContents(const core::ChatRequest& req) {
    json contents = json::array();

    if (!req.input) {
        return contents;
    }

    // InputString → single user content
    if (req.input->GetInputType() == core::InputType::kString) {
        auto str = std::static_pointer_cast<core::InputString>(req.input);
        contents.push_back(json{{"role", "user"}, {"parts", json::array({json{{"text", str->text}}})}});
        return contents;
    }

    // InputItems → iterate and dispatch by item type
    auto items = std::static_pointer_cast<core::InputItems>(req.input);

    for (const auto& item : items->items) {
        if (!item) continue;

        switch (item->GetItemType()) {
            case core::InputItemType::kToolCall: {
                auto tcr = std::static_pointer_cast<core::ToolCallRequest>(item);
                // Parse stringified arguments back to JSON object for Gemini.
                json args = json::parse(tcr->arguments, nullptr, false);
                if (args.is_discarded()) {
                    args = json::object();
                }
                json part = json{{"functionCall", json{{"name", tcr->name}, {"args", args}}}};
                contents.push_back(json{{"role", "model"}, {"parts", json::array({std::move(part)})}});
                break;
            }
            case core::InputItemType::kToolResult: {
                auto tr = std::static_pointer_cast<core::ToolResult>(item);
                // Wrap output in a response object as Gemini expects.
                json response_obj;
                if (tr->output.is_object()) {
                    response_obj = tr->output;
                } else if (tr->output.is_string()) {
                    response_obj = json{{"result", tr->output.get<std::string>()}};
                } else {
                    response_obj = json{{"result", tr->output.dump()}};
                }
                json part = json{{"functionResponse", json{{"name", tr->name}, {"response", std::move(response_obj)}}}};
                contents.push_back(json{{"role", "user"}, {"parts", json::array({std::move(part)})}});
                break;
            }
            default: {
                // kString items in InputItems are always user messages.
                contents.push_back(json{{"role", "user"}, {"parts", json::array({json{{"text", item->ToString()}}})}});
                break;
            }
        }
    }

    return contents;
}

// Maps core::Options to Gemini generationConfig with camelCase keys.
static void IncludeGenerationConfig(json& j, const core::Options& opts,
                                    const foresthub::Optional<core::ResponseFormat>& response_format) {
    json config;

    if (opts.temperature) {
        config["temperature"] = *opts.temperature;
    }
    if (opts.max_tokens) {
        config["maxOutputTokens"] = *opts.max_tokens;
    }
    if (opts.top_p) {
        config["topP"] = *opts.top_p;
    }
    if (opts.top_k) {
        config["topK"] = *opts.top_k;
    }
    if (opts.presence_penalty) {
        config["presencePenalty"] = *opts.presence_penalty;
    }
    if (opts.frequency_penalty) {
        config["frequencyPenalty"] = *opts.frequency_penalty;
    }
    if (opts.seed) {
        config["seed"] = *opts.seed;
    }

    // Structured output via responseJsonSchema.
    // Gemini expects the JSON Schema object directly (type, properties, required),
    // not a wrapper. The schema name/description are not used by the Gemini API.
    if (response_format.HasValue()) {
        config["responseMimeType"] = "application/json";
        config["responseJsonSchema"] = PrepareSchemaForGemini(response_format->schema);
    }

    if (!config.empty()) {
        j["generationConfig"] = std::move(config);
    }
}

// Builds the tools array, handling the Search + Function Calling exclusion.
static void IncludeTools(json& j, const std::vector<std::shared_ptr<core::Tool>>& tools) {
    if (tools.empty()) {
        return;
    }

    bool has_external_tools = false;
    bool has_web_search = false;
    json function_declarations = json::array();

    for (const auto& tool : tools) {
        if (!tool) continue;

        if (tool->IsExternal()) {
            has_external_tools = true;
            auto ext = std::static_pointer_cast<core::ExternalTool>(tool);
            json decl;
            decl["name"] = ext->ToolName();
            decl["description"] = ext->ToolDescription();
            decl["parameters"] = PrepareSchemaForGemini(ext->ToolParameters());
            function_declarations.push_back(std::move(decl));
        } else if (tool->GetToolType() == core::ToolType::kWebSearch) {
            has_web_search = true;
        }
    }

    json tools_array = json::array();

    if (has_external_tools) {
        // Gemini does not support combining function calling with Google Search
        // in the same request. Function calling takes priority.
        tools_array.push_back(json{{"functionDeclarations", std::move(function_declarations)}});
    } else if (has_web_search) {
        tools_array.push_back(json{{"google_search", json::object()}});
    }

    if (!tools_array.empty()) {
        j["tools"] = std::move(tools_array);
    }
}

json ToGeminiRequest(const core::ChatRequest& req) {
    json j;

    // System instruction (top-level, not in contents)
    if (!req.system_prompt.empty()) {
        j["systemInstruction"] = json{{"parts", json::array({json{{"text", req.system_prompt}}})}};
    }

    // Contents (conversation history)
    j["contents"] = ToGeminiContents(req);

    // Tools
    IncludeTools(j, req.tools);

    // Generation config + structured output
    IncludeGenerationConfig(j, req.options, req.response_format);

    return j;
}

std::shared_ptr<core::ChatResponse> FromGeminiResponse(const json& j) {
    // Validate candidates array exists
    if (!j.contains("candidates") || !j["candidates"].is_array() || j["candidates"].empty()) {
        return nullptr;
    }

    auto resp = std::make_shared<core::ChatResponse>();

    // Extract text and tool calls from candidates[0].content.parts
    const json& candidate = j["candidates"][0];
    if (candidate.contains("content") && candidate["content"].contains("parts") &&
        candidate["content"]["parts"].is_array()) {
        int call_index = 0;
        for (const auto& part : candidate["content"]["parts"]) {
            // Text parts
            if (part.contains("text")) {
                std::string text = part.value("text", "");
                if (!text.empty()) {
                    if (!resp->text.empty()) {
                        resp->text += text;
                    } else {
                        resp->text = std::move(text);
                    }
                }
            }

            // Function call parts — Gemini returns args as JSON object, not stringified.
            if (part.contains("functionCall") && part["functionCall"].is_object()) {
                const json& fc = part["functionCall"];
                core::ToolCallRequest tcr;
                tcr.call_id = "gemini_call_" + std::to_string(call_index);
                tcr.name = fc.value("name", "");
                // Stringify args for the SDK's ToolCallRequest.arguments format.
                if (fc.contains("args") && fc["args"].is_object()) {
                    tcr.arguments = fc["args"].dump();
                } else {
                    tcr.arguments = "{}";
                }
                resp->tool_call_requests.push_back(std::move(tcr));
                ++call_index;
            }
        }
    }

    // Token usage from usageMetadata
    if (j.contains("usageMetadata") && j["usageMetadata"].is_object()) {
        const json& usage = j["usageMetadata"];
        resp->tokens_used = usage.value("totalTokenCount", 0);
    }

    // Gemini has no response ID equivalent — leave empty.

    return resp;
}

}  // namespace remote
}  // namespace provider
}  // namespace foresthub
