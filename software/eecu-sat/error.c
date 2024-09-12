/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2018.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU Lesser General Public License as published   *
* by the Free Software Foundation, either version 3 or (at your option)   *
* any later version. This program is distributed without any warranty.    *
* See the files COPYING.lgpl-v3 and COPYING.gpl-v3 for details.           *
\*************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "ename.c.inc"
#include "error.h"

   /* Diagnose 'errno' error by:
    *  * outputting a string containing the error name (if available
    *    in 'ename' array) corresponding to the value in 'err', along
    *    with the corresponding error message from strerror(), and
    *  * outputting the caller-supplied error message specified in
    *    'format' and 'ap'.
    */
static void output_error(bool use_err, int err, bool flush_stdout, const char *format, va_list ap)
{
#define BUF_SIZE 500
    char buf[BUF_SIZE], user_msg[BUF_SIZE], err_text[BUF_SIZE];

    vsnprintf(user_msg, BUF_SIZE, format, ap);

    if (use_err)
        snprintf(err_text, BUF_SIZE, " [%s %s]",
                 (err > 0 && err <= MAX_ENAME) ? ename[err] : "?UNKNOWN?", strerror(err));
    else
        snprintf(err_text, BUF_SIZE, ":");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    snprintf(buf, BUF_SIZE, "ERROR%s %s\n", err_text, user_msg);
#pragma GCC diagnostic pop

    if (flush_stdout)
        fflush(stdout);         /* Flush any pending stdout */
    fputs(buf, stderr);
    fflush(stderr);             /* In case stderr is not line-buffered */
}

void err_msg(const char *format, ...)
{
    va_list arg_list;
    int saved_errno;
    bool show_errno = false;

    saved_errno = errno;        /* In case we change it here */

    if (saved_errno)
        show_errno = true;

    va_start(arg_list, format);
    output_error(show_errno, errno, true, format, arg_list);
    va_end(arg_list);

    errno = saved_errno;
}

