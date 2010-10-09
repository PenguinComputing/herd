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

void orev10_show_config(void)
{
    if (options.debug_mode >= 2) {
	int node;
	for (node = 0; node < pci_num_nodes(); node++) {
	    int i;
	    u32 f2x110 = pci_read_config(0, 0x18 + node, 2, 0x110);
	    u32 f3xe8 = pci_read_config(0, 0x18 + node, 3, 0xe8);
	    int ganged = ((f2x110 & (1 << 4)) != 0);

	    log_msg("-- Node %d ---------------------------------------------\n",
		    node);

	    log_msg("This is a %s-node setup\n", 
		    ((f3xe8 & (1 << 29)) ? "multi" : "single"));

	    log_msg("This is node %d\n", ((f3xe8 >> 30)));

	    u32 dramconflow = pci_read_config(0, 0x18, 2, 0x90);

	    // Show DRAM Interface width
	    u32 di128 = dramconflow & (1 << 11);
	    log_msg("DRAM interface: %s\n", (di128 ? "128-bit":"64-bit"));

	    // Show buffered/unbuffered
	    u32 unbuffdimm = dramconflow & (1 << 16);
	    log_msg("DIMMs are %s\n", (unbuffdimm ? "unbuffered" : "registered"));

	    // Show ECC capability
	    u32 dimmeccen = dramconflow & (1 << 19);
	    log_msg("Error Checking and Correction (ECC)%s available on all "
		    "DIMMs\n", (dimmeccen ? "" : " NOT"));

	    log_msg("Channel interleave: %s\n",
		    ((f2x110 & (1 << 2)) ? "enabled" : "disabled"));

	    if (f2x110 & (1 << 2))
		log_msg("  Interleaving field: %d\n", ((f2x110 >> 6) & 0x03));

	    log_msg("Data Interleave: %s\n",
		    ((f2x110 & (1 << 5)) ? "enabled" : "disabled"));

	    log_msg("Ganging: %s\n", (ganged ? "enabled" : "disabled"));

	    // Display DRAM ECC Correction mode
	    u32 nbcfg = pci_read_config(0, 0x18 + node, 3, 0x44);
	    u32 extnbcfg = pci_read_config(0, 0x18 + node, 3, 0x180);
	    
	    log_msg("DRAM ECC correction enabled\n",
		    ((nbcfg & (1 << 22)) ? "enabled" : "disabled"));

	    log_msg("ChipKill ECC mode: %s\n",
		    ((nbcfg & (1 << 23)) ? "enabled" : "disabled"));

	    log_msg("ECC symbol size: %d\n", ((extnbcfg & (1 << 25)) ? 8 : 4));

	    if (ganged) {
		for (i = 0; i < 8; i++) {
		    u32 base = pci_read_config(0, 24 + node, 2, 0x40 + (i * 4));
		    u32 mask = pci_read_config(0, 24 + node, 2, 0x60 + (i * 4));
		    log_msg("Cs%d: base=%08x  mask=%08x%s%s%s%s\n",
			    i, base & 0xfffffff0, mask,
			    ((base & 0x01) ? " (ENABLED)" : ""),
			    ((base & 0x02) ? " (SPARE RANK)" : ""),
			    ((base & 0x03) ? " (TEST FAIL)" : ""),
			    ((base & 0x04) ? " (ON-DIMM MIRRORING)" : ""));
		}
	    } else {
		// Unganged
		for (i = 0; i < 8; i++) {
		    u32 dct0base = pci_read_config(0, 24 + node, 2, 0x40 + (i * 4));
		    u32 dct0mask = pci_read_config(0, 24 + node, 2, 0x60 + (i * 4));
		    u32 dct1base = pci_read_config(0, 24 + node, 2, 0x140 + (i * 4));
		    u32 dct1mask = pci_read_config(0, 24 + node, 2, 0x160 + (i * 4));
		    log_msg("CS%d: %08x:%08x (%c%c%c%c) %08x:%08x (%c%c%c%c)\n", i,
			    dct0base & 0xfffffff0, dct0mask,
			    (dct0base & 0x01 ? 'E' : ' '),
			    (dct0base & 0x02 ? 'S' : ' '),
			    (dct0base & 0x04 ? 'T' : ' '),
			    (dct0base & 0x08 ? 'M' : ' '),
			    dct1base & 0xfffffff0, dct1mask,
			    (dct1base & 0x01 ? 'E' : ' '),
			    (dct1base & 0x02 ? 'S' : ' '),
			    (dct1base & 0x04 ? 'T' : ' '),
			    (dct1base & 0x08 ? 'M' : ' '));
		}
	    }
	}
    }
}


