// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_LLM_TYPES_HPP
#define FORESTHUB_LLM_TYPES_HPP

/// @file
/// ChatRequest and ChatResponse types with fluent builder API.

#include <memory>
#include <string>
#include <vector>

#include "foresthub/llm/input.hpp"
#include "foresthub/llm/model.hpp"
#include "foresthub/llm/options.hpp"
#include "foresthub/llm/tools.hpp"
#include "foresthub/util/json.hpp"
#include "foresthub/util/optional.hpp"
#include "foresthub/util/schema.hpp"

namespace foresthub {
namespace llm {

using json = nlohmann::json;

/// Unique identifier for an uploaded file.
using FileID = std::string;

/// Structured output format with JSON schema constraints.
struct ResponseFormat {
    std::string name;         ///< Format name.
    json schema;              ///< JSON schema the response must conform to.
    std::string description;  ///< Human-readable description.
};

/// Chat completion request sent to an LLM provider.
struct ChatRequest {
    ModelID model;                                        ///< Target model identifier.
    std::shared_ptr<Input> input;                         ///< Polymorphic input (InputString or InputItems).
    std::string system_prompt;                            ///< System-level instructions.
    std::string previous_response_id;                     ///< Previous response ID for multi-turn context.
    util::Optional<ResponseFormat> response_format;       ///< Optional structured output format.
    std::vector<std::shared_ptr<Tool>> tools;             ///< Tools available to the model.
    std::vector<FileID> file_ids;                         ///< Attached file identifiers.
    std::vector<FileID> image_ids;                        ///< Attached image identifiers.
    std::vector<std::string> image_urls;                  ///< Attached image URLs.
    Options options;                                      ///< Generation parameters.

    /// Default constructor. Use the two-argument constructor or builder methods for configuration.
    ChatRequest() = default;

    /// Construct a request with model and input.
    /// @param model Target model identifier.
    /// @param input Polymorphic input (InputString or InputItems).
    ChatRequest(ModelID model, std::shared_ptr<Input> input) : model(std::move(model)), input(std::move(input)) {}

    // Builder methods for fluent configuration.

    /// Set the system prompt.
    ChatRequest& WithSystemPrompt(std::string prompt) {
        system_prompt = std::move(prompt);
        return *this;
    }

    /// Link to a previous response for multi-turn context.
    ChatRequest& WithPreviousResponseId(std::string id) {
        previous_response_id = std::move(id);
        return *this;
    }

    /// Set the structured output format.
    ChatRequest& WithResponseFormat(ResponseFormat format) {
        format.schema = util::NormalizeSchema(std::move(format.schema));
        response_format = std::move(format);
        return *this;
    }

    /// Append a tool to the request.
    ChatRequest& AddTool(std::shared_ptr<Tool> tool) {
        tools.push_back(std::move(tool));
        return *this;
    }

    /// Attach an uploaded file to the request.
    ChatRequest& AddFileId(FileID file_id) {
        file_ids.push_back(std::move(file_id));
        return *this;
    }

    /// Attach an uploaded image to the request.
    ChatRequest& AddImageId(FileID image_id) {
        image_ids.push_back(std::move(image_id));
        return *this;
    }

    /// Attach an image by URL.
    ChatRequest& AddImageUrl(std::string url) {
        image_urls.push_back(std::move(url));
        return *this;
    }

    /// Set the sampling temperature.
    ChatRequest& WithTemperature(float temperature) {
        options.WithTemperature(temperature);
        return *this;
    }

    /// Set the maximum number of tokens to generate.
    ChatRequest& WithMaxTokens(int max_tokens) {
        options.WithMaxTokens(max_tokens);
        return *this;
    }

    /// Set the top-k sampling limit.
    ChatRequest& WithTopK(int top_k) {
        options.WithTopK(top_k);
        return *this;
    }

    /// Set the nucleus sampling cutoff.
    ChatRequest& WithTopP(float top_p) {
        options.WithTopP(top_p);
        return *this;
    }

    /// Set the frequency penalty.
    ChatRequest& WithFrequencyPenalty(float frequency_penalty) {
        options.WithFrequencyPenalty(frequency_penalty);
        return *this;
    }

    /// Set the presence penalty.
    ChatRequest& WithPresencePenalty(float presence_penalty) {
        options.WithPresencePenalty(presence_penalty);
        return *this;
    }

    /// Set the random seed for deterministic output.
    ChatRequest& WithSeed(int seed) {
        options.WithSeed(seed);
        return *this;
    }
};

/// Chat completion response from an LLM provider.
struct ChatResponse {
    std::string text;                                             ///< Generated text output.
    std::vector<std::shared_ptr<InternalToolCall>> tools_called;  ///< Log of internal tools invoked by the model.
    std::vector<ToolCallRequest> tool_call_requests;              ///< Pending function calls requested by the model.
    std::string response_id;                                      ///< Provider-assigned response identifier.
    int tokens_used = 0;                                          ///< Total tokens consumed.
};

}  // namespace llm
}  // namespace foresthub

#endif  // FORESTHUB_LLM_TYPES_HPP
