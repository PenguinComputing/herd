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
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "mce.h"
#include "options.h"
#include "oipmi.h"
#include "log.h"
#include "dimm.h"
#include "dimm_lib.h"
#include "serd.h"

static struct {
    char* device;
    char* mce_log;
    int   fd;
    int   record_len;
    int   log_len;

    // Set to 1 if device file is a regular file, i.e. in debug mode.
    int   is_regular;
} self;

void mce_init()
{
    memset(&self, 0, sizeof(self));
    self.device = "/dev/mcelog";

    struct stat st;
    if (stat(self.device, &st) == 0 &&
	st.st_mode & S_IFREG) {
	self.is_regular = 1;
    }
}

int mce_filter(struct mce* mce)
{
    // Ignore GART errors
    if (mce->bank == M_K8_BANK_NB) {
	unsigned short ecode = (mce->status >> 16) & 0x0f;
	if (ecode == 0x05 && (mce->status & (1ULL << 61)))
	    return 1;
    }
    return 0;
}

const char* mce_bank_name(int id)
{
    static char* optbank[] = {
	"data cache",
	"instruction cache",
	"bus unit",
	"load/store unit",
	"northbridge"
    };
    static char* optthreshold[] = {
	"MC0_MISC threshold",
	"MC1_MISC threshold",
	"MC2_MISC threshold",
	"MC3_MISC threshold",
	"MC4_MISC DRAM errors threshold"
    };

    if (id >= 0 && id < 5)
	return optbank[id];

    if (id >= M_THRESHOLD_BASE && id <= M_THRESHOLD_DRAM_ECC)
	return optthreshold[id - M_THRESHOLD_BASE];

    return "unknown";
}

void mce_decode_generic(u64 status)
{
    static char* transaction[] = { 
	"instruction", "data", "generic", "reserved"
    }; 
    static char* memtrans[] = { 
	"generic error", "generic read", "generic write", "data read",
	"data write", "instruction fetch", "prefetch", "evict", "snoop",
	"?", "?", "?", "?", "?", "?", "?"
    };
    static char* partproc[] = { 
	"local node origin", "local node response", 
	"local node observed", "generic participation"
    };
    static char* cachelevel[] = { "0", "1", "2", "generic" };
    static char* timeout[] = { "request didn't time out","request timed out" };
    static char* memoryio[] = { "memory", "res.", "i/o", "generic" };

    static char* highbits[32] = { 
	[31] = "valid",
	[30] = "error overflow (multiple errors)",
	[29] = "error uncorrected",
	[28] = "error enable",
	[27] = "misc error valid",
	[26] = "error address valid", 
	[25] = "processor context corrupt", 
	[24] = "res24",
	[23] = "res23",
	[14] = "corrected ecc error",
	[13] = "uncorrected ecc error",
	[12] = "res12",
	[11] = "res11",
	[10] = "res10",
	[9] = "res9",
	[8] = "error found by scrub", 
	[7] = "res7",
	[3] = "res3",
	[2] = "res2",
	[1] = "err cpu1",
	[0] = "err cpu0",
    };

    u16 errcode = status & 0xffff;
    int i;

    for (i = 0; i < 32; i++) {
	if (i == 31 || i == 28 || i == 26)
	    continue;
	if (highbits[i] && (status & (1ULL<<(i+32)))) {
	    log_msg( "       bit%d = %s\n", i + 32, highbits[i]);
	}
    }

    if ((errcode & 0xFFF0) == 0x0010) {
	log_msg("  TLB error '%s transaction, level %s'\n",
		transaction[(errcode >> 2) & 3],
		cachelevel[errcode & 3]);

    } else if ((errcode & 0xFF00) == 0x0100) {
	log_msg("  memory/cache error '%s mem transaction, %s transaction, "
		"level %s'\n",
		memtrans[(errcode >> 4) & 0xf],
		transaction[(errcode >> 2) & 3],
		cachelevel[errcode & 3]);

    } else if ((errcode & 0xF800) == 0x0800) {
	log_msg("  bus error '%s, %s\n      %s mem transaction\n      %s "
		"access, level %s'\n",
		partproc[(errcode >> 9) & 0x3],
		timeout[(errcode >> 8) & 1],
		memtrans[(errcode >> 4) & 0xf],
		memoryio[(errcode >> 2) & 0x3],
		cachelevel[(errcode & 0x3)]);
    }
}

