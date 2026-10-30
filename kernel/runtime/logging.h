/**
 * @file logging.h
 * @author Charles Averill
 * @brief Function headers, ANSI defines, and enums for logging and raising fatal errors
 */

#ifndef GRACE_LOGGING_H
#define GRACE_LOGGING_H

#include <stdarg.h>

#define ANSI_BOLD   "\033[1m"
#define ANSI_RED    "\033[38;5;196m"
#define ANSI_ORANGE "\033[38;5;208m"
#define ANSI_YELLOW "\033[38;5;178m"
#define ANSI_RESET  "\033[0m"

#define ERROR_RED    ANSI_RED ANSI_BOLD
#define ERROR_ORANGE ANSI_ORANGE ANSI_BOLD
#define ERROR_YELLOW ANSI_YELLOW ANSI_BOLD

/**
 * @brief Severity levels of logging statements. CakeML's Log.int_of_log
 * must match this order.
 */
typedef enum {
    LOG_NONE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_CRITICAL
} LogLevel;

typedef struct {
    LogLevel level;
    const char* name;
    const char* color;
} LogInfo;

extern const LogInfo logInfoLevels[];

/**
 * @brief Return codes used in different scenarios. CakeML's Log.int_of_rc
 * must match this order.
 */
typedef enum {
    RC_OK,
    RC_ERROR,
    RC_MEMORY_ERROR,
    RC_INIT_ERROR,
    RC_END
} ReturnCode;

/**
 * @brief String representation of return codes
 */
extern const char* returnCodeStrings[];

/**
 * @brief Statements less severe than this are not printed
 */
extern LogLevel global_log_level;

__attribute__((noreturn, format(printf, 2, 3))) void fatal(ReturnCode rc, const char* fmt, ...);

__attribute__((format(printf, 2, 3))) void log(LogLevel level, const char* fmt, ...);
__attribute__((format(printf, 2, 0))) void vlog(LogLevel level, const char* fmt, va_list args);

// Shorthands for log at each level
__attribute__((format(printf, 1, 2))) void debug(const char* fmt, ...);
__attribute__((format(printf, 1, 2))) void info(const char* fmt, ...);
__attribute__((format(printf, 1, 2))) void warn(const char* fmt, ...);
__attribute__((format(printf, 1, 2))) void error(const char* fmt, ...);
__attribute__((format(printf, 1, 2))) void critical(const char* fmt, ...);

#endif /* GRACE_LOGGING_H */
