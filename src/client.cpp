#include "foresthub/client.hpp"

#include "foresthub/provider/remote/anthropic.hpp"
#include "foresthub/provider/remote/foresthub.hpp"
#include "foresthub/provider/remote/gemini.hpp"
#include "foresthub/provider/remote/openai.hpp"

namespace foresthub {
using std::shared_ptr;

std::unique_ptr<Client> Client::Create(const config::ClientConfig& cfg,
                                       const std::shared_ptr<core::HttpClient>& http_client) {
    auto client = std::make_unique<Client>();

    if (cfg.remote.anthropic.HasValue()) {
        auto provider = std::make_shared<provider::remote::AnthropicProvider>(*cfg.remote.anthropic, http_client);
        client->RegisterProvider(provider);
    }

    if (cfg.remote.foresthub.HasValue()) {
        auto provider = std::make_shared<provider::remote::ForestHubProvider>(*cfg.remote.foresthub, http_client);
        client->RegisterProvider(provider);
    }

    if (cfg.remote.gemini.HasValue()) {
        auto provider = std::make_shared<provider::remote::GeminiProvider>(*cfg.remote.gemini, http_client);
        client->RegisterProvider(provider);
    }

    if (cfg.remote.openai.HasValue()) {
        auto provider = std::make_shared<provider::remote::OpenAIProvider>(*cfg.remote.openai, http_client);
        client->RegisterProvider(provider);
    }

    return client;
}

Client::Client() {}

void Client::RegisterProvider(const shared_ptr<core::Provider>& provider) {
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

bool Client::SupportsModel(const core::ModelID& model) const {
    return InferProvider(model) != nullptr;
}

std::shared_ptr<core::ChatResponse> Client::Chat(const core::ChatRequest& req) {
    std::shared_ptr<core::Provider> provider = InferProvider(req.model);
    if (!provider) {
        return nullptr;
    }
    return provider->Chat(req);
}

std::shared_ptr<core::Provider> Client::InferProvider(const core::ModelID& model) const {
    for (const auto& entry : providers_) {
        if (entry.second->SupportsModel(model)) {
            return entry.second;
        }
    }
    return nullptr;
}

}  // namespace foresthub