void mce_decode_dc(u64 status)
{
    u16 exterrcode = (status >> 16) & 0x0f;
    u16 errcode = status & 0xffff;

    if (status & (3ULL << 45)) {
	log_msg("  Data cache ECC error (syndrome %x)",
		(u32)(status >> 47) & 0xff);
	if (status & (1ULL << 40)) {
	    log_msg(" found by scrubber");
	}
	log_msg("\n");
    }

    if ((errcode & 0xFFF0) == 0x0010) {
	log_msg( "  TLB parity error in %s array\n",
		 (exterrcode == 0) ? "physical" : "virtual");
    }

    mce_decode_generic(status);
}

void mce_decode_ic(u64 status)
{
    u16 exterrcode = (status >> 16) & 0x0f;
    u16 errcode = status & 0xffff;

    if (status & (3ULL << 45)) {
	log_msg("  Instruction cache ECC error\n");
    }

    if ((errcode & 0xFFF0) == 0x0010) {
	log_msg("  TLB parity error in %s array\n",
		(exterrcode == 0) ? "physical" : "virtual");
    }

    mce_decode_generic(status);
}

void mce_decode_bu(u64 status)
{
    u16 exterrcode = (status >> 16) & 0x0f;

    if (status & (3ULL << 45)) {
	log_msg("  L2 cache ECC error\n");
    }

    log_msg("  %s array error\n",
	    (exterrcode == 0) ? "Bus or cache" : "Cache tag");

    mce_decode_generic(status);
}

void mce_decode_nb(u64 status)
{
    static char* extended_errs[] = { 
	"ECC error", "CRC error", "Sync error",	"Master abort",
	"Target abort",	"GART error", "RMW error", "Watchdog error",
	"Chipkill ECC error"
    };

    u16  exterrcode = (status >> 16) & 0x0f;

    if (exterrcode < (sizeof(extended_errs) / sizeof(char*))) {
	log_msg("  Northbridge %s\n", extended_errs[exterrcode]);
    }

    switch (exterrcode) { 
    case 0:
	log_msg("  ECC syndrome = %x\n", (u32)(status >> 47) & 0xff);
	break;
    case 8:	
	log_msg("  Chipkill ECC syndrome = %x\n",
		(u32)((((status >> 24) & 0xff) << 8) |
		      ((status >> 47) & 0xff)));
	break;
    case 1: 
    case 2:
    case 3:
    case 4:
    case 6:
	log_msg("  link number = %x\n", (u32)(status >> 36) & 0x7);
	break;		   
    }

    mce_decode_generic(status);
}

void mce_decode_threshold(u64 misc)
{
    if (misc & M_THRESHOLD_OVER)
	log_msg("  Threshold error count overflow\n");
}

typedef void (*mce_decoder_t)(u64); 

static mce_decoder_t mce_decoders[] = {
    mce_decode_dc,
    mce_decode_ic,
    mce_decode_bu,
    mce_decode_generic,
    mce_decode_nb
}; 


