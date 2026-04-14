# Contributing to fh-sdk

Thank you for your interest in contributing to fh-sdk! Please read our [Code of Conduct](https://github.com/ForestHubAI/fh-sdk/blob/main/.github/CODE_OF_CONDUCT.md) before participating.

## How to Contribute

1. **Report bugs** using the [bug report template](https://github.com/ForestHubAI/fh-sdk/issues/new?template=bug_report.yml)
2. **Request features** using the [feature request template](https://github.com/ForestHubAI/fh-sdk/issues/new?template=feature_request.yml)
3. **Submit pull requests** for bug fixes, features, or documentation improvements

## Development Setup

### Prerequisites

- C++14-compatible compiler (GCC 5+, Clang 3.4+, MSVC 2015+)
- CMake 3.14+
- clang-format and clang-tidy (recommended)
- PlatformIO (for embedded development only)

### Build (PC)

```bash
mkdir -p build && cd build && cmake -DBUILD_TESTING=ON .. && make -j4
```

### Run Tests

```bash
cd build && ctest --output-on-failure
```

### Format Code

```bash
find include src tests -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
```

### Build (Embedded / PlatformIO)

```bash
pio run -d pio/build_test -e esp32dev
```

## Code Style

- **C++14 only** — no C++17 features (`<optional>`, `<string_view>`, `<any>`, `if constexpr`, structured bindings, `std::variant`). Use `foresthub::Optional<T>` instead of `std::optional`.
- Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
- 4-space indent, 120-character line limit. See `.clang-format`.

**Naming conventions:**

| Element | Convention | Examples |
|---------|-----------|----------|
| Classes/Structs/Methods | PascalCase | `ChatRequest`, `AddTool()` |
| Getters/Accessors | snake_case | `tool_name()`, `call_id()` |
| Variables/Params/Struct members | snake_case | `base_url`, `api_key` |
| Private class members | snake_case_ | `timeout_ms_`, `start_time_` |
| Constants/Enum values | kPascalCase | `kPlainText`, `kConnected` |
| Namespaces | lowercase | `foresthub::core` |
| Macros | SCREAMING_CASE | `FORESTHUB_PLATFORM_PC` |
| Files | snake_case | `http_client.cpp` |

## Architectural Rules

The SDK enforces strict rules to support embedded platforms. See [ARCHITECTURE.md](https://github.com/ForestHubAI/fh-sdk/blob/main/.github/ARCHITECTURE.md) for full details.

- **No exceptions** — return `std::string` for errors (empty = success)
- **No RTTI** — no `dynamic_cast`, `dynamic_pointer_cast`, or `typeid`
- **Core/HAL separation** — core code must not depend on platform implementations
- **Platform isolation** — public headers must never include `<Arduino.h>` or other platform-specific headers

## Testing

- ~422 tests across 8 GoogleTest executables
- Hand-rolled mocks in `tests/mocks/` (no GMock — incompatible with `-fno-rtti`)
- New features should include corresponding tests
- All tests must pass before submitting a PR

## Pull Request Process

1. **Fork** the repository and create a feature branch from `main`
2. **Implement** your changes following the code style and architectural rules above
3. **Test** your changes: `cd build && ctest --output-on-failure`
4. **Format** your code: run clang-format on all modified files
5. **Submit** a pull request using the [PR template](https://github.com/ForestHubAI/fh-sdk/blob/main/.github/PULL_REQUEST_TEMPLATE.md)
6. **Review** — a maintainer will review your PR. Address any feedback.

### Commit Messages

Use clear, descriptive commit messages. Prefix with the type of change:

- `feat:` new feature
- `fix:` bug fix
- `refactor:` code restructuring without behavior change
- `test:` adding or updating tests
- `docs:` documentation changes

## Reporting Issues

Use the GitHub issue templates:
- [Bug Report](https://github.com/ForestHubAI/fh-sdk/issues/new?template=bug_report.yml)
- [Feature Request](https://github.com/ForestHubAI/fh-sdk/issues/new?template=feature_request.yml)

## License and Contributor Agreement

fh-sdk is dual-licensed: the public release is distributed under [AGPL-3.0](https://github.com/ForestHubAI/fh-sdk/blob/main/LICENSE), and ForestHub offers separate commercial licenses for use cases that are incompatible with AGPL. To keep this model viable, every contribution must grant ForestHub the rights needed to offer it under both license regimes.

**By submitting a contribution (pull request, patch, or any other code or documentation change), you agree to the following terms:**

1. **Grant of copyright license.** You grant ForestHub and recipients of software distributed by ForestHub a perpetual, worldwide, non-exclusive, royalty-free, irrevocable copyright license to reproduce, prepare derivative works of, publicly display, publicly perform, sublicense, and distribute your contribution and such derivative works.

2. **Right to relicense.** You expressly agree that ForestHub may license your contribution, in whole or in part, under the AGPL-3.0 **and/or under any other license terms of its choosing, including proprietary and commercial licenses.** This right covers future versions of the AGPL as well as any successor or alternative licenses ForestHub may adopt.

3. **Grant of patent license.** You grant ForestHub and recipients of the software a perpetual, worldwide, non-exclusive, royalty-free, irrevocable patent license to make, have made, use, offer to sell, sell, import, and otherwise transfer your contribution, where such license applies only to those patent claims licensable by you that are necessarily infringed by your contribution alone or by combination of your contribution with the project.

4. **Originality and right to submit.** You represent that each of your contributions is your original creation and that you are legally entitled to grant the above licenses. If your employer has rights to intellectual property that you create, you represent that you have received permission to make the contribution on behalf of that employer, or that your employer has waived such rights for your contribution to fh-sdk.

5. **No obligation and no warranty.** ForestHub is under no obligation to accept, include, or maintain your contribution. You provide your contribution "AS IS", without warranties or conditions of any kind, either express or implied.

6. **Governing law and jurisdiction.** This Contributor License Agreement and any contribution submitted under it shall be governed exclusively by the laws of the Federal Republic of Germany, excluding its conflict-of-laws rules. To the extent legally permissible, the exclusive place of jurisdiction for any dispute arising out of or in connection with this agreement is Germany.

You retain copyright in your contribution — this agreement grants rights, it does not transfer ownership. Under German law (§29 UrhG), the grants above are to be read as the broadest exclusive exploitation rights ("ausschließliche Nutzungsrechte") permissible, including for presently unknown forms of use to the extent allowed by §31a UrhG.
