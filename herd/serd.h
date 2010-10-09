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

#ifndef SERD_H
#define SERD_H

#ifndef WIN32
#include <asm/ioctls.h>
#include <asm/types.h>
#endif /* WIN32 */

#include "mce.h"
#include "dimm.h"


#define SERD_N     24
#define SERD_T     24*60*60     // seconds


int serd_filter(struct mce* mce);


struct tentry_s {
    unsigned long int lhs;
    void *rhs;
    struct tentry_s *next;
};


struct serd_eng_s {
    u32  id;                    // CPU/DIMM
    unsigned int N;
    unsigned long int T;
    unsigned long int *tobs;    // array of length N
    decode_input_t *addr_syndr; // array of length N
    int current;
};


#endif