int fUnaryXOR(u64 bits)
{
    int val = 0;

    while (bits) {
	val = val ^ (bits & 0x01);
	bits >>= 1;
    }
    return val;
}

// Translate a 64-bit physical address into a node ID, DRAM controller
// (or channel, 0 or 1) and dimm number (aka chip select).
int orev10_translate(decode_input_t* dinput, decode_result_t* result)
{
    // Unlike the 2 algorithms above, this one starts with the full 64-bit
    // physical address.
    u64 SystemAddr = dinput->address;

    int SwapDone, BadDramCs;

    // CPU to read configuration information. Defaults to first cpu.
    int thisNode = 0;

    // Output variables
    int CSFound, NodeID, CS = 0, ChannelSelect = 0;

    int F1Offset, F2Offset, F2MaskOffset, Ilog;
    int HiRangeSelected, DramRange;
    u32 IntlvEn, IntlvSel;
    u32 DramBaseLow, DramLimitLow, DramEn;
    u32 HoleOffset, HoleEn;
    u32 CSBase, CSMask, CSEn;
    u32 InputAddr, Temp;
    u32 OnlineSpareCTL;
    u32 DctSelBaseAddr, DctSelIntLvAddr, DctGangEn, DctSelIntLvEn;
    u32 DctSelHiRngEn,DctSelHi;
    u64 DramBaseLong, DramLimitLong;
    u64 DctSelBaseOffsetLong, ChannelOffsetLong,ChannelAddrLong;
    // device is a user supplied value for the PCI device ID of the processor
    // from which CSRs are initially read from (current processor is fastest).
    // CH0SPARE_RANK and CH1SPARE_RANK are user supplied values, determined
    // by BIOS during DIMM sizing.

    u32 f2x110;
    int ganged_mode;

    CSFound = 0;
    for (DramRange = 0; DramRange < 8; DramRange++) {
	F1Offset = 0x40 + (DramRange << 3);
	// DramBaseLow = Get_PCI(bus0, device, func1, F1Offset);
	DramBaseLow = pci_read_config(0, 24 + thisNode, 1, F1Offset);
	DramEn = DramBaseLow & 0x00000003;
	IntlvEn = (DramBaseLow & 0x00000700) >> 8;
	DramBaseLow = DramBaseLow & 0xFFFF0000;
	// DramBaseLong=((Get_PCI(bus0, device, func1, F1Offset + 0x100))<<32 +
	// DramBaseLow)<<8;
	DramBaseLong =
	    ((((u64)pci_read_config(0, 24 + thisNode, 1,
				    F1Offset + 0x100) & 0xFF)<<32) +
	     DramBaseLow)<<8;
	// DramLimitLow = Get_PCI(bus0, device, func1, F1Offset + 4);
	DramLimitLow = pci_read_config(0, 24 + thisNode, 1, F1Offset + 4);
	NodeID = DramLimitLow & 0x00000007;
	IntlvSel = (DramLimitLow & 0x00000700) >> 8;
	DramLimitLow = DramLimitLow | 0x0000FFFF;
	//DramLimitLong=((Get_PCI(bus0,device, func1, F1Offset + 0x104))<<32 +
	// DramLimitLow)<<8 | 0xFF;
	DramLimitLong =
	    (((((u64)pci_read_config(0, 24 + thisNode, 1,
				     F1Offset + 0x104) & 0xFF) << 32) +
	      DramLimitLow) <<8) | 0xFFLL;
	// HoleEn = Get_PCI(bus0, dev24 + NodeID, func1, 0xF0);
	HoleEn = pci_read_config(0, 24 + NodeID, 1, 0xF0);
	HoleOffset = (HoleEn & 0x0000FF80);
	HoleEn = (HoleEn &0x00000003);

	if (options.debug_mode && DramEn) {
	  log_msg("  Node %d: DRAM base %04x000000, limit %04xffffff%s%s\n",
		  NodeID, (DramBaseLow >> 16), (DramLimitLow >> 16),
		  (HoleEn ? ", memory hoisted" : ""),
		  (IntlvSel ? ", interleaved" : ""));
	}

	if (DramEn && DramBaseLong <= SystemAddr &&
	    SystemAddr <= DramLimitLong) {

	    if (IntlvEn == 0 || IntlvSel == ((SystemAddr >> 12) & IntlvEn)) {

		if (IntlvEn == 1) Ilog = 1;
		else if (IntlvEn == 3) Ilog = 2;
		else if (IntlvEn == 7) Ilog = 3;
		else Ilog = 0;
		// Temp = Get_PCI(bus0, device, func2, 0x110);
		Temp = pci_read_config(0, 24 + NodeID, 2, 0x110);
		DctSelHiRngEn = Temp & 1;
		DctSelHi = Temp>>1 & 1;
		DctSelIntLvEn = Temp & 4;
		DctGangEn = Temp & 0x10;
		DctSelIntLvAddr = (Temp >> 6) & 3;
		DctSelBaseAddr = Temp & 0xFFFFF800;
		// DctSelBaseOffsetLong=Get_PCI(bus0,device,func2,0x114)<<16;
		DctSelBaseOffsetLong =
		  (((u64)pci_read_config(0, 24 + NodeID, 2, 0x114))
		   & 0xFFFFFC00) << 16;
		//Determine if High range is selected
		if( DctSelHiRngEn && DctGangEn==0 && (SystemAddr >> 27) >=
		    (DctSelBaseAddr >> 11))
		    HiRangeSelected = 1;
		else
		    HiRangeSelected = 0;
		//Determine Channel
		if (DctGangEn)
		    ChannelSelect = 0;
		else if (HiRangeSelected)
		    ChannelSelect = DctSelHi;
		else if (DctSelIntLvEn && DctSelIntLvAddr == 0)
		    ChannelSelect = SystemAddr>>6 & 1;
		else if (DctSelIntLvEn && DctSelIntLvAddr>>1 & 1) {
		    // Function returns odd parity
		    Temp = fUnaryXOR(SystemAddr>>16&0x1F);
		    // 1= number of set bits in argument is odd.
		    // 0= number of set bits in argument is even.
		    if (DctSelIntLvAddr & 1)
			ChannelSelect = (SystemAddr>>9 & 1) ^ Temp;
		    else
			ChannelSelect = (SystemAddr>>6 & 1) ^ Temp;
		}
		else if (DctSelIntLvEn && IntlvEn & 4)
		    ChannelSelect = SystemAddr >> 15 & 1;
		else if (DctSelIntLvEn && IntlvEn & 2)
		    ChannelSelect = SystemAddr >> 14 & 1;
		else if (DctSelIntLvEn && IntlvEn & 1)
		    ChannelSelect = SystemAddr >> 13 & 1;
		else if (DctSelIntLvEn)
		    ChannelSelect = SystemAddr >> 12 & 1;
		else if (DctSelHiRngEn && DctGangEn==0)
		    ChannelSelect = ~DctSelHi&1;
		else ChannelSelect = 0;
		// Determine Base address Offset to use
		if (HiRangeSelected) {
		    if(!(DctSelBaseAddr & 0xFFFF0000) && (HoleEn & 1) &&
		       (SystemAddr >= 0x100000000LL))
			ChannelOffsetLong = HoleOffset<<16;
		    else
			ChannelOffsetLong= DctSelBaseOffsetLong;

		} else {
		    if((HoleEn & 1) && (SystemAddr >= 0x100000000LL))
			ChannelOffsetLong = HoleOffset << 16;
		    else
			ChannelOffsetLong = DramBaseLong & 0xFFFFF8000000LL;
		}

		// Remove hoisting offset and normalize to DRAM bus addresses
		ChannelAddrLong = (SystemAddr & 0x0000FFFFFFFFFFC0LL) -
		  (ChannelOffsetLong & 0x0000FFFFFF800000LL);
		// Remove Node ID (in case of processor interleaving)
		Temp = ChannelAddrLong & 0xFC0;
		ChannelAddrLong =
		    (ChannelAddrLong >> Ilog & 0xFFFFFFFFF000LL) | Temp;
		// Remove Channel interleave and hash
		if (DctSelIntLvEn && HiRangeSelected==0 && DctGangEn==0) {
		    if ((DctSelIntLvAddr & 1) != 1)
			ChannelAddrLong =
			    (ChannelAddrLong>>1) & 0xFFFFFFFFFFFFFFC0LL;
		    else if (DctSelIntLvAddr == 1) {
			Temp = ChannelAddrLong & 0xFC0;
			ChannelAddrLong =
			    ((ChannelAddrLong & 0xFFFFFFFFFFFFE000LL) >> 1) | Temp;
		    } else {
			Temp = ChannelAddrLong & 0x1C0;
			ChannelAddrLong =
			    ((ChannelAddrLong & 0xFFFFFFFFFFFFFC00LL) >> 1) | Temp;
		    }
		}
		InputAddr = ChannelAddrLong >> 8;

		for(CS = 0; CS < 8; CS++) {
		    F2Offset = 0x40 + (CS << 2);
		    if ((CS % 2) == 0)
			F2MaskOffset = 0x60 + (CS << 1);
		    else
			F2MaskOffset = 0x60 + ((CS-1) << 1);

		    if (ChannelSelect) {
			F2Offset += 0x100;
			F2MaskOffset += 0x100;
		    }
		    // CSBase = Get_PCI(bus0, dev24 + NodeID, func2, F2Offset);
		    CSBase = pci_read_config(0, 24 + NodeID, 2, F2Offset);
		    CSEn = CSBase & 0x00000001;
		    CSBase = CSBase & 0x1FF83FE0;
		    // CSMask = Get_PCI(bus0,dev24+NodeID,func2,F2MaskOffset);
		    CSMask = pci_read_config(0, 24 + NodeID, 2, F2MaskOffset);
		    CSMask = (CSMask | 0x0007C01F) & 0x1FFFFFFF;
		    if (options.debug_mode && CSEn) {
			log_msg("      Ch %d CS %d: base %08x00, mask %08x00\n",
				ChannelSelect, CS, CSBase, ~CSMask);
		    }
		    if (CSEn && ((InputAddr & ~CSMask) == (CSBase & ~CSMask))) {
			CSFound = 1;
			// OnlineSpareCTL=Get_PCI(bus0,dev24+NodeID,func3,0xB0);
			OnlineSpareCTL = pci_read_config(0, 24 + NodeID, 3,
							 0xB0);

			if(ChannelSelect) {
			    SwapDone = (OnlineSpareCTL >> 3) & 0x00000001;
			    BadDramCs = (OnlineSpareCTL >> 8) & 0x00000007;
			    if (SwapDone && CS == BadDramCs) {
				CS = 0; // CH1SPARE_RANK;
			    }
			} else {
			    SwapDone = (OnlineSpareCTL >> 1) & 0x00000001;
			    BadDramCs = (OnlineSpareCTL >> 4) & 0x00000007;
			    if (SwapDone && CS == BadDramCs) {
				CS = 0; // CH0SPARE_RANK;
			    }
			}
			break;
		    }
		}
	    }
	}
	if (CSFound) break;
    } // for each DramRange

    memset(result, 0, sizeof(*result));
    result->node = NodeID;
    result->dimm = (CS / 2);

    f2x110 = pci_read_config(0, 0x18 + NodeID, 2, 0x110);
    ganged_mode = ((f2x110 & (1 << 4)) != 0);

    if (ganged_mode) {
	// 128-bit interface.
	int symbol, bits;

	if (dinput->syndrome &&
	    opteron_get_ck_syndrome(pci_get_cpu(), dinput->syndrome,
				    &symbol, &bits)) {
	    // BKDG page 173. Symbols 0 to 0x0f identify bits
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
	    result->dimm = (result->dimm * 2);
	}
    } else {
	// Unganged mode, 64-bit interface
	result->dimm = (result->dimm * 2) + ChannelSelect;
    }

    return CSFound;
}

void orev10_init_cpu(Cpu* cpu)
{
#ifdef WIN32
	wOptions.CpuDecode = cpu;
#endif /* WIN32 */

    cpu->show_config = orev10_show_config;
    cpu->translate = orev10_translate;
}
