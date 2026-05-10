#pragma once

#include <string_view>

namespace messenger::log {

void init();

void info(std::string_view message);
void warn(std::string_view message);
void error(std::string_view message);
void debug(std::string_view message);

} // namespace messenger::log

#ifdef MESSENGER_WITH_LOG4CPLUS
#include <log4cplus/logger.h>
#endif
