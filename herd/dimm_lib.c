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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <sys/param.h>
#include <dirent.h>
#include <sys/types.h>
#include "dimm_lib.h"

char *dimm_map(int cpu, int dimm);
void update_map();
char *strip(char *name);
dimm_t *dimm_head = NULL;
int verbose = 0;

char *
strip(char *name)
{
    char *str1 = name;
    char *str2;
    static char str[128];
    int c;

    memset(&str, 0, sizeof(str));
    str2 = str;
    for(; (c= *((unsigned char*)str1)); str1++, str2++) {
	if (isspace(c)) {
	    str2--; continue;
	}
	if (isupper(c)) {
	    *str2 = tolower(c);
        } else {
	    *str2 = c;
	}
    }
    *str2 = 0;
    return(str);
}


void
cleanup_dimm()
{
    dimm_t *node;
    dimm_t *tmp;
    node = dimm_head;

    while (node) {
	if (verbose) printf("pxdy = %s\n",node->data->pxdy);
	tmp = node;
	node = node->next;
	node->peer = NULL;
	if (tmp->data->pxdy) free(tmp->data->pxdy); 
	if (tmp->data->manufacturer) free(tmp->data->manufacturer);
	if (tmp->data->name) free(tmp->data->name); 
	if (tmp->data->partnumber) free(tmp->data->partnumber); 
	if (tmp->data->version) free(tmp->data->version); 
	if (tmp->data->serial) free(tmp->data->serial); 
	if (tmp) free(tmp);
    }

    node = sys.fru_dimm_head;

    while (node) {
	if (verbose) printf("pxdy = %s\n",node->data->pxdy);
	tmp = node;
	if (tmp->peer) tmp->peer = NULL;
	if (tmp->data->pxdy) free(tmp->data->pxdy); 
	if (tmp->data->manufacturer) free(tmp->data->manufacturer);
	if (tmp->data->name) free(tmp->data->name); 
	if (tmp->data->partnumber) free(tmp->data->partnumber); 
	if (tmp->data->version) free(tmp->data->version); 
	if (tmp->data->serial) free(tmp->data->serial); 
	if (tmp) free(tmp);
	node = node->next;
    }

    node = sys.dmi_dimm_head;

    while (node) {
	if (verbose) printf("pxdy = %s\n",node->data->pxdy);
	tmp = node;
	if (tmp->peer) tmp->peer = NULL;
	if (! tmp->data) continue;
	if (tmp->data->manufacturer) free(tmp->data->manufacturer);
	if (tmp->data->name) free(tmp->data->name); 
	if (tmp->data->partnumber) free(tmp->data->partnumber); 
	if (tmp->data->version) free(tmp->data->version); 
	if (tmp->data->serial) free(tmp->data->serial); 
	if (tmp) free(tmp);
	node = node->next;
    }
}

#define	SMBIOS "/usr/sbin/dmidecode"

char *
platform_name()
{
	char fname[] = "/tmp/_platform.XXXXXX";
	char command[512];
	int fd;
	static	char buf[128];
	FILE *fp;

	memset(command, 0, sizeof (command));
	memset(buf, 0, sizeof (buf));
	if (access(SMBIOS, X_OK)  == 0) {
        fd = mkstemp(fname);
	    sprintf(command,
"%s | grep Product | head -1 | sed -e 's!Product Name: !!g' -e 's! $!!g' -e 's!	!!g' > %s ",
		SMBIOS, fname);
	    (void) system(command);
	    if ((fp = fopen(fname, "r")) == NULL) {
		(void) fprintf(stderr,
		"Cannot open %s for read\n", fname);
		return (NULL);
	    }
	    memset(buf, 0, sizeof (buf));
	    fgets(buf, sizeof (buf), fp);
	    if (unlink(fname) < 0)
		printf("can't unlink %s\n", fname);
	    return (buf);
	}
	return (NULL);
}


/* fru                dimi locator                          */
/* ipmi_tag           bank   dimm     silkscreen   cpu dimm */
silkscreen_t  sunfirev40z[] = {
  { "cpu0.mem0.vpd", "CPU0", "DIMM0", "CPU0 DIMM0", 0, 0 }, 
  { "cpu0.mem1.vpd", "CPU0", "DIMM1", "CPU0 DIMM1", 0, 1 }, 
  { "cpu0.mem2.vpd", "CPU0", "DIMM2", "CPU0 DIMM2", 0, 2 }, 
  { "cpu0.mem3.vpd", "CPU0", "DIMM3", "CPU0 DIMM3", 0, 3 }, 
  { "cpu1.mem0.vpd", "CPU1", "DIMM0", "CPU1 DIMM0", 1, 0 }, 
  { "cpu1.mem1.vpd", "CPU1", "DIMM1", "CPU1 DIMM1", 1, 1 }, 
  { "cpu1.mem2.vpd", "CPU1", "DIMM2", "CPU1 DIMM2", 1, 2 }, 
  { "cpu1.mem3.vpd", "CPU1", "DIMM3", "CPU1 DIMM3", 1, 3 }, 
  { "cpu2.mem0.vpd", "CPU2", "DIMM0", "CPU2 DIMM0", 2, 0 }, 
  { "cpu2.mem1.vpd", "CPU2", "DIMM1", "CPU2 DIMM1", 2, 1 }, 
  { "cpu2.mem2.vpd", "CPU2", "DIMM2", "CPU2 DIMM2", 2, 2 }, 
  { "cpu2.mem3.vpd", "CPU2", "DIMM3", "CPU2 DIMM3", 2, 3 }, 
  { "cpu3.mem0.vpd", "CPU3", "DIMM0", "CPU3 DIMM0", 3, 0 },
  { "cpu3.mem1.vpd", "CPU3", "DIMM1", "CPU3 DIMM1", 3, 1 }, 
  { "cpu3.mem2.vpd", "CPU3", "DIMM2", "CPU3 DIMM2", 3, 2 }, 
  { "cpu3.mem3.vpd", "CPU3", "DIMM3", "CPU3 DIMM3", 3, 3 }, 
  { NULL, NULL, NULL, NULL, -1, -1  },
};

