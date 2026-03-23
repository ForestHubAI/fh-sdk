# Hardware Abstraction Layer (HAL)

The HAL is the foundation of the SDK. It abstracts platform-specific services (networking, console I/O, time, crypto, GPIO) behind interfaces so that all code above it -- the LLM client, agent framework, RAG -- runs unchanged on any supported platform.

## Design Principles

### Compile-time platform selection, no runtime registry

There is no plugin system or runtime registration. Platform selection happens entirely at compile time through two mechanisms:

1. **Preprocessor guards** -- each `platform.cpp` is wrapped in `#ifdef FORESTHUB_PLATFORM_PC` or `#ifdef FORESTHUB_PLATFORM_ARDUINO`. Both files define the same factory function (`CreatePlatform()`), but only one compiles.
2. **Build system exclusion** -- on top of the `#ifdef` guards, PlatformIO's `srcFilter` in `library.json` physically excludes `platform/pc/` from embedded builds. CMake only lists `platform/pc/` sources in `src/CMakeLists.txt`.

The compile flag is set by the build system: CMake defines `FORESTHUB_PLATFORM_PC`, PlatformIO defines `FORESTHUB_PLATFORM_ARDUINO` (via `library.json` build flags).

### Two-layer selection

Platform activation has two layers:

1. **Which platform** -- `FORESTHUB_PLATFORM_PC` or `FORESTHUB_PLATFORM_ARDUINO` selects which implementation of `CreatePlatform()` compiles. This is mandatory.
2. **Which subsystems** -- on Arduino, individual subsystems are opt-in via additional macros (`FORESTHUB_ENABLE_NETWORK`, `FORESTHUB_ENABLE_CRYPTO`, `FORESTHUB_ENABLE_GPIO`). If a macro is not defined, that subsystem's field in `PlatformContext` is `nullptr`. Console and Time are always included.

On PC, all subsystems are always enabled.

### Interfaces enforce full implementation, subsystems are optional

Each subsystem is defined as an abstract interface with pure virtual methods (`= 0`). If a platform provides a subsystem, it **must implement every method** in that interface -- the compiler enforces this. For example, any class implementing `NetworkInterface` must provide `Connect()`, `Disconnect()`, `GetStatus()`, `GetLocalIp()`, and `GetSignalStrength()`.

However, whether a platform *provides* a subsystem at all is optional. `PlatformContext` holds subsystems as `shared_ptr` fields -- any of them can be `nullptr`:

```cpp
class PlatformContext {
public:
    std::shared_ptr<NetworkInterface> network;   // nullptr if FORESTHUB_ENABLE_NETWORK not defined
    std::shared_ptr<ConsoleInterface> console;   // always set
    std::shared_ptr<TimeInterface> time;         // always set
    std::shared_ptr<CryptoInterface> crypto;     // nullptr if FORESTHUB_ENABLE_CRYPTO not defined
    std::shared_ptr<GpioInterface> gpio;         // nullptr if FORESTHUB_ENABLE_GPIO not defined

    virtual std::shared_ptr<core::HttpClient> CreateHttpClient(const HttpClientConfig& config) = 0;
};
```

In summary: **within an interface, all methods are mandatory; but which interfaces a platform wires up is optional.**

## File Layout

```
include/foresthub/platform/     Abstract interfaces (the contract)
    platform.hpp                PlatformContext + CreatePlatform() factory
    network.hpp                 NetworkInterface
    console.hpp                 ConsoleInterface
    time.hpp                    TimeInterface
    crypto.hpp                  CryptoInterface
    gpio.hpp                    GpioInterface

src/platform/
    pc/                         PC implementations (CPR, stdin/stdout, std::chrono)
        platform.cpp            PcPlatformContext + CreatePlatform()
        network.cpp             PcNetwork : NetworkInterface
        console.cpp             PcConsole : ConsoleInterface
        time.cpp                PcTime : TimeInterface
        crypto.cpp              PcCrypto : CryptoInterface
        gpio.cpp                PcGpio : GpioInterface
        http_client.cpp         PcHttpClient : HttpClient (uses CPR/libcurl)
    arduino/                    Arduino implementations (WiFi, Serial, NTP)
        platform.cpp            ArduinoPlatformContext + CreatePlatform()
        network.cpp             ArduinoNetwork : NetworkInterface
        console.cpp             ArduinoConsole : ConsoleInterface
        time.cpp                ArduinoTime : TimeInterface
        crypto.cpp              ArduinoCrypto : CryptoInterface
        gpio.cpp                ArduinoGpio : GpioInterface
        http_client.cpp         ArduinoHttpClientWrapper : HttpClient
    common/                     Shared data (not an abstraction layer)
        tls_certificates.hpp    Root CA certs (used by Arduino crypto)
```

## How `CreatePlatform()` works

Both `pc/platform.cpp` and `arduino/platform.cpp` define the same free function:

```cpp
std::shared_ptr<PlatformContext> CreatePlatform(const PlatformConfig& config);
```

On PC it returns a `PcPlatformContext` with all subsystems wired up. On Arduino it returns an `ArduinoPlatformContext` with subsystems conditionally wired based on `FORESTHUB_ENABLE_*` macros. Application code calls `CreatePlatform()` without knowing which implementation it gets.

## The `common/` folder

This is **not** an override mechanism. It contains shared data that any platform implementation can `#include`. Currently it only holds TLS root certificates (`tls_certificates.hpp`) used by the Arduino crypto subsystem. The PC platform doesn't use it because CPR/libcurl manages its own certificate chain.

## Adding a new platform

To add support for a new platform (e.g. Zephyr RTOS):

1. Create `src/platform/zephyr/` with implementations of the interfaces you need
2. Add a `platform.cpp` guarded by `#ifdef FORESTHUB_PLATFORM_ZEPHYR` that defines `CreatePlatform()`
3. Set `-DFORESTHUB_PLATFORM_ZEPHYR` in your build system along with whichever `FORESTHUB_ENABLE_*` macros apply
4. Ensure the build system excludes other platform directories

No changes to core library code or headers are needed.
