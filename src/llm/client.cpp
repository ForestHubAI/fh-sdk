// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "foresthub/llm/client.hpp"

#include "llm/provider/remote/anthropic/provider.hpp"
#include "llm/provider/remote/foresthub/provider.hpp"
#include "llm/provider/remote/gemini/provider.hpp"
#include "llm/provider/remote/openai/provider.hpp"

namespace foresthub {
namespace llm {
using std::shared_ptr;

std::unique_ptr<Client> Client::Create(const ClientConfig& cfg,
                                       const std::shared_ptr<hal::HttpClient>& http_client) {
    auto client = std::make_unique<Client>();

    if (cfg.remote.anthropic.HasValue()) {
        auto provider = std::make_shared<anthropic::AnthropicProvider>(*cfg.remote.anthropic, http_client);
        client->RegisterProvider(provider);
    }

    if (cfg.remote.foresthub.HasValue()) {
        auto provider = std::make_shared<fh::ForestHubProvider>(*cfg.remote.foresthub, http_client);
        client->RegisterProvider(provider);
    }

    if (cfg.remote.gemini.HasValue()) {
        auto provider = std::make_shared<gemini::GeminiProvider>(*cfg.remote.gemini, http_client);
        client->RegisterProvider(provider);
    }

    if (cfg.remote.openai.HasValue()) {
        auto provider = std::make_shared<openai::OpenAIProvider>(*cfg.remote.openai, http_client);
        client->RegisterProvider(provider);
    }

    return client;
}

Client::Client() {}

void Client::RegisterProvider(const shared_ptr<Provider>& provider) {
    if (provider) {
        providers_[provider->ProviderId()] = provider;
    }
}

std::string Client::Health() const {
    if (providers_.empty()) {
        return "No providers configured";
    }

    std::string errors;
    for (const auto& entry : providers_) {
        std::string err = entry.second->Health();
        if (!err.empty()) {
            if (!errors.empty()) {
                errors += "; ";
            }
            errors += entry.first;
            errors += ": ";
            errors += err;
        }
    }

    return errors;
}

bool Client::SupportsModel(const ModelID& model) const {
    return InferProvider(model) != nullptr;
}

std::shared_ptr<ChatResponse> Client::Chat(const ChatRequest& req) {
    std::shared_ptr<Provider> provider = InferProvider(req.model);
    if (!provider) {
        return nullptr;
    }
    return provider->Chat(req);
}

std::shared_ptr<Provider> Client::InferProvider(const ModelID& model) const {
    for (const auto& entry : providers_) {
        if (entry.second->SupportsModel(model)) {
            return entry.second;
        }
    }
    return nullptr;
}

}  // namespace llm
}  // namespace foresthub
