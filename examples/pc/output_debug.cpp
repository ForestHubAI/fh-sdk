// PC debug wrapper for the codegen output example.
// Reuses workflow_logic.cpp from examples/embedded/output/ unchanged.
// Build: cmake -DBUILD_TESTING=ON .. && make foresthub_output_debug
// Debug: lldb ./foresthub_output_debug

#include <cstdio>
#include <cstdlib>

#include "../../examples/embedded/output/include/workflow_logic.hpp"
#include "platform_setup.hpp"

// Provide the env symbols that workflow_logic.cpp expects.
const char* kWifiSsid = "";
const char* kWifiPassword = "";
const char* kForesthubApiKey = nullptr;

int main() {
    // API key from environment (same as other PC examples)
    kForesthubApiKey = std::getenv("FORESTHUB_API_KEY");
    if (!kForesthubApiKey || kForesthubApiKey[0] == '\0') {
        std::fprintf(stderr, "[FATAL] FORESTHUB_API_KEY not set\n");
        return 1;
    }

    auto platform = app::SetupPlatform();
    WorkflowInit(platform);

    // Run the state machine a few ticks (kIdle -> kAgent1 -> kLedOn/kLedOff -> kIdle)
    for (int i = 0; i < 4; ++i) {
        WorkflowTick();
    }

    return 0;
}