silkscreen_t  sunfirev20z[] = {
  { "cpu0.mem0.vpd", "CPU0", "DIMM0", "CPU0 DIMM0", 0, 0 }, 
  { "cpu0.mem1.vpd", "CPU0", "DIMM1", "CPU0 DIMM1", 0, 1 }, 
  { "cpu0.mem2.vpd", "CPU0", "DIMM2", "CPU0 DIMM2", 0, 2 }, 
  { "cpu0.mem3.vpd", "CPU0", "DIMM3", "CPU0 DIMM3", 0, 3 }, 
  { "cpu1.mem0.vpd", "CPU1", "DIMM0", "CPU1 DIMM0", 1, 0 }, 
  { "cpu1.mem1.vpd", "CPU1", "DIMM1", "CPU1 DIMM1", 1, 1 }, 
  { "cpu1.mem2.vpd", "CPU1", "DIMM2", "CPU1 DIMM2", 1, 2 }, 
  { "cpu1.mem3.vpd", "CPU1", "DIMM3", "CPU1 DIMM3", 1, 3 }, 
  { NULL, NULL, NULL, NULL, -1, -1  },
};

/* Sun Fire X2200 M2 */
silkscreen_t  sunfirex2200m2[] = {
  { "-", "CPU0", "DIMM7", "CPU0 DIMM7", 0, 0 }, 
  { "-", "CPU0", "DIMM6", "CPU0 DIMM6", 0, 1 }, 
  { "-", "CPU0", "DIMM5", "CPU0 DIMM5", 0, 2 }, 
  { "-", "CPU0", "DIMM4", "CPU0 DIMM4", 0, 3 }, 
  { "-", "CPU0", "DIMM3", "CPU0 DIMM3", 0, 4 }, 
  { "-", "CPU0", "DIMM2", "CPU0 DIMM2", 0, 5 }, 
  { "-", "CPU0", "DIMM1", "CPU0 DIMM1", 0, 6 }, 
  { "-", "CPU0", "DIMM0", "CPU0 DIMM0", 0, 7 }, 
  { "-", "CPU1", "DIMM7", "CPU1 DIMM7", 1, 0 }, 
  { "-", "CPU1", "DIMM6", "CPU1 DIMM6", 1, 1 }, 
  { "-", "CPU1", "DIMM5", "CPU1 DIMM5", 1, 2 }, 
  { "-", "CPU1", "DIMM4", "CPU1 DIMM4", 1, 3 }, 
  { "-", "CPU1", "DIMM3", "CPU1 DIMM3", 1, 4 },
  { "-", "CPU1", "DIMM2", "CPU1 DIMM2", 1, 5 }, 
  { "-", "CPU1", "DIMM1", "CPU1 DIMM1", 1, 6 }, 
  { "-", "CPU1", "DIMM0", "CPU1 DIMM0", 1, 7 }, 
  { NULL, NULL, NULL, NULL, -1, -1  },
};


