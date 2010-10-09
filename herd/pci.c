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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pci.h"
#include "log.h"
#include "options.h"
#include "opteron.h"
#include "set.h"

struct {

    int  num_nodes;
    OpteronInfo* cpu_info;

    const char* proc_path;

    Set* configs;

} self;

void* pci_read_configuration(int bus, int device, int function);

// Read from Extended PCI Configuration Space. We have to use the
// extended cf8/cfc mechanism here, current linux kernels don't map
// the extended space into memory yet.

u32 config_pci_read(int bus, int device, int function, int offset)
{
    char key[80];
    void *setvalue;
    u32* config;

    sprintf(key, "%d.%d.%d", bus, device, function);

    if (set_lookup(self.configs, key, &setvalue)) {
	config = (u32*)setvalue;
	return config[offset / 4];
    }

    config = pci_read_configuration(bus, device, function);
    if (!config)
	return 0;

    set_add(self.configs, key, (void*)config);

    return config[offset / 4];
}

unsigned int extended_pci_read(int bus, int device, int func, int offset)
{
    unsigned int tmp, value;
    
    tmp = 0x80000000 | ((offset & 0xf00) << 16) | ((bus & 0xff) << 16)
	| ((device & 0x1f) << 11) | ((func & 0x7) << 0x8) | (offset & 0xfc);

    outl_p(tmp, 0xcf8);
    value = inl_p(0xcfc);
    return value;
}

void extended_pci_write(int bus, int device, int func, int offset,
			unsigned int value)
{
    unsigned int tmp;
    
    tmp = 0x80000000 | ((offset & 0xf00) << 16) | ((bus & 0xff) << 16)
	| ((device & 0x1f) << 11) | ((func & 0x7) << 0x8) | (offset & 0xfc);

    outl_p(tmp, 0xcf8);
    outl_p(value, 0xcfc);
}

// Quad-Core specific initialization. For Quad-Core, we need access to
// extended PCI configuration space, and this requires some register
// settings.

void quadcore_init()
{
    int i;
    unsigned int data;

    assert(self.num_nodes > 0);

    if (iopl(3) != 0) {
	log_fatal("Unable to change I/O privilage to access system "
		  "information\n");
    }

    // Set cf8ExtCfg bit in all cpus
    for (i = 0; i < self.num_nodes; i++) {
	data = extended_pci_read(0, 24 + i, 3, 0x8c);
	data |= 0x00004000;
	extended_pci_write(0, 24 + i, 3, 0x8c, data);
    }
}

void pci_init()
{
    char buf[1024];

    memset(&self, 0, sizeof(self));

    self.configs = set_new(64);

    // Identify CPU. Identification can be forced by the force_cpu
    // parameter. If parameter was not set, use cpuid instruction.
    const char* force_cpu = options_get_string("force_cpu");

    if (force_cpu) {
	int f, m, s;

	int ret = sscanf(force_cpu, "%d,%d,%d", &f, &m, &s);
	if (ret == 3)
	    self.cpu_info = opteron_new_from_fms(f, m, s);
	else
	    log_fatal("Could not parse force_cpu parameter\n");
    }
    if (!self.cpu_info) {
	self.cpu_info = opteron_new();
    }

    // Read in the PCI configuration device list to obtain number of
    // available nodes.
    self.proc_path = options_get_string("proc_pci_devices");

    FILE* f = fopen(self.proc_path, "r");
    if (!f) {
	log_fatal_errno("Unable to read PCI information from %s",
			self.proc_path);
	return;
    }

    // The CPU node device ID to look for.
    unsigned optDevID = 0x1100;
    if (self.cpu_info->family == AMD_QUAD_CORE_FAMILY)
	optDevID = 0x1200;

    while (fgets(buf, sizeof(buf)-1, f)) {
	unsigned int dfn, vend, irq;
	unsigned int base_addr[6], sizes[6];
	unsigned rom_base_addr, rom_size;

	int cnt = sscanf(buf, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x "
			 "%x %x", &dfn, &vend, &irq,
			 base_addr, base_addr+1, base_addr+2,
			 base_addr+3, base_addr+4, base_addr+5,
			 &rom_base_addr,
			 sizes, sizes+1, sizes+2, sizes+3, sizes+4, sizes+5,
			 &rom_size);
	if (cnt != 17)
	    break;

	u32 device = (dfn >> 3) & 0x1f;
	// u32 function = (dfn & 0x07);
	u32 vendor_id = vend >> 16;
	u32 device_id = vend & 0xffff;
	if (vendor_id != 0x1022) continue;

	if (device_id < optDevID || device_id > (optDevID + 8))
	    continue;

	int node_id = device - 0x18;
	if (node_id < 0 || node_id >= 8) {
	    log_msg("pci_init(): unexpected node_id %d\n", node_id);
	    continue;
	}

	if (node_id + 1 > self.num_nodes)
	    self.num_nodes = node_id + 1;
    }

    fclose(f);

    if (self.num_nodes == 0) {
	log_fatal("Unable to obtain system information. HERD does not "
		  "support this architecture.\n");
    }	  

    if (self.cpu_info->family == AMD_QUAD_CORE_FAMILY) {
	quadcore_init();
    }

    if (options.debug_mode) {
	log_msg("%d core%s found, family %d, model %d, stepping %d "
		"(revision %c)\n",
		self.num_nodes, (self.num_nodes> 1 ? "s" : ""),
		self.cpu_info->family,
		self.cpu_info->model,
		self.cpu_info->stepping,
		self.cpu_info->revision);
	log_msg("CPU description: %s\n", self.cpu_info->description);
	if (self.cpu_info->cpu.show_config)
	    (*(self.cpu_info->cpu.show_config))();
    }
}

void* pci_read_configuration(int bus, int device, int function)
{
    struct stat st;
    char buf[512];
    sprintf(buf, "%s/%02x_%02x.%d", options_get_string("proc_pci_bus"),
	    bus, device, function);
    if (stat(buf, &st) != 0) {
	sprintf(buf, "%s/%02x/%02x.%d", options_get_string("proc_pci_bus"),
		bus, device, function);
	if (stat(buf, &st) != 0) {
	    return NULL;
	}
    }

    FILE* f = fopen(buf, "r");
    if (!f) {
	log_errno("Unable to read PCI configuration data from %s", buf);
	return NULL;
    }
    void* data = malloc(st.st_size);
    int n = fread(data, st.st_size, 1, f);

    if (n != 1) {
	log_errno("Unable to read the CPU PCI configuration data. You may not "
		  "have the necessary user privileges, please retry with "
		  "administrator root privileges.\n");
	return NULL;
    }
    fclose(f);
    return data;
}

void pci_exit()
{
    if (self.cpu_info)
	opteron_delete(self.cpu_info);
    if (self.configs)
	set_delete(self.configs, free);
}

unsigned int pci_read_config(int bus, int device, int function, int offset)
{
    // First case, regular configuration space read.
    if (offset < 0x100) {
	return config_pci_read(bus, device, function, offset);
    }

    // Second case, extended configuration read.
    return extended_pci_read(bus, device, function, offset);
}

int pci_num_nodes()
{
    return self.num_nodes;
}

int pci_opteron_family()
{
    return self.cpu_info->family;
}

char pci_opteron_revision()
{
    return self.cpu_info->revision;
}

Cpu* pci_get_cpu()
{
    return (Cpu*)(self.cpu_info);
}
