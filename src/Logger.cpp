#include "Logger.h"

namespace doip {


// Static member definition
std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> Logger::m_loggers;

} // namespace doip