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
#include <string.h>

#ifndef WIN32
#include <sys/time.h>
#endif /* WIN32 */

#include "mce.h"
#include "serd.h"
#include "dimm.h"
#include "pci.h"
#include "options.h"
#include "log.h"

#ifdef WIN32
////////////////////////WIN32 only/////////////////////////////
struct timeval {
	long int tv_sec;
	long int tv_usec;
};
int gettimeofday(struct timeval *tv, void *restrict);

#undef SERD_N
#undef SERD_T

#include "../../windows/sbemon/decode.h"

extern WinOptions wOptions;

#define SERD_N wOptions.serd_n
#define SERD_T wOptions.serd_t

//Use a DotNet friendly malloc
void w_free(void *Ptr);
void * w_malloc(size_t Size);
#define malloc(x) w_malloc(x)
#define free(x) w_free(x)
//////////////////////////////////////////////////////////////
#endif /* WIN32 */

static struct tentry_s *sroot = NULL;

static int
serd_add(struct tentry_s **root, unsigned long int lhs, void *rhs)
{
    struct tentry_s *tmp;

    for (tmp = *root; tmp != NULL; tmp = tmp->next) {
        if (tmp->lhs == lhs) {
            /* already in tree, replace node */
            tmp->rhs = rhs;

            return (0);
        }
    }

    /* either empty tree or not in tree, so create new node */
    tmp = (struct tentry_s *)malloc(sizeof(struct tentry_s));
    tmp->lhs = lhs;
    tmp->rhs = rhs;

    if (*root == NULL)
        tmp->next = NULL;
    else
        tmp->next = *root;

    *root = tmp;

    return (0);
}


static void *
serd_lookup(struct tentry_s *root, unsigned long int lhs)
{
    struct tentry_s *lp;

    for (lp = root; lp != NULL; lp = lp->next) {
        if (lp->lhs == lhs)
            return (lp->rhs);
    }

    return (NULL);
}

static int
serd_free(struct tentry_s **root, unsigned long int lhs)
{
    struct tentry_s *lp, *prev;

    prev = NULL;
    for (lp = *root; lp != NULL; lp = lp->next) {
        if (lp->lhs == lhs) {
            if (prev == NULL)
                *root = lp->next;
            else
                prev->next = lp->next;

            free(lp);

            return (0);
        }

        prev = lp;
    }

    return (1);
}

static void
free_serdstruct(void *addr)
{
    free(((struct serd_eng_s *)addr)->tobs);
    free(((struct serd_eng_s *)addr)->addr_syndr);
    free(addr);
}

int serd_filter(struct mce* mce)
{
    u32 dimm_id;  // Unique DIMM ID
    struct serd_eng_s *sp;
    unsigned long int tdiff;
    int oldest, i, ier;
    unsigned long int curr_time;
    struct timeval start;
	decode_input_t dinput;
	decode_result_t result;

    // Can only decode node and dimm for bank 4, so filter otherwise
    if (mce->bank != 4) {
        return (1);
    }

    // Get corresponding CPU + DIMM
	dinput.address = mce->addr;
	// If it's a ChipKill ECC error, set syndrome value
	if (((mce->status >> 16) & 0x0f) == 0x08) {
	    dinput.syndrome = (u32)((((mce->status >> 24) & 0xff) << 8) |
				    ((mce->status >> 47) & 0xff));
	} else {
	    dinput.syndrome = 0;
	}

	if (dimm_translate_address(&dinput, &result) == 0) {
        // Unable to map physical address to specific DIMM
        log_msg("serd unable to map physical address to DIMM\n");
        return (1);
    }
    
    gettimeofday(&start, 0);
    curr_time = start.tv_sec;

    // Retrieve record corresponding to CPU/DIMM
    dimm_id = (u32)((result.node << 16) | result.dimm);

    sp = (struct serd_eng_s *)serd_lookup(sroot, dimm_id);

    if (sp == NULL) {
        /*
         * Create new SERD entry
         */
        sp = (struct serd_eng_s *)malloc(sizeof (struct serd_eng_s));
        sp->id = dimm_id;

        sp->N = (unsigned int) SERD_N;
        sp->T = (unsigned long int) SERD_T;

        sp->current = sp->N - 1;

        sp->tobs = (unsigned long *)malloc(sizeof (unsigned long int) * sp->N);

        // Allocate storage to store syndrome for each occurence
        sp->addr_syndr = (decode_input_t *)malloc(sizeof (decode_input_t) * sp->N);

        for (i = 0; i < sp->N; i++) {
            sp->tobs[i] = 0;
            sp->addr_syndr[i].address = 0;
            sp->addr_syndr[i].syndrome = 0;
        }
        
        ier = serd_add(&sroot, sp->id, sp);
        
        if (ier != 0) {
            free(sp->tobs);
            free(sp);

            return (1);
        }
    }
    
    // Recurring CEs from the same address with the same syndrome should 
    // not be counted more than once in a given evaluation period (T). 
    for (i = 0; i < sp->N; i++) {
        if ((sp->addr_syndr[i].address == dinput.address) &&
            (sp->addr_syndr[i].syndrome == dinput.syndrome)) {
            // Found matching address/syndrome
            // Update the time to this occurence
            sp->tobs[sp->current] = curr_time;
            return (1);

        }
    }

    // Check for N occurences in T time. Return 0 if threshold 
    // reached, otherwise return 1.

    oldest = sp->current + 1;

    // Wrap when more than N occurences
    if (oldest >= sp->N)
    {
        oldest = 0;
    }

    // tdiff will be set to curr_time the first time
    tdiff = curr_time - sp->tobs[oldest];

    if (sp->tobs[oldest] > 0 && tdiff <= sp->T) {
        /*
         * SERD engine is firing, return 0.  
         * Reset the time array.
         */
        sp->current = sp->N - 1;
        for (i = 0; i < sp->N; i++)
        {
            sp->tobs[i] = 0;
            sp->addr_syndr[i].address = 0;
            sp->addr_syndr[i].syndrome = 0;
        }

        // SERD engine has fired!
        return (0);

    } else {
        // Store current time, incremented counter, and address/syndrome
        sp->current = oldest;
        sp->tobs[sp->current] = curr_time;
        sp->addr_syndr[sp->current].address = dinput.address;
        sp->addr_syndr[sp->current].syndrome = dinput.syndrome;
    }

    return (1);

}