void mce_process_mce(struct mce* mce)
{
    // The textual output is meant to be compatible with that of
    // mcelog.
    log_msg("HARDWARE ERROR. This is *NOT* a software problem!\n");
    log_msg("Please contact your hardware vendor\n");

    if (!mce->finished)
	log_msg("not finished?\n");

    log_msg("CPU %d %d %s\n", mce->cpu, mce->bank, mce_bank_name(mce->bank));

    if (mce->tsc)
	log_msg("TSC %Lx %s\n", mce->tsc, (mce->mcgstatus & M_STATUS_UC) ? 
		"(upper bound, found by polled driver)" : "");

    if (mce->rip) 
	log_msg("RIP%s %02x:%Lx ", 
		!(mce->mcgstatus & M_STATUS_EIPV) ? " !INEXACT!" : "",
		mce->cs, mce->rip);

    if (mce->misc)
	log_msg("MISC %Lx ", mce->misc);

    if (mce->addr)
	log_msg("ADDR %Lx ", mce->addr);

    if (mce->rip | mce->misc | mce->addr)	
	log_msg("\n");

    if (mce->bank < 5)
	mce_decoders[mce->bank](mce->status);
    else if (mce->bank >= M_THRESHOLD_BASE &&
	     mce->bank <= M_THRESHOLD_DRAM_ECC)
	mce_decode_threshold(mce->misc);
    else
	log_msg("  no decoder for unknown bank %u\n", mce->bank);

    log_msg("STATUS %Lx MCGSTATUS %Lx\n", mce->status, mce->mcgstatus);

    if (mce->bank == 4) {
	decode_input_t dinput;
	decode_result_t result;

	dinput.address = mce->addr;
	// If it's a ChipKill ECC error, set syndrome value
	if (((mce->status >> 16) & 0x0f) == 0x08) {
	    dinput.syndrome = (u32)((((mce->status >> 24) & 0xff) << 8) |
				    ((mce->status >> 47) & 0xff));
	} else {
	    dinput.syndrome = 0;
	}

	if (dimm_translate_address(&dinput, &result)) {
	    const char *silkscreen1 = NULL, *silkscreen2 = NULL;

	    if (result.is_dimm_pair) {
		silkscreen1 = dimm_map(result.node, result.dimm * 2);
		silkscreen2 = dimm_map(result.node, (result.dimm * 2) + 1);
	    
		if (silkscreen1 && silkscreen2) {
		    log_msg("Physical address maps to: Cpu Node %d, DIMM pair %d "
			    "(\"%s\" and \"%s\")\n",
			    result.node, result.dimm,
			    silkscreen1, silkscreen2);
		} else {
		    log_msg("Physical address maps to: Cpu Node %d, DIMM pair %d "
			    "(DIMMs %d and %d)\n",
			    result.node, result.dimm,
			    result.dimm * 2, (result.dimm * 2) + 1);
		}
	    } else {
		silkscreen1 = dimm_map(result.node, result.dimm);
		if (silkscreen1) {
		    if (result.bits)
			log_msg("Physical address maps to: Cpu Node %d, DIMM %d "
				"(\"%s\") (bits %016llx)\n",
				result.node, result.dimm, silkscreen1, result.bits);
		    else
			log_msg("Physical address maps to: Cpu Node %d, DIMM %d (\"%s\")\n",
				result.node, result.dimm, silkscreen1);
		} else {
		    if (result.bits)
			log_msg("Physical address maps to: Cpu Node %d, DIMM %d "
				"(bits %016llx)\n",
				result.node, result.dimm, result.bits);
		    else
			log_msg("Physical address maps to: Cpu Node %d, DIMM %d\n",
				result.node, result.dimm);
		}
	    }

	} else {
	    log_msg("Unable to map physical address to specific DIMM\n");
	}
    }
}

void mce_process_log()
{
    self.fd = open(self.device, O_RDONLY);
    if (self.fd < 0) {
	if (options.ignore_no_dev) exit(0);
	log_errno("Cannot open %s\n", self.device);
	return;
    }

    if (self.is_regular) {
	// This is for testing only. When the /dev/mcelog is a
	// regular file, we assume it is meant as a test file and
	// contains the raw MCE binary data. We must therefore use
	// stat() to compute how many MCEs we have in the file.
	struct stat st;
	if (fstat(self.fd, &st)) {
	    log_errno("Unable to stat test mcelog file\n");
	    close(self.fd);
	    return;
	}
	if ((st.st_size % sizeof(struct mce)) != 0) {
	    log_msg("Test mcelog file has inconsistent size\n");
	    close(self.fd);
	    return;
	}

	self.record_len = sizeof(struct mce);
	self.log_len = st.st_size / sizeof(struct mce);

    } else {
	// Ye normal case. Get log length from kernel driver.
	if (ioctl(self.fd, M_GET_RECORD_LEN, &self.record_len) < 0 ||
	    ioctl(self.fd, M_GET_LOG_LEN, &self.log_len) < 0) {
	    log_errno("MCE_GET");
	    return;
	}
    }

    if (self.record_len > sizeof(struct mce)) {
	log_msg("Incompatible kernel version. Exiting");
	return;
    }

    // Allocate buffer to hold the MCEs
    self.mce_log = calloc(self.record_len, self.log_len);
    if (!self.mce_log) {
	log_errno("Could not allocate mce log buffer");
	return;
    }

    // Read them all.
    int len = read(self.fd, self.mce_log, self.record_len * self.log_len);
    if (len < 0) {
	log_errno("Reading %s failed", self.device);
    }
    if (options.debug_mode) {
	log_msg("Read %d bytes from %s\n", len, self.device);
    }

    // Process each MCE.
    int i;
    for (i = 0; i < len / self.record_len; i++) {
	struct mce* mce = ((struct mce*)(self.mce_log)) + i;

	if (mce_filter(mce))
	    continue;

	// Filter using SERD engine. Only log this fault if
	// N occurrences in T time.
	if (serd_filter(mce)) {
	    // SERD filtered this mce
	    if (options.debug_mode) {
		log_msg("serd filtered this mce");
	    }
	    continue;
	}

	// Send MCE to syslog.
	mce_process_mce(mce);

	// Send MCE to BMC SEL through IPMI interface.
	oipmi_process_mce(mce);
    }

    close(self.fd);
    self.fd = 0;
}

void mce_exit()
{
}