/* Sun Fire X4600 M2  - G4F */
/* fru                dimi locator                          */
/* ipmi_tag           bank   dimm     silkscreen   cpu dimm */
silkscreen_t  sunfirex4600m2[] = {
  { "p0.d0.fru", "BANK0", "DIMM0", "CPU0 DIMM0", 0, 0 },
  { "p0.d1.fru", "BANK1", "DIMM1", "CPU0 DIMM1", 0, 1 },
  { "p0.d2.fru", "BANK2", "DIMM2", "CPU0 DIMM2", 0, 2 },
  { "p0.d3.fru", "BANK3", "DIMM3", "CPU0 DIMM3", 0, 3 },
  { "p0.d4.fru", "BANK4", "DIMM4", "CPU0 DIMM4", 0, 4 },
  { "p0.d5.fru", "BANK5", "DIMM5", "CPU0 DIMM5", 0, 5 },
  { "p0.d6.fru", "BANK6", "DIMM6", "CPU0 DIMM6", 0, 6 },
  { "p0.d7.fru", "BANK7", "DIMM7", "CPU0 DIMM7", 0, 7 },
  { "p1.d0.fru", "BANK8", "DIMM8", "CPU1 DIMM0", 1, 0 },
  { "p1.d1.fru", "BANK9", "DIMM9", "CPU1 DIMM1", 1, 1 },
  { "p1.d2.fru", "BANK10", "DIMM10", "CPU1 DIMM2", 1, 2 },
  { "p1.d3.fru", "BANK11", "DIMM11", "CPU1 DIMM3", 1, 3 },
  { "p1.d4.fru", "BANK12", "DIMM12", "CPU1 DIMM4", 1, 4 },
  { "p1.d5.fru", "BANK13", "DIMM13", "CPU1 DIMM5", 1, 5 },
  { "p1.d6.fru", "BANK14", "DIMM14", "CPU1 DIMM6", 1, 6 },
  { "p1.d7.fru", "BANK15", "DIMM15", "CPU1 DIMM7", 1, 7 },
  { "p2.d0.fru", "BANK16", "DIMM16", "CPU2 DIMM0", 2, 0 },
  { "p2.d1.fru", "BANK17", "DIMM17", "CPU2 DIMM1", 2, 1 },
  { "p2.d2.fru", "BANK18", "DIMM18", "CPU2 DIMM2", 2, 2 },
  { "p2.d3.fru", "BANK19", "DIMM19", "CPU2 DIMM3", 2, 3 },
  { "p2.d4.fru", "BANK20", "DIMM20", "CPU2 DIMM4", 2, 4 },
  { "p2.d5.fru", "BANK21", "DIMM21", "CPU2 DIMM5", 2, 5 },
  { "p2.d6.fru", "BANK22", "DIMM22", "CPU2 DIMM6", 2, 6 },
  { "p2.d7.fru", "BANK23", "DIMM23", "CPU2 DIMM7", 2, 7 },
  { "p3.d0.fru", "BANK24", "DIMM24", "CPU3 DIMM0", 3, 0 },
  { "p3.d1.fru", "BANK25", "DIMM25", "CPU3 DIMM1", 3, 1 },
  { "p3.d2.fru", "BANK26", "DIMM26", "CPU3 DIMM2", 3, 2 },
  { "p3.d3.fru", "BANK27", "DIMM27", "CPU3 DIMM3", 3, 3 },
  { "p3.d4.fru", "BANK28", "DIMM28", "CPU3 DIMM4", 3, 4 },
  { "p3.d5.fru", "BANK29", "DIMM29", "CPU3 DIMM5", 3, 5 },
  { "p3.d6.fru", "BANK30", "DIMM30", "CPU3 DIMM6", 3, 6 },
  { "p3.d7.fru", "BANK31", "DIMM31", "CPU3 DIMM7", 3, 7 },
  { "p4.d0.fru", "BANK32", "DIMM32", "CPU4 DIMM0", 4, 0 },
  { "p4.d1.fru", "BANK33", "DIMM33", "CPU4 DIMM1", 4, 1 },
  { "p4.d2.fru", "BANK34", "DIMM34", "CPU4 DIMM2", 4, 2 },
  { "p4.d3.fru", "BANK35", "DIMM35", "CPU4 DIMM3", 4, 3 },
  { "p4.d4.fru", "BANK36", "DIMM36", "CPU4 DIMM4", 4, 4 },
  { "p4.d5.fru", "BANK37", "DIMM37", "CPU4 DIMM5", 4, 5 },
  { "p4.d6.fru", "BANK38", "DIMM38", "CPU4 DIMM6", 4, 6 },
  { "p4.d7.fru", "BANK39", "DIMM39", "CPU4 DIMM7", 4, 7 },
  { "p5.d0.fru", "BANK40", "DIMM40", "CPU5 DIMM0", 5, 0 },
  { "p5.d1.fru", "BANK41", "DIMM41", "CPU5 DIMM1", 5, 1 },
  { "p5.d2.fru", "BANK42", "DIMM42", "CPU5 DIMM2", 5, 2 },
  { "p5.d3.fru", "BANK43", "DIMM43", "CPU5 DIMM3", 5, 3 },
  { "p5.d4.fru", "BANK44", "DIMM44", "CPU5 DIMM4", 5, 4 },
  { "p5.d5.fru", "BANK45", "DIMM45", "CPU5 DIMM5", 5, 5 },
  { "p5.d6.fru", "BANK46", "DIMM46", "CPU5 DIMM6", 5, 6 },
  { "p5.d7.fru", "BANK47", "DIMM47", "CPU5 DIMM7", 5, 7 },
  { "p6.d0.fru", "BANK48", "DIMM48", "CPU6 DIMM0", 6, 0 },
  { "p6.d1.fru", "BANK49", "DIMM49", "CPU6 DIMM1", 6, 1 },
  { "p6.d2.fru", "BANK50", "DIMM50", "CPU6 DIMM2", 6, 2 },
  { "p6.d3.fru", "BANK51", "DIMM51", "CPU6 DIMM3", 6, 3 },
  { "p6.d4.fru", "BANK52", "DIMM52", "CPU6 DIMM4", 6, 4 },
  { "p6.d5.fru", "BANK53", "DIMM53", "CPU6 DIMM5", 6, 5 },
  { "p6.d6.fru", "BANK54", "DIMM54", "CPU6 DIMM6", 6, 6 },
  { "p6.d7.fru", "BANK55", "DIMM55", "CPU6 DIMM7", 6, 7 },
  { "p7.d0.fru", "BANK56", "DIMM56", "CPU7 DIMM0", 7, 0 },
  { "p7.d1.fru", "BANK57", "DIMM57", "CPU7 DIMM1", 7, 1 },
  { "p7.d2.fru", "BANK58", "DIMM58", "CPU7 DIMM2", 7, 2 },
  { "p7.d3.fru", "BANK59", "DIMM59", "CPU7 DIMM3", 7, 3 },
  { "p7.d4.fru", "BANK60", "DIMM60", "CPU7 DIMM4", 7, 4 },
  { "p7.d5.fru", "BANK61", "DIMM61", "CPU7 DIMM5", 7, 5 },
  { "p7.d6.fru", "BANK62", "DIMM62", "CPU7 DIMM6", 7, 6 },
  { "p7.d7.fru", "BANK63", "DIMM63", "CPU7 DIMM7", 7, 7 },
  { NULL, NULL, NULL, NULL, -1, -1  },
};

