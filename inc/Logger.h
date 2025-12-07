#pragma once

#include "AnsiColors.h"
#include <cstdlib>
#include <memory>
#include <mutex>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h> // For fmt::streamed() support
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>

#if !defined(FMT_VERSION) || FMT_VERSION < 90000
#include <optional>
#include <sstream>
namespace fmt {
template <typename T>
struct streamed_t {
    const T &value;
    explicit streamed_t(const T &v) : value(v) {}
};

template <typename T>
streamed_t<T> streamed(const T &v) { return streamed_t<T>(v); }

template <typename OStream, typename T>
OStream &operator<<(OStream &os, const streamed_t<T> &s) {
    os << s.value;
    return os;
}

// Fallback for std::optional<T>
template <typename OStream, typename T>
OStream &operator<<(OStream &os, const streamed_t<std::optional<T>> &s) {
    if (s.value.has_value()) {
        os << *s.value;
    } else {
        os << "<nullopt>";
    }
    return os;
}
} // namespace fmt

// namespace std {
// template <typename T>
// std::ostream& operator<<(std::ostream& os, const std::optional<T>& opt) {
//     if (opt) {
//         os << *opt;
//     } else {
//         os << "<nullopt>";
//     }
//     return os;
// }
//}
#endif

namespace doip {

constexpr const char *DEFAULT_PATTERN = "[%H:%M:%S.%e] [%n] [%^%l%$] %v";

/**
 * @brief Pattern for short output without timestamp
 *
 */
constexpr const char *SHORT_PATTERN = "[%n] [%^%l%$] %v";

/**
 * @brief Centralized logger for the DoIP library
 */
class Logger {
  public:
    static std::shared_ptr<spdlog::logger> get(const std::string &name = "doip", spdlog::level::level_enum level = spdlog::level::info) {
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock(mutex);

        if (auto it = m_loggers.find(name); it != m_loggers.end()) {
            return it->second;
        }

        auto new_log = spdlog::stdout_color_mt(name);
        new_log->set_level(level);
        new_log->set_pattern(DEFAULT_PATTERN);
        m_loggers.emplace(name, new_log);
        return new_log;
    }

    static std::shared_ptr<spdlog::logger> getUdp() {
        return get("udp ");
    }

    static std::shared_ptr<spdlog::logger> getTcp() {
        return get("tcp ");
    }

    static void setLevel(spdlog::level::level_enum level) {
        get()->set_level(level);
    }

    static void setPattern(const std::string &pattern) {
        get()->set_pattern(pattern);
    }

    static bool colorsSupported() {
        const char *term = std::getenv("TERM");
        const char *colorterm = std::getenv("COLORTERM");

        if (term == nullptr)
            return false;

        std::string termStr(term);
        return termStr.find("color") != std::string::npos ||
               termStr.find("xterm") != std::string::npos ||
               termStr.find("screen") != std::string::npos ||
               colorterm != nullptr;
    }

  private:
    static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> m_loggers;
};

} // namespace doip

// Logging macros
#define LOG_DOIP_TRACE(...) doip::Logger::get()->trace(__VA_ARGS__)
#define LOG_DOIP_DEBUG(...) doip::Logger::get()->debug(__VA_ARGS__)
#define LOG_DOIP_INFO(...) doip::Logger::get()->info(__VA_ARGS__)
#define LOG_DOIP_WARN(...) doip::Logger::get()->warn(__VA_ARGS__)
#define LOG_DOIP_ERROR(...) doip::Logger::get()->error(__VA_ARGS__)
#define LOG_DOIP_CRITICAL(...) doip::Logger::get()->critical(__VA_ARGS__)

// Logging macros for UDP socket
#define LOG_UDP_TRACE(...) doip::Logger::getUdp()->trace(__VA_ARGS__)
#define LOG_UDP_DEBUG(...) doip::Logger::getUdp()->debug(__VA_ARGS__)
#define LOG_UDP_INFO(...) doip::Logger::getUdp()->info(__VA_ARGS__)
#define LOG_UDP_WARN(...) doip::Logger::getUdp()->warn(__VA_ARGS__)
#define LOG_UDP_ERROR(...) doip::Logger::getUdp()->error(__VA_ARGS__)
#define LOG_UDP_CRITICAL(...) doip::Logger::getUdp()->critical(__VA_ARGS__)

// Logging macros for TCP socket
#define LOG_TCP_TRACE(...) doip::Logger::getTcp()->trace(__VA_ARGS__)
#define LOG_TCP_DEBUG(...) doip::Logger::getTcp()->debug(__VA_ARGS__)
#define LOG_TCP_INFO(...) doip::Logger::getTcp()->info(__VA_ARGS__)
#define LOG_TCP_WARN(...) doip::Logger::getTcp()->warn(__VA_ARGS__)
#define LOG_TCP_ERROR(...) doip::Logger::getTcp()->error(__VA_ARGS__)
#define LOG_TCP_CRITICAL(...) doip::Logger::getTcp()->critical(__VA_ARGS__)

// Colored logging macros
#define LOG_DOIP_SUCCESS(...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_green) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

#define LOG_DOIP_ERROR_COLORED(...) \
    doip::Logger::get()->error(std::string(doip::ansi::bold_red) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

#define LOG_DOIP_PROTOCOL(...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_blue) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

#define LOG_DOIP_CONNECTION(...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_magenta) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

#define LOG_DOIP_HIGHLIGHT(...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_cyan) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

// Convenience macros for types with stream operators (using fmt::streamed)
// These automatically wrap arguments with fmt::streamed() for seamless logging of DoIP types
#define LOG_DOIP_STREAM_INFO(obj, ...) LOG_DOIP_INFO(fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)))
#define LOG_DOIP_STREAM_DEBUG(obj, ...) LOG_DOIP_DEBUG(fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)))
#define LOG_DOIP_STREAM_WARN(obj, ...) LOG_DOIP_WARN(fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)))
#define LOG_DOIP_STREAM_ERROR(obj, ...) LOG_DOIP_ERROR(fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)))

// Colored stream logging macros for DoIP types
#define LOG_DOIP_STREAM_SUCCESS(obj, ...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_green) + fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)) + doip::ansi::reset)

#define LOG_DOIP_STREAM_PROTOCOL(obj, ...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_blue) + fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)) + doip::ansi::reset)

#define LOG_DOIP_STREAM_CONNECTION(obj, ...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_magenta) + fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)) + doip::ansi::reset)
