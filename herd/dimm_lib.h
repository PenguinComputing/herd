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

#include "mce.h"

typedef struct _spd {
    char *fru_pxdy; /* p#d# */
    char *fru_manufacturer;
    char *fru_name;
    char *fru_partnumber;
    char *fru_version; 
    char *fru_serial; 

    char *pxdy; /* p#d# */
    char *manufacturer;
    char *name;
    char *partnumber;
    char *version; 
    char *serial; 

    char *handle; /* smbios.. */
    char *total_width;
    char *data_width;
    char *size;
    char *form_factor;
    char *set;
    char *locator; /* used for lookup silkscreen */
    char *bank_locator; /* used for lookup silkscreen */
    char *type;
    char *type_detail;
    char *speed;
    char *assettag;
    char *startaddr;
    char *endaddr;
    char *range_size;
    char *dmi_manufacturer;
    char *dmi_partnumber;
    char *dmi_serial;
    char present; /* DMI type 17 - Size */
} spd_t;

typedef struct _dimms {
	char *manufacturer;
	char *name;
	int count;
} dimms_t;
#define MAXTYPE_DIMM   16 /* type of dimm */
dimms_t dimms[MAXTYPE_DIMM];

typedef struct _dimm {
        spd_t   *data;
        struct _dimm *next;
        struct _dimm *peer; /* link between fru_dimm and dmi_dimm */
	char linked;
} dimm_t;

#define WORD(x) (u16)(*(const u16 *)(x))
#define DWORD(x) (u32)(*(const u32 *)(x))

#define DEFAULT_MEM_DEV "/dev/mem"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

int checksum(const u8 *buf, size_t len);
void *mem_chunk(size_t base, size_t len, const char *devmem);

struct dmi_header
{
	u8 type;
	u8 length;
	u16 handle;
	u8 *data;
};


struct string_keyword
{
	const char *keyword;
	u8 type;
	u8 offset;
	const char *(*lookup)(u8);
	void (*print)(u8 *);
};

typedef struct _opt
{
	const char* devmem;
	unsigned int flags;
	u8 *type;
	const struct string_keyword *string;
} opt_t;

#define FLAG_VERSION            (1<<0)
#define FLAG_HELP               (1<<1)
#define FLAG_DUMP               (1<<2)
#define FLAG_QUIET              (1<<3)

enum state_flag { FRU_FLAG, DMI_FLAG };

typedef struct _cpu {
    char *fru_cpu;
    char *fru_manufacturer;
    char *fru_name;
    char *fru_partnumber;
    char *fru_version;
    char *dmi_socket; /* - mandatory */
    char *dmi_type;
    char *dmi_family;
    char *dmi_manufacturer; /* mandatory */
    char *dmi_version; /* mandatory */
    char *dmi_status; /* present or absent */
    struct _cpu *next;
} cpu_t;


typedef struct _sys {
	int num_physical_cpu;
	int num_logical_cpu;
	char *sys_manufacturer;
	char *sys_product_name;
	char *brd_manufacturer;
	char *brd_product_name;
	char *os_phymem;
	char *fru_bios;
	char *dmi_bios;
	char *dmi_smbios;
	char *platform;  
	char *fru_lookup1; /* platform specific fru */ 
	char *fru_lookup2;
	char *fru_lookup3;
	char *dmi_lookup1; /* platform specific dmi */
	char *dmi_lookup2;
	int imap; /* index */
	int fru_dimm_count;
	int dmi_dimm_count;
	dimm_t *dimm_tail;
	dimm_t *dimm_head; 
	dimm_t *fru_dimm_tail; 
	dimm_t *fru_dimm_head; 
	dimm_t *dmi_dimm_tail;
	dimm_t *dmi_dimm_head; 

	cpu_t *fru_cpu_head;
	cpu_t *fru_cpu_tail;
	cpu_t *dmi_cpu_head;
	cpu_t *dmi_cpu_tail;
	int dmi_cpu_count;
	int fru_cpu_count;
} sys_t;

sys_t sys;

#define IPMI_FLAG  1
#define DMI_FLAG   2

/*
 * the ipmi_tag(fru), dmi_bank_locator/dmi_dimm_locator(dmi)
 * read from the platform are used as keys to look up the 
 * silkscreen_map table 
 */
typedef struct _silkscreen {
    char *ipmi_tag;
    char *dmi_bank_locator;
    char *dmi_dimm_locator;
    char *silkscreen;
    int cpu;
    int dimm;
} silkscreen_t;

typedef struct _map {
	char *platform;
	char *alias;
	char *fru_lookup1;
	char *fru_lookup2;
	char *fru_lookup3;
	char *dmi_lookup1;
	char *dmi_lookup2;
	silkscreen_t *map;
	char *oname;
} map_t;

#define	DEFAULT_FRU_LOOKUP1	".d"
#define	DEFAULT_FRU_LOOKUP2	"p"
#define	DEFAULT_FRU_LOOKUP3	"fru"


typedef struct _blacklist {
	char *name;
} blacklist_t;


extern char* dimm_map(int, int);
