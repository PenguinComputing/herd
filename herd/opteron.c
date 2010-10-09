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
#ifndef WIN32
#include <stdint.h>
#else /* WIN32 */
#include "..\..\windows\sbemon\win_linux.h"
#include "..\..\windows\sbemon\decode.h"
#endif /* WIN32 */

#include <stdlib.h>
#include <string.h>

#include "opteron.h"
#include "opteron_reve.h"
#include "opteron_revf.h"
#include "opteron_rev10.h"

#ifndef WIN32
#define cpuid(in,a,b,c,d)\
  asm("cpuid": "=a" (a), "=b" (b), "=c" (c), "=d" (d) : "a" (in));
#else
extern WinOptions	wOptions;
#define cpuid(in,a,b,c,d) {a = wOptions.CPUId;}
#endif /* WIN32 */

static u16 cksyndromes[][15] = {
    { 0xe821, 0x7c32, 0x9413, 0xbb44, 0x5365, 0xc776, 0x2f57, 0xdd88,
      0x35a9, 0xa1ba, 0x499b, 0x66cc, 0x8eed, 0x1afe, 0xf2df },
    { 0x5d31, 0xa612, 0xfb23, 0x9584, 0xc8b5, 0x3396, 0x6ea7, 0xeac8,
      0xb7f9, 0x4cda, 0x11eb, 0x7f4c, 0x227d, 0xd95e, 0x846f },
    { 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008,
      0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f },
    { 0x2021, 0x3032, 0x1013, 0x4044, 0x6065, 0x7076, 0x5057, 0x8088,
      0xa0a9, 0xb0ba, 0x909b, 0xc0cc, 0xe0ed, 0xf0fe, 0xd0df },
    { 0x5041, 0xa082, 0xf0c3, 0x9054, 0xc015, 0x30d6, 0x6097, 0xe0a8,
      0xb0e9, 0x402a, 0x106b, 0x70fc, 0x20bd, 0xd07e, 0x803f },
    { 0xbe21, 0xd732, 0x6913, 0x2144, 0x9f65, 0xf676, 0x4857, 0x3288,
      0x8ca9, 0xe5ba, 0x5b9b, 0x13cc, 0xaded, 0xc4fe, 0x7adf },
    { 0x4951, 0x8ea2, 0xc7f3, 0x5394, 0x1ac5, 0xdd36, 0x9467, 0xa1e8,
      0xe8b9, 0x2f4a, 0x661b, 0xf27c, 0xbb2d, 0x7cde, 0x358f },
    { 0x74e1, 0x9872, 0xec93, 0xd6b4, 0xa255, 0x4ec6, 0x3a27, 0x6bd8,
      0x1f39, 0xf3aa, 0x874b, 0xbd6c, 0xc98d, 0x251e, 0x51ff },
    { 0x15c1, 0x2a42, 0x3f83, 0xcef4, 0xdb35, 0xe4b6, 0xf177, 0x4758,
      0x5299, 0x6d1a, 0x78db, 0x89ac, 0x9c6d, 0xa3ee, 0xb62f },
    { 0x3d01, 0x1602, 0x2b03, 0x8504, 0xb805, 0x9306, 0xae07, 0xca08,
      0xf709, 0xdc0a, 0xe10b, 0x4f0c, 0x720d, 0x590e, 0x640f },
    { 0x9801, 0xec02, 0x7403, 0x6b04, 0xf305, 0x8706, 0x1f07, 0xbd08,
      0x2509, 0x510a, 0xc90b, 0xd60c, 0x4e0d, 0x3a0e, 0xa20f },
    { 0xd131, 0x6212, 0xb323, 0x3884, 0xe9b5, 0x5a96, 0x8ba7, 0x1cc8,
      0xcdf9, 0x7eda, 0xafeb, 0x244c, 0xf57d, 0x465e, 0x976f },
    { 0xe1d1, 0x7262, 0x93b3, 0xb834, 0x59e5, 0xca56, 0x2b87, 0xdc18,
      0x3dc9, 0xae7a, 0x4fab, 0x642c, 0x85fd, 0x164e, 0xf79f },
    { 0x6051, 0xb0a2, 0xd0f3, 0x1094, 0x70c5, 0xa036, 0xc067, 0x20e8,
      0x40b9, 0x904a, 0xf01b, 0x307c, 0x502d, 0x80de, 0xe08f },
    { 0xa4c1, 0xf842, 0x5c83, 0xe6f4, 0x4235, 0x1eb6, 0xba77, 0x7b58,
      0xdf99, 0x831a, 0x27db, 0x9dac, 0x396d, 0x65ee, 0xc12f },
    { 0x11c1, 0x2242, 0x3383, 0xc8f4, 0xd935, 0xeab6, 0xfb77, 0x4c58,
      0x5d99, 0x6e1a, 0x7fdb, 0x84ac, 0x956d, 0xa6ee, 0xb72f },
    { 0x45d1, 0x8a62, 0xcfb3, 0x5e34, 0x1be5, 0xd456, 0x9187, 0xa718,
      0xe2c9, 0x2d7a, 0x68ab, 0xf92c, 0xbcfd, 0x734e, 0x369f },
    { 0x63e1, 0xb172, 0xd293, 0x14b4, 0x7755, 0xa5c6, 0xc627, 0x28d8,
      0x4b39, 0x99aa, 0xfa4b, 0x3c6c, 0x5f8d, 0x8d1e, 0xeeff },
    { 0xb741, 0xd982, 0x6ec3, 0x2254, 0x9515, 0xfbd6, 0x4c97, 0x33a8,
      0x84e9, 0xea2a, 0x5d6b, 0x11fc, 0xa6bd, 0xc87e, 0x7f3f },
    { 0xdd41, 0x6682, 0xbbc3, 0x3554, 0xe815, 0x53d6, 0x8e97, 0x1aa8,
      0xc7e9, 0x7c2a, 0xa16b, 0x2ffc, 0xf2bd, 0x497e, 0x943f },
    { 0x2bd1, 0x3d62, 0x16b3, 0x4f34, 0x64e5, 0x7256, 0x5987, 0x8518,
      0xaec9, 0xb87a, 0x93ab, 0xca2c, 0xe1fd, 0xf74e, 0xdc9f },
    { 0x83c1, 0xc142, 0x4283, 0xa4f4, 0x2735, 0x65b6, 0xe677, 0xf858,
      0x7b99, 0x391a, 0xbadb, 0x5cac, 0xdf6d, 0x9dee, 0x1e2f },
    { 0x8fd1, 0xc562, 0x4ab3, 0xa934, 0x26e5, 0x6c56, 0xe387, 0xfe18,
      0x71c9, 0x3b7a, 0xb4ab, 0x572c, 0xd8fd, 0x924e, 0x1d9f },
    { 0x4791, 0x89e2, 0xce73, 0x5264, 0x15f5, 0xdb86, 0x9c17, 0xa3b8,
      0xe429, 0x2a5a, 0x6dcb, 0xf1dc, 0xb64d, 0x783e, 0x3faf },
    { 0x5781, 0xa9c2, 0xfe43, 0x92a4, 0xc525, 0x3b66, 0x6ce7, 0xe3f8,
      0xb479, 0x4a3a, 0x1dbb, 0x715c, 0x26dd, 0xd89e, 0x8f1f },
    { 0xbf41, 0xd582, 0x6ac3, 0x2954, 0x9615, 0xfcd6, 0x4397, 0x3ea8,
      0x81e9, 0xeb2a, 0x546b, 0x17fc, 0xa8bd, 0xc27e, 0x7d3f },
    { 0x9391, 0xe1e2, 0x7273, 0x6464, 0xf7f5, 0x8586, 0x1617, 0xb8b8,
      0x2b29, 0x595a, 0xcacb, 0xdcdc, 0x4f4d, 0x3d3e, 0xaeaf },
    { 0xcce1, 0x4472, 0x8893, 0xfdb4, 0x3155, 0xb9c6, 0x7527, 0x56d8,
      0x9a39, 0x12aa, 0xde4b, 0xab6c, 0x678d, 0xef1e, 0x23ff },
    { 0xa761, 0xf9b2, 0x5ed3, 0xe214, 0x4575, 0x1ba6, 0xbcc7, 0x7328,
      0xd449, 0x8a9a, 0x2dfb, 0x913c, 0x365d, 0x688e, 0xcfef },
    { 0xff61, 0x55b2, 0xaad3, 0x7914, 0x8675, 0x2ca6, 0xd3c7, 0x9e28,
      0x6149, 0xcb9a, 0x34fb, 0xe73c, 0x185d, 0xb28e, 0x4def },
    { 0x5451, 0xa8a2, 0xfcf3, 0x9694, 0xc2c5, 0x3e36, 0x6a67, 0xebe8,
      0xbfb9, 0x434a, 0x171b, 0x7d7c, 0x292d, 0xd5de, 0x818f },
    { 0x6fc1, 0xb542, 0xda83, 0x19f4, 0x7635, 0xacb6, 0xc377, 0x2e58,
      0x4199, 0x9b1a, 0xf4db, 0x37ac, 0x586d, 0x82ee, 0xed2f },
    { 0xbe01, 0xd702, 0x6903, 0x2104, 0x9f05, 0xf606, 0x4807, 0x3208,
      0x8c09, 0xe50a, 0x5b0b, 0x130c, 0xad0d, 0xc40e, 0x7a0f },
    { 0x4101, 0x8202, 0xc303, 0x5804, 0x1905, 0xda06, 0x9b07, 0xac08,
      0xed09, 0x2e0a, 0x6f0b, 0xf40c, 0xb50d, 0x760e, 0x370f },
    { 0xc441, 0x4882, 0x8cc3, 0xf654, 0x3215, 0xbed6, 0x7a97, 0x5ba8,
      0x9fe9, 0x132a, 0xd76b, 0xadfc, 0x69bd, 0xe57e, 0x213f },
    { 0x7621, 0x9b32, 0xed13, 0xda44, 0xac65, 0x4176, 0x3757, 0x6f88,
      0x19a9, 0xf4ba, 0x829b, 0xb5cc, 0xc3ed, 0x2efe, 0x58df },
};

