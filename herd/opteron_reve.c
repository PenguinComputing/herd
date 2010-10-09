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

#include <string.h>

#include "pci.h"
#include "log.h"
#include "dimm.h"
#include "options.h"
#include "opteron.h"

#ifdef WIN32
#include "..\..\windows\sbemon\decode.h"
extern WinOptions wOptions;
#endif /* WIN32 */

void oreve_show_config(void)
{
    u32 dramconflow = pci_read_config(0, 0x18, 2, 0x90);

    // Show DRAM Interface width
    u32 di128 = dramconflow & (1 << 16);
    log_msg("DRAM interface: %s\n",	(di128 ? "128-bit":"64-bit"));

    // Show buffered/unbuffered
    u32 unbuffdimm = dramconflow & (1 << 18);
    log_msg("DIMMs are %s\n", (unbuffdimm ? "unbuffered" : "buffered"));

    // Show ECC capability
    u32 dimmeccen = dramconflow & (1 << 17);
    log_msg("Error Checking and Correction (ECC)%s available on all DIMMs\n",
	    (dimmeccen ? "" : " NOT"));

    // Display DRAM ECC Correction mode
    u32 nbcfg = pci_read_config(0, 0x18, 3, 0x44);
    if (nbcfg & (1 << 22)) {
	if (nbcfg & (1 << 23))
	    log_msg("Chip Kill ECC mode enabled\n");
	else
	    log_msg("Normal ECC mode enabled\n");
    }
}

int oreve_translate(decode_input_t* dinput, decode_result_t* result)
{
    // The algorithm System address input variable should be equal to
    // bits 39-8 of the system address.
    u32 SystemAddr = (u32)(dinput->address >> 8);

    if (options.debug_mode) {
	log_msg("herd: dimm translation against system address %08x\n",
		SystemAddr);
    }

    int CSFound, NodeID, CS = 0, F1Offset, F2Offset, Ilog;
    u32 IntlvEn, IntlvSel;
    u32 DramBase, DramLimit, DramEn;
    u32 HoleOffset, HoleEn;
    u32 CSBase, CSMask, CSEn;
    u32 InputAddr, Temp;
    CSFound = 0;
    for(NodeID = 0; NodeID < 8; NodeID++){
	F1Offset = 0x40 + (NodeID << 3);
	DramBase = pci_read_config(0, 24 + NodeID, 1, F1Offset);
	DramEn = DramBase & 0x00000003;
	IntlvEn = (DramBase & 0x00000700) >> 8;
	DramBase = DramBase & 0xFFFF0000;
	DramLimit = pci_read_config(0, 24 + NodeID, 1, F1Offset + 4);
	IntlvSel = (DramLimit & 0x00000700) >> 8;
	DramLimit = DramLimit | 0x0000FFFF;
	HoleEn = pci_read_config(0, 24 + NodeID, 1, 0xF0);
	HoleOffset = (HoleEn & 0x0000FF00) << 8;
	HoleEn = (HoleEn &0x00000001);
	if (options.debug_mode && NodeID < pci_num_nodes()) {
	  log_msg("  Node %d: DRAM base %04x000000, limit %04xffffff%s%s\n",
		  NodeID, (DramBase >> 16), (DramLimit >> 16),
		  (HoleEn ? ", memory hoisted" : ""),
		  (IntlvSel ? ", interleaved" : ""));
	}
	if(DramEn && DramBase <= SystemAddr && SystemAddr <= DramLimit){
	    if(HoleEn && SystemAddr > 0x00FFFFFF)
		InputAddr = SystemAddr - HoleOffset;
	    else
		InputAddr = SystemAddr - DramBase;
	    if(IntlvEn){
		if(IntlvSel == ((SystemAddr >> 4) & IntlvEn)){
		    if(IntlvEn == 1) Ilog = 1;
		    else if(IntlvEn == 3) Ilog = 2;
		    else if(IntlvEn == 7) Ilog = 3;
		    else break;
		    Temp = (SystemAddr >> (4 + Ilog)) << 4;
		    InputAddr = (Temp | (SystemAddr & 0x0000000F)) << 4;
		}
		else continue;
	    }
	    InputAddr = InputAddr << 4;
	    for(CS = 0; CS < 8; CS++){
		F2Offset = 0x40 + (CS << 2);
		CSBase = pci_read_config(0, 24 + NodeID, 2, F2Offset);
		CSEn = CSBase & 0x00000001;
		CSBase = CSBase & 0xFFE0FE00;
		CSMask = pci_read_config(0, 24 + NodeID, 2, F2Offset + 0x20);
		CSMask = (CSMask | 0x001F01FF) & 0x3FFFFFFF;
		if (options.debug_mode && CSEn) {
		  log_msg("      CS %d: base %08x00, mask %08x00\n", CS, CSBase, ~CSMask);
		}
		if(CSEn && ((InputAddr & ~CSMask) == (CSBase & ~CSMask))){
		    CSFound = 1;
		    break;
		}
	    }
	}
	if(CSFound) break;
    }

    memset(result, 0, sizeof(*result));
    result->node = NodeID;
    result->dimm = (CS / 2);

    if (pci_read_config(0, 0x18 + NodeID, 2, 0x90) & (1 << 16)) {

	int symbol, bits;

	if (dinput->syndrome &&
	    opteron_get_ck_syndrome(pci_get_cpu(), dinput->syndrome,
				    &symbol, &bits)) {
	    // BKDG page 163. Symbols 0 to 0x0f identify bits
	    // 0-63. 0x10 to 0x1f bits 64-128. Symbols 0x20 to 0x23
	    // map ECC check bits. Each symbol covers 4 bits.

	    if (symbol < 0x20) {
		result->bits = ((u64)bits) << (symbol * 4);
	    }

	    if (symbol < 0x10 || symbol == 0x20 || symbol == 0x21)
		result->dimm = (result->dimm * 2);
	    else
		result->dimm = (result->dimm * 2) + 1;

	} else {
	    result->is_dimm_pair = 1;
	}
    }
    return CSFound;
}

void oreve_init_cpu(Cpu* cpu)
{
#ifdef WIN32
	wOptions.CpuDecode = cpu;
#endif /* WIN32 */

    cpu->show_config = oreve_show_config;
    cpu->translate = oreve_translate;
}
