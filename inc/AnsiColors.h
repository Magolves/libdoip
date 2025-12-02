#pragma once

namespace doip {
namespace ansi {

// ANSI Color Reset
constexpr const char* reset = "\033[0m";

// Standard Colors
constexpr const char* black   = "\033[30m";
constexpr const char* red     = "\033[31m";
constexpr const char* green   = "\033[32m";
constexpr const char* yellow  = "\033[33m";
constexpr const char* blue    = "\033[34m";
constexpr const char* magenta = "\033[35m";
constexpr const char* cyan    = "\033[36m";
constexpr const char* white   = "\033[37m";

// Bold/Bright Colors
constexpr const char* bold_black   = "\033[1;30m";
constexpr const char* bold_red     = "\033[1;31m";
constexpr const char* bold_green   = "\033[1;32m";
constexpr const char* bold_yellow  = "\033[1;33m";
constexpr const char* bold_blue    = "\033[1;34m";
constexpr const char* bold_magenta = "\033[1;35m";
constexpr const char* bold_cyan    = "\033[1;36m";
constexpr const char* bold_white   = "\033[1;37m";

// Background Colors
constexpr const char* bg_black   = "\033[40m";
constexpr const char* bg_red     = "\033[41m";
constexpr const char* bg_green   = "\033[42m";
constexpr const char* bg_yellow  = "\033[43m";
constexpr const char* bg_blue    = "\033[44m";
constexpr const char* bg_magenta = "\033[45m";
constexpr const char* bg_cyan    = "\033[46m";
constexpr const char* bg_white   = "\033[47m";

// Text Styles
constexpr const char* bold      = "\033[1m";
constexpr const char* dim       = "\033[2m";
constexpr const char* italic    = "\033[3m";
constexpr const char* underline = "\033[4m";
constexpr const char* blink     = "\033[5m";
constexpr const char* reverse   = "\033[7m";
constexpr const char* strikethrough = "\033[9m";

} // namespace ansi
} // namespace doip

// Backward compatibility macros (deprecated - use doip::ansi:: instead)
#define DOIP_COLOR_RESET     doip::ansi::reset
#define DOIP_COLOR_RED       doip::ansi::red
#define DOIP_COLOR_GREEN     doip::ansi::green
#define DOIP_COLOR_YELLOW    doip::ansi::yellow
#define DOIP_COLOR_BLUE      doip::ansi::blue
#define DOIP_COLOR_MAGENTA   doip::ansi::magenta
#define DOIP_COLOR_CYAN      doip::ansi::cyan
#define DOIP_COLOR_BOLD_RED     doip::ansi::bold_red
#define DOIP_COLOR_BOLD_GREEN   doip::ansi::bold_green
#define DOIP_COLOR_BOLD_YELLOW  doip::ansi::bold_yellow
#define DOIP_COLOR_BOLD_BLUE    doip::ansi::bold_blue
#define DOIP_COLOR_BOLD_MAGENTA doip::ansi::bold_magenta
#define DOIP_COLOR_BOLD_CYAN    doip::ansi::bold_cyan
