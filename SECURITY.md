# Security Policy

## Reporting a Vulnerability

**Please do NOT open a public issue for security vulnerabilities.**

If you discover a security vulnerability in fh-sdk, please report it responsibly:

1. Use GitHub's [private vulnerability reporting](https://github.com/ForestHubAI/fh-sdk/security/advisories/new) feature
2. Or email us directly at root@foresthub.ai

We will acknowledge your report within 48 hours and provide a timeline for a fix.

## Supported Versions

| Version | Supported |
|---------|-----------|
| Latest release | Yes |
| Older versions | No |

## Security Update Process

- Security fixes are released as patch versions
- Advisories are published via [GitHub Security Advisories](https://github.com/ForestHubAI/fh-sdk/security/advisories)
- Users are notified through GitHub's dependency alert system

## Scope

The following are considered security vulnerabilities in the context of this SDK:

- Credential or API key leakage through logs, error messages, or serialization
- Injection vulnerabilities in JSON parsing or HTTP request construction
- Unsafe memory access (buffer overflows, use-after-free, dangling pointers)
- Denial of service through unbounded resource consumption
- Man-in-the-middle vulnerabilities in TLS/HTTPS handling

Issues related to the security of upstream LLM providers (OpenAI, Anthropic, Google, ForestHub) should be reported to those providers directly.