#define SYNDR_KEY_FORMAT "%04x"

static struct {
    uint32_t eax;
    const char* desc;
} opteronRevisions[] = {
    { 0x00f50, "AMD Opteron(tm), rev SH-B0 (940)" },
    { 0x00f51, "AMD Opteron(tm), rev SH-B3 (940)" },
    { 0x00f58, "AMD Opteron(tm), rev SH-C0 (940)" },
    { 0x00f5a, "AMD Opteron(tm), rev SH-CG (940)" },
    { 0x10f50, "AMD Opteron(tm), rev SH-D0 (940)" },
    { 0x20f10, "Dual-Core AMD Opteron(tm), rev JH-E1 (940)" },
    { 0x20f12, "Dual-Core AMD Opteron(tm), rev JH-E6 (940)" },
    { 0x20f32, "Dual-Core AMD Opteron(tm), rev JH-E6 (939)" },
    { 0x20f50, "AMD Opteron(tm), rev SH-E0 (940)" },
    { 0x20f51, "AMD Opteron(tm), rev SH-E4 (940)" },
    { 0x20f71, "AMD Opteron(tm), rev SH-E4 (939)" },
    { 0x30f51, "AMD Opteron(tm), rev SH-E4 (940)" },
    // Rev. F
    { 0x40f11, "Dual-Core AMD Opteron(tm), rev JH-F1, Socket F (1207)" },
    { 0x40f12, "Dual-Core AMD Opteron(tm), rev JH-F2, Socket F (1207)" },
    { 0x40f13, "Dual-Core AMD Opteron(tm), rev JH-F3, Socket F (1207)" },
    { 0x40f32, "Dual-Core AMD Opteron(tm), rev JH-F2, Socket AM2" },
    { 0x40f33, "Dual-Core AMD Opteron(tm), rev JH-F3, Socket AM2" },
    { 0x40f50, "AMD Opteron(tm), rev SH-F0, Socket F (1207)" },
    { 0x50fd3, "AMD Opteron(tm), rev JH-F3, Socket F (1207)" },
    // Rev. 10
    { 0x100f00, "AMD Opteron(tm) Engineering Sample, rev DR-A0" },
    { 0x100f01, "AMD Opteron(tm) Engineering Sample, rev DR-A1" },
    { 0x100f02, "AMD Opteron(tm) Engineering Sample, rev DR-A2" },
    { 0x100f20, "AMD Opteron(tm) Engineering Sample, rev DR-B0" },
    { 0x100f21, "Quad-Core AMD Opteron(tm), rev DR-B1" },
    { 0x100f2a, "Quad-Core AMD Opteron(tm), rev DR-BA" },
    { 0x100f22, "Quad-Core AMD Opteron(tm), rev DR-B2" },
    { 0x100f23, "Quad-Core AMD Opteron(tm), rev DR-B3" },
    { 0x100f40, "Quad-Core AMD Opteron(tm), rev RB-C0" },
    { 0x100f41, "Quad-Core AMD Opteron(tm), rev RB-C1" },
    { 0x100f42, "Quad-Core AMD Opteron(tm), rev RB-C2" },
    { 0x100f52, "Quad-Core AMD Opteron(tm), rev BL-C2" },
    { 0x100f62, "Quad-Core AMD Opteron(tm), rev DA-C2" },
    { 0x100f80, "AMD Opteron(tm) Engineering Sample, rev HY-D0" },
    { 0, NULL },
};

