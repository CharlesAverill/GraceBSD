/**
 * @file logging.c
 * @author Charles Averill
 * @brief Logging statements and fatal error handling, written to COM1
 */

#include "logging.h"

#include <stdarg.h>

#include "hw.h"
#include "serial.h"

const LogInfo logInfoLevels[] = {
    {LOG_NONE,      "",        ANSI_RESET           },
    { LOG_DEBUG,    "[DEBUG]", ANSI_BOLD            },
    { LOG_INFO,     "[INFO] ", ANSI_BOLD            },
    { LOG_WARNING,  "[WARN] ", ANSI_BOLD ANSI_YELLOW},
    { LOG_ERROR,    "[ERROR]", ANSI_BOLD ANSI_ORANGE},
    { LOG_CRITICAL, "[CRIT] ", ANSI_BOLD ANSI_RED   }
};

const char* returnCodeStrings[] = { "OK", "ERROR", "MEMORY ERROR", "INITIALIZATION ERROR", "" };

LogLevel global_log_level = LOG_DEBUG;

/**
 * @brief Raises a fatal error that halts the machine
 *
 * @param rc Return code to report
 * @param fmt Format string for printed error
 * @param ... Varargs for printed error
 */
void fatal(ReturnCode rc, const char* fmt, ...) {
    va_list func_args;

    const char* rc_name = rc < RC_END ? returnCodeStrings[rc] : "UNKNOWN";

    va_start(func_args, fmt);
    serial_printf(ERROR_RED "[%s] " ANSI_RESET, rc_name);
    serial_vprintf(fmt, func_args);
    serial_printf("\n");
    va_end(func_args);

    hcf();
}

/**
 * @brief Raises a non-fatal logging statement
 *
 * @param level Severity of statement
 * @param fmt Format string for printed statement
 * @param ... Varargs for printed statement
 */
void log(LogLevel level, const char* fmt, ...) {
    va_list func_args;

    va_start(func_args, fmt);
    vlog(level, fmt, func_args);
    va_end(func_args);
}

/**
 * @brief log, taking its varargs as a va_list
 *
 * @param level Severity of statement
 * @param fmt Format string for printed statement
 * @param args Varargs for printed statement
 */
void vlog(LogLevel level, const char* fmt, va_list args) {
    if (global_log_level == LOG_NONE || global_log_level > level || level > LOG_CRITICAL) {
        return;
    }

    serial_printf("%s%s " ANSI_RESET, logInfoLevels[level].color, logInfoLevels[level].name);
    serial_vprintf(fmt, args);
    serial_write("\n");
}

// Define a shorthand for log at one level
#define LOG_SHORTHAND(name, level)                                                                                     \
    void name(const char* fmt, ...) {                                                                                  \
        va_list func_args;                                                                                             \
        va_start(func_args, fmt);                                                                                      \
        vlog(level, fmt, func_args);                                                                                   \
        va_end(func_args);                                                                                             \
    }

LOG_SHORTHAND(debug, LOG_DEBUG)
LOG_SHORTHAND(info, LOG_INFO)
LOG_SHORTHAND(warn, LOG_WARNING)
LOG_SHORTHAND(error, LOG_ERROR)
LOG_SHORTHAND(critical, LOG_CRITICAL)
