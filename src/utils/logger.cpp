#include "../../include/utils/logger.hpp"

#include <log4cplus/consoleappender.h>
#include <log4cplus/configurator.h>
#include <log4cplus/loggingmacros.h>

namespace messenger::log {

namespace {
log4cplus::Logger &logger() {
  static log4cplus::Logger instance =
      log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("messenger"));
  return instance;
}

bool initialized = false;
} // namespace

void init() {
  if (initialized)
    return;

  static log4cplus::SharedAppenderPtr appender(
      new log4cplus::ConsoleAppender());
  appender->setName(LOG4CPLUS_TEXT("console"));
  appender->setLayout(std::unique_ptr<log4cplus::Layout>(
      new log4cplus::PatternLayout(LOG4CPLUS_TEXT("[%D{%H:%M:%S}] %-5p %m%n"))));

  logger().addAppender(appender);
  logger().setLogLevel(log4cplus::DEBUG_LOG_LEVEL);
  initialized = true;
}

void info(std::string_view message) {
  LOG4CPLUS_INFO(logger(), log4cplus::tstring(message.begin(), message.end()));
}

void warn(std::string_view message) {
  LOG4CPLUS_WARN(logger(), log4cplus::tstring(message.begin(), message.end()));
}

void error(std::string_view message) {
  LOG4CPLUS_ERROR(logger(), log4cplus::tstring(message.begin(), message.end()));
}

void debug(std::string_view message) {
  LOG4CPLUS_DEBUG(logger(), log4cplus::tstring(message.begin(), message.end()));
}

} // namespace messenger::log
