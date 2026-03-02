#include "foresthub/core/serialization.hpp"

namespace foresthub {
namespace core {

// 1. Core Structures (Options, ResponseFormat)

void to_json(json& j, const Options& opts) {
    j = json::object();
    if (opts.max_tokens) j["maxTokens"] = *opts.max_tokens;
    if (opts.temperature) j["temperature"] = *opts.temperature;
    if (opts.top_k) j["topK"] = *opts.top_k;
    if (opts.top_p) j["topP"] = *opts.top_p;
    if (opts.frequency_penalty) j["frequencyPenalty"] = *opts.frequency_penalty;
    if (opts.presence_penalty) j["presencePenalty"] = *opts.presence_penalty;
    if (opts.seed) j["seed"] = *opts.seed;
}

void from_json(const json& j, Options& opts) {
    if (j.contains("maxTokens")) opts.max_tokens = j.value("maxTokens", 0);
    if (j.contains("temperature")) opts.temperature = j.value("temperature", 0.0f);
    if (j.contains("topK")) opts.top_k = j.value("topK", 0);
    if (j.contains("topP")) opts.top_p = j.value("topP", 0.0f);
    if (j.contains("frequencyPenalty")) opts.frequency_penalty = j.value("frequencyPenalty", 0.0f);
    if (j.contains("presencePenalty")) opts.presence_penalty = j.value("presencePenalty", 0.0f);
    if (j.contains("seed")) opts.seed = j.value("seed", 0);
}

void to_json(json& j, const ResponseFormat& format) {
    j = json{{"name", format.name}};
    if (!format.description.empty()) j["description"] = format.description;

    j["schema"] = format.schema;
}

void from_json(const json& j, ResponseFormat& format) {
    format.name = j.value("name", "");
    format.description = j.value("description", "");
    if (j.contains("schema")) format.schema = j["schema"];
}

// 2. Polymorphism Helpers (Input, Tools)

void to_json(json& j, const std::shared_ptr<InputItem>& item) {
    if (!item) {
        j = nullptr;
        return;
    }
    item->ToJson(j);
}

void to_json(json& j, const std::shared_ptr<Tool>& tool) {
    if (!tool) {
        j = nullptr;
        return;
    }
    tool->ToJson(j);
}

// 3. Main Structs (Request / Response)

void to_json(json& j, const ChatRequest& req) {
    j = json::object();
    j["model"] = req.model;

    if (req.input) {
        json input_j;
        req.input->ToJson(input_j);
        j["input"] = std::move(input_j);
    }

    if (!req.system_prompt.empty()) j["systemPrompt"] = req.system_prompt;
    if (!req.previous_response_id.empty()) j["previousResponseID"] = req.previous_response_id;

    if (req.response_format) {
        j["responseFormat"] = *req.response_format;
    }

    if (!req.tools.empty()) j["tools"] = req.tools;
    if (!req.file_ids.empty()) j["fileIDs"] = req.file_ids;
    if (!req.image_ids.empty()) j["imageIDs"] = req.image_ids;
    if (!req.image_urls.empty()) j["imageURLs"] = req.image_urls;

    json opts_json;
    to_json(opts_json, req.options);
    if (!opts_json.empty()) {
        j["options"] = opts_json;
    }
}

void from_json(const json& j, ChatRequest& req) {
    req.model = j.value("model", "");
    req.system_prompt = j.value("systemPrompt", "");
    req.previous_response_id = j.value("previousResponseID", "");

    if (j.contains("responseFormat")) {
        ResponseFormat rf;
        from_json(j["responseFormat"], rf);
        req.response_format = rf;
    }

    if (j.contains("options")) {
        from_json(j["options"], req.options);
    }

    if (j.contains("input")) {
        const json& inp = j["input"];

        // Plain text input: {"value": "..."}
        if (inp.is_object() && inp.contains("value")) {
            req.input = std::make_shared<InputString>(inp.value("value", ""));
        } else if (inp.is_array()) {
            // Conversation history: array of typed items
            auto list = std::make_shared<InputItems>();
            for (const json& item : inp) {
                if (item.is_object() && item.contains("value")) {
                    // User/assistant text message
                    list->PushBack(std::make_shared<InputString>(item.value("value", "")));
                } else if (item.contains("callId") && item.contains("arguments")) {
                    // Tool call request from the model
                    auto tool_call = std::make_shared<ToolCallRequest>();
                    tool_call->call_id = item.value("callId", "");
                    tool_call->name = item.value("name", "");
                    const json& args = item["arguments"];
                    tool_call->arguments = args.is_string() ? args.get<std::string>() : args.dump();
                    list->PushBack(tool_call);
                } else if (item.contains("callId") && item.contains("output")) {
                    // Tool result returned to the model
                    auto tool_result = std::make_shared<ToolResult>();
                    tool_result->call_id = item.value("callId", "");
                    tool_result->name = item.value("name", "");
                    tool_result->output = item["output"];
                    list->PushBack(tool_result);
                }
            }
            req.input = list;
        }
    }

    if (j.contains("tools") && j["tools"].is_array()) {
        for (const json& item : j["tools"]) {
            std::string type = item.value("type", "");

            // "external" is the server's type name for user-defined function tools
            if (item.contains("name") || type == "function" || type == "external") {
                auto ft = std::make_shared<FunctionTool>();
                ft->name = item.value("name", "");
                ft->description = item.value("description", "");
                if (item.contains("parameters")) {
                    ft->parameters = item["parameters"];
                } else {
                    ft->parameters = json::object();
                }
                req.tools.push_back(ft);
            } else if (type == "web_search") {
                req.tools.push_back(std::make_shared<WebSearch>());
            }
        }
    }

    if (j.contains("fileIDs") && j["fileIDs"].is_array()) {
        for (const json& item : j["fileIDs"]) {
            if (item.is_string()) req.file_ids.push_back(item.get<std::string>());
        }
    }

    if (j.contains("imageIDs") && j["imageIDs"].is_array()) {
        for (const json& item : j["imageIDs"]) {
            if (item.is_string()) req.image_ids.push_back(item.get<std::string>());
        }
    }

    if (j.contains("imageURLs") && j["imageURLs"].is_array()) {
        for (const json& item : j["imageURLs"]) {
            if (item.is_string()) req.image_urls.push_back(item.get<std::string>());
        }
    }
}

void from_json(const json& j, ChatResponse& resp) {
    resp.text = j.value("text", "");
    resp.response_id = j.value("responseID", "");
    resp.tokens_used = j.value("tokensUsed", 0);

    if (j.contains("toolCallRequests")) {
        for (const json& item : j["toolCallRequests"]) {
            ToolCallRequest tool_call;
            tool_call.call_id = item.value("callId", "");
            tool_call.name = item.value("name", "");
            if (item.contains("arguments")) {
                const json& args = item["arguments"];
                tool_call.arguments = args.is_string() ? args.get<std::string>() : args.dump();
            }
            resp.tool_call_requests.push_back(tool_call);
        }
    }
}

// 4. File Operations

void to_json(json& j, const FileUploadResponse& response) {
    j = json{{"fileID", response.file_id}, {"fileName", response.file_name}};
}

void from_json(const json& j, FileUploadResponse& response) {
    response.file_id = j.value("fileID", "");
    response.file_name = j.value("fileName", "");
}

void to_json(json& j, const FileDeleteRequest& req) {
    j = json{{"fileID", req.file_id}, {"providerID", req.provider_id}};
}

void to_json(json& j, const FileUploadRequest& req) {
    j = json{{"fileName", req.file_name}, {"providerID", req.provider_id}, {"fileType", req.file_type}};
    if (!req.purpose.empty()) j["purpose"] = req.purpose;
}

}  // namespace core
}  // namespace foresthub
