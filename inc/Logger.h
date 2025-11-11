#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>

namespace doip {

/**
 * @brief Centralized logger for the DoIP library
 *
 * This class provides a singleton logger instance for consistent logging
 * throughout the DoIP library.
 */
class Logger {
public:
    /**
     * @brief Get the singleton logger instance
     * @return Shared pointer to the logger
     */
    static std::shared_ptr<spdlog::logger> get() {
        static std::once_flag once_flag;
        std::call_once(once_flag, []() {
            instance_ = spdlog::stdout_color_mt("doip");
            instance_->set_level(spdlog::level::info);
            instance_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
        });
        return instance_;
    }

    /**
     * @brief Set the log level for the DoIP logger
     * @param level The log level to set
     */
    static void setLevel(spdlog::level::level_enum level) {
        get()->set_level(level);
    }

    /**
     * @brief Set a custom pattern for log messages
     * @param pattern The pattern string (see spdlog documentation)
     */
    static void setPattern(const std::string& pattern) {
        get()->set_pattern(pattern);
    }

private:
    static std::shared_ptr<spdlog::logger> instance_;
};

} // namespace doip

// Convenience macros for logging
#define DOIP_LOG_TRACE(...)    doip::Logger::get()->trace(__VA_ARGS__)
#define DOIP_LOG_DEBUG(...)    doip::Logger::get()->debug(__VA_ARGS__)
#define DOIP_LOG_INFO(...)     doip::Logger::get()->info(__VA_ARGS__)
#define DOIP_LOG_WARN(...)     doip::Logger::get()->warn(__VA_ARGS__)
#define DOIP_LOG_ERROR(...)    doip::Logger::get()->error(__VA_ARGS__)
#define DOIP_LOG_CRITICAL(...) doip::Logger::get()->critical(__VA_ARGS__)