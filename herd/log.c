/*
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * Copyright 2007-2009 Sun Microsystems, Inc. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the only version 2 of the GNU General
 * Public License, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included in the COPYING file that accompanied this file).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include "log.h"
#include "options.h"

#define HERD_MAX_LOG_LENGTH 256

void log_init()
{
    if (!options.nosyslog_mode)
	openlog("herd", 0, LOG_DAEMON);
}

void log_exit()
{
    closelog();
}

void log_dolog(const char* fmt, va_list args)
{
    if (options.nosyslog_mode)
	vfprintf(stderr, fmt, args);
    else
	vsyslog(LOG_NOTICE, fmt, args);
}

void log_dolog_errno(const char* fmt, va_list args)
{
    static char buf[HERD_MAX_LOG_LENGTH];

    int n = vsnprintf(buf, HERD_MAX_LOG_LENGTH, fmt, args);
    if (buf[--n] == '\n')
	buf[n] = 0;
    perror(buf);
}

void log_fatal(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_dolog(fmt, args);
    va_end(args);
    exit(1);
}

void log_fatal_errno(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_dolog_errno(fmt, args);
    va_end(args);
    exit(1);
}

void log_errno(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_dolog_errno(fmt, args);
    va_end(args);
}

void log_msg(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_dolog(fmt, args);
    va_end(args);
}