const char* opteron_get_description(uint32_t eax)
{
    int i = 0;
    while (opteronRevisions[i].eax) {
	if (opteronRevisions[i].eax == eax)
	    return opteronRevisions[i].desc;
	i++;
    }

    return "Unknown CPU";
}

char opteron_get_revision(int family, int model, int stepping)
{
    if (family == 15) {
	if ((model == 4 || model == 5) && (stepping <= 1))
	    return 'B';

	if (model < 16)
	    return 'C';

	if (model < 32)
	    return 'D';

	if (model < 64)
	    return 'E';

	return 'F';
    }

    if (family == 31) {
	if (model < 2)
	    return 'A';

	if (model < 4)
	    return 'B';

	return 'C';
    }

    return 0;
}

void opteron_init_common(OpteronInfo* info, u32 eax)
{
    info->revision = opteron_get_revision(info->family, info->model,
					  info->stepping);
    info->description = opteron_get_description(eax);

    if (info->family == 0x0f) {
	if (info->revision == 'F')
	    orevf_init_cpu(&(info->cpu));
	else
	    oreve_init_cpu(&(info->cpu));
    } else if (info->family == AMD_QUAD_CORE_FAMILY) {
	orev10_init_cpu(&(info->cpu));
    }

    // Populate the syndrome hash table.
    info->syndromes = set_new(256);
    {
	int i, j;
	int cksize = sizeof(cksyndromes) / sizeof(u16[15]);
	for (i = 0; i < cksize; i++) {
	    for (j = 0; j < 15; j++) {
		char buf[16];
		long val = (i << 16) + (j + 1);
		u16 syn = cksyndromes[i][j];
		snprintf(buf, sizeof(buf), SYNDR_KEY_FORMAT, syn);
		set_add(info->syndromes, buf, (void*)val);
	    }
	}
    }
}

