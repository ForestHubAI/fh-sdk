// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_LLM_TOOLS_HPP
#define FORESTHUB_LLM_TOOLS_HPP

/// @file
/// Tool system: ExternalTool, FunctionTool, WebSearch, and tool call types.

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "foresthub/llm/input.hpp"
#include "foresthub/util/json.hpp"
#include "foresthub/util/schema.hpp"

namespace foresthub {
namespace llm {

using json = nlohmann::json;

// 1. Interfaces

/// Type discriminator for Tool subclasses.
enum class ToolType : uint8_t {
    kFunction,   ///< User-defined function tool with C++ callback.
    kWebSearch,  ///< Built-in web search tool.
    kHandoff     ///< Agent handoff tool.
};

/// Base interface for all tools.
class Tool {
public:
    virtual ~Tool() = default;

    /// Returns the unique name identifying this tool in the LLM schema.
    virtual std::string ToolName() const = 0;

    /// Returns the type discriminator for safe downcasting via static_pointer_cast.
    virtual ToolType GetToolType() const = 0;

    /// Serialize this tool to JSON.
    /// @param j Output JSON object, populated by the method.
    virtual void ToJson(json& j) const = 0;

    /// Check if this tool is an ExternalTool (enables static_pointer_cast<ExternalTool>).
    /// @return True for ExternalTool subclasses, false otherwise.
    virtual bool IsExternal() const { return false; }
};

/// Represents a tool call initiated internally by the model.
class InternalToolCall {
public:
    virtual ~InternalToolCall() = default;

    /// Returns the name of the internal tool that was called.
    virtual std::string ToolName() const = 0;
};

// 2. Data Structures

/// Request from the model to call a tool.
class ToolCallRequest : public InputItem {
public:
    std::string call_id;    ///< Provider-assigned call identifier for matching results.
    std::string name;       ///< Name of the tool to invoke.
    std::string arguments;  ///< Raw JSON string.

    /// Identifies this item as a tool call request.
    InputItemType GetItemType() const override { return InputItemType::kToolCall; }

    /// Human-readable summary of this tool call.
    std::string ToString() const override { return "Function tool call: " + name + " with arguments " + arguments; }

    /// Serialize this tool call request to JSON.
    void ToJson(json& j) const override {
        j = json{{"callId", call_id}, {"name", name}};
        json args_json = json::parse(arguments, nullptr, false);
        if (!args_json.is_discarded()) {
            j["arguments"] = args_json;
        } else {
            j["arguments"] = arguments;
        }
    }
};

/// Output of an executed tool.
class ToolResult : public InputItem {
public:
    std::string call_id;  ///< Matching call identifier from the request.
    std::string name;     ///< Name of the tool that produced this result.
    json output;          ///< Tool return value (string or structured JSON).

    /// Identifies this item as a tool execution result.
    InputItemType GetItemType() const override { return InputItemType::kToolResult; }

    /// Human-readable summary of the tool output.
    std::string ToString() const override {
        if (output.is_string()) {
            return "Function " + name + " returned: " + output.get<std::string>();
        }
        return "Function " + name + " returned: " + output.dump();
    }

    /// Serialize this tool result to JSON.
    void ToJson(json& j) const override {
        j = json{{"callId", call_id}, {"name", name}};
        j["output"] = output;
    }
};

// 3. External Tools

/// Tool with description and parameter schema, handled outside the LLM provider.
class ExternalTool : public Tool {
public:
    /// Human-readable description of this tool's purpose, sent to the LLM in the tool schema.
    virtual std::string ToolDescription() const = 0;

    /// JSON schema defining the expected parameters for this tool.
    virtual const json& ToolParameters() const = 0;

    /// Always returns true — this type is an ExternalTool.
    bool IsExternal() const override { return true; }
};

/// Base implementation of ExternalTool with name, description, and parameters.
class ExternalToolBase : public ExternalTool {
public:
    std::string name;         ///< Tool name sent to the LLM.
    std::string description;  ///< Human-readable description sent to the LLM.
    json parameters;          ///< JSON schema defining expected parameters.

    /// Returns the stored name field.
    std::string ToolName() const override { return name; }
    /// Returns the stored description field.
    std::string ToolDescription() const override { return description; }
    /// Returns the stored parameters field.
    const json& ToolParameters() const override { return parameters; }

    /// Serialize external tool definition to JSON.
    void ToJson(json& j) const override {
        j = json{{"type", "external"}, {"name", name}, {"description", description}, {"parameters", parameters}};
    }
};

/// ExternalTool with a C++ callback for execution.
class FunctionTool : public ExternalToolBase {
public:
    std::function<json(const std::string&)>
        tool_call;  ///< Callback invoked with raw JSON arguments; returns a json value used as ToolResult::output.
    /// Identifies this tool as a user-defined function.
    ToolType GetToolType() const override { return ToolType::kFunction; }
};

// 4. Factory Function

/// Create a FunctionTool with type-safe argument parsing.
/// @tparam T Argument struct type, deserialized from JSON via from_json().
///   T's from_json() MUST NOT throw — on no-exception platforms a throwing from_json()
///   causes undefined behavior (typically abort). Use j.value() with defaults, not j.at().
/// @tparam R Return type of the handler, convertible to nlohmann::json.
/// @param name Tool name visible to the LLM in the tool schema.
/// @param description Human-readable description sent to the LLM.
/// @param schema JSON schema for tool parameters.
/// @param handler Callback that receives parsed arguments and returns a result.
/// @return Configured FunctionTool ready for use with Agent::AddTool() or ChatRequest::AddTool().
template <typename T, typename R>
std::shared_ptr<FunctionTool> NewFunctionTool(std::string name, std::string description, const json& schema,
                                              const std::function<R(T)>& handler) {
    auto tool = std::make_shared<FunctionTool>();

    tool->name = std::move(name);
    tool->description = std::move(description);
    tool->parameters = util::NormalizeSchema(schema);

    tool->tool_call = [handler, tool_name = tool->name](const std::string& raw_args) -> json {
        json j_args = json::parse(raw_args, nullptr, false);
        if (j_args.is_discarded()) {
            return json(std::string("Error: Tool arguments for " + tool_name + " were not valid JSON."));
        }

        T args = j_args.get<T>();

        R result = handler(args);
        return result;
    };

    return tool;
}

// 5. Internal Tools

/// Built-in web search tool.
class WebSearch : public Tool {
public:
    /// Always returns "web_search".
    std::string ToolName() const override { return "web_search"; }
    /// Identifies this as a web search tool.
    ToolType GetToolType() const override { return ToolType::kWebSearch; }

    /// Serialize web search tool to JSON.
    void ToJson(json& j) const override { j = json{{"type", "web_search"}}; }
};

/// Log entry for a web search invocation.
class WebSearchToolCall : public InternalToolCall {
public:
    std::string query;  ///< Search query issued by the model.
    /// Always returns "web_search".
    std::string ToolName() const override { return "web_search"; }
};

}  // namespace llm
}  // namespace foresthub

#endif  // FORESTHUB_LLM_TOOLS_HPP
