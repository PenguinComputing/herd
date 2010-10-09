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

#ifndef DIMM_H
#define DIMM_H

#include "mce.h"

typedef struct {

    // CPU Node that triggered the error
    int node;
    // DIMM or DIMM pair number (0 being first)
    int dimm;
    // If set, DIMM number above is a dimm pair
    int is_dimm_pair;
    // DRAM controller number, currently unused
    int channel;
    // Affected bits, if any. Shown as a bit mask, where a bit set to
    // 1 indicates the corresponding bit caused the error.
    u64 bits;

} decode_result_t;

typedef struct {

    // Physical address that triggered the MCE */
    u64 address;

    // ECC Syndrome, or 0 if none
    u32 syndrome;

} decode_input_t;

int dimm_translate_address(decode_input_t* input, decode_result_t* result);

#endif
