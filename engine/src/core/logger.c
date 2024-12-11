#include "logger.h"
#include "assertion.h"
#include "platform/platform.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void report_assert_failure(const char* expression, const char* msg, const char* file, i32 line)
{
    log_output(LOG_TYPE_FATAL, "Assertion Failure: %s, message: '%s', on file: %s, line: %d\n", expression, msg, file, line);
}

b8 init_log()
{
    return TRUE;
}

void shutdown_log() {}

void log_output(log_type type, const char* message, ...)
{
    const char* level_strings[6] = { "[FATAL]: ", "[ERROR]: ", "[WARN]: ", "[INFO]: ", "[DEBUG]: ", "[TRACE]: " };
    b8 is_error = type < LOG_TYPE_WARN;

    const i16 length = 16000;
    char out_message[length];
    memset(out_message, 0, sizeof(out_message));

    // Create formatted message in string
    __builtin_va_list arg_ptr;
    va_start(arg_ptr, message);
    vsnprintf(out_message, length, message, arg_ptr);
    va_end(arg_ptr);

    char out_message2[length];
    sprintf(out_message2, "%s%s\n", level_strings[type], out_message);

    if (is_error)
    {
        platform_console_write_error(out_message2, type);
    }
    else
    {
        platform_console_write(out_message2, type);
    }
}
