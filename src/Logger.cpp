#include "Logger.h"

namespace doip {

// Static member definition
std::shared_ptr<spdlog::logger> Logger::m_log;
std::shared_ptr<spdlog::logger> Logger::m_udpLog;
std::shared_ptr<spdlog::logger> Logger::m_tcpLog;

} // namespace doip