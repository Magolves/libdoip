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
LOG_DOIP_TRACE("Detailed trace information");
LOG_DOIP_DEBUG("Debug information");
LOG_DOIP_INFO("General information");
LOG_DOIP_WARN("Warning message");
LOG_DOIP_ERROR("Error occurred");
LOG_DOIP_CRITICAL("Critical error");
```

### Formatted Logging

```cpp
#include "Logger.h"

int port = 13400;
std::string interface = "eth0";
LOG_DOIP_INFO("DoIP server starting on interface '{}' port {}", interface, port);

// Works with any type that supports fmt formatting
auto timestamp = std::chrono::system_clock::now();
LOG_DOIP_DEBUG("Connection established at {}", timestamp);
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