/* Sun Fire X4600 - G4e */
/* fru                dimi locator                          */
/* ipmi_tag           bank   dimm     silkscreen   cpu dimm */
silkscreen_t  sunfirex4600[] = {
  { "p0.d0.fru", "BANK0", "DIMM0", "CPU0 DIMM0", 0, 0 },
  { "p0.d1.fru", "BANK1", "DIMM1", "CPU0 DIMM1", 0, 1 },
  { "p0.d2.fru", "BANK2", "DIMM2", "CPU0 DIMM2", 0, 2 },
  { "p0.d3.fru", "BANK3", "DIMM3", "CPU0 DIMM3", 0, 3 },
  { "p1.d0.fru", "BANK4", "DIMM4", "CPU1 DIMM0", 0, 0 },
  { "p1.d1.fru", "BANK5", "DIMM5", "CPU1 DIMM1", 1, 1 },
  { "p1.d2.fru", "BANK6", "DIMM6", "CPU1 DIMM2", 1, 2 },
  { "p1.d3.fru", "BANK7", "DIMM7", "CPU1 DIMM3", 1, 3 },
  { "p2.d0.fru", "BANK8", "DIMM8", "CPU2 DIMM0", 2, 0 },
  { "p2.d1.fru", "BANK9", "DIMM9", "CPU2 DIMM1", 2, 1 },
  { "p2.d2.fru", "BANK10", "DIMM10", "CPU2 DIMM2", 2, 2 },
  { "p2.d3.fru", "BANK11", "DIMM11", "CPU2 DIMM3", 2, 3 },
  { "p3.d0.fru", "BANK12", "DIMM12", "CPU3 DIMM0", 3, 0 },
  { "p3.d1.fru", "BANK13", "DIMM13", "CPU3 DIMM1", 3, 1 },
  { "p3.d2.fru", "BANK14", "DIMM14", "CPU3 DIMM2", 3, 2 },
  { "p3.d3.fru", "BANK15", "DIMM15", "CPU3 DIMM3", 3, 3 },
  { "p4.d0.fru", "BANK16", "DIMM16", "CPU4 DIMM0", 4, 0 },
  { "p4.d1.fru", "BANK17", "DIMM17", "CPU4 DIMM1", 4, 1 },
  { "p4.d2.fru", "BANK18", "DIMM18", "CPU4 DIMM2", 4, 2 },
  { "p4.d3.fru", "BANK19", "DIMM19", "CPU4 DIMM3", 4, 3 },
  { "p5.d0.fru", "BANK20", "DIMM20", "CPU5 DIMM0", 5, 0 },
  { "p5.d1.fru", "BANK21", "DIMM21", "CPU5 DIMM1", 5, 1 },
  { "p5.d2.fru", "BANK22", "DIMM22", "CPU5 DIMM2", 5, 2 },
  { "p5.d3.fru", "BANK23", "DIMM23", "CPU5 DIMM3", 5, 3 },
  { "p6.d0.fru", "BANK24", "DIMM24", "CPU6 DIMM0", 6, 0 },
  { "p6.d1.fru", "BANK25", "DIMM25", "CPU6 DIMM1", 6, 1 },
  { "p6.d2.fru", "BANK26", "DIMM26", "CPU6 DIMM2", 6, 2 },
  { "p6.d3.fru", "BANK27", "DIMM27", "CPU6 DIMM3", 6, 3 },
  { "p7.d0.fru", "BANK28", "DIMM28", "CPU7 DIMM0", 7, 0 },
  { "p7.d1.fru", "BANK29", "DIMM29", "CPU7 DIMM1", 7, 1 },
  { "p7.d2.fru", "BANK30", "DIMM30", "CPU7 DIMM2", 7, 2 },
  { "p7.d3.fru", "BANK31", "DIMM31", "CPU7 DIMM3", 7, 3 },
  { NULL, NULL, NULL, NULL, -1, -1  },
};



/* System Product Name: Sun Fire X4200 Server */
/* fru                dimi locator                          */
/* ipmi_tag           bank   dimm     silkscreen   cpu dimm */
silkscreen_t  sunfirex4200server[] = {
  { "p0.d0.fru", "BANK0", "H0_DIMM0", "CPU0 DIMM0", 0, 0 },
  { "p0.d1.fru", "BANK1", "H0_DIMM1", "CPU0 DIMM1", 0, 1 },
  { "p0.d2.fru", "BANK2", "H0_DIMM2", "CPU0 DIMM2", 0, 2 },
  { "p0.d3.fru", "BANK3", "H0_DIMM3", "CPU0 DIMM3", 0, 3 },
  { "p1.d0.fru", "BANK4", "H1_DIMM0", "CPU1 DIMM0", 1, 0 },
  { "p1.d1.fru", "BANK5", "H1_DIMM1", "CPU1 DIMM1", 1, 1 },
  { "p1.d2.fru", "BANK6", "H1_DIMM2", "CPU1 DIMM2", 1, 2 },
  { "p1.d3.fru", "BANK7", "H1_DIMM3", "CPU1 DIMM3", 1, 3 },
  { NULL, NULL, NULL, NULL, -1, -1  },
};

