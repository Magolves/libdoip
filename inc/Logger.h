#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>  // For fmt::streamed() support
#include <memory>
#include <mutex>
#include <cstdlib>
#include <string>
#include "AnsiColors.h"

namespace doip {

constexpr const char* DEFAULT_PATTERN = "[%H:%M:%S.%e] [%n] [%^%l%$] %v";

/**
 * @brief Pattern for short output without timestamp
 *
 */
constexpr const char* SHORT_PATTERN = "[%n] [%^%l%$] %v";

/**
 * @brief Centralized logger for the DoIP library
 */
class Logger {
public:
    static std::shared_ptr<spdlog::logger> get() {
        static std::once_flag once_flag;
        std::call_once(once_flag, []() {
            m_log = spdlog::stdout_color_mt("doip");
            m_log->set_level(spdlog::level::info);
            m_log->set_pattern(DEFAULT_PATTERN);
        });
        return m_log;
    }

    static std::shared_ptr<spdlog::logger> getUdp() {
        static std::once_flag once_flag;
        std::call_once(once_flag, []() {
            m_udpLog = spdlog::stdout_color_mt("udp ");
            m_udpLog->set_level(spdlog::level::info);
            m_udpLog->set_pattern(DEFAULT_PATTERN);
        });
        return m_udpLog;
    }

    static std::shared_ptr<spdlog::logger> getTcp() {
        static std::once_flag once_flag;
        std::call_once(once_flag, []() {
            m_tcpLog = spdlog::stdout_color_mt("tcp ");
            m_tcpLog->set_level(spdlog::level::info);
            m_tcpLog->set_pattern(DEFAULT_PATTERN);
        });
        return m_tcpLog;
    }

    static void setLevel(spdlog::level::level_enum level) {
        get()->set_level(level);
    }

    static void setUdpLevel(spdlog::level::level_enum level) {
        getUdp()->set_level(level);
    }

    static void setTcpLevel(spdlog::level::level_enum level) {
        getUdp()->set_level(level);
    }

    static void setPattern(const std::string& pattern) {
        get()->set_pattern(pattern);
    }

    static bool colorsSupported() {
        const char* term = std::getenv("TERM");
        const char* colorterm = std::getenv("COLORTERM");

        if (term == nullptr) return false;

        std::string termStr(term);
        return termStr.find("color") != std::string::npos ||
               termStr.find("xterm") != std::string::npos ||
               termStr.find("screen") != std::string::npos ||
               colorterm != nullptr;
    }

private:
    static std::shared_ptr<spdlog::logger> m_log;
    static std::shared_ptr<spdlog::logger> m_udpLog;
    static std::shared_ptr<spdlog::logger> m_tcpLog;
};

}

// Logging macros
#define DOIP_LOG_TRACE(...)    doip::Logger::get()->trace(__VA_ARGS__)
#define DOIP_LOG_DEBUG(...)    doip::Logger::get()->debug(__VA_ARGS__)
#define DOIP_LOG_INFO(...)     doip::Logger::get()->info(__VA_ARGS__)
#define DOIP_LOG_WARN(...)     doip::Logger::get()->warn(__VA_ARGS__)
#define DOIP_LOG_ERROR(...)    doip::Logger::get()->error(__VA_ARGS__)
#define DOIP_LOG_CRITICAL(...) doip::Logger::get()->critical(__VA_ARGS__)

// Logging macros for UDP socket
#define UDP_LOG_TRACE(...)    doip::Logger::getUdp()->trace(__VA_ARGS__)
#define UDP_LOG_DEBUG(...)    doip::Logger::getUdp()->debug(__VA_ARGS__)
#define UDP_LOG_INFO(...)     doip::Logger::getUdp()->info(__VA_ARGS__)
#define UDP_LOG_WARN(...)     doip::Logger::getUdp()->warn(__VA_ARGS__)
#define UDP_LOG_ERROR(...)    doip::Logger::getUdp()->error(__VA_ARGS__)
#define UDP_LOG_CRITICAL(...) doip::Logger::getUdp()->critical(__VA_ARGS__)

// Logging macros for TCP socket
#define TCP_LOG_TRACE(...)    doip::Logger::getTcp()->trace(__VA_ARGS__)
#define TCP_LOG_DEBUG(...)    doip::Logger::getTcp()->debug(__VA_ARGS__)
#define TCP_LOG_INFO(...)     doip::Logger::getTcp()->info(__VA_ARGS__)
#define TCP_LOG_WARN(...)     doip::Logger::getTcp()->warn(__VA_ARGS__)
#define TCP_LOG_ERROR(...)    doip::Logger::getTcp()->error(__VA_ARGS__)
#define TCP_LOG_CRITICAL(...) doip::Logger::getTcp()->critical(__VA_ARGS__)

// Colored logging macros
#define DOIP_LOG_SUCCESS(...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_green) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

#define DOIP_LOG_ERROR_COLORED(...) \
    doip::Logger::get()->error(std::string(doip::ansi::bold_red) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

#define DOIP_LOG_PROTOCOL(...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_blue) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

#define DOIP_LOG_CONNECTION(...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_magenta) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

#define DOIP_LOG_HIGHLIGHT(...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_cyan) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

// Convenience macros for types with stream operators (using fmt::streamed)
// These automatically wrap arguments with fmt::streamed() for seamless logging of DoIP types
#define DOIP_LOG_STREAM_INFO(obj, ...)     DOIP_LOG_INFO(fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)))
#define DOIP_LOG_STREAM_DEBUG(obj, ...)    DOIP_LOG_DEBUG(fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)))
#define DOIP_LOG_STREAM_WARN(obj, ...)     DOIP_LOG_WARN(fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)))
#define DOIP_LOG_STREAM_ERROR(obj, ...)    DOIP_LOG_ERROR(fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)))

// Colored stream logging macros for DoIP types
#define DOIP_LOG_STREAM_SUCCESS(obj, ...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_green) + fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)) + doip::ansi::reset)

#define DOIP_LOG_STREAM_PROTOCOL(obj, ...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_blue) + fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)) + doip::ansi::reset)

#define DOIP_LOG_STREAM_CONNECTION(obj, ...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_magenta) + fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)) + doip::ansi::reset)
