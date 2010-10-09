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
#include <unistd.h>
#include <getopt.h>
#include <assert.h>

#include "config.h"
#include "options.h"

Options options;

// Default parameters

static struct {
    const char* key;
    const char* defval;
    const char* description;
} defaults[] = {
    { "check_timer_secs", "30",
      "  Delay in seconds between MCE log checks" },

    { "proc_pci_devices", "/proc/bus/pci/devices",
      "  Path of procfs file containing PCI devices information" },

    { "proc_pci_bus", "/proc/bus/pci",
      "  Path of procfs directory containing PCI devices configuration data" },

    { "force_cpu", NULL,
      "  Sets the CPU version information. Should be formatted as\n  "
      "\"family,model,stepping\" with decimal values. If not set, "
      "the CPU version is\n  auto-detected" },

    { NULL, NULL, NULL }
};

void options_usage(char** argv);

// Initialize parameters data structure

void options_set_defaults()
{
    if (options.parameters == NULL) {
	options.parameters = set_new(64);
    }

    int i;
    for (i = 0; defaults[i].key; i++) {
	if (defaults[i].defval)
	    set_add(options.parameters, defaults[i].key,
		    strdup(defaults[i].defval));
    }
}


// Add a new parameter. The passed string is expected to be of the
// format "a=b", when both a and b can't be empty.

void options_add_parameter(const char* assignment)
{
    char* buf = (char*)alloca(strlen(assignment) + 1);
    strcpy(buf, assignment);
    char* equal = strchr(buf, '=');

    if (equal)
	*equal++ = 0;

    if (!equal || strlen(buf) == 0 || strlen(equal) == 0) {
	fprintf(stderr, "Parameter should be of the format \"key=value\"\n");
	return;
    }

    set_add(options.parameters, buf, strdup(equal));
}

void options_print_parameters()
{
    int i;
    for (i = 0; defaults[i].key; i++) {
	printf("%s ", defaults[i].key);
	if (defaults[i].defval)
	    printf(" [%s]", defaults[i].defval);
	printf("\n%s.\n\n", defaults[i].description);
    }
}

void options_process(int argc, char** argv)
{
    memset(&options, 0, sizeof(options));

    options_set_defaults();

    extern char* optarg;
    extern int optind;
    struct option long_options[] = {
	{ "nodaemon",    0, 0, 'D' },
	{ "debug",       0, 0, 'd' },
	{ "help",        0, 0, 'h' },
	{ "ignorenodev", 0, 0,  1  },
	{ "dmi",         0, 0,  2  },
	{ "filter",      0, 0,  3  },
	{ "user",        1, 0,  4  },
	{ "password",    1, 0,  5  },
	{ "ip",          1, 0,  6  },
	{ "decode",      1, 0, 'e' },
	{ "seltest",     1, 0, 't' },
	{ "params",      0, 0,  7  },
	{ "setparam",    1, 0,  8  },
	{ "single",      0, 0, 's' },
	{ "noserdfiltering", 0, 0, 'f' },
	{ 0, 0, 0, 0 }
    };
	
    int opt;

    while ((opt = getopt_long(argc, argv, "Ddhe:t:sf",
			      long_options, &optind)) != -1) {
	switch (opt) {
	case 'D':
	    options.no_daemon_flag = 1;
	    break;
	case 'd':
	    options.debug_mode++;
	    options.nosyslog_mode = 1;
	    break;
        case 'e':
            options.decode_mode = 1;
            options.nosyslog_mode = 1;
            options.decode_address = strtoll(optarg, NULL, 16);
	    {
		char* colon = strchr(optarg, ':');
		if (colon)
		    options.decode_syndrome = strtol(colon+1, NULL, 16);
		else
		    options.decode_syndrome = 0;
	    }
            break;
        case 't':
            options.seltest_mode = 1;
	    options.debug_mode++;
            options.nosyslog_mode = 1;
	    options.no_daemon_flag = 1;
            options.seltest_address = strtoll(optarg, NULL, 16);
	    {
		char* colon = strchr(optarg, ':');
		if (colon)
		    options.decode_syndrome = strtol(colon+1, NULL, 16);
		else
		    options.decode_syndrome = 0;
            }

            break;
	case 's':
	    options.singleiter_mode = 1;
	    options.no_daemon_flag = 1;
	    break;
	case 'f':
	    options.no_serd_filter = 1;
	    break;
	case 1:
	    options.ignore_no_dev = 1;
	    break;
	case 2:
	    options.dmi_decode = 1;
	    break;
	case 3:
	    options.filter_bogus = 1;
	    break;
	case 4:
	    options.i_user = optarg;
	    break;
	case 5:
	    options.i_password = optarg;
	    break;
	case 6:
	    options.i_ip = optarg;
	    break;
	case 7:
	    options_print_parameters();
	    exit(0);
	case 8:
	    options_add_parameter(optarg);
	    break;
	case 'h':
	    options_usage(argv);
	    exit(0);
	default:
	    options_usage(argv);
	    exit(1);
	}
    }
}

void options_usage(char** argv)
{
    fprintf(stderr, "Usage: %s [options]\n", PACKAGE);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, " -e, --decode <addr>{:<syndrome>}\n\t\t\tDecode the given"
	    " 64-bit hex address and optional\n\t\t\tsyndrome,"
            " then exit\n");
    fprintf(stderr, " -D, --nodaemon\t\tDon't detach and become a daemon\n");
    fprintf(stderr, " -d, --debug\t\tDebug mode\n");
    fprintf(stderr, "     --ignorenodev\tSilent exit if device missing\n");
    fprintf(stderr, "     --filter\t\tFilter out known bogus MCEs\n");
    fprintf(stderr, "     --dmi\t\tLookup MCE address in BIOS tables\n");
    fprintf(stderr, "     --params\t\tDisplay herd parameters information\n");
    fprintf(stderr, "     --setparam <key>=<value>\n\t\t\tSet or override parameter value\n");
    fprintf(stderr, " -s, --single\t\tRun a single check/report iteration\n");
    fprintf(stderr, " -f, --noserdfiltering\tDo not filter CEs based on N occurrences in T time\n");
    fprintf(stderr, " -h, --help\t\tThis message\n");
}

const char* options_get_string(const char* key)
{
    void* value = NULL;

    if (!set_lookup(options.parameters, key, &value))
	return NULL;

    return (const char*)value;
}

int options_get_int(const char* key)
{
    void* value = NULL;

    if (!set_lookup(options.parameters, key, &value))
	return 0;

    return atoi((const char*)value);
}

void options_free()
{
    if (options.parameters) {
	set_delete(options.parameters, free);
	options.parameters = NULL;
    }
}