OpteronInfo* opteron_new()
{
    OpteronInfo* info = (OpteronInfo*)malloc(sizeof(OpteronInfo));
    memset(info, 0, sizeof(*info));

    // Run cpuid() instruction on running host CPU.
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, eax, ebx, ecx, edx);

    info->family =    ((eax & 0x00000f00) >> 8) + ((eax & 0x0ff00000) >> 16);
    info->model =     ((eax & 0x000000f0) >> 4) + ((eax & 0x000f0000) >> 12);
    info->stepping =   (eax & 0x0000000f);

    opteron_init_common(info, eax);

    return info;
}

OpteronInfo* opteron_new_from_fms(int f, int m, int s)
{
    OpteronInfo* info = (OpteronInfo*)malloc(sizeof(OpteronInfo));
    memset(info, 0, sizeof(*info));

    info->family = f;
    info->model = m;
    info->stepping = s;

    info->revision = opteron_get_revision(f, m, s);

    uint32_t eax = s & 0x0f;
    eax += ((m & 0x0f) << 4);
    eax += ((m & 0xf0) << 12);
    eax += ((f & 0x0f) << 8);
    eax += ((f & 0xff0) << 16);

    opteron_init_common(info, eax);

    return info;
}

void opteron_delete(OpteronInfo* info)
{
    if (info->syndromes) {
	set_delete(info->syndromes, NULL);
    }
    free((void*)info);
}

int opteron_get_ck_syndrome(Cpu* cpu, u32 syndrome, int* symbol, int* bit)
{
    // Needs to be reorganized and cleaned up. Cpu is supposed to be a
    // base class for CPU specific data structures. Here we're
    // downcasting the base class to the Opteron-specific data
    // structure.
    OpteronInfo* cpu_info = (OpteronInfo*)cpu;
    char buf[16];
    u32 data;

    if (syndrome == 0)
	return 0;

    snprintf(buf, sizeof(buf), SYNDR_KEY_FORMAT, syndrome);

#ifdef WIN32
    if (!set_lookup(cpu_info->syndromes, buf, (void**)&data))
#else
    if (!set_lookup(cpu_info->syndromes, buf, (void*)&data))
#endif /* WIN32 */
	return 0;

    if (symbol) *symbol = (data >> 16);
    if (bit) *bit = data & 0xffff;
    return 1;
}