/* sunfirex4540 */ 
/* fru                dimi locator                          */
/* ipmi_tag           bank   dimm     silkscreen   cpu dimm */
silkscreen_t  sunfirex4540[] = {
  { "p0.d0.fru", "BANK0", "DIMM0", "CPU0 D0", 0, 0 },
  { "p0.d1.fru", "BANK1", "DIMM1", "CPU0 D1", 0, 1 },
  { "p0.d2.fru", "BANK2", "DIMM2", "CPU0 D2", 0, 2 },
  { "p0.d3.fru", "BANK3", "DIMM3", "CPU0 D3", 0, 3 },
  { "p0.d4.fru", "BANK4", "DIMM4", "CPU0 D4", 0, 4 },
  { "p0.d5.fru", "BANK5", "DIMM5", "CPU0 D5", 0, 5 },
  { "p0.d6.fru", "BANK6", "DIMM6", "CPU0 D6", 0, 6 },
  { "p0.d7.fru", "BANK7", "DIMM7", "CPU0 D7", 0, 7 },
  { "p1.d0.fru", "BANK8", "DIMM8", "CPU1 D0", 1, 0 },
  { "p1.d1.fru", "BANK9", "DIMM9", "CPU1 D1", 1, 1 },
  { "p1.d2.fru", "BANK10", "DIMM10", "CPU1 D2", 1, 2 },
  { "p1.d3.fru", "BANK11", "DIMM11", "CPU1 D3", 1, 3 },
  { "p1.d4.fru", "BANK12", "DIMM12", "CPU1 D4", 1, 4 },
  { "p1.d5.fru", "BANK13", "DIMM13", "CPU1 D5", 1, 5 },
  { "p1.d6.fru", "BANK14", "DIMM14", "CPU1 D6", 1, 6 },
  { "p1.d7.fru", "BANK15", "DIMM15", "CPU1 D7", 1, 7 },
  { NULL, NULL, NULL, NULL, -1, -1  },
};

/* sunbladex6420servermodule  - pegasus */
/* fru                dimi locator                          */
/* ipmi_tag   bank   dimm     silkscreen   cpu dimm */
silkscreen_t  sunbladex6420[] = {
  { "p0.d0.fru", "BANK0", "DIMM0", "CPU0 d0", 0, 0 },
  { "p0.d1.fru", "BANK1", "DIMM1", "CPU0 d1", 0, 1 },
  { "p0.d2.fru", "BANK2", "DIMM2", "CPU0 d2", 0, 2 },
  { "p0.d4.fru", "BANK3", "DIMM3", "CPU0 d3", 0, 3 },
  { "p0.d5.fru", "BANK4", "DIMM4", "CPU0 d4", 0, 4 },
  { "p0.d6.fru", "BANK5", "DIMM5", "CPU0 d5", 0, 5 },
  { "p0.d3.fru", "BANK6", "DIMM6", "CPU0 d6", 0, 5 },
  { "p0.d7.fru", "BANK7", "DIMM7", "CPU0 d7", 0, 7 },
  { "p1.d0.fru", "-", "-", "CPU1 d0", 1, 0 }, /* pegasus smbios ? */
  { "p1.d1.fru", "-", "-", "CPU1 d1", 1, 1 },
  { "p1.d2.fru", "-", "-", "CPU1 d2", 1, 2 },
  { "p1.d3.fru", "-", "-", "CPU1 d3", 1, 3 },
  { "p1.d4.fru", "-", "-", "CPU1 d4", 1, 4 },
  { "p1.d5.fru", "-", "-", "CPU1 d5", 1, 5 },
  { "p1.d6.fru", "-", "-", "CPU1 d6", 1, 6 },
  { "p1.d7.fru", "-", "-", "CPU1 d7", 1, 7 },
  { "p2.d0.fru", "-", "-", "CPU2 d0", 2, 0 },
  { "p2.d1.fru", "-", "-", "CPU2 d1", 2, 1 },
  { "p2.d2.fru", "-", "-", "CPU2 d2", 2, 2 },
  { "p2.d3.fru", "-", "-", "CPU2 d3", 2, 3 },
  { "p2.d4.fru", "-", "-", "CPU2 d4", 2, 4 },
  { "p2.d5.fru", "-", "-", "CPU2 d5", 2, 5 },
  { "p2.d6.fru", "-", "-", "CPU2 d6", 2, 6 },
  { "p2.d7.fru", "-", "-", "CPU2 d7", 2, 7 },
  { "p3.d0.fru", "-", "-", "CPU3 d0", 3, 0 },
  { "p3.d1.fru", "-", "-", "CPU3 d1", 3, 1 },
  { "p3.d2.fru", "-", "-", "CPU3 d2", 3, 2 },
  { "p3.d3.fru", "-", "-", "CPU3 d3", 3, 3 },
  { "p3.d4.fru", "-", "-", "CPU3 d4", 3, 4 },
  { "p3.d5.fru", "-", "-", "CPU3 d5", 3, 5 },
  { "p3.d6.fru", "-", "-", "CPU3 d6", 3, 6 },
  { "p3.d7.fru", "-", "-", "CPU3 d7", 3, 7 },
  { NULL, NULL, NULL, NULL, -1, -1  },
};

/* sunbladex6220servermodule  - Gemini */
/* fru                dimi locator                          */
/* ipmi_tag   bank   dimm     silkscreen   cpu dimm */
silkscreen_t  sunbladex6220[] = {
  { "p0.d0.fru", " NODE0", "DIMM0", "CPU0 d0", 0, 0 },
  { "p0.d1.fru", " NODE0", "DIMM1", "CPU0 d1", 0, 1 },
  { "p0.d2.fru", " NODE0", "DIMM2", "CPU0 d2", 0, 2 },
  { "p0.d3.fru", " NODE0", "DIMM3", "CPU0 d3", 0, 3 },
  { "p0.d4.fru", " NODE0", "DIMM4", "CPU0 d4", 0, 4 },
  { "p0.d5.fru", " NODE0", "DIMM5", "CPU0 d5", 0, 5 },
  { "p0.d6.fru", " NODE0", "DIMM6", "CPU0 d6", 0, 6 },
  { "p0.d7.fru", " NODE0", "DIMM7", "CPU0 d7", 0, 7 },
  { "p1.d0.fru", " NODE1", "DIMM0", "CPU1 d0", 1, 0 },
  { "p1.d1.fru", " NODE1", "DIMM1", "CPU1 d1", 1, 1 },
  { "p1.d2.fru", " NODE1", "DIMM2", "CPU1 d2", 1, 2 },
  { "p1.d3.fru", " NODE1", "DIMM3", "CPU1 d3", 1, 3 },
  { "p1.d4.fru", " NODE1", "DIMM4", "CPU1 d4", 1, 4 },
  { "p1.d5.fru", " NODE1", "DIMM5", "CPU1 d5", 1, 5 },
  { "p1.d6.fru", " NODE1", "DIMM6", "CPU1 d6", 1, 6 },
  { "p1.d7.fru", " NODE1", "DIMM7", "CPU1 d7", 1, 7 },
  { NULL, NULL, NULL, NULL, -1, -1  },
};

