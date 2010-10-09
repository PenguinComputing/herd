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
#include <errno.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "config.h"
#include "log.h"
#include "options.h"
#include "mce.h"
#include "oipmi.h"
#include "pci.h"
#include "dimm.h"
#include "dimm_lib.h"

int main(int argc, char** argv)
{
    options_process(argc, argv);

    if (getuid() != 0 && !options.decode_mode) {
	fprintf(stderr,
		"Herd must be run as root or with superuser privileges in "
		"order to collect necessary system configuration data.\n");
	exit(1);
    }

    // Special case: do a quick address decode and exit. No need to
    // initialize mce or ipmi stuff.
    if (options.decode_mode) {
	decode_input_t dinput;

        pci_init();
        printf("%012llx", options.decode_address);
	if (options.decode_syndrome)
	    printf(" (synd %x)", options.decode_syndrome);
	printf(": ");

	memset(&dinput, 0, sizeof(dinput));
	dinput.address = options.decode_address;
	dinput.syndrome = options.decode_syndrome;
        decode_result_t result;

        if (dimm_translate_address(&dinput, &result)) {

	    if (result.is_dimm_pair) {
		const char *silkscreen1 = NULL, *silkscreen2 = NULL;
		silkscreen1 = dimm_map(result.node, result.dimm * 2);
		silkscreen2 = dimm_map(result.node, (result.dimm * 2) + 1);
		if (silkscreen1 && silkscreen2) {
		    printf("Cpu Node %d, CS %d (\"%s\" and \"%s\")\n",
			   result.node, result.dimm * 2,
			   silkscreen1, silkscreen2);
		} else {
		    printf("Cpu Node %d, DIMM pair %d (DIMMs %d and %d)\n",
			   result.node, result.dimm,
			   result.dimm * 2, (result.dimm * 2) + 1);
		}
	    } else {
		const char* silkscreen = dimm_map(result.node, result.dimm);
		if (silkscreen) {
		    printf("Cpu Node %d, CS %d (\"%s\")", result.node, result.dimm,
			   silkscreen);
		} else {
		    printf("Cpu Node %d, DIMM %d", result.node, result.dimm);
		}
		if (result.bits)
		    printf(" (bits %016llx)", result.bits);
		printf("\n");
	    }
	} else {
            printf("Address is invalid\n");
	}

        pci_exit();
	options_free();
        exit(0);
    }

    // By default, daemonize the process.
    if (!options.no_daemon_flag && !options.seltest_mode) {
	if (daemon(0, 0) < 0)
	    log_fatal_errno("daemon() failed: %s", strerror(errno));

    }

    log_init();
    mce_init();
    oipmi_init();
    pci_init();
    
    if (options.seltest_mode) { 
      // Special case: SEL test mode. File a fake SEL and exit.
      oipmi_test(options.seltest_address, options.decode_syndrome);

    } else {

      // Main loop
      oipmi_main_loop();

    }

    pci_exit();
    oipmi_exit();
    mce_exit();
    log_exit();
    options_free();

    return 0;
}
