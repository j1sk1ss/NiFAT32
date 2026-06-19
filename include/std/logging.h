/*
*  License:
*  Copyright (C) 2024 Nikolaj Fot
*
*  This program is free software: you can redistribute it and/or modify it under the terms of 
*  the GNU General Public License as published by the Free Software Foundation, version 3.
*  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
*  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
*  See the GNU General Public License for more details.
*  You should have received a copy of the GNU General Public License along with this program. 
*  If not, see https://www.gnu.org/licenses/.
* 
*  CordellDBMS source code: https://github.com/j1sk1ss/CordellDBMS.EXMPL
*  Credits: j1sk1ss
*/

#ifndef LOGGING_H_
#define LOGGING_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <std/null.h>
#include <stdarg.h>

typedef struct {
    int (*fd_fprintf)(const char*, ...);
    int (*fd_vfprintf)(const char*, va_list);
} log_io_t;

#ifdef ERROR_LOGS
    #define print_error(message, ...)   LOG_emmit_message("ERROR", __FILE__, __LINE__, message, ##__VA_ARGS__)
#else
    #define print_error(message, ...)   (void)0
#endif

#ifdef WARNING_LOGS
    #define print_warn(message, ...)    LOG_emmit_message("WARN", __FILE__, __LINE__, message, ##__VA_ARGS__)
#else
    #define print_warn(message, ...)    (void)0
#endif

#ifdef INFO_LOGS
    #define print_info(message, ...)    LOG_emmit_message("INFO", __FILE__, __LINE__, message, ##__VA_ARGS__)
#else
    #define print_info(message, ...)    (void)0
#endif

#ifdef DEBUG_LOGS
    #define print_debug(message, ...)   LOG_emmit_message("DEBUG", __FILE__, __LINE__, message, ##__VA_ARGS__)
#else
    #define print_debug(message, ...)   (void)0
#endif

#ifdef IO_OPERATION_LOGS
    #define print_io(message, ...)      LOG_emmit_message("I/O", __FILE__, __LINE__, message, ##__VA_ARGS__)
#else
    #define print_io(message, ...)      (void)0
#endif

#ifdef MEM_OPERATION_LOGS
    #define print_mm(message, ...)      LOG_emmit_message("MEM", __FILE__, __LINE__, message, ##__VA_ARGS__)
#else
    #define print_mm(message, ...)      (void)0
#endif

#ifdef LOGGING_LOGS
    #define print_log(message, ...)     LOG_emmit_message("LOG", __FILE__, __LINE__, message, ##__VA_ARGS__)
#else
    #define print_log(message, ...)     (void)0
#endif

#ifdef SPECIAL_LOGS
    #define print_spec(message, ...)    LOG_emmit_message("SPEC", __FILE__, __LINE__, message, ##__VA_ARGS__)
#else
    #define print_spec(message, ...)    (void)0
#endif

#if !defined(ERROR_LOGS)        && !defined(WARNING_LOGS) &&       \
    !defined(INFO_LOGS)         && !defined(DEBUG_LOGS) &&         \
    !defined(IO_OPERATION_LOGS) && !defined(MEM_OPERATION_LOGS) && \
    !defined(LOGGING_LOGS)      && !defined(SPECIAL_LOGS)
    #define NO_LOGGING
#endif 

/*
Logging init.
Params:
    - `fd_fprintf` - fprintf function in your platform. Can be 'NULL' (Will disable all logging).
    - `fd_vfprintf` - vfprintf function in your platform. Can be 'NULL'.
*/
void LOG_setup(
    int (*fd_fprintf)(const char*, ...),
    int (*fd_vfprintf)(const char*, va_list)
);

/*
Create log message.
Params:
    - `level` - Log level.
    - `file` - File name.
    - `line` - Code line number.
    - `message` - Additional info message.
*/
void LOG_emmit_message(const char* level, const char* file, int line, const char* message, ...);

#ifdef __cplusplus
}
#endif
#endif