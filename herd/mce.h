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

#ifndef MCE_H
#define MCE_H

#ifndef WIN32
#include <asm/ioctls.h>
#include <asm/types.h>
#endif /* WIN32 */

typedef unsigned long long u64;
typedef unsigned int       u32;
typedef unsigned short     u16;
typedef unsigned char      u8;

void mce_init();
void mce_exit();
void mce_process_log();

#define M_K8_BANK_DC 0
#define M_K8_BANK_IC 1
#define M_K8_BANK_BU 2
#define M_K8_BANK_LS 3
#define M_K8_BANK_NB 4

#define M_EXTENDED_BANK      128
#define M_THRESHOLD_BASE     (M_EXTENDED_BANK + 1)
#define M_THRESHOLD_DRAM_ECC (M_THRESHOLD_BASE + 4)
#define M_THRESHOLD_OVER     (1ULL << 48)

#define M_STATUS_UC   (1ULL << 61)
#define M_STATUS_EIPV (1ULL << 1)

#define M_GET_RECORD_LEN _IOR('M', 1, int)
#define M_GET_LOG_LEN    _IOR('M', 2, int)

struct mce {
    u64 status;
    u64 misc;
    u64 addr;
    u64 mcgstatus;
    u64 rip;
    u64 tsc;
    u64 res1;
    u64 res2;
    u8  cs;
    u8  bank;
    u8  cpu;
    u8  finished;
    u32 pad;
};

typedef struct {
    unsigned ErrorCode :   16;
    unsigned ErrorCodeExt : 4;
    unsigned res5 :         4;
    unsigned Syndrome :     8;
    unsigned ErrCPU0 :      1;
    unsigned ErrCPU1 :      1;
    unsigned res4 :         2;
    unsigned LDTLink :      3;
    unsigned res3 :         1;
    unsigned ErrScrub :     1;
    unsigned res2 :         4;
    unsigned UnCorrECC :    1;
    unsigned CorrECC :      1;
    unsigned ECC_Synd :     8;
    unsigned res1 :         2;
    unsigned PCC :          1;
    unsigned ErrAddrVal :   1;
    unsigned ErrMiscVal :   1;
    unsigned ErrEn :        1;
    unsigned ErrUnCorr :    1;
    unsigned ErrOver :      1;
    unsigned ErrValid :     1;
} McaNbStatus;


#endif