/* x2100m2  - Taurus */
/* fru                dimi locator                          */
/* ipmi_tag   bank   dimm     silkscreen   cpu dimm */
silkscreen_t  x2100m2[] = {
  { "-", "BANK0", "DIMMA1", "DIMM1", 0, 0 },
  { "-", "BANK1", "DIMMB1", "DIMM2", 0, 1 },
  { "-", "BANK2", "DIMMA0", "DIMM3", 0, 2 },
  { "-", "BANK3", "DIMMB0", "DIMM4", 0, 3 },
  { NULL, NULL, NULL, NULL, -1, -1  },
};

/* W1100z/2100z   - Metropolis */
/* 4 physical dimm slot with 8 logical smbios entries ? */
/* fru                dimi locator                          */
/* ipmi_tag   bank   dimm     silkscreen   cpu dimm */
silkscreen_t  W1100z_2100z[] = {
  { "-", "Bank 0", "DIMM1", "DIMM1", 0, 0 }, /* pair ? */
  { "-", "Bank 0", "DIMM2", "DIMM2", 0, 1 },
  { "-", "Bank 1", "DIMM3", "DIMM3", 0, 2 },
  { "-", "Bank 1", "DIMM4", "DIMM4", 0, 3 },
  { "-", "Bank 2", "DIMM5", "DIMM5", 1, 0 },
  { "-", "Bank 2", "DIMM6", "DIMM6", 1, 1 },
  { "-", "Bank 3", "DIMM7", "DIMM7", 1, 2 },
  { "-", "Bank 3", "DIMM8", "DIMM8", 1, 3 },
  { NULL, NULL, NULL, NULL, -1, -1  },
};

/* sunfirex4500 - thumper */
/* fru                dimi locator                          */
/* ipmi_tag           bank   dimm     silkscreen   cpu dimm */
silkscreen_t  sunfirex4500[] = {
  { "p0.d0.fru", "BANK0", "H0_DIMM0", "CPU0 DIMM0", 0, 0 },
  { "p0.d1.fru", "BANK1", "H0_DIMM1", "CPU0 DIMM1", 0, 1 },
  { "p0.d2.fru", "BANK2", "H0_DIMM2", "CPU0 DIMM2", 0, 2 },
  { "p0.d3.fru", "BANK3", "H0_DIMM3", "CPU0 DIMM3", 0, 3 },
  { "p1.d0.fru", "BANK4", "H1_DIMM0", "CPU1 DIMM0", 1, 0 },
  { "p1.d1.fru", "BANK5", "H1_DIMM1", "CPU1 DIMM1", 1, 1 },
  { "p1.d2.fru", "BANK6", "H1_DIMM2", "CPU1 DIMM2", 1, 2 },
  { "p1.d3.fru", "BANK7", "H1_DIMM3", "CPU1 DIMM3", 1, 3 },
  { NULL, NULL, NULL, NULL, -1, -1  },
};

 
/* sunultra20workstation - Marrakesh */
/* fru                dimi locator                          */
/* ipmi_tag           bank   dimm     silkscreen   cpu dimm */
silkscreen_t  sunultra20workstation[] = {
  { "-", "Bank0/1", "A0", "DIMM 1", 0, 0 },
  { "-", "Bank2/3", "A1", "DIMM 2", 0, 1 },
  { "-", "Bank4/5", "A2", "DIMM 3", 0, 2 },
  { "-", "Bank6/7", "A3", "DIMM 4", 0, 3 },
  { NULL, NULL, NULL, NULL, -1, -1  },
};

/* ultra20m2 - Munich */
/* fru                dimi locator                          */
/* ipmi_tag           bank   dimm     silkscreen   cpu dimm */
silkscreen_t ultra20m2[] = {
  { "-", "Bank 0", "DIMM 0", "DIMM A0", 0, 0 },
  { "-", "Bank 1", "DIMM 1", "DIMM B0", 0, 1 },
  { "-", "Bank 2", "DIMM 2", "DIMM A1", 0, 2 },
  { "-", "Bank 3", "DIMM 3", "DIMM B1", 0, 3 },
  { NULL, NULL, NULL, NULL, -1, -1  },
};

