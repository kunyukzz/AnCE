#pragma once

#include "define.h"

#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1

// Disabling debug and trace logging for release builds
#if ACRELEASE == 1
    #define LOG_DEBUG_ENABLED 0
    #define LOG_TRACE_ENABLED 0
#endif //ACRELEASE == 1

typedef enum
{
    LOG_TYPE_FATAL = 0,
    LOG_TYPE_ERROR = 1,
    LOG_TYPE_WARN = 2,
    LOG_TYPE_INFO = 3,
    LOG_TYPE_DEBUG = 4,
    LOG_TYPE_TRACE = 5,
} log_type;

b8 init_log();
void shutdown_log();

ACAPI void log_output(log_type type, const char* message, ...);

#define ACFATAL(message, ...) log_output(LOG_TYPE_FATAL, message, ##__VA_ARGS__);

#ifndef ACERROR
    #define ACERROR(message, ...) log_output(LOG_TYPE_ERROR, message, ##__VA_ARGS__);
#endif // !ACERROR

#if LOG_WARN_ENABLED == 1
    #define ACWARN(message, ...) log_output(LOG_TYPE_WARN, message, ##__VA_ARGS__);
#else
    #define ACWARN(message, ...)
#endif

#if LOG_INFO_ENABLED == 1
    #define ACINFO(message, ...) log_output(LOG_TYPE_INFO, message, ##__VA_ARGS__);
#else
    #define ACACINFO(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
    #define ACDEBUG(message, ...) log_output(LOG_TYPE_DEBUG, message, ##__VA_ARGS__);
#else
    #define ACDACDEBUG(message, ...)
#endif

#if LOG_TRACE_ENABLED == 1
    #define ACTRACE(message, ...) log_output(LOG_TYPE_TRACE, message, ##__VA_ARGS__);
#else
    #define ACTACTRACE(message, ...)
#endif

