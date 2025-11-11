#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/fmt.h>
#include <memory>
#include <mutex>
#include <cstdlib>
#include <string>
#include "AnsiColors.h"

namespace doip {

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
            m_log->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
        });
        return m_log;
    }

    static void setLevel(spdlog::level::level_enum level) {
        get()->set_level(level);
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
};

}

// Logging macros
#define DOIP_LOG_TRACE(...)    doip::Logger::get()->trace(__VA_ARGS__)
#define DOIP_LOG_DEBUG(...)    doip::Logger::get()->debug(__VA_ARGS__)
#define DOIP_LOG_INFO(...)     doip::Logger::get()->info(__VA_ARGS__)
#define DOIP_LOG_WARN(...)     doip::Logger::get()->warn(__VA_ARGS__)
#define DOIP_LOG_ERROR(...)    doip::Logger::get()->error(__VA_ARGS__)
#define DOIP_LOG_CRITICAL(...) doip::Logger::get()->critical(__VA_ARGS__)

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