/* sunfirex4440 -  Tucana 2U - DTa */
/* fru                dimi locator                          */
/* ipmi_tag           bank   dimm     silkscreen   cpu dimm */
silkscreen_t  sunfirex4440[] = {
  { "p0.d0.fru", "BANK0", "DIMM0", "CPU0 D0", 0, 0 },
  { "p0.d1.fru", "BANK1", "DIMM1", "CPU0 D1", 0, 1 },
  { "p0.d2.fru", "BANK2", "DIMM2", "CPU0 D2", 0, 2 },
  { "p0.d3.fru", "BANK3", "DIMM3", "CPU0 D3", 0, 3 },
  { "p0.d4.fru", "BANK4", "DIMM4", "CPU0 D4", 0, 4 },
  { "p0.d5.fru", "BANK5", "DIMM5", "CPU0 D5", 0, 5 },
  { "p0.d6.fru", "BANK6", "DIMM6", "CPU0 D6", 0, 6 },
  { "p0.d7.fru", "BANK7", "DIMM7", "CPU0 D7", 0, 7 },
  { "p1.d0.fru", "BANK8", "DIMM8", "CPU1 D0", 1, 0 },
  { "p1.d1.fru", "BANK9", "DIMM9", "CPU1 D1", 1, 1 },
  { "p1.d2.fru", "BANK10", "DIMM10", "CPU1 D2", 1, 2 },
  { "p1.d3.fru", "BANK11", "DIMM11", "CPU1 D3", 1, 3 },
  { "p1.d4.fru", "BANK12", "DIMM12", "CPU1 D4", 1, 4 },
  { "p1.d5.fru", "BANK13", "DIMM13", "CPU1 D5", 1, 5 },
  { "p1.d6.fru", "BANK14", "DIMM14", "CPU1 D6", 1, 6 },
  { "p1.d7.fru", "BANK15", "DIMM15", "CPU1 D7", 1, 7 },
  { "p2.d0.fru", "BANK16", "DIMM16", "CPU2 D0", 2, 0 },
  { "p2.d1.fru", "BANK17", "DIMM17", "CPU2 D1", 2, 1 },
  { "p2.d2.fru", "BANK18", "DIMM18", "CPU2 D2", 2, 2 },
  { "p2.d3.fru", "BANK19", "DIMM19", "CPU2 D3", 2, 3 },
  { "p2.d4.fru", "BANK20", "DIMM20", "CPU2 D4", 2, 4 },
  { "p2.d5.fru", "BANK21", "DIMM21", "CPU2 D5", 2, 5 },
  { "p2.d6.fru", "BANK22", "DIMM22", "CPU2 D6", 2, 6 },
  { "p2.d7.fru", "BANK23", "DIMM23", "CPU2 D7", 2, 7 },
  { "p3.d0.fru", "BANK24", "DIMM24", "CPU3 D0", 3, 0 },
  { "p3.d1.fru", "BANK25", "DIMM25", "CPU3 D1", 3, 1 },
  { "p3.d2.fru", "BANK26", "DIMM26", "CPU3 D2", 3, 2 },
  { "p3.d3.fru", "BANK27", "DIMM27", "CPU3 D3", 3, 3 },
  { "p3.d4.fru", "BANK28", "DIMM28", "CPU3 D4", 3, 4 },
  { "p3.d5.fru", "BANK29", "DIMM29", "CPU3 D5", 3, 5 },
  { "p3.d6.fru", "BANK30", "DIMM30", "CPU3 D6", 3, 6 },
  { "p3.d7.fru", "BANK31", "DIMM31", "CPU3 D7", 3, 7 },
  { NULL, NULL, NULL, NULL, -1, -1  },
};

/* cpu = HT node, dimm = (CS & ~1) + DCT */
/* Penguin Computing Altus 1800 */ 
/* fru             dmi     locator                          */
/* ipmi_tag       bank      dimm       silkscreen  cpu dimm */
silkscreen_t  PenguinAltus1800[] = {
  { "-",        "BANK1",  "P1_DIMM1A", "P1_DIMM1A", 0, 2 },
  { "-",        "BANK0",  "P1_DIMM1B", "P1_DIMM1B", 0, 0 },
  { "-",        "BANK3",  "P1_DIMM2A", "P1_DIMM2A", 0, 3 },
  { "-",        "BANK2",  "P1_DIMM2B", "P1_DIMM2B", 0, 1 },
  { "-",        "BANK5",  "P1_DIMM3A", "P1_DIMM3A", 1, 2 },
  { "-",        "BANK4",  "P1_DIMM3B", "P1_DIMM3B", 1, 0 },
  { "-",        "BANK7",  "P1_DIMM4A", "P1_DIMM4A", 1, 3 },
  { "-",        "BANK6",  "P1_DIMM4B", "P1_DIMM4B", 1, 1 },
  { "-",        "BANK9",  "P2_DIMM1A", "P2_DIMM1A", 3, 2 },
  { "-",        "BANK8",  "P2_DIMM1B", "P2_DIMM1B", 3, 0 },
  { "-",        "BANK11", "P2_DIMM2A", "P2_DIMM2A", 3, 3 },
  { "-",        "BANK10", "P2_DIMM2B", "P2_DIMM2B", 3, 1 },
  { "-",        "BANK13", "P2_DIMM3A", "P2_DIMM3A", 2, 2 },
  { "-",        "BANK12", "P2_DIMM3B", "P2_DIMM3B", 2, 0 },
  { "-",        "BANK15", "P2_DIMM4A", "P2_DIMM4A", 2, 3 },
  { "-",        "BANK14", "P2_DIMM4B", "P2_DIMM4B", 2, 1 },
  { NULL,       NULL, NULL, NULL, -1, -1  },
};


