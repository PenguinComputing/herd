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

#ifndef OPTERON_H
#define OPTERON_H

#include "cpu.h"
#include "set.h"

typedef struct opteron_info {

    Cpu cpu;

    Set* syndromes;

    int family;
    int model;
    int stepping;

    char revision;
    const char* description;
} OpteronInfo;


#define AMD_QUAD_CORE_FAMILY 0x1f

// Create Opteron information by inspecting host CPU.
OpteronInfo* opteron_new();

// Create Opteron information from specified <family, model, stepping>
// triplet.
OpteronInfo* opteron_new_from_fms(int f, int m, int s);

void opteron_delete(OpteronInfo*);

int opteron_get_ck_syndrome(Cpu* cpu, u32 syndrome, int* symbol, int* bit);

#endif
