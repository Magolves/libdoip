# Logging in libdoip

This library uses [spdlog](https://github.com/gabime/spdlog) for high-performance logging.

## Features

- **High Performance**: spdlog is one of the fastest C++ logging libraries
- **Thread Safe**: All logging operations are thread-safe
- **Multiple Log Levels**: TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL
- **Formatted Output**: Support for fmt-style formatting
- **Configurable Patterns**: Customize log message format
- **Color Support**: Colored console output for better readability

## Usage

### Basic Logging

```cpp
#include "Logger.h"

// Different log levels
DOIP_LOG_TRACE("Detailed trace information");
DOIP_LOG_DEBUG("Debug information");
DOIP_LOG_INFO("General information");
DOIP_LOG_WARN("Warning message");
DOIP_LOG_ERROR("Error occurred");
DOIP_LOG_CRITICAL("Critical error");
```

### Formatted Logging

```cpp
#include "Logger.h"

int port = 13400;
std::string interface = "eth0";
DOIP_LOG_INFO("DoIP server starting on interface '{}' port {}", interface, port);

// Works with any type that supports fmt formatting
auto timestamp = std::chrono::system_clock::now();
DOIP_LOG_DEBUG("Connection established at {}", timestamp);
```

### Configuration

```cpp
#include "Logger.h"

// Set log level (only messages at this level or higher will be shown)
doip::Logger::setLevel(spdlog::level::debug);

// Set custom pattern
doip::Logger::setPattern("[%H:%M:%S] [%^%l%$] %v");

// Available levels: trace, debug, info, warn, err, critical, off
```

### Pattern Format

The default pattern is: `[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v`

Common pattern flags:
- `%Y` - Year (4 digits)
- `%m` - Month (01-12)
- `%d` - Day (01-31)
- `%H` - Hour (00-23)
- `%M` - Minute (00-59)
- `%S` - Second (00-59)
- `%e` - Milliseconds (000-999)
- `%n` - Logger name
- `%l` - Log level
- `%^` - Start color range
- `%$` - End color range
- `%v` - The actual message

## Examples

See the `examples/loggerExample.cpp` file for a complete demonstration of logging features.

## Integration

The logging system is automatically initialized when first used. No manual setup is required, but you can customize the behavior using the `doip::Logger` class static methods.

## Thread Safety

All logging operations are thread-safe. You can safely log from multiple threads without any additional synchronization.

## Performance

spdlog provides excellent performance characteristics:
- Asynchronous logging support (if needed)
- Minimal overhead for disabled log levels
- Fast formatting using the fmt library
- Efficient memory usage

## Dependencies

The logging system automatically manages the spdlog dependency through CMake's FetchContent, so no manual installation is required.