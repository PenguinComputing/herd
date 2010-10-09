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

#ifndef OPTIONS_H
#define OPTIONS_H

#include "mce.h"
#include "set.h"

typedef struct {

    int debug_mode;
    int nosyslog_mode;
    int verbose_mode;
    int no_daemon_flag;
    int ignore_no_dev;
    int filter_bogus;
    int dmi_decode;
    
    int decode_mode;
    u64 decode_address;
    u32 decode_syndrome;

    int seltest_mode;
    u64 seltest_address;

    int singleiter_mode;

    int no_serd_filter;

    const char* i_user;
    const char* i_password;
    const char* i_ip;

    Set* parameters;

} Options;

extern Options options;

void options_process(int argc, char** argv);

void options_usage();

void options_free();

// Return string parameter. Either the default value (see options.c),
// or the value overridden by the --setparam option.
const char* options_get_string(const char* key);

// Return an integer parameter. Returns INT_MIN if key was not found.
int         options_get_int(const char* key);


#endif