map_t maps[] = {
  /* platform, alias, fru_lu1, fru_lu2, fru_lu3, dmi_lu1, dmi_lu2, silkscreen map, oname */
  { "sunfirev40z", "v40", "cpu", "mem", "vpd", "CPU", "DIMM", sunfirev40z, "stinger4" },
  { "sunfirev20z", "v20", "cpu", "mem", "vpd", "CPU", "DIMM", sunfirev20z, "stinger2" },
  { "sunfirex4600m2", "x4600m2", ".d", "p", "fru", "BANK", "DIMM", sunfirex4600m2, "galaxy4f" },
  { "sunfirex4600", "x4600", ".d", "p", "fru", "BANK", "DIMM", sunfirex4600, "galaxy4e" },
  { "sunfirex4200server", "x4200", ".d", "p", "fru", "BANK", "DIMM", sunfirex4200server, "galaxy2x" },
  { "sunfirex4100server", "x4100", ".d", "p", "fru", "BANK", "DIMM", sunfirex4200server, "galaxy1x" },

  { "sunfirex4440", "x4440", ".d", "p", "fru", "BANK", "DIMM", sunfirex4440, "Tucana 2U" },
  { "sunfirex4240", "x4240", ".d", "p", "fru", "BANK", "DIMM", sunfirex4440, "Dorado 2U" },
  { "sunfirex4140", "x4140", ".d", "p", "fru", "BANK", "DIMM", sunfirex4440, "Dorado 1U" },
  { "sunfirex4500", "x4500", ".d", "p", "fru", "BANK", "DIMM", sunfirex4500, "thumper" },
  { "sunfirex4540", "x4540", ".d", "p", "fru", "BANK", "DIMM", sunfirex4540, "thor" },
  { "sunbladex6420", "x6420", ".d", "p", "fru", "BANK", "DIMM", sunbladex6420, "pegasus" },
  { "sunbladex6220", "x6220", ".d", "p", "fru", "BANK", "DIMM", sunbladex6220, "gemini" },
  { "ultra20m2", "ultra20m2", ".d", "p", "fru", "BANK", "DIMM", ultra20m2, "Munich" },
  { "sunultra20workstation", "ultra20w", ".d", "p", "fru", "BANK", "DIMM", 
	sunultra20workstation, "Marrakesh" },
  { "w1100z_2100z", "w1100z/2100z", ".d", "p", "fru", "BANK", "DIMM", W1100z_2100z, "Metropolis" },
  { "x2100m2", "x2100m2", ".d", "p", "fru", "BANK", "DIMM", x2100m2, "Taurus" },
  { "sunfirex2200m2", "x2200m2", "cpu", "mem", "vpd", "CPU", "DIMM", sunfirex2200m2, "taurus" },
  { "h8dgu", "altus1800,altus2800,h8dgu", ".d", "p", "fru", "BANK", "DIMM", PenguinAltus1800, "H8DGU" },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
};

typedef struct _p {
 char *name;
} p_t;
p_t plist[] = {
    { "p0.fru" },
    { "p1.fru" },
    { "p2.fru" },
    { "p3.fru" },
    { "p4.fru" },
    { "p5.fru" },
    { "p6.fru" },
    { "p7.fru" },
    { "cpu0.vpd" },
    { "cpu1.vpd" },
    { "cpu2.vpd" },
    { "cpu3.vpd" },
    { NULL }
};

/* update sys struct based on the sys.sys_product_name */
void
update_map()
{
    int i;
    char *platform = NULL;

    memset(&sys, 0, sizeof(sys_t));
    sys.imap = -1;
    sys.fru_lookup1 = DEFAULT_FRU_LOOKUP1;
    sys.fru_lookup2 = DEFAULT_FRU_LOOKUP2;
    sys.fru_lookup3 = DEFAULT_FRU_LOOKUP3;
    sys.sys_product_name = platform_name();
    if (sys.sys_product_name) {
	platform = strip(sys.sys_product_name); /* normalize */
	if (platform) {
	    sys.platform = strdup(platform);
	    for (i = 0; maps[i].platform != NULL; i++) {
		if (verbose)
		    printf("platform alias = %s\n", maps[i].alias);
		if (strstr(maps[i].alias, platform)) {
		    if (verbose) 
			printf("found alias = %s in name=%s, i=%d\n",
		            maps[i].alias, platform, i);
		    sys.imap = i;
		    if (maps[i].fru_lookup1) 
			sys.fru_lookup1 = maps[i].fru_lookup1;
		    if (maps[i].fru_lookup2) 
			sys.fru_lookup2 = maps[i].fru_lookup2;
		    if (maps[i].fru_lookup3) 
			sys.fru_lookup3 = maps[i].fru_lookup3;
		    if (maps[i].dmi_lookup1) 
			sys.dmi_lookup1 = maps[i].dmi_lookup1;
		    if (maps[i].dmi_lookup2) 
			sys.dmi_lookup2 = maps[i].dmi_lookup2;
		    break;
		}
	    }
	} 
    }
    if (verbose) {
    if (sys.fru_lookup1) printf ("fru_lookup1 = %s\n", sys.fru_lookup1);
    if (sys.fru_lookup2) printf ("fru_lookup2 = %s\n", sys.fru_lookup2);
    if (sys.fru_lookup3) printf ("fru_lookup3 = %s\n", sys.fru_lookup3);
    if (sys.dmi_lookup1) printf ("dmi_lookup1 = %s\n", sys.dmi_lookup1);
    if (sys.dmi_lookup2) printf ("dmi_lookup2 = %s\n", sys.dmi_lookup2);
    }
    if (sys.imap == -1) {
	    if (verbose) printf (" %s not supported yet\n", sys.platform);
    }
}


char *
dimm_map(int cpu, int dimm)
{
    int i = sys.imap;
    int j;
    char *status = NULL;

    update_map(); /* based on smbios' product name */

    if (sys.imap == -1) return (status);
    i = sys.imap;
    if (verbose) printf("platform = %s\n", maps[i].platform);

    for (j = 0; maps[i].map[j].silkscreen != NULL; j++) {
	if ((maps[i].map[j].cpu == cpu) &&
	    (maps[i].map[j].dimm == dimm)) {
		if (verbose) printf(" cpu=%d, dimm=%d, silkscreen=%s \n",
			maps[i].map[j].cpu,
			maps[i].map[j].dimm,
			maps[i].map[j].silkscreen);
		status = maps[i].map[j].silkscreen;
		break;
	}
    }
    cleanup_dimm();
    return (status);
}
